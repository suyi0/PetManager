import http from "@/api/http";
import { unwrapMessage, unwrapList } from "@/api/response";
import { queueItemsMock } from "./doctorMock";
import {
  CreateOrderRecordPayload,
  DoctorDutyStatus,
  DoctorUserProfile,
  DoctorUserSummary,
  OrderDetailItem,
  QueueItem,
  OrderRecordItem,
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
const normalizeQueueItems = (items: QueueItem[]) =>
  items.map((item) => {
    const source = item as QueueItem & Record<string, unknown>;

    return {
      ...item,
      ownerId: toNumber(source.ownerId ?? source.owner_id ?? source.userId),
      petId: toNumber(source.petId ?? source.pet_id),
    };
  });

const normalizeOrderStatus = (
  value: unknown
): OrderRecordItem["order_status"] =>
  String(value || "待付款") as OrderRecordItem["order_status"];

/**
 * 将公共订单摘要接口返回的数据统一成医生端订单摘要结构。
 */
const normalizeOrderRecordItems = (items: Array<Record<string, unknown>>) =>
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
      created_at: String(item.created_at ?? item.createdAt ?? ""),
    };
  });

/**
 * 将创建订单接口返回的数据转为订单摘要，方便立即插入医生端订单摘要缓存。
 */
const normalizeOrderRecordItem = (
  item: Record<string, unknown>
): OrderRecordItem => normalizeOrderRecordItems([item])[0];

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
    const fallbackStatus: DoctorDutyStatus = {
      is_online: false,
      date: "",
      check_in_time: "",
      check_out_time: "",
      status: "offline",
    };

    try {
      const { data } = await http.get("/api/doctor/dutyStatus");

      if (data && typeof data === "object" && !Array.isArray(data)) {
        return data as DoctorDutyStatus;
      }

      return unwrapList<DoctorDutyStatus>(data)[0] ?? fallbackStatus;
    } catch (error) {
      console.warn("getDutyStatus fallback to default status", error);
      return fallbackStatus;
    }
  },

  /**
   * 获取医生用户档案信息
   * @returns 返回医生用户档案列表，每个档案包含用户的基本信息、宠物档案列表和订单详情列表等数据,如果接口请求失败或返回数据格式不正确，则返回预设的模拟数据列表
   */
  async getUserProfiles(): Promise<DoctorUserProfile[]> {
    try {
      const { data } = await http.get("/api/doctor/userProfiles");
      const profiles = unwrapList<DoctorUserProfile>(data);

      return profiles;
    } catch (error) {
      console.warn("getUserProfiles fallback to empty data", error);
      return [];
    }
  },

  /**
   * 按用户名或手机号获取医生端用户摘要列表。
   */
  async getUserList(payload: {
    data: string;
    identifier: "name" | "phone";
  }): Promise<DoctorUserSummary[]> {
    try {
      const { data } = await http.post("/api/doctor/getUserList", payload);

      return normalizeDoctorUserSummaries(
        unwrapList<Record<string, unknown>>(data)
      );
    } catch (error) {
      console.warn("getUserList fallback to empty data", error);
      return [];
    }
  },

  /**
   * 获取待接诊队列信息
   * @returns 返回待接诊队列列表，每个队列项包含宠物的基本信息、症状描述和到院时间等数据,如果接口请求失败或返回数据格式不正确，则返回预设的模拟数据列表
   */
  async getQueueProfiles(): Promise<QueueItem[]> {
    try {
      const { data } = await http.get("/api/doctor/queue");
      const queueItems = normalizeQueueItems(unwrapList<QueueItem>(data));

      return queueItems.length > 0 ? queueItems : queueItemsMock;
    } catch (error) {
      console.warn("getQueueProfiles fallback to mock data", error);
      return queueItemsMock;
    }
  },

  /**
   * 获取预约档案信息
   * @returns 返回预约档案列表，每个档案包含预约的宠物信息、预约项目和当前状态等数据,如果接口请求失败或返回数据格式不正确，则返回预设的模拟数据列表
   */
  async getReservationsProfiles(): Promise<ReservationSummaryItem[]> {
    try {
      const { data } = await http.get("/api/doctor/reservations/summary");
      const reservationItems = normalizeReservationSummaries(
        unwrapList<Record<string, unknown>>(data)
      );

      return reservationItems;
    } catch (error) {
      console.warn("getReservationsProfiles fallback to empty data", error);
      return [];
    }
  },

  /**
   * 按预约编号获取完整预约详情。
   */
  async getReservationDetail(
    reservationId: number
  ): Promise<ReservationItem | null> {
    try {
      const { data } = await http.get(
        `/api/doctor/reservations/reservationInformation/${reservationId}`
      );
      return normalizeReservationDetail(unwrapData<ReservationItem>(data));
    } catch (error) {
      console.warn("getReservationDetail fallback to empty data", error);
      return null;
    }
  },

  /**
   * 更新医生名下预约记录的状态。
   */
  async updateReservationStatus(
    reservationId: number,
    status: string
  ): Promise<void> {
    await http.post(`/api/doctor/reservations/${reservationId}/status`, {
      status,
    });
  },

  /**
   * 创建订单记录
   * @param payload 诊单信息对象，包含宠物信息、症状描述、到院时间等数据
   */
  async createOrderRecord(
    payload: CreateOrderRecordPayload
  ): Promise<OrderRecordItem> {
    try {
      const { data } = await http.post(
        "/api/doctor/createOrderRecord",
        payload
      );
      const createdRecord = unwrapData<OrderRecordItem>(data);

      if (!createdRecord) {
        throw new Error("创建订单接口未返回订单记录");
      }

      return normalizeOrderRecordItem(
        createdRecord as unknown as Record<string, unknown>
      );
    } catch (error) {
      console.warn("createOrderRecord fallback to empty data", error);
      throw error;
    }
  },

  /**
   * 获取订单记录信息
   * @returns 返回订单记录列表，每个记录包含订单的基本信息和当前状态等数据,如果接口请求失败或返回数据格式不正确，则返回预设的模拟数据列表
   */
  async getOrderRecords(): Promise<OrderRecordItem[]> {
    try {
      const { data } = await http.get("/api/doctor/order/getOrderSummary");
      const orderRecordItems = normalizeOrderRecordItems(
        unwrapList<Record<string, unknown>>(data)
      );

      return orderRecordItems;
    } catch (error) {
      console.warn("getOrderRecord fallback to empty data", error);
      return [];
    }
  },

  /**
   * 按订单编号获取完整诊单详情。
   */
  async getOrderDetail(orderId: number): Promise<OrderDetailItem | null> {
    try {
      const { data } = await http.get(
        `/api/doctor/order/getOrderInformation/${orderId}`
      );
      return unwrapOrderDetail(data);
    } catch (error) {
      console.warn("getOrderDetail fallback to empty data", error);
      return null;
    }
  },

  /**
   * 获取待接诊队列信息
   * @returns 返回待接诊队列列表，每个队列项包含宠物的基本信息、症状描述和到院时间等数据,如果接口请求失败或返回数据格式不正确，则返回预设的模拟数据列表
   */
  async getQueueItems(): Promise<QueueItem[]> {
    try {
      const { data } = await http.get("/api/doctor/queue");
      const queueItems = normalizeQueueItems(unwrapList<QueueItem>(data));

      return queueItems.length > 0 ? queueItems : queueItemsMock;
    } catch (error) {
      console.warn("getQueue fallback to mock data", error);
      return queueItemsMock;
    }
  },

  /**
   * 更新医生值班状态
   */
  async updateDutyStatus(status: DoctorDutyStatus["status"]): Promise<string> {
    const { data } = await http.post("/api/doctor/dutyStatus/action", {
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
