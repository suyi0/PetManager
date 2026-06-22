#include "staticFileHandler.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace
{
std::string lowerExtension(const std::filesystem::path &path)
{
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return extension;
}

bool shouldCacheStaticFile(const std::filesystem::path &path)
{
    return path.filename() != "service-worker.js";
}
}

crow::response StaticFileHandler::handleRequest(
    const crow::request &req,
    const std::filesystem::path &staticRoot)
{
    const Decision decision = resolveRequest(req.url, staticRoot);
    if (decision.type == DecisionType::NotFound)
    {
        return crow::response(404);
    }

    return fileResponse(decision);
}

StaticFileHandler::Decision StaticFileHandler::resolveRequest(
    const std::string &rawPath,
    const std::filesystem::path &staticRoot)
{
    const std::filesystem::path indexPath = staticRoot / "index.html";
    const std::filesystem::path requestPath = normalizeRequestPath(rawPath);
    const std::string normalizedUrl = "/" + requestPath.generic_string();

    if (isBackendOwnedPath(normalizedUrl))
    {
        return {DecisionType::NotFound, {}, "", false};
    }

    const std::filesystem::path candidate = staticRoot / requestPath;
    if (std::filesystem::exists(candidate) && std::filesystem::is_regular_file(candidate))
    {
        return {
            DecisionType::ServeFile,
            candidate,
            contentTypeForPath(candidate),
            shouldCacheStaticFile(candidate),
        };
    }

    if (std::filesystem::exists(indexPath) && std::filesystem::is_regular_file(indexPath))
    {
        return {
            DecisionType::ServeIndex,
            indexPath,
            "text/html; charset=UTF-8",
            false,
        };
    }

    return {DecisionType::NotFound, {}, "", false};
}

std::string StaticFileHandler::contentTypeForPath(const std::filesystem::path &path)
{
    static const std::unordered_map<std::string, std::string> contentTypes = {
        {".html", "text/html; charset=UTF-8"},
        {".css", "text/css; charset=UTF-8"},
        {".js", "application/javascript; charset=UTF-8"},
        {".json", "application/json; charset=UTF-8"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif", "image/gif"},
        {".svg", "image/svg+xml"},
        {".ico", "image/x-icon"},
        {".webp", "image/webp"},
        {".woff", "font/woff"},
        {".woff2", "font/woff2"},
        {".ttf", "font/ttf"},
        {".txt", "text/plain; charset=UTF-8"},
    };

    const auto match = contentTypes.find(lowerExtension(path));
    if (match != contentTypes.end())
    {
        return match->second;
    }

    return "application/octet-stream";
}

bool StaticFileHandler::isBackendOwnedPath(const std::string &path)
{
    return path == "/api" || path.rfind("/api/", 0) == 0 ||
           path == "/uploads" || path.rfind("/uploads/", 0) == 0 ||
           path == "/assets" || path.rfind("/assets/", 0) == 0 ||
           path == "/realtime" || path.rfind("/realtime/", 0) == 0;
}

std::filesystem::path StaticFileHandler::normalizeRequestPath(const std::string &rawPath)
{
    std::string path = rawPath.empty() ? "/" : rawPath;
    const std::size_t queryStart = path.find_first_of("?#");
    if (queryStart != std::string::npos)
    {
        path = path.substr(0, queryStart);
    }

    while (!path.empty() && path.front() == '/')
    {
        path.erase(path.begin());
    }

    std::filesystem::path safePath;
    for (const auto &part : std::filesystem::path(path))
    {
        const std::string segment = part.string();
        if (segment.empty() || segment == "." || segment == "..")
        {
            continue;
        }
        safePath /= segment;
    }

    return safePath;
}

crow::response StaticFileHandler::fileResponse(const Decision &decision)
{
    std::ifstream file(decision.filePath, std::ios::binary);
    if (!file)
    {
        return crow::response(500);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    crow::response res(200, buffer.str());
    res.set_header("Content-Type", decision.contentType);
    if (decision.cacheable)
    {
        res.set_header("Cache-Control", "public, max-age=31536000, immutable");
    }
    else
    {
        res.set_header("Cache-Control", "no-cache");
    }
    return res;
}
