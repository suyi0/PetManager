#include "../utils/staticFileHandler.h"

#include <cassert>
#include <filesystem>
#include <fstream>

namespace
{
void writeFile(const std::filesystem::path &path, const std::string &body)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file << body;
}
}

int main()
{
    const auto root = std::filesystem::temp_directory_path() / "petmanager-static-file-handler-tests";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    writeFile(root / "index.html", "<div id=\"app\"></div>");
    writeFile(root / "js" / "app.js", "console.log('app');");
    writeFile(root / "service-worker.js", "self.addEventListener('install', () => {});");

    auto rootDecision = StaticFileHandler::resolveRequest("/", root);
    assert(rootDecision.type == StaticFileHandler::DecisionType::ServeIndex);
    assert(rootDecision.filePath == root / "index.html");
    assert(!rootDecision.cacheable);

    auto assetDecision = StaticFileHandler::resolveRequest("/js/app.js", root);
    assert(assetDecision.type == StaticFileHandler::DecisionType::ServeFile);
    assert(assetDecision.filePath == root / "js" / "app.js");
    assert(assetDecision.contentType == "application/javascript; charset=UTF-8");
    assert(assetDecision.cacheable);

    auto serviceWorkerDecision = StaticFileHandler::resolveRequest("/service-worker.js", root);
    assert(serviceWorkerDecision.type == StaticFileHandler::DecisionType::ServeFile);
    assert(!serviceWorkerDecision.cacheable);

    auto spaDecision = StaticFileHandler::resolveRequest("/user/home/orders?tab=active", root);
    assert(spaDecision.type == StaticFileHandler::DecisionType::ServeIndex);
    assert(spaDecision.filePath == root / "index.html");

    auto apiDecision = StaticFileHandler::resolveRequest("/api/users/sessions", root);
    assert(apiDecision.type == StaticFileHandler::DecisionType::NotFound);

    auto traversalDecision = StaticFileHandler::resolveRequest("/../../js/app.js", root);
    assert(traversalDecision.type == StaticFileHandler::DecisionType::ServeFile);
    assert(traversalDecision.filePath == root / "js" / "app.js");

    std::filesystem::remove_all(root);
    return 0;
}
