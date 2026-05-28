#ifndef DOCTORROUTES_H
#define DOCTORROUTES_H

#include "../../controllers/modules/doctor/doctorHandler.h"
#include "../../controllers/auth/authHandler/authHandler.h"
#include "../../controllers/common/orderCommon/orderCommonHandler.h"
#include "../../controllers/common/reservationCommon/reservationCommonHandler.h"

class DoctorRoutes {
public:
    // Setup routes
    static void setupDoctorRoutes(CrowApp& app, std::shared_ptr<DatabaseManagerInterface> dbManager);
};

#endif
