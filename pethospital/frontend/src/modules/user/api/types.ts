import { DoctorDataItem } from "@/modules/doctor/api/types";

export interface PetProfile {
  id: string;
  name: string;
  species: string;
  breed: string;
  age: string;
  gender: string;
  neutered: string;
  vaccineStatus: string;
  preference: string;
  notes: string;
}

export interface ReservationSummary {
  id: number;
  doctor_name: string;
  pet_name: string;
  reservation_type: string;
  date: string;
  time_slot: string;
  schedule: string;
  status: string;
}

export interface ReservationOrderRecordItem extends ReservationSummary {
  user_id: number;
  user_name: string;
  phone: string;
  doctor_id: number;
  pet_id: number;
  created_at: string;
  price: number;
}

export interface OrderSummary {
  id: number;
  pet_name: string;
  doctor_name: string;
  order_type: string;
  order_data: string;
  order_status: string;
  order_totalprice: number;
  created_at: string;
}

export interface OrderMedicine {
  id: number;
  order_id: number;
  medicine_id: number;
  medicine_name: string;
  medicine_type: string;
  quantity: number;
  price: number;
  total_price: number;
  created_at: string;
  updated_at: string;
}

export interface OrderDetail extends OrderSummary {
  owner_id: number;
  owner_name: string;
  pet_id: number;
  pet_type: string;
  pet_age: string;
  pet_sex: string;
  doctor_id: number;
  updated_at: string;
  orderMedicines: OrderMedicine[];
}

export interface ReservationScheduleState {
  doctorData: DoctorDataItem[];
  year: string[];
  month: string[];
  day: string[];
  weekday: string[];
  slots: string[][];
}

/**
 * 用户订单记录接口返回的预约时间表项结构。
 */
export interface ReservationScheduleResponseItem {
  year: string | number;
  date: string;
  weekday: string;
  time_slots?: Record<string, unknown>;
}
