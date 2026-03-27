#pragma once

#include "DatabaseManagerInterface.h"

namespace DatabaseMigrations
{
void run(DatabaseManagerInterface &dbManager);
}
