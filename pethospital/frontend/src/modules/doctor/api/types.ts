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
 * 预约项数据类型定义
 */
export interface ReservationItem {
  id: number;
  petName: string;
  ownerName: string;
  phone: string;
  doctorName: string;
  schedule: string;
  project: string;
  status: "待确认" | "已确认" | "已到院";
}

/**
 * 订单记录项数据类型定义
 */
export interface OrderRecordItem {
  id: string;
  petName: string;
  ownerName: string;
  createdAt: string;
  medicineCount: number;
  totalFee: number;
  status: "待付款" | "已完成" | "已取消";
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
export interface DoctorOrderMedicineItem {
  id: number;
  name: string;
  dosage: string;
  quantity: number;
  price: number;
}

/**
 * 医生订单数据类型定义
 */
export interface DoctorOrderSummaryItem {
  id: string;
  petId: string;
  ownerName: string;
  petName: string;
  createdAt: string;
  totalFee: number;
  status: "待付款" | "已完成" | "已取消";
}

/**
 * 医生订单详情数据类型定义，包含订单的基本信息和药品列表
 */
export interface DoctorOrderDetailItem extends DoctorOrderSummaryItem {
  doctorName: string;
  symptom: string;
  diagnosis: string;
  remark: string;
  medicines: DoctorOrderMedicineItem[];
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
  orders: DoctorOrderDetailItem[]; // 订单详情列表
}

export interface DoctorDataItem {
  id: number;
  name: string;
  specialty: string;
  status?: string;
}
