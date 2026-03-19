export interface DoctorStat {
  label: string;
  value: string | number;
  hint: string;
}

export interface QueueItem {
  id: number;
  petName: string;
  ownerName: string;
  symptom: string;
  level: "普通" | "优先" | "紧急";
  arrivedAt: string;
}

export interface ReservationItem {
  id: number;
  petName: string;
  ownerName: string;
  doctorName: string;
  schedule: string;
  project: string;
  status: "待确认" | "已确认" | "已到院";
}

export interface OrderRecordItem {
  id: string;
  petName: string;
  ownerName: string;
  createdAt: string;
  medicineCount: number;
  totalFee: number;
  status: "待付款" | "已完成" | "已取消";
}

export interface MedicineSearchItem {
  id: number;
  name: string;
  type: string;
  price: number;
  stock: number;
  spec: string;
  selected?: boolean;
}

export interface SelectedMedicineItem {
  id: number;
  name: string;
  dosage: string;
  days: number;
  unitPrice: number;
  subtotal: number;
}

export interface DoctorDutyStatus {
  is_online: boolean;
  date: string;
  check_in_time: string;
  check_out_time: string;
  status: "online" | "offline";
}

export interface DoctorOrderMedicineItem {
  id: number;
  name: string;
  dosage: string;
  quantity: number;
  price: number;
}

export interface DoctorOrderSummaryItem {
  id: string;
  petId: string;
  ownerName: string;
  petName: string;
  createdAt: string;
  totalFee: number;
  status: "待付款" | "已完成" | "已取消";
}

export interface DoctorOrderDetailItem extends DoctorOrderSummaryItem {
  doctorName: string;
  symptom: string;
  diagnosis: string;
  remark: string;
  medicines: DoctorOrderMedicineItem[];
}

export interface DoctorPetProfile {
  id: string;
  name: string;
  species: string;
  breed: string;
  age: string;
  gender: string;
  weight: string;
  orderIds: string[];
}

export interface DoctorUserProfile {
  id: string;
  ownerName: string;
  phone: string;
  email: string;
  address: string;
  memberLevel: string;
  balance: number;
  note: string;
  pets: DoctorPetProfile[];
  orders: DoctorOrderDetailItem[];
}
