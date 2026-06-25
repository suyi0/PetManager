#ifndef SMS_SCRIPT_RUNNER_H
#define SMS_SCRIPT_RUNNER_H

#include <string>
#include <utility>

namespace SmsScriptRunner
{
std::string resolvePythonExecutable();
std::string resolveScriptPath();
std::string buildCommand(const std::string &phone, const std::string &code);
std::pair<bool, std::string> execute(const std::string &phone, const std::string &code);
}

#endif
