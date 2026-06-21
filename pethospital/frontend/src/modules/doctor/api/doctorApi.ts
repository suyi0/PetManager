import http from "@/api/http";
import { unwrapMessage, unwrapList } from "@/api/response";
import {
  CreateOrderRecordPayload,
  DoctorManagedPetProfile,
  DoctorDutyStatus,
  DoctorUserProfile,
  DoctorUserSummary,
  MedicineSearchItem,
  OrderDetailItem,
  QueueItem,
  OrderSummaryItem,
  ReservationItem,
  ReservationSummaryItem,
} from "./types";

/**
 * 解包数据，提取其中的 data 字段
 * @param payload 接受一个未知类型的响应数据，尝试从中提取 data 字段。
 * @returns 返回提取出的 data 字段，如果不存在则返回 null。
 */
const unwrapData = <T>(payload: unknown): T | null => {
  if (payload && typeof payload === "object" && "data" in payload) {
    return (payload as { data?: T }).data ?? null;
  }

  return (payload as T) ?? null;
};

/**
 * 从宠物档案接口响应中提取数组数据。
 */
const unwrapDoctorPetProfiles = (
  payload: unknown
): DoctorManagedPetProfile[] => {
  const rows = unwrapData<unknown>(payload);
  return Array.isArray(rows) ? (rows as DoctorManagedPetProfile[]) : [];
};

/**
 * 将值转换为数字，如果无法转换则返回 undefined
 * @param value 接受一个未知类型的值，尝试将其转换为数字。
 * @returns 返回转换后的数字，如果输入值无法转换为有效数字，则返回 undefined。
 */
const toNumber = (value: unknown) => {
  const numericValue = Number(value);

  return Number.isFinite(numericValue) ? numericValue : undefined;
};

/**
 * 兼容后端可能返回的 snake_case 字段，统一成前端队列项需要的 camelCase。
 */
export const normalizeQueueItems = (items: QueueItem[]) =>
  items.map((item) => {
    const source = item as QueueItem & Record<string, unknown>;

    return {
      ...item,
      ownerId: toNumber(source.ownerId ?? source.owner_id ?? source.userId),
      petId: toNumber(source.petId ?? source.pet_id),
      petName: String(source.petName ?? source.pet_name ?? ""),
      ownerName: String(source.ownerName ?? source.owner_name ?? ""),
      symptom: String(source.symptom ?? ""),
      level: String(
        source.level ?? source.triage_level ?? "普通"
      ) as QueueItem["level"],
      arrivedAt: String(source.arrivedAt ?? source.arrived_at ?? ""),
      sex: String(source.sex ?? ""),
      breed: String(source.breed ?? ""),
      age: String(source.age ?? ""),
    };
  });

const normalizeOrderStatus = (
  value: unknown
): OrderSummaryItem["order_status"] =>
  String(value || "待付款") as OrderSummaryItem["order_status"];

/**
 * 将公共订单摘要接口返回的数据统一成医生端订单摘要结构。
 */
const normalizeOrderSummaryItems = (items: Array<Record<string, unknown>>) =>
  items.map((item) => {
    return {
      id: toNumber(item.id) ?? 0,
      pet_name: String(item.pet_name ?? item.petName ?? ""),
      doctor_name: String(item.doctor_name ?? item.doctorName ?? ""),
      order_type: String(item.order_type ?? item.orderType ?? ""),
      order_data: String(item.order_data ?? item.orderData ?? ""),
      order_status: normalizeOrderStatus(item.order_status ?? item.status),
      order_totalprice:
        toNumber(item.order_totalprice ?? item.totalFee ?? item.total_fee) ?? 0,
    };
  });

/**
 * 将创建订单接口返回的数据转为订单摘要，方便立即插入医生端订单摘要缓存。
 */
const normalizeOrderSummaryItem = (
  item: Record<string, unknown>
): OrderSummaryItem => normalizeOrderSummaryItems([item])[0];

const normalizeReservationSummaries = (
  items: Array<Record<string, unknown>>
): ReservationSummaryItem[] =>
  items.map((item) => {
    const date = String(item.date ?? "");
    const timeSlot = String(item.time_slot ?? item.timeSlot ?? "");

    return {
      id: toNumber(item.id) ?? 0,
      pet_name: String(item.pet_name ?? item.petName ?? ""),
      doctor_name: String(item.doctor_name ?? item.doctorName ?? ""),
      reservation_type: String(
        item.reservation_type ?? item.reservationType ?? ""
      ),
      date,
      time_slot: timeSlot,
      schedule:
        String(item.schedule ?? "") ||
        [date, timeSlot].filter(Boolean).join(" "),
      status: String(item.status ?? "预约成功"),
    };
  });

const normalizeReservationDetail = (
  detail: ReservationItem | null
): ReservationItem | null => {
  if (!detail) {
    return null;
  }

  return {
    ...detail,
    schedule:
      detail.schedule ||
      [detail.date, detail.time_slot].filter(Boolean).join(" "),
    price: typeof detail.price === "number" ? detail.price : 0,
  };
};

/**
 * 将医生端用户摘要接口返回的数据统一成前端列表卡片需要的结构。
 */
const normalizeDoctorUserSummaries = (
  items: Array<Record<string, unknown>>
): DoctorUserSummary[] =>
  items.map((item) => ({
    id: toNumber(item.id) ?? 0,
    type_id: toNumber(item.type_id ?? item.typeId) ?? 0,
    name: String(item.name ?? ""),
    phone: String(item.phone ?? ""),
    email: String(item.email ?? ""),
    head_image: String(item.head_image ?? item.headImage ?? ""),
    pets: Array.isArray(item.pets)
      ? item.pets
          .map((pet) => {
            const source = pet as Record<string, unknown>;

            return {
              id: toNumber(source.id) ?? 0,
              pet_name: String(source.pet_name ?? source.petName ?? ""),
            };
          })
          .filter((pet) => pet.id > 0 || pet.pet_name)
      : [],
  }));

const normalizeMedicineSearchItems = (
  items: Array<Record<string, unknown>>
): MedicineSearchItem[] =>
  items
    .map((item) => ({
      id: toNumber(item.id) ?? 0,
      name: String(item.name ?? item.item_name ?? ""),
      type: String(item.type ?? item.item_type ?? ""),
      price: toNumber(item.price ?? item.item_price) ?? 0,
      stock: toNumber(item.stock ?? item.item_number) ?? 0,
      spec: String(item.spec ?? item.item_spec ?? ""),
    }))
    .filter((item) => item.id > 0 && item.stock > 0);

type DoctorUserSearchPayload = {
  data: string;
  identifier: "name" | "phone";
};

const buildDoctorUserSearchPayload = (
  keyword: string
): DoctorUserSearchPayload => {
  const normalizedKeyword = keyword.trim();
  const digitKeyword = keyword.replace(/\D/g, "");
  const phoneKeywordPattern = /^[+\d\s().-]+$/;

  if (digitKeyword.length >= 4 && phoneKeywordPattern.test(normalizedKeyword)) {
    return {
      data:
        digitKeyword.length === 13 && digitKeyword.startsWith("86")
          ? digitKeyword.slice(2)
          : digitKeyword,
      identifier: "phone",
    };
  }

  return {
    data: normalizedKeyword,
    identifier: "name",
  };
};

/**
 * 将医生端用户详情接口返回的数据统一成页面直接使用的结构。
 */
const normalizeDoctorUserProfile = (
  item: Record<string, unknown>
): DoctorUserProfile => ({
  id: toNumber(item.id) ?? 0,
  type_id: toNumber(item.type_id ?? item.typeId) ?? 0,
  type_name: String(item.type_name ?? item.typeName ?? ""),
  name: String(item.name ?? item.ownerName ?? ""),
  phone: String(item.phone ?? ""),
  email: String(item.email ?? ""),
  birthday: String(item.birthday ?? ""),
  head_image: String(item.head_image ?? item.headImage ?? ""),
  user_specialty: String(item.user_specialty ?? item.userSpecialty ?? ""),
  user_introduction: String(
    item.user_introduction ?? item.userIntroduction ?? ""
  ),
  user_level: toNumber(item.user_level ?? item.userLevel) ?? 0,
  funds: toNumber(item.funds ?? item.salary) ?? 0,
  created_at: String(item.created_at ?? item.createdAt ?? ""),
  pets: Array.isArray(item.pets)
    ? item.pets.map((pet) => {
        const source = pet as Record<string, unknown>;

        return {
          id: toNumber(source.id) ?? 0,
          pet_name: String(source.pet_name ?? source.petName ?? ""),
          pet_type: String(source.pet_type ?? source.petType ?? ""),
          pet_age: String(source.pet_age ?? source.petAge ?? ""),
          pet_sex: String(source.pet_sex ?? source.petSex ?? ""),
          pet_breed: String(source.pet_breed ?? source.petBreed ?? ""),
        };
      })
    : [],
  orders: Array.isArray(item.orders)
    ? item.orders.map((order) => {
        const source = order as Record<string, unknown>;

        return {
          id: toNumber(source.id) ?? 0,
          pet_name: String(source.pet_name ?? source.petName ?? ""),
          doctor_name: String(source.doctor_name ?? source.doctorName ?? ""),
          order_type: String(source.order_type ?? source.orderType ?? ""),
          order_data: String(source.order_data ?? source.orderData ?? ""),
          order_status: normalizeOrderStatus(
            source.order_status ?? source.status
          ),
          order_totalprice:
            toNumber(
              source.order_totalprice ?? source.totalFee ?? source.total_fee
            ) ?? 0,
        };
      })
    : [],
});

/**
 * 从订单详情接口响应中提取完整订单信息。
 */
const unwrapOrderDetail = (payload: unknown) => {
  const detail = unwrapData<OrderDetailItem>(payload);
  return detail && typeof detail === "object" ? detail : null;
};

export const doctorApi = {
  /**
   * 获取医生值班状态
   * @returns 直接返回医生值班状态对象，包含是否在线和当前日期等信息
   */
  async getDutyStatus(): Promise<DoctorDutyStatus> {
    const { data } = await http.get("/api/doctors/duty-status");

    if (data && typeof data === "object" && !Array.isArray(data)) {
      return data as DoctorDutyStatus;
    }

    return (
      unwrapList<DoctorDutyStatus>(data)[0] ?? {
        is_online: false,
        date: "",
        check_in_time: "",
        check_out_time: "",
        status: "offline",
      }
    );
  },

  /**
   * 按用户编号获取医生端用户详情。
   */
  async getUserProfiles(userId: number): Promise<DoctorUserProfile | null> {
    const { data } = await http.get(`/api/doctors/user-profiles/${userId}`);
    const profile = unwrapList<Record<string, unknown>>(data)[0];

    return profile ? normalizeDoctorUserProfile(profile) : null;
  },

  /**
   * 获取指定用户的完整宠物档案列表，供医生端用户档案管理页使用。
   */
  async getUserPetProfiles(userId: number): Promise<DoctorManagedPetProfile[]> {
    const { data } = await http.get(
      `/api/doctors/user-profiles/${userId}/pet-profiles`
    );

    return unwrapDoctorPetProfiles(data);
  },

  /**
   * 为指定用户创建宠物档案。
   */
  async createUserPetProfile(
    userId: number,
    payload: Omit<DoctorManagedPetProfile, "id">
  ): Promise<DoctorManagedPetProfile | null> {
    const { data } = await http.post(
      `/api/doctors/user-profiles/${userId}/pet-profiles`,
      payload
    );

    return unwrapDoctorPetProfiles(data)[0] ?? null;
  },

  /**
   * 更新指定用户的一条宠物档案。
   */
  async updateUserPetProfile(
    userId: number,
    payload: DoctorManagedPetProfile
  ): Promise<DoctorManagedPetProfile | null> {
    const { data } = await http.put(
      `/api/doctors/user-profiles/${userId}/pet-profiles/${payload.id}`,
      payload
    );

    return unwrapDoctorPetProfiles(data)[0] ?? null;
  },

  /**
   * 删除指定用户的一条宠物档案。
   */
  async deleteUserPetProfile(userId: number, petId: string): Promise<void> {
    await http.delete(
      `/api/doctors/user-profiles/${userId}/pet-profiles/${petId}`
    );
  },

  /**
   * 按关键词搜索医生端用户摘要列表。
   */
  async getUserList(searchByKeyword: string): Promise<DoctorUserSummary[]> {
    const { data } = await http.post(
      "/api/doctors/user-summaries",
      buildDoctorUserSearchPayload(searchByKeyword)
    );

    return normalizeDoctorUserSummaries(
      unwrapList<Record<string, unknown>>(data)
    );
  },

  /**
   * 按关键词搜索可开单药品。
   */
  async searchMedicines(keyword: string): Promise<MedicineSearchItem[]> {
    const { data } = await http.post("/api/doctors/medicine-search", {
      keyword,
    });

    return normalizeMedicineSearchItems(
      unwrapList<Record<string, unknown>>(data)
    );
  },

  /**
   * 获取待接诊队列信息
   * @returns 返回待接诊队列列表；接口为空时返回空列表。
   */
  async getReservationsSummary(): Promise<ReservationSummaryItem[]> {
    const { data } = await http.get("/api/doctors/reservation-summaries");
    const reservationItems = normalizeReservationSummaries(
      unwrapList<Record<string, unknown>>(data)
    );

    return reservationItems;
  },

  /**
   * 使用后端搜索医生端预约摘要，searchType 固定为 reservations。
   */
  async searchReservationSummaries(
    searchByKeyword: string
  ): Promise<ReservationSummaryItem[]> {
    const { data } = await http.post("/api/doctors/search-keyword", {
      searchType: "reservations",
      searchByKeyword,
    });

    return normalizeReservationSummaries(
      unwrapList<Record<string, unknown>>(data)
    );
  },

  /**
   * 按预约编号获取完整预约详情。
   */
  async getReservationInformation(
    reservationId: number
  ): Promise<ReservationItem | null> {
    const { data } = await http.get(
      `/api/doctors/reservations/${reservationId}/information`
    );
    return normalizeReservationDetail(unwrapData<ReservationItem>(data));
  },

  /**
   * 更新医生名下预约记录的状态。
   */
  async updateReservationStatus(
    reservationId: number,
    status: string
  ): Promise<void> {
    await http.post(`/api/doctors/reservations/${reservationId}/statuses`, {
      status,
    });
  },

  /**
   * 创建订单记录
   * @param payload 诊单信息对象，包含宠物信息、症状描述、到院时间等数据
   */
  async createOrderRecord(
    payload: CreateOrderRecordPayload
  ): Promise<OrderSummaryItem> {
    const { data } = await http.post("/api/doctors/order-records", payload);
    const createdRecord = unwrapData<OrderSummaryItem>(data);

    if (!createdRecord) {
      throw new Error("创建订单接口未返回订单记录");
    }

    return normalizeOrderSummaryItem(
      createdRecord as unknown as Record<string, unknown>
    );
  },

  /**
   * 获取订单记录信息
   * @returns 返回订单记录列表；接口为空时返回空列表。
   */
  async getOrderSummary(): Promise<OrderSummaryItem[]> {
    const { data } = await http.get("/api/doctors/order-summaries");
    const orderRecordItems = normalizeOrderSummaryItems(
      unwrapList<Record<string, unknown>>(data)
    );

    return orderRecordItems;
  },

  /**
   * 使用后端搜索医生端订单摘要，searchType 固定为 orders。
   */
  async searchOrderSummaries(
    searchByKeyword: string
  ): Promise<OrderSummaryItem[]> {
    const { data } = await http.post("/api/doctors/search-keyword", {
      searchType: "orders",
      searchByKeyword,
    });

    return normalizeOrderSummaryItems(
      unwrapList<Record<string, unknown>>(data)
    );
  },

  /**
   * 更新医生端搜索历史记录。
   */
  async updateSearchHistory(searchText: string): Promise<void> {
    await http.post("/api/doctors/search-history", { searchText });
  },

  /**
   * 按订单编号获取完整诊单详情。
   */
  async getOrderInformation(orderId: number): Promise<OrderDetailItem | null> {
    const { data } = await http.get(
      `/api/doctors/orders/${orderId}/information`
    );
    return unwrapOrderDetail(data);
  },

  /**
   * 获取待接诊队列信息
   * @returns 返回待接诊队列列表，每个队列项包含宠物的基本信息、症状描述和到院时间等数据。
   */
  async queue(): Promise<QueueItem[]> {
    const { data } = await http.get("/api/doctors/queues");
    return normalizeQueueItems(unwrapList<QueueItem>(data));
  },

  /**
   * 更新医生值班状态
   */
  async updateDutyStatus(status: DoctorDutyStatus["status"]): Promise<string> {
    const { data } = await http.post("/api/doctors/duty-status-changes", {
      status,
    });
    return unwrapMessage(data, status === "online" ? "签到成功!" : "签退成功!");
  },

  async online(): Promise<string> {
    return this.updateDutyStatus("online");
  },

  async offline(): Promise<string> {
    return this.updateDutyStatus("offline");
  },
};
