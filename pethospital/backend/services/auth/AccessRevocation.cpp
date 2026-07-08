#include "AccessRevocation.h"

#include "AuthSessionStore.h"
#include "../realtime/adminBroadcaster/adminHomeDataBroadcaster.h"
#include "../realtime/doctorBroadcaster/doctorQueueBroadcaster.h"
#include "../realtime/doctorListBroadcaster/doctorListBroadcaster.h"
#include "../realtime/financeBroadcaster/financeHomeDataBroadcaster.h"
#include "../realtime/medicineBroadcaster/medicineStockBroadcaster.h"
#include "../redis/userRoleCache/UserRoleCache.h"

namespace AccessRevocation
{
    void revokeUserSessions(int userId)
    {
        if (userId <= 0)
        {
            return;
        }
        UserRoleCache::invalidate(userId);
        AuthSessionStore::bumpSessionVersionForUser(userId);
    }

    void closeRealtimeConnections()
    {
        AdminHomeDataBroadcaster::instance().closeAllConnections("access_changed");
        FinanceHomeDataBroadcaster::instance().closeAllConnections("access_changed");
        DoctorQueueBroadcaster::instance().closeAllConnections("access_changed");
        MedicineStockBroadcaster::instance().closeAllConnections("access_changed");
        DoctorListBroadcaster::instance().closeAllConnections("access_changed");
    }

    void onUserAccessChanged(int userId)
    {
        revokeUserSessions(userId);
        closeRealtimeConnections();
    }
}
