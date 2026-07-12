/**
 * 医生端相关接口数据类型定义
 */
export interface DoctorStat {
  label: string;
  value: string | number;
  hint: string;
}

/**
 * 待接诊队列项数据类型定义
 */
export interface QueueItem {
  id: number;
  ownerId?: number;
  petId?: number;
  petName: string;
  ownerName: string;
  symptom: string;
  level: "普通" | "优先" | "紧急";
  arrivedAt: string;
  sex?: string;
  breed?: string;
  age?: string;
}

/**
 * 预约摘要项数据类型定义。
 * 列表只缓存这些轻量字段，完整联系人信息进入详情后按预约 id 获取。
 */
export interface ReservationSummaryItem {
  id: number;
  doctor_name: string;
  pet_name: string;
  reservation_type: string;
  date: string;
  time_slot: string;
  schedule: string;
  status: string;
}

/**
 * 预约详情项数据类型定义。
 */
export interface ReservationItem extends ReservationSummaryItem {
  user_id: number;
  user_name: string;
  phone: string;
  doctor_id: number;
  pet_id: number;
  created_at: string;
  price?: number;
}

export type OrderStatus =
  | "待付款"
  | "已付款"
  | "已取消"
  | "已退款"
  | "部分退款";

/**
 * 医生端创建订单记录时提交给后端的数据结构。
 */
export interface CreateOrderRecordPayload {
  queueId?: number;
  ownerId: number;
  petId: number;
  orderType: string;
  orderData: string;
  orderTotalPrice: number;
  orderMedicines: Array<{
    medicineId: number;
    medicineName: string;
    quantity: number;
    price: number;
    totalPrice: number;
    specification?: string;
    unit?: string;
    dosage?: string;
    frequency?: string;
    route?: string;
    durationDays?: number;
    instructions?: string;
  }>;
  medicalDocument: MedicalDocumentDraftFields;
}

export interface MedicalDocumentDraftFields {
  chiefComplaint: string;
  presentIllness: string;
  pastHistory: string;
  allergies: string;
  physicalExam: string;
  diagnosis: string;
  treatmentPlan: string;
  dischargeAdvice: string;
  followUpAt: string;
  structuredData?: Record<string, unknown>;
}

export interface MedicalPrescriptionItem {
  id?: number;
  medicineId: number;
  medicineName: string;
  specification: string;
  unit: string;
  dosage: string;
  frequency: string;
  route: string;
  durationDays: number;
  quantity: number;
  instructions: string;
  unitPrice: number;
  totalPrice: number;
}

export interface MedicalDocument extends MedicalDocumentDraftFields {
  id: number;
  documentNo: string;
  orderId: number;
  ownerId: number;
  ownerName: string;
  petId: number;
  petName: string;
  petType: string;
  petBreed: string;
  petAge: string;
  petSex: string;
  doctorId: number;
  doctorName: string;
  status: "draft" | "finalized" | "amended" | "voided";
  revisionNo: number;
  lockVersion: number;
  finalizedAt: string;
  createdAt: string;
  updatedAt: string;
  totalPrice: number;
  prescriptionItems: MedicalPrescriptionItem[];
}

export interface MedicalDocumentVersion {
  id: number;
  revisionNo: number;
  changeReason: string;
  contentHash: string;
  createdAt: string;
  hasPdf: boolean;
  pdfSha256: string;
  pdfByteSize: number;
  snapshot: Record<string, unknown>;
}

/**
 * 药品搜索结果项数据类型定义
 */
export interface MedicineSearchItem {
  id: number;
  name: string;
  type: string;
  price: number;
  stock: number;
  spec: string;
  selected?: boolean;
}

/**
 * 医生开药时选中的药品项数据类型定义
 */
export interface SelectedMedicineItem {
  id: number;
  name: string;
  type?: string;
  stock?: number;
  dosage: string;
  frequency?: string;
  route?: string;
  durationDays?: number;
  specification?: string;
  unit?: string;
  instructions?: string;
  days: number;
  unitPrice: number;
  subtotal: number;
}

/**
 * 医生值班状态数据类型定义
 */
export interface DoctorDutyStatus {
  is_online: boolean;
  date: string;
  check_in_time: string;
  check_out_time: string;
  status: "online" | "offline";
}

/**
 * 医生订单中药品项数据类型定义
 */
export interface OrderMedicineItem {
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

/**
 * 订单摘要项数据类型定义。
 * 列表页只保存轻量字段，完整药品明细进入详情页后按订单 id 单独获取。
 */
export interface OrderSummaryItem {
  id: number;
  pet_name: string;
  doctor_name: string;
  order_type: string;
  order_data: string;
  order_status: OrderStatus;
  order_totalprice: number;
}

/**
 * 订单详情项数据类型定义，进入详情页时按订单 id 从后端获取。
 */
export interface OrderDetailItem extends OrderSummaryItem {
  owner_id: number;
  owner_name: string;
  pet_id: number;
  pet_type: string;
  pet_age: string;
  pet_sex: string;
  doctor_id: number;
  created_at: string;
  updated_at: string;
  orderMedicines: OrderMedicineItem[];
}

/**
 * 医生端用户详情页的宠物摘要。
 */
export interface DoctorUserProfilePetSummary {
  id: number;
  pet_name: string;
  pet_type: string;
  pet_sex: string;
  pet_age?: string;
  pet_breed?: string;
}

/**
 * 医生端管理用户宠物档案时使用的完整宠物数据。
 */
export interface DoctorManagedPetProfile {
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

/**
 * 医生端用户详情数据，基础信息完整展示，宠物与订单只保留摘要。
 */
export interface DoctorUserProfile {
  id: number;
  type_id: number;
  type_name: string;
  name: string;
  phone: string;
  email: string;
  birthday: string;
  head_image: string;
  user_specialty: string;
  user_introduction: string;
  user_level: number;
  funds: number;
  created_at: string;
  pets: DoctorUserProfilePetSummary[];
  orders: OrderSummaryItem[];
}

/**
 * 医生端用户摘要中的宠物项。
 */
export interface DoctorUserSummaryPet {
  id: number;
  pet_name: string;
}

/**
 * 医生端用户摘要数据。
 * 用户列表卡片只展示基础信息，完整档案进入详情页后再按用户 id 获取。
 */
export interface DoctorUserSummary {
  id: number;
  type_id: number;
  name: string;
  phone: string;
  email: string;
  head_image: string;
  pets: DoctorUserSummaryPet[];
}

export interface DoctorDataItem {
  id: number;
  name: string;
  specialty: string;
  status?: string;
}
