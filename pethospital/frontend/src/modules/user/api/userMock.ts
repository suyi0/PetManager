import { DoctorDataItem } from "@/modules/doctor/api/types";

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
