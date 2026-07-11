#pragma once

#include <cstdint>
#include <string>

namespace Reporting
{
struct RenderedArtifact
{
    bool success{false};
    std::string storageKey;
    std::string absolutePath;
    std::string sha256;
    std::uintmax_t byteSize{0};
    std::string error;
};

class IReportRenderer
{
public:
    virtual ~IReportRenderer() = default;
    virtual RenderedArtifact renderPdf(
        const std::string &html,
        const std::string &storageKey) const = 0;
};

class ChromeReportRenderer final : public IReportRenderer
{
public:
    RenderedArtifact renderPdf(
        const std::string &html,
        const std::string &storageKey) const override;
};

std::string renderTemplate(const std::string &source, const std::string &payloadJson);
std::string reportStorageRoot();
std::string sha256File(const std::string &path);
}
