#include "smsScriptRunner.h"

#include <array>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <stdio.h>

namespace
{
std::string getEnvString(const char *name)
{
    const char *value = std::getenv(name);
    return value == nullptr ? "" : std::string(value);
}

std::string shellQuote(const std::string &value)
{
    std::string quoted = "\"";
    for (char ch : value)
    {
        if (ch == '"' || ch == '\\' || ch == '$' || ch == '`')
        {
            quoted += '\\';
        }
        quoted += ch;
    }
    quoted += "\"";
    return quoted;
}

std::string firstExistingPath(const std::initializer_list<std::string> &paths)
{
    for (const auto &path : paths)
    {
        if (!path.empty() && std::filesystem::exists(path))
        {
            return path;
        }
    }
    return "";
}
}

namespace SmsScriptRunner
{
std::string resolvePythonExecutable()
{
    const std::string configured = getEnvString("PYTHON_BIN");
    if (!configured.empty())
    {
        return configured;
    }

    const std::string existing = firstExistingPath({
        "/usr/bin/python3",
        "/opt/homebrew/bin/python3",
        "/usr/local/bin/python3",
    });

    return existing.empty() ? "python3" : existing;
}

std::string resolveScriptPath()
{
    const std::string configured = getEnvString("PETMANAGER_SMS_SCRIPT");
    if (!configured.empty())
    {
        return configured;
    }

    return "pethospital/backend/services/verification/sendSmsVerifyCode.py";
}

std::string buildCommand(const std::string &phone, const std::string &code)
{
    return shellQuote(resolvePythonExecutable()) + " " +
           shellQuote(resolveScriptPath()) + " " +
           shellQuote(phone) + " " +
           shellQuote(code) + " --json 2>&1";
}

std::pair<bool, std::string> execute(const std::string &phone, const std::string &code)
{
    const std::string python = resolvePythonExecutable();
    const std::string script = resolveScriptPath();

    if (python != "python3" && !std::filesystem::exists(python))
    {
        return std::make_pair(false, "Python executable not found: " + python);
    }

    if (!std::filesystem::exists(script))
    {
        return std::make_pair(false, "SMS script not found: " + script);
    }

    const std::string command = buildCommand(phone, code);
    std::cout << "Executing command: " << command << std::endl;

    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);

    if (!pipe)
    {
        std::cout << "Failed to execute python script: popen() failed" << std::endl;
        return std::make_pair(false, "Failed to execute python script");
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        result += buffer.data();
    }

    pclose(pipe.release());

    try
    {
        auto json_result = nlohmann::json::parse(result);
        bool success = json_result.value("success", false);
        std::string message = json_result.value("message", "");

        std::cout << "[INFO] Python script result - Success: " + std::to_string(success) + ", Message: " + message << std::endl;

        return std::make_pair(success, message);
    }
    catch (const std::exception &e)
    {
        std::cerr << "[ERROR] Failed to parse JSON result: " + std::string(e.what()) << std::endl;
        std::cerr << "[ERROR] Raw result: " + result << std::endl;
        return std::make_pair(false, "Failed to parse script result: " + result);
    }
}
}
