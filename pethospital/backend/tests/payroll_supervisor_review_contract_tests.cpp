#include <cassert>
#include <fstream>
#include <sstream>
#include <string>

#ifndef COLUMN_MIGRATIONS_SOURCE_PATH
#error "COLUMN_MIGRATIONS_SOURCE_PATH is required"
#endif

#ifndef FINANCE_HANDLER_SOURCE_PATH
#error "FINANCE_HANDLER_SOURCE_PATH is required"
#endif

namespace
{
std::string readFile(const char *path)
{
    std::ifstream file(path);
    assert(file.is_open());
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void assertContains(const std::string &source, const std::string &needle)
{
    assert(source.find(needle) != std::string::npos);
}
}

int main()
{
    const std::string migration = readFile(COLUMN_MIGRATIONS_SOURCE_PATH);
    const std::string handler = readFile(FINANCE_HANDLER_SOURCE_PATH);

    // Legacy second_reviewed rows must be normalized when their period enters the
    // new supervisor queue, otherwise a returned period cannot be resubmitted.
    assertContains(migration, "UPDATE salary s JOIN payrollPeriod p ON p.id=s.payroll_period_id");
    assertContains(migration, "SET s.review_status='first_reviewed'");
    assertContains(migration, "AND s.review_status='second_reviewed'");

    // The UI's first-review count and targeted return gate must match the submit gate.
    assertContains(handler, "SUM(CASE WHEN s.review_status='first_reviewed' THEN 1 ELSE 0 END)");
    assertContains(handler, "WHERE id=? AND payroll_period_id=? AND review_status='first_reviewed'");
    assertContains(handler, "WHERE id=? AND review_status IN ('pending','returned')");
    return 0;
}
