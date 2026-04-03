import { DoctorDataItem } from "@/modules/doctor/api/types";
import { OrderRecordItem, OrderSummary } from "@/modules/user/api/types";

export const userOrderRecordsMock: OrderRecordItem[] = [
  {
    id: 1001,
    name: "可乐复诊订单",
    time: 1772390400000,
    price: 41.5,
  },
  {
    id: 1002,
    name: "橘子营养评估",
    time: 1772304000000,
    price: 86,
  },
];

export const userOrderListMock: OrderSummary[] = [
  {
    id: 1001,
    pet_id: 1,
    pet_name: "可乐",
    doctor_id: 102,
    order_type: "门诊",
    order_data: "呕吐复诊，肠胃炎恢复观察",
    order_status: "待付款",
    order_totalprice: 41.5,
    created_at: "2026-03-31 10:00:00",
  },
  {
    id: 1002,
    pet_id: 2,
    pet_name: "橘子",
    doctor_id: 102,
    order_type: "复查",
    order_data: "营养评估与毛发复查",
    order_status: "已完成",
    order_totalprice: 86,
    created_at: "2026-03-30 09:20:00",
  },
];

export const reservationDoctorsMock: DoctorDataItem[] = [
  {
    id: 102,
    name: "林安 医师",
    specialty: "内科 / 肠胃方向",
    status: "offline",
  },
  {
    id: 108,
    name: "周衡 医师",
    specialty: "皮肤科 / 复诊",
    status: "online",
  },
];

export const reservationScheduleMock = [
  {
    year: "2026",
    date: "04-03",
    weekday: "周五",
    time_slots: {
      am1: "09:00",
      am2: "10:30",
      am3: "11:30",
      pm1: "14:00",
    },
  },
  {
    year: "2026",
    date: "04-04",
    weekday: "周六",
    time_slots: {
      am1: "09:30",
      am2: "11:00",
      pm1: "15:00",
    },
  },
];

export const userReservationRecordsMock: OrderRecordItem[] = [
  {
    id: 2001,
    name: "可乐门诊预约",
    time: 1772476800000,
    price: 0,
  },
  {
    id: 2002,
    name: "橘子复查预约",
    time: 1772563200000,
    price: 0,
  },
];
