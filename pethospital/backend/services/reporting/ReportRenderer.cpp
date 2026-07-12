#include "ReportRenderer.h"

#include "../../utils/Utils.h"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <chrono>
#include <condition_variable>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <signal.h>
#include <sstream>
#include <system_error>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

namespace
{
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

int positiveEnvInt(const char *name, int fallback, int maximum)
{
    try
    {
        const int parsed = std::stoi(getEnvVar(name, std::to_string(fallback)));
        return parsed > 0 ? std::min(parsed, maximum) : fallback;
    }
    catch (...)
    {
        return fallback;
    }
}

class RenderLimiter
{
public:
    bool acquire(std::chrono::seconds timeout)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!condition_.wait_for(lock, timeout, [this] { return active_ < limit_; })) return false;
        ++active_;
        return true;
    }

    void release()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (active_ > 0) --active_;
        condition_.notify_one();
    }

private:
    const int limit_{positiveEnvInt("REPORT_RENDER_MAX_CONCURRENCY", 2, 8)};
    int active_{0};
    std::mutex mutex_;
    std::condition_variable condition_;
};

RenderLimiter &renderLimiter()
{
    static RenderLimiter limiter;
    return limiter;
}

class RenderSlotGuard
{
public:
    explicit RenderSlotGuard(std::chrono::seconds timeout)
        : acquired_(renderLimiter().acquire(timeout)) {}
    ~RenderSlotGuard() { if (acquired_) renderLimiter().release(); }
    bool acquired() const { return acquired_; }

private:
    bool acquired_{false};
};

std::string uniqueRenderToken()
{
    const auto ticks = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto threadId = std::hash<std::thread::id>{}(std::this_thread::get_id());
    return std::to_string(static_cast<long long>(::getpid())) + "-" +
           std::to_string(static_cast<unsigned long long>(threadId)) + "-" +
           std::to_string(static_cast<long long>(ticks));
}

int runChromeWithTimeout(
    const std::string &chrome,
    const std::filesystem::path &htmlPath,
    const std::filesystem::path &pdfPath,
    const std::filesystem::path &profilePath,
    std::chrono::seconds timeout,
    bool &timedOut)
{
    const pid_t child = ::fork();
    if (child < 0) return -1;
    if (child == 0)
    {
        ::setpgid(0, 0);
        const int nullFd = ::open("/dev/null", O_WRONLY);
        if (nullFd >= 0)
        {
            ::dup2(nullFd, STDOUT_FILENO);
            ::dup2(nullFd, STDERR_FILENO);
            ::close(nullFd);
        }

        const std::string pdfArg = "--print-to-pdf=" + pdfPath.string();
        const std::string profileArg = "--user-data-dir=" + profilePath.string();
        const std::string htmlUrl = "file://" + htmlPath.string();
        const char *arguments[] = {
            chrome.c_str(),
            "--headless=new",
            "--disable-gpu",
            "--disable-extensions",
            "--disable-background-networking",
            "--disable-sync",
            "--no-first-run",
            "--no-default-browser-check",
            profileArg.c_str(),
            "--no-pdf-header-footer",
            "--blink-settings=scriptEnabled=false",
            "--proxy-server=127.0.0.1:1",
            "--host-resolver-rules=MAP * 0.0.0.0",
            pdfArg.c_str(),
            htmlUrl.c_str(),
            nullptr,
        };
        ::execv(chrome.c_str(), const_cast<char *const *>(arguments));
        ::_exit(127);
    }

    ::setpgid(child, child);

    const auto deadline = std::chrono::steady_clock::now() + timeout;
    int status = 0;
    while (std::chrono::steady_clock::now() < deadline)
    {
        const pid_t result = ::waitpid(child, &status, WNOHANG);
        if (result == child)
        {
            if (WIFEXITED(status)) return WEXITSTATUS(status);
            if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
            return -1;
        }
        if (result < 0)
        {
            if (errno == EINTR) continue;
            ::kill(-child, SIGKILL);
            ::waitpid(child, &status, 0);
            return -1;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    timedOut = true;
    ::kill(-child, SIGKILL);
    ::waitpid(child, &status, 0);
    return -1;
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

std::string hardenReportHtml(const std::string &html)
{
    static const std::string policy =
        "<meta http-equiv=\"Content-Security-Policy\" "
        "content=\"default-src 'none'; style-src 'unsafe-inline'; img-src data:; font-src data:; base-uri 'none'; form-action 'none'\">";
    std::string hardened = html;
    std::string normalized = html;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    const auto findOpenTag = [&normalized](const std::string &tag) {
        std::size_t position = normalized.find("<" + tag);
        while (position != std::string::npos)
        {
            const std::size_t boundary = position + tag.size() + 1;
            if (boundary < normalized.size() &&
                (normalized[boundary] == '>' || std::isspace(static_cast<unsigned char>(normalized[boundary])))) return position;
            position = normalized.find("<" + tag, boundary);
        }
        return std::string::npos;
    };
    const std::size_t head = findOpenTag("head");
    if (head != std::string::npos)
    {
        const std::size_t headClose = normalized.find('>', head + 5);
        if (headClose != std::string::npos)
        {
            hardened.insert(headClose + 1, policy);
            return hardened;
        }
    }
    const std::size_t htmlTag = findOpenTag("html");
    if (htmlTag != std::string::npos)
    {
        const std::size_t htmlClose = normalized.find('>', htmlTag + 5);
        if (htmlClose != std::string::npos)
        {
            hardened.insert(htmlClose + 1, "<head>" + policy + "</head>");
            return hardened;
        }
    }
    return "<!doctype html><html><head>" + policy + "</head><body>" + hardened + "</body></html>";
}

RenderedArtifact ChromeReportRenderer::renderPdf(const std::string &html, const std::string &storageKey) const
{
    RenderedArtifact result;
    result.storageKey = storageKey;
    const std::filesystem::path relativeKey(storageKey);
    if (storageKey.empty() || relativeKey.is_absolute())
    {
        result.error = "Report storage key must be a non-empty relative path";
        return result;
    }
    for (const auto &component : relativeKey)
    {
        if (component == "..")
        {
            result.error = "Report storage key cannot contain parent traversal";
            return result;
        }
    }
    const std::string chrome = resolveChromeExecutable();
    if (chrome.empty())
    {
        result.error = "REPORT_CHROME_BIN is not configured and Chrome/Chromium was not found";
        return result;
    }

    const auto timeout = std::chrono::seconds(positiveEnvInt("REPORT_RENDER_TIMEOUT_SECONDS", 30, 120));
    RenderSlotGuard renderSlot(timeout);
    if (!renderSlot.acquired())
    {
        result.error = "Report renderer is busy; concurrency slot wait timed out";
        return result;
    }

    const std::filesystem::path root = reportStorageRoot();
    const std::filesystem::path output = root / relativeKey;
    const std::filesystem::path temporaryDir = root / ".tmp";
    const std::string token = uniqueRenderToken();
    const std::filesystem::path temporaryHtml = temporaryDir / (token + ".html");
    const std::filesystem::path temporaryPdf = temporaryDir / (token + ".pdf");
    const std::filesystem::path temporaryProfile = temporaryDir / (token + "-profile");
    std::error_code error;
    std::filesystem::create_directories(output.parent_path(), error);
    if (error)
    {
        result.error = "Failed to create report directory: " + error.message();
        return result;
    }
    std::filesystem::create_directories(temporaryDir, error);
    if (error)
    {
        result.error = "Failed to create report temporary directory: " + error.message();
        return result;
    }

    {
        std::ofstream stream(temporaryHtml, std::ios::binary | std::ios::trunc);
        if (!stream)
        {
            result.error = "Failed to create temporary report HTML";
            return result;
        }
        stream << hardenReportHtml(html);
    }

    bool timedOut = false;
    const int exitCode = runChromeWithTimeout(chrome, temporaryHtml, temporaryPdf, temporaryProfile, timeout, timedOut);
    std::filesystem::remove(temporaryHtml, error);
    std::filesystem::remove_all(temporaryProfile, error);
    if (exitCode != 0 || !std::filesystem::exists(temporaryPdf))
    {
        std::filesystem::remove(temporaryPdf, error);
        result.error = timedOut
            ? "Chrome PDF rendering timed out after " + std::to_string(timeout.count()) + " seconds"
            : "Chrome PDF rendering failed with exit code " + std::to_string(exitCode);
        return result;
    }

    result.sha256 = sha256File(temporaryPdf.string());
    result.byteSize = std::filesystem::file_size(temporaryPdf, error);
    if (error || result.sha256.empty())
    {
        std::filesystem::remove(temporaryPdf, error);
        result.error = "PDF was created but integrity metadata could not be calculated";
        return result;
    }

    if (::rename(temporaryPdf.c_str(), output.c_str()) != 0)
    {
        std::filesystem::remove(temporaryPdf, error);
        result.error = "Failed to atomically publish rendered PDF";
        return result;
    }

    result.absolutePath = output.string();
    result.success = true;
    return result;
}
}
