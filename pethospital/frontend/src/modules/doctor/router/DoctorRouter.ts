import { RouteRecordRaw } from "vue-router";

const doctorRoutes: Array<RouteRecordRaw> = [
  {
    path: "/doctor",
    component: () => import("../views/DoctorLayout.vue"),
    meta: { requiresAuth: true, allowedRoles: [2] },
    children: [
      {
        path: "",
        redirect: "/doctor/home",
      },
      {
        path: "home",
        name: "doctorHome",
        component: () => import("../views/pages/DoctorWorkbench.vue"),
      },
      {
        path: "queue",
        name: "doctorQueue",
        component: () => import("../views/pages/DoctorQueue.vue"),
      },
      {
        path: "reservations",
        name: "doctorReservations",
        component: () => import("../views/pages/DoctorReservations.vue"),
      },
      {
        path: "order-records",
        name: "doctorOrderRecords",
        component: () => import("../views/pages/DoctorOrderRecords.vue"),
      },
      {
        path: "create-order",
        name: "doctorCreateOrder",
        component: () => import("../views/pages/DoctorCreateOrder.vue"),
      },
      {
        path: "users/:userId",
        name: "doctorUserProfile",
        component: () => import("../views/pages/DoctorUserProfile.vue"),
      },
      {
        path: "orders/:orderId",
        name: "doctorOrderDetail",
        component: () => import("../views/pages/DoctorOrderDetail.vue"),
      },
    ],
  },
  {
    path: "/preview/doctor",
    component: () => import("../views/DoctorLayout.vue"),
    children: [
      {
        path: "",
        redirect: "/preview/doctor/home",
      },
      {
        path: "home",
        name: "previewDoctorHome",
        component: () => import("../views/pages/DoctorWorkbench.vue"),
      },
      {
        path: "queue",
        name: "previewDoctorQueue",
        component: () => import("../views/pages/DoctorQueue.vue"),
      },
      {
        path: "reservations",
        name: "previewDoctorReservations",
        component: () => import("../views/pages/DoctorReservations.vue"),
      },
      {
        path: "order-records",
        name: "previewDoctorOrderRecords",
        component: () => import("../views/pages/DoctorOrderRecords.vue"),
      },
      {
        path: "create-order",
        name: "previewDoctorCreateOrder",
        component: () => import("../views/pages/DoctorCreateOrder.vue"),
      },
      {
        path: "users/:userId",
        name: "previewDoctorUserProfile",
        component: () => import("../views/pages/DoctorUserProfile.vue"),
      },
      {
        path: "orders/:orderId",
        name: "previewDoctorOrderDetail",
        component: () => import("../views/pages/DoctorOrderDetail.vue"),
      },
    ],
  },
];

export default doctorRoutes;
