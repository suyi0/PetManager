import http from "@/api/http";
import { unwrapMessage, unwrapList } from "@/api/response";
import { queueItemsMock } from "./doctorMock";
import {
  CreateOrderRecordPayload,
  DoctorDutyStatus,
  DoctorUserProfile,
  QueueItem,
  OrderRecordItem,
  ReservationItem,
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
  async getReservationsProfiles(): Promise<ReservationItem[]> {
    try {
      const { data } = await http.get("/api/doctor/reservations");
      const reservationItems = unwrapList<ReservationItem>(data);

      return reservationItems;
    } catch (error) {
      console.warn("getReservationsProfiles fallback to empty data", error);
      return [];
    }
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

      return {
        ...createdRecord,
        id: String(createdRecord.id),
        status: createdRecord.status || "待付款",
      };
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
      const { data } = await http.get("/api/doctor/getOrderRecord");
      const orderRecordItems = unwrapList<OrderRecordItem>(data);

      return orderRecordItems;
    } catch (error) {
      console.warn("getOrderRecord fallback to empty data", error);
      return [];
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
