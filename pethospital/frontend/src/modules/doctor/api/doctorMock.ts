import {
  DoctorStat,
  DoctorUserProfile,
  MedicineSearchItem,
  OrderRecordItem,
  QueueItem,
  ReservationItem,
  SelectedMedicineItem,
} from "./types";

/**
 * 医生工作台数据统计项列表
 *  - label: 显示在界面上的统计项名称
 *  - value: 统计项的数值
 *  - hint: 对统计项数值的简要说明，显示在界面上以辅助理解数据来源和含义
 */
export const doctorWorkbenchStats: DoctorStat[] = [
  { label: "今日待接诊", value: 12, hint: "来自接诊队列" },
  { label: "今日预约", value: 8, hint: "待确认 + 已确认" },
  { label: "已开诊单", value: 16, hint: "今日订单记录" },
];

/**
 * 待接诊队列，展示最近到院且等待接诊的宠物。
 */
export const queueItemsMock: QueueItem[] = [
  {
    id: 201,
    petName: "可乐",
    ownerName: "周女士",
    symptom: "呕吐、精神不振",
    level: "紧急",
    arrivedAt: "09:45",
    sex: "公",
    breed: "柯基",
    age: "4岁",
  },
  {
    id: 202,
    petName: "团子",
    ownerName: "李先生",
    symptom: "腹泻两天",
    level: "优先",
    arrivedAt: "10:05",
    sex: "母",
    breed: "布偶猫",
    age: "2岁",
  },
  {
    id: 203,
    petName: "糯米",
    ownerName: "吴女士",
    symptom: "免疫复诊",
    level: "普通",
    arrivedAt: "10:20",
    sex: "母",
    breed: "银渐层",
    age: "1岁",
  },
];

/**
 * 预约列表，包含预约的宠物信息、预约项目和当前状态
 */
export const reservationItemsMock: ReservationItem[] = [
  {
    id: 101,
    petName: "团子",
    ownerName: "李先生",
    phone: "13800138000",
    doctorName: "林安 医师",
    schedule: "2026-03-10 14:00",
    project: "肠胃检查",
    status: "待确认",
  },
  {
    id: 102,
    petName: "糯米",
    ownerName: "吴女士",
    phone: "13666661234",
    doctorName: "林安 医师",
    schedule: "2026-03-10 15:30",
    project: "免疫复诊",
    status: "已确认",
  },
  {
    id: 103,
    petName: "小七",
    ownerName: "王女士",
    phone: "13988884321",
    doctorName: "林安 医师",
    schedule: "2026-03-10 16:10",
    project: "呼吸道复查",
    status: "已到院",
  },
];

/**
 * 订单记录列表，包含订单的基本信息和当前状态
 */
export const orderRecordItemsMock: OrderRecordItem[] = [
  {
    id: "ZD-0310-08",
    petName: "可乐",
    ownerName: "周女士",
    createdAt: "2026-03-10 10:12",
    medicineCount: 2,
    totalFee: 41.5,
    status: "待付款",
  },
  {
    id: "ZD-0310-05",
    petName: "橘子",
    ownerName: "刘先生",
    createdAt: "2026-03-10 09:03",
    medicineCount: 3,
    totalFee: 86,
    status: "已完成",
  },
  {
    id: "ZD-0309-18",
    petName: "米粒",
    ownerName: "黄女士",
    createdAt: "2026-03-09 18:40",
    medicineCount: 1,
    totalFee: 23,
    status: "已取消",
  },
];

/**
 * 仓库药品列表，包含药品的基本信息和库存状态
 */
export const medicineSearchItemsMock: MedicineSearchItem[] = [
  {
    id: 1,
    name: "胃复安片",
    type: "止吐类",
    price: 23,
    stock: 28,
    spec: "12片 / 盒",
    selected: true,
  },
  {
    id: 2,
    name: "蒙脱石散",
    type: "肠胃保护",
    price: 18.5,
    stock: 44,
    spec: "10袋 / 盒",
    selected: false,
  },
  {
    id: 3,
    name: "益生菌粉",
    type: "调理类",
    price: 36,
    stock: 15,
    spec: "20袋 / 盒",
    selected: false,
  },
];

/**
 * 选中的药品列表，包含药品的基本信息和数量
 */
export const selectedMedicinesMock: SelectedMedicineItem[] = [
  {
    id: 1,
    name: "胃复安片",
    dosage: "2片 / 次",
    days: 3,
    unitPrice: 23,
    subtotal: 23,
  },
  {
    id: 2,
    name: "蒙脱石散",
    dosage: "1袋 / 次",
    days: 3,
    unitPrice: 18.5,
    subtotal: 18.5,
  },
];

/**
 * 医生获取到的用户基本信息列表，包含用户的个人资料、宠物信息和订单记录等详细数据
 */
export const userProfilesMock: DoctorUserProfile[] = [
  {
    id: "owner-zhou",
    ownerName: "周女士",
    phone: "1380000001",
    email: "zhou@example.com",
    address: "滨江区春潮路 18 号 2-201",
    memberLevel: "金卡会员",
    balance: 328.5,
    note: "偏好短信通知，复诊响应快。",
    pets: [
      {
        id: "pet-cola",
        name: "可乐",
        species: "犬",
        breed: "柯基",
        age: "4岁",
        sex: "公",
        weight: "11.2kg",
        orderIds: ["ZD-0310-08", "ZD-0227-03"],
      },
      {
        id: "pet-orange",
        name: "橘子",
        species: "猫",
        breed: "橘猫",
        age: "2岁",
        sex: "母",
        weight: "4.3kg",
        orderIds: ["ZD-0215-06"],
      },
    ],
    orders: [
      {
        id: "ZD-0310-08",
        petId: "pet-cola",
        ownerName: "周女士",
        petName: "可乐",
        createdAt: "2026-03-10 10:12",
        totalFee: 41.5,
        status: "待付款",
        doctorName: "林安 医师",
        symptom: "呕吐、精神不振",
        diagnosis: "急性肠胃炎",
        remark: "建议清淡饮食 3 天并复查体温。",
        medicines: [
          {
            id: 1,
            name: "胃复安片",
            dosage: "2片 / 次",
            quantity: 1,
            price: 23,
          },
          {
            id: 2,
            name: "蒙脱石散",
            dosage: "1袋 / 次",
            quantity: 1,
            price: 18.5,
          },
        ],
      },
      {
        id: "ZD-0227-03",
        petId: "pet-cola",
        ownerName: "周女士",
        petName: "可乐",
        createdAt: "2026-02-27 15:18",
        totalFee: 68,
        status: "已完成",
        doctorName: "林安 医师",
        symptom: "食欲下降",
        diagnosis: "消化功能紊乱",
        remark: "补水观察，72 小时后回访。",
        medicines: [
          {
            id: 3,
            name: "益生菌粉",
            dosage: "1袋 / 次",
            quantity: 1,
            price: 36,
          },
          {
            id: 4,
            name: "肠胃舒口服液",
            dosage: "5ml / 次",
            quantity: 1,
            price: 32,
          },
        ],
      },
      {
        id: "ZD-0215-06",
        petId: "pet-orange",
        ownerName: "周女士",
        petName: "橘子",
        createdAt: "2026-02-15 09:03",
        totalFee: 86,
        status: "已完成",
        doctorName: "林安 医师",
        symptom: "挑食、轻微掉毛",
        diagnosis: "营养失衡",
        remark: "调整粮食配比，两周后复查皮毛情况。",
        medicines: [
          {
            id: 5,
            name: "复合维生素",
            dosage: "1粒 / 日",
            quantity: 1,
            price: 28,
          },
          {
            id: 6,
            name: "皮毛营养膏",
            dosage: "3cm / 日",
            quantity: 1,
            price: 58,
          },
        ],
      },
    ],
  },
  {
    id: "owner-zhang",
    ownerName: "张先生",
    phone: "1380000002",
    email: "zhang@example.com",
    address: "西湖区晴川路 66 号 1-502",
    memberLevel: "标准会员",
    balance: 119.2,
    note: "习惯周末到院，优先安排晚间复诊。",
    pets: [
      {
        id: "pet-milk",
        name: "奶糖",
        species: "犬",
        breed: "比熊",
        age: "3岁",
        sex: "母",
        weight: "6.1kg",
        orderIds: ["ZD-0312-04", "ZD-0301-11"],
      },
    ],
    orders: [
      {
        id: "ZD-0312-04",
        petId: "pet-milk",
        ownerName: "张先生",
        petName: "奶糖",
        createdAt: "2026-03-12 11:26",
        totalFee: 58,
        status: "待付款",
        doctorName: "林安 医师",
        symptom: "皮肤瘙痒复查",
        diagnosis: "过敏性皮炎恢复期",
        remark: "继续佩戴伊丽莎白圈，避免舔咬。",
        medicines: [
          {
            id: 7,
            name: "外用喷剂",
            dosage: "每日2次",
            quantity: 1,
            price: 58,
          },
        ],
      },
      {
        id: "ZD-0301-11",
        petId: "pet-milk",
        ownerName: "张先生",
        petName: "奶糖",
        createdAt: "2026-03-01 16:05",
        totalFee: 132,
        status: "已完成",
        doctorName: "林安 医师",
        symptom: "局部皮肤泛红、抓挠",
        diagnosis: "过敏性皮炎",
        remark: "建议更换洗护用品并保持环境干燥。",
        medicines: [
          {
            id: 8,
            name: "抗过敏片",
            dosage: "1片 / 次",
            quantity: 1,
            price: 45,
          },
          { id: 9, name: "药浴液", dosage: "每周2次", quantity: 1, price: 87 },
        ],
      },
    ],
  },
  {
    id: "owner-li",
    ownerName: "李先生",
    phone: "1390000004",
    email: "li@example.com",
    address: "拱墅区清远街 9 号 3-902",
    memberLevel: "银卡会员",
    balance: 452,
    note: "长期在本院进行肠胃方向复诊。",
    pets: [
      {
        id: "pet-dumpling",
        name: "团子",
        species: "猫",
        breed: "英短",
        age: "5岁",
        sex: "公",
        weight: "5.8kg",
        orderIds: ["ZD-0314-02", "ZD-0210-09"],
      },
      {
        id: "pet-seven",
        name: "小七",
        species: "猫",
        breed: "狸花",
        age: "1岁",
        sex: "母",
        weight: "3.7kg",
        orderIds: ["ZD-0308-10"],
      },
    ],
    orders: [
      {
        id: "ZD-0314-02",
        petId: "pet-dumpling",
        ownerName: "李先生",
        petName: "团子",
        createdAt: "2026-03-14 14:40",
        totalFee: 96,
        status: "已完成",
        doctorName: "林安 医师",
        symptom: "肠胃检查复诊",
        diagnosis: "慢性胃炎稳定期",
        remark: "饮食继续分餐，避免高油脂零食。",
        medicines: [
          {
            id: 10,
            name: "胃黏膜保护剂",
            dosage: "1包 / 次",
            quantity: 2,
            price: 48,
          },
        ],
      },
      {
        id: "ZD-0210-09",
        petId: "pet-dumpling",
        ownerName: "李先生",
        petName: "团子",
        createdAt: "2026-02-10 13:12",
        totalFee: 123,
        status: "已完成",
        doctorName: "林安 医师",
        symptom: "呕吐、软便",
        diagnosis: "胃肠应激",
        remark: "观察精神状态，如复发需复查血常规。",
        medicines: [
          {
            id: 11,
            name: "止吐针剂",
            dosage: "院内注射",
            quantity: 1,
            price: 55,
          },
          {
            id: 12,
            name: "调理处方粮",
            dosage: "7日装",
            quantity: 1,
            price: 68,
          },
        ],
      },
      {
        id: "ZD-0308-10",
        petId: "pet-seven",
        ownerName: "李先生",
        petName: "小七",
        createdAt: "2026-03-08 16:55",
        totalFee: 39,
        status: "已取消",
        doctorName: "林安 医师",
        symptom: "呼吸道复查",
        diagnosis: "上呼吸道感染观察",
        remark: "主人改约，建议一周内重新到院。",
        medicines: [
          {
            id: 13,
            name: "雾化套餐",
            dosage: "院内处理",
            quantity: 1,
            price: 39,
          },
        ],
      },
    ],
  },
];
