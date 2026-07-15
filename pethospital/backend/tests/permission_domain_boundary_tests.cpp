#include <cassert>
#include <fstream>
#include <sstream>
#include <string>

#ifndef RBAC_SERVICE_SOURCE_PATH
#error "RBAC_SERVICE_SOURCE_PATH is required"
#endif
#ifndef ADMIN_ROUTES_SOURCE_PATH
#error "ADMIN_ROUTES_SOURCE_PATH is required"
#endif

namespace {
std::string readFile(const char *path) { std::ifstream file(path); assert(file); std::ostringstream out; out << file.rdbuf(); return out.str(); }
void contains(const std::string &source, const std::string &value) { assert(source.find(value) != std::string::npos); }
}

int main()
{
    const std::string service = readFile(RBAC_SERVICE_SOURCE_PATH);
    contains(service, "systemKey == \"president\" || systemKey == \"vice-president\" || systemKey == \"super-admin\"");
    contains(service, "staffKind == \"doctor\" || staffKind == \"nurse\"");
    contains(service, "staffKind == \"finance\"");
    contains(service, "staffKind == \"personnel\"");
    contains(service, "staffKind == \"warehouse\"");
    contains(service, "staffKind != \"management\"");
    contains(service, "std::set<Domain> domains{Domain::General, Domain::Management}");
    contains(service, "return generalOnly");

    const std::string routes = readFile(ADMIN_ROUTES_SOURCE_PATH);
    contains(routes, "permissionsOutsideAllowedDomains");
    contains(routes, "domainBoundaryError");
    // 越域用 400（ValidationError），不用 422：Crow 1.2.x 状态码表无 422，未知码回落 500。
    contains(routes, "ResponseHelper::validation(req, message)");
    contains(routes, "DELETE FROM user_permissions WHERE user_id=?");
    contains(routes, "PositionPermissionService::replacePermissions");
    contains(routes, "EmploymentAssignmentService::assign");
    return 0;
}
