#ifndef DOCTORROUTES_H
#define DOCTORROUTES_H

#include "../../controllers/doctor/doctorHandler.h"
#include "../../controllers/auth/authHandler/authHandler.h"

class DoctorRoutes {
public:
    // Setup routes
    static void setupDoctorRoutes(CrowApp& app, std::shared_ptr<DatabaseManagerInterface> dbManager);
};

#endif