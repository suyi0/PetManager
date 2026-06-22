#pragma once

#include <crow.h>
#include <filesystem>
#include <string>

class StaticFileHandler
{
public:
    enum class DecisionType
    {
        ServeFile,
        ServeIndex,
        NotFound,
    };

    struct Decision
    {
        DecisionType type;
        std::filesystem::path filePath;
        std::string contentType;
        bool cacheable;
    };

    static crow::response handleRequest(
        const crow::request &req,
        const std::filesystem::path &staticRoot);

    static Decision resolveRequest(
        const std::string &rawPath,
        const std::filesystem::path &staticRoot);

    static std::string contentTypeForPath(const std::filesystem::path &path);

private:
    static bool isBackendOwnedPath(const std::string &path);
    static std::filesystem::path normalizeRequestPath(const std::string &rawPath);
    static crow::response fileResponse(const Decision &decision);
};
