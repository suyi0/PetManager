#include "../services/reporting/ReportRenderer.h"

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <string>
#include <vector>

std::string getEnvVar(const std::string &name, const std::string &defaultValue)
{
    const char *value = std::getenv(name.c_str());
    return value ? std::string(value) : defaultValue;
}

std::string getProjectRoot()
{
    return getEnvVar("PROJECT_ROOT", std::filesystem::current_path().string());
}

namespace
{
void writeExecutable(const std::filesystem::path &path, const std::string &body)
{
    std::ofstream file(path);
    file << body;
    file.close();
    std::filesystem::permissions(
        path,
        std::filesystem::perms::owner_read |
            std::filesystem::perms::owner_write |
            std::filesystem::perms::owner_exec,
        std::filesystem::perm_options::replace);
}

std::string readFile(const std::filesystem::path &path)
{
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}
}

int main()
{
    const auto root = std::filesystem::temp_directory_path() / "petmanager-report-renderer-tests";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);
    setenv("REPORT_STORAGE_DIR", root.c_str(), 1);
    setenv("REPORT_RENDER_MAX_CONCURRENCY", "2", 1);
    setenv("REPORT_RENDER_TIMEOUT_SECONDS", "5", 1);

    const std::string payload = R"({
        "owner":{"name":"<Admin & User>"},
        "prescription":{"items":[
            {"medicineName":"A&1","quantity":1},
            {"medicineName":"<B>","quantity":2}
        ]}
    })";
    const std::string rendered = Reporting::renderTemplate(
        "<h1>{{owner.name}}</h1>{{#prescription.items}}<p>{{medicineName}}:{{quantity}}/{{owner.name}}</p>{{/prescription.items}}<i>{{missing}}</i>",
        payload);
    assert(rendered.find("&lt;Admin &amp; User&gt;") != std::string::npos);
    assert(rendered.find("A&amp;1:1") != std::string::npos);
    assert(rendered.find("&lt;B&gt;:2") != std::string::npos);
    assert(rendered.find("{{") == std::string::npos);
    const std::string hardened = Reporting::hardenReportHtml("<html><head></head><body><img src=\"https://example.invalid/x\"></body></html>");
    assert(hardened.find("default-src 'none'") != std::string::npos);
    assert(hardened.find("form-action 'none'") != std::string::npos);
    assert(hardened.find("<head><meta http-equiv=\"Content-Security-Policy\"") != std::string::npos);
    const std::string attributedHead = Reporting::hardenReportHtml("<HTML><HEAD data-test=\"x\"></HEAD><BODY>x</BODY></HTML>");
    assert(attributedHead.find("<HEAD data-test=\"x\"><meta http-equiv=\"Content-Security-Policy\"") != std::string::npos);
    const std::string noHead = Reporting::hardenReportHtml("<html lang=\"zh\"><body>x</body></html>");
    assert(noHead.find("<html lang=\"zh\"><head><meta http-equiv=\"Content-Security-Policy\"") != std::string::npos);
    const std::string fragment = Reporting::hardenReportHtml("<h1>fragment</h1>");
    assert(fragment.find("<!doctype html><html><head><meta http-equiv=\"Content-Security-Policy\"") == 0);
    const std::string headerFragment = Reporting::hardenReportHtml("<header>title</header>");
    assert(headerFragment.find("<!doctype html><html><head><meta http-equiv=\"Content-Security-Policy\"") == 0);

    const auto argsLog = root / "chrome-args.log";
    const auto successChrome = root / "fake-chrome-success";
    writeExecutable(successChrome, R"SH(#!/bin/sh
for arg in "$@"; do
  printf '%s\n' "$arg" >> "$FAKE_CHROME_ARGS_LOG"
  case "$arg" in --print-to-pdf=*) output="${arg#--print-to-pdf=}";; esac
done
sleep "${FAKE_CHROME_SLEEP:-0}"
printf '%%PDF-1.4\n%% fake report\n' > "$output"
)SH");
    setenv("REPORT_CHROME_BIN", successChrome.c_str(), 1);
    setenv("FAKE_CHROME_ARGS_LOG", argsLog.c_str(), 1);

    Reporting::ChromeReportRenderer renderer;
    assert(!renderer.renderPdf("x", "../outside.pdf").success);
    assert(!renderer.renderPdf("x", "/tmp/outside.pdf").success);
    const auto artifact = renderer.renderPdf("<h1>中文诊疗单</h1>", "medical/1/revision-1.pdf");
    assert(artifact.success);
    assert(artifact.byteSize > 0);
    assert(artifact.sha256.size() == 64);
    assert(std::filesystem::exists(artifact.absolutePath));
    const std::string args = readFile(argsLog);
    assert(args.find("--blink-settings=scriptEnabled=false") != std::string::npos);
    assert(args.find("--proxy-server=127.0.0.1:1") != std::string::npos);
    assert(args.find("--host-resolver-rules=MAP * 0.0.0.0") != std::string::npos);
    assert(args.find("--user-data-dir=") != std::string::npos);

    setenv("FAKE_CHROME_SLEEP", "1", 1);
    const auto concurrentStart = std::chrono::steady_clock::now();
    std::vector<std::future<Reporting::RenderedArtifact>> futures;
    for (int index = 0; index < 4; ++index)
    {
        futures.push_back(std::async(std::launch::async, [index, &renderer] {
            return renderer.renderPdf("<p>concurrent</p>", "concurrent/" + std::to_string(index) + ".pdf");
        }));
    }
    for (auto &future : futures) assert(future.get().success);
    const auto concurrentElapsed = std::chrono::steady_clock::now() - concurrentStart;
    assert(concurrentElapsed >= std::chrono::milliseconds(1800));
    assert(concurrentElapsed < std::chrono::seconds(5));
    unsetenv("FAKE_CHROME_SLEEP");

    const auto slowChrome = root / "fake-chrome-slow";
    writeExecutable(slowChrome, "#!/bin/sh\nexec sleep 4\n");
    setenv("REPORT_CHROME_BIN", slowChrome.c_str(), 1);
    setenv("REPORT_RENDER_TIMEOUT_SECONDS", "1", 1);
    const auto timeoutStart = std::chrono::steady_clock::now();
    const auto timedOut = renderer.renderPdf("<p>timeout</p>", "timeout/report.pdf");
    const auto timeoutElapsed = std::chrono::steady_clock::now() - timeoutStart;
    assert(!timedOut.success);
    assert(timedOut.error.find("timed out") != std::string::npos);
    assert(timeoutElapsed < std::chrono::seconds(3));
    assert(!std::filesystem::exists(root / "timeout" / "report.pdf"));

    const auto temporaryDir = root / ".tmp";
    if (std::filesystem::exists(temporaryDir))
    {
        assert(std::filesystem::is_empty(temporaryDir));
    }

    const char *realChrome = std::getenv("REAL_REPORT_CHROME_BIN");
    if (realChrome && realChrome[0] != '\0')
    {
        setenv("REPORT_CHROME_BIN", realChrome, 1);
        setenv("REPORT_RENDER_TIMEOUT_SECONDS", "30", 1);
        std::string rows;
        for (int index = 1; index <= 20; ++index)
        {
            rows += "<tr><td>示例药品" + std::to_string(index) + "</td><td>每日两次，连续服用三日</td></tr>";
        }
        const std::string realHtml =
            "<!doctype html><meta charset=\"utf-8\"><style>body{font-family:'PingFang SC','Noto Sans CJK SC',sans-serif}"
            "table{width:100%;border-collapse:collapse}thead{display:table-header-group}td,th{border:1px solid #333;padding:8px}"
            "</style><h1>宠物医院诊疗单</h1><h2>诊断</h2><p>中文渲染验证：急性胃肠炎，建议复诊观察。</p>"
            "<table><thead><tr><th>药品</th><th>用法</th></tr></thead><tbody>" + rows + "</tbody></table>";
        const auto realArtifact = renderer.renderPdf(realHtml, "real/chinese-20-items.pdf");
        if (!realArtifact.success) std::cerr << realArtifact.error << std::endl;
        assert(realArtifact.success);
        assert(realArtifact.byteSize > 1000);
    }

    if (!std::getenv("KEEP_REPORT_TEST_OUTPUT")) std::filesystem::remove_all(root);
    return 0;
}
