#include "ReportRenderer.h"

#include "../../utils/Utils.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <sstream>
#include <system_error>

namespace
{
std::string shellQuote(const std::string &value)
{
    std::string result = "'";
    for (char ch : value)
    {
        result += ch == '\'' ? "'\\''" : std::string(1, ch);
    }
    return result + "'";
}

std::string htmlEscape(const std::string &value)
{
    std::string escaped;
    escaped.reserve(value.size());
    for (char ch : value)
    {
        switch (ch)
        {
        case '&': escaped += "&amp;"; break;
        case '<': escaped += "&lt;"; break;
        case '>': escaped += "&gt;"; break;
        case '"': escaped += "&quot;"; break;
        case '\'': escaped += "&#39;"; break;
        default: escaped += ch; break;
        }
    }
    return escaped;
}

std::string jsonText(const nlohmann::json &value)
{
    if (value.is_null()) return "";
    if (value.is_string()) return value.get<std::string>();
    if (value.is_boolean()) return value.get<bool>() ? "是" : "否";
    if (value.is_number_float())
    {
        std::ostringstream stream;
        stream << std::fixed << std::setprecision(2) << value.get<double>();
        return stream.str();
    }
    return value.dump();
}

const nlohmann::json *findPath(const nlohmann::json &root, const std::string &path)
{
    const nlohmann::json *current = &root;
    std::size_t start = 0;
    while (start < path.size())
    {
        const std::size_t dot = path.find('.', start);
        const std::string key = path.substr(start, dot == std::string::npos ? std::string::npos : dot - start);
        if (!current->is_object() || !current->contains(key)) return nullptr;
        current = &current->at(key);
        if (dot == std::string::npos) break;
        start = dot + 1;
    }
    return current;
}

void replaceAll(std::string &text, const std::string &needle, const std::string &replacement)
{
    if (needle.empty()) return;
    std::size_t offset = 0;
    while ((offset = text.find(needle, offset)) != std::string::npos)
    {
        text.replace(offset, needle.size(), replacement);
        offset += replacement.size();
    }
}

std::string renderScalarTokens(std::string text, const nlohmann::json &scope, const nlohmann::json &root)
{
    std::size_t start = 0;
    while ((start = text.find("{{", start)) != std::string::npos)
    {
        const std::size_t end = text.find("}}", start + 2);
        if (end == std::string::npos) break;
        const std::string key = text.substr(start + 2, end - start - 2);
        if (key.empty() || key.front() == '#' || key.front() == '/')
        {
            start = end + 2;
            continue;
        }
        const nlohmann::json *value = findPath(scope, key);
        if (!value) value = findPath(root, key);
        const std::string replacement = value ? htmlEscape(jsonText(*value)) : "";
        text.replace(start, end + 2 - start, replacement);
        start += replacement.size();
    }
    return text;
}

std::string resolveChromeExecutable()
{
    const std::string configured = getEnvVar("REPORT_CHROME_BIN", "");
    const std::string candidates[] = {
        configured,
        "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome",
        "/usr/bin/google-chrome",
        "/usr/bin/chromium",
        "/usr/bin/chromium-browser",
    };
    for (const auto &candidate : candidates)
    {
        if (!candidate.empty() && std::filesystem::exists(candidate)) return candidate;
    }
    return "";
}
}

namespace Reporting
{
std::string reportStorageRoot()
{
    return getEnvVar("REPORT_STORAGE_DIR", getProjectRoot() + "/data/reports");
}

std::string sha256File(const std::string &path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) return "";

    EVP_MD_CTX *context = EVP_MD_CTX_new();
    if (!context) return "";
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digestLength = 0;
    bool ok = EVP_DigestInit_ex(context, EVP_sha256(), nullptr) == 1;
    char buffer[8192];
    while (ok && input.good())
    {
        input.read(buffer, sizeof(buffer));
        const std::streamsize count = input.gcount();
        if (count > 0)
        {
            ok = EVP_DigestUpdate(context, buffer, static_cast<std::size_t>(count)) == 1;
        }
    }
    ok = ok && EVP_DigestFinal_ex(context, digest, &digestLength) == 1;
    EVP_MD_CTX_free(context);
    if (!ok) return "";

    std::ostringstream result;
    for (unsigned int index = 0; index < digestLength; ++index)
    {
        result << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[index]);
    }
    return result.str();
}

std::string renderTemplate(const std::string &source, const std::string &payloadJson)
{
    const nlohmann::json root = nlohmann::json::parse(payloadJson);
    std::string output = source;
    const std::string open = "{{#prescription.items}}";
    const std::string close = "{{/prescription.items}}";
    const std::size_t blockStart = output.find(open);
    const std::size_t blockEnd = blockStart == std::string::npos ? std::string::npos : output.find(close, blockStart + open.size());
    if (blockStart != std::string::npos && blockEnd != std::string::npos)
    {
        const std::string block = output.substr(blockStart + open.size(), blockEnd - blockStart - open.size());
        std::string rows;
        const nlohmann::json *items = findPath(root, "prescription.items");
        if (items && items->is_array())
        {
            for (const auto &item : *items) rows += renderScalarTokens(block, item, root);
        }
        output.replace(blockStart, blockEnd + close.size() - blockStart, rows);
    }
    return renderScalarTokens(output, root, root);
}

RenderedArtifact ChromeReportRenderer::renderPdf(const std::string &html, const std::string &storageKey) const
{
    RenderedArtifact result;
    result.storageKey = storageKey;
    const std::string chrome = resolveChromeExecutable();
    if (chrome.empty())
    {
        result.error = "REPORT_CHROME_BIN is not configured and Chrome/Chromium was not found";
        return result;
    }

    const std::filesystem::path root = reportStorageRoot();
    const std::filesystem::path output = root / storageKey;
    const std::filesystem::path temporary = output.string() + ".html";
    std::error_code error;
    std::filesystem::create_directories(output.parent_path(), error);
    if (error)
    {
        result.error = "Failed to create report directory: " + error.message();
        return result;
    }

    {
        std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
        if (!stream)
        {
            result.error = "Failed to create temporary report HTML";
            return result;
        }
        stream << html;
    }

    const std::string command = shellQuote(chrome) +
        " --headless=new --disable-gpu --no-pdf-header-footer --print-to-pdf=" +
        shellQuote(output.string()) + " " + shellQuote("file://" + temporary.string()) + " >/dev/null 2>&1";
    const int exitCode = std::system(command.c_str());
    std::filesystem::remove(temporary, error);
    if (exitCode != 0 || !std::filesystem::exists(output))
    {
        result.error = "Chrome PDF rendering failed with exit code " + std::to_string(exitCode);
        return result;
    }

    result.absolutePath = output.string();
    result.byteSize = std::filesystem::file_size(output, error);
    result.sha256 = sha256File(output.string());
    result.success = !error && !result.sha256.empty();
    if (!result.success) result.error = "PDF was created but integrity metadata could not be calculated";
    return result;
}
}
