#pragma once

#include <string>

namespace DataScope
{
enum class Kind
{
    All,
    MedicalAssigned,
    Owner
};

struct Scope
{
    Kind kind;
    int userId;
};

Scope resolveForRole(const std::string &roleName, int userId);
bool bindsUserId(const Scope &scope);
}
