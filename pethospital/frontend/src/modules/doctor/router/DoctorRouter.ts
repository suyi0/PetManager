import { RouteRecordRaw } from "vue-router";
import { DOCTOR_PORTAL_ROLES } from "@/core/auth/utils/roleUtils";

const doctorRoutes: Array<RouteRecordRaw> = [
  {
    path: "/doctor",
    component: () => import("@/modules/doctor/views/desktop/DoctorLayout.vue"),
    meta: { requiresAuth: true, allowedRoles: [...DOCTOR_PORTAL_ROLES] },
    children: [
      {
        path: "",
        redirect: "/doctor/home",
      },
      {
        path: "home",
        name: "doctorHome",
        component: () =>
          import("@/modules/doctor/views/desktop/pages/DoctorWorkbench.vue"),
      },
      {
        path: "queue",
        name: "doctorQueue",
        component: () =>
          import("@/modules/doctor/views/desktop/pages/DoctorQueue.vue"),
      },
      {
        path: "reservations",
        name: "doctorReservations",
        component: () =>
          import("@/modules/doctor/views/desktop/pages/DoctorReservations.vue"),
      },
      {
        path: "order-records",
        name: "doctorOrderRecords",
        component: () =>
          import(
            "@/modules/doctor/views/desktop/pages/Order-module/DoctorOrderRecords.vue"
          ),
      },
      {
        path: "create-order",
        name: "doctorCreateOrder",
        component: () =>
          import(
            "@/modules/doctor/views/desktop/pages/Order-module/DoctorCreateOrder.vue"
          ),
      },
      {
        path: "drafts",
        name: "doctorDrafts",
        component: () =>
          import(
            "@/modules/doctor/views/desktop/pages/Order-module/DoctorDraftsOrder.vue"
          ),
      },
      {
        path: "create-order/:queueId",
        name: "doctorCreateOrderFromQueue",
        component: () =>
          import(
            "@/modules/doctor/views/desktop/pages/Order-module/DoctorCreateOrder.vue"
          ),
      },
      {
        path: "users/:userId",
        name: "doctorUserProfile",
        component: () =>
          import(
            "@/modules/doctor/views/desktop/pages/User-module/DoctorUserProfile.vue"
          ),
      },
      {
        path: "orders/:orderId",
        name: "doctorOrderDetail",
        component: () =>
          import(
            "@/modules/doctor/views/desktop/pages/Order-module/DoctorOrderDetail.vue"
          ),
      },
      {
        path: "personal",
        name: "doctorPersonal",
        component: () =>
          import(
            "@/modules/user/views/desktop/Personal-module/UserPersonalView.vue"
          ),
        props: { roleBadge: "医生端" },
      },
    ],
  },
];

export default doctorRoutes;
