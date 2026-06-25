#include "../controllers/auth/smsScriptRunner.h"

#include <cassert>
#include <cstdlib>
#include <string>

int main()
{
    setenv("PYTHON_BIN", "/usr/local/bin/python3", 1);
    setenv("PETMANAGER_SMS_SCRIPT", "/tmp/send sms.py", 1);

    const std::string command = SmsScriptRunner::buildCommand("13800138000", "123456");

    assert(command.find("\"/usr/local/bin/python3\"") != std::string::npos);
    assert(command.find("\"/tmp/send sms.py\"") != std::string::npos);
    assert(command.find("\"13800138000\"") != std::string::npos);
    assert(command.find("\"123456\"") != std::string::npos);
    assert(command.find("--json 2>&1") != std::string::npos);

    unsetenv("PYTHON_BIN");
    unsetenv("PETMANAGER_SMS_SCRIPT");

    return 0;
}
