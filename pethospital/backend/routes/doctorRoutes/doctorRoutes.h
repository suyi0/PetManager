#ifndef DOCTORROUTES_H
#define DOCTORROUTES_H

#include "../../controllers/modules/doctor/doctorHandler.h"
#include "../../controllers/auth/authHandler/authHandler.h"
#include "../../middleware/authMiddleware/authMiddleware.h"
#include "../../controllers/common/orderCommon/orderCommonHandler.h"
#include "../../controllers/common/reservationCommon/reservationCommonHandler.h"
#include "../../controllers/common/petCommon/petCommonHandler.h"

class DoctorRoutes {
public:
    // Setup routes
    static void setupDoctorRoutes(CrowApp& app, std::shared_ptr<DatabaseManagerInterface> dbManager);
};

#endif
