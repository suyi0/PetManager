#ifndef ATTENDANCEROUTES_H
#define ATTENDANCEROUTES_H

#include "../../middleware/authMiddleware/authMiddleware.h"
#include "../../utils/Utils.h"

class attendanceRoutes : public BaseHandler
{
public:
    static void setupAttendanceRoutes(CrowApp &app, std::shared_ptr<DatabaseManagerInterface> dbManager);
};

#endif
