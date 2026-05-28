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
 * 订单摘要项数据类型定义。
 * 列表页只保存轻量字段，完整药品明细进入详情页后按订单 id 单独获取。
 */
export interface OrderRecordItem {
  id: number;
  pet_name: string;
  doctor_name: string;
  order_type: string;
  order_data: string;
  order_status: OrderStatus;
  order_totalprice: number;
  created_at: string;
}

/**
 * 医生端创建订单记录时提交给后端的数据结构。
 */
export interface CreateOrderRecordPayload {
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
  }>;
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
  dosage: string;
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
 * 订单详情项数据类型定义，进入详情页时按订单 id 从后端获取。
 */
export interface OrderDetailItem extends OrderRecordItem {
  owner_id: number;
  owner_name: string;
  pet_id: number;
  pet_type: string;
  pet_age: string;
  pet_sex: string;
  doctor_id: number;
  updated_at: string;
  orderMedicines: OrderMedicineItem[];
}

/**
 * 用户档案页面的旧订单简略项。
 * 该页面仍来自医生用户档案接口，不参与医生订单摘要缓存。
 */
export interface LegacyOrderRecordItem {
  id: string;
  petId?: string;
  petName: string;
  ownerName: string;
  doctorName?: string;
  createdAt: string;
  totalFee: number;
  status: OrderStatus;
  symptom?: string;
  diagnosis?: string;
  remark?: string;
  medicines?: Array<{
    id: number;
    name: string;
    dosage: string;
    quantity: number;
    price: number;
  }>;
}

/**
 * 医生端宠物档案数据类型定义
 */
export interface DoctorPetProfile {
  id: string;
  name: string;
  species: string;
  breed: string;
  age: string;
  sex: string;
  weight: string;
  orderIds: string[];
}

/**
 * 医生端用户档案数据类型定义，包含用户的基本信息、宠物档案列表和订单详情列表
 */
export interface DoctorUserProfile {
  id: string; // 用户ID
  ownerName: string; // 用户名
  phone: string; // 手机号
  email: string; // 邮箱
  address: string; // 地址
  memberLevel: string; // 会员等级
  balance: number; // 账户余额
  note: string; // 备注信息
  pets: DoctorPetProfile[]; // 宠物档案列表
  orders: LegacyOrderRecordItem[]; // 订单简略列表
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
