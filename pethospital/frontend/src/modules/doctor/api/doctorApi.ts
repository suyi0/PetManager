import http from "@/api/http";
import { unwrapMessage, unwrapList } from "@/api/response";
import {
  userProfilesMock,
  reservationItemsMock,
  queueItemsMock,
  orderRecordItemsMock,
} from "./doctorMock";
import {
  DoctorDutyStatus,
  DoctorUserProfile,
  QueueItem,
  OrderRecordItem,
  ReservationItem,
} from "./types";

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

      return profiles.length > 0 ? profiles : userProfilesMock;
    } catch (error) {
      console.warn("getUserProfiles fallback to mock data", error);
      return userProfilesMock;
    }
  },

  /**
   * 获取待接诊队列信息
   * @returns 返回待接诊队列列表，每个队列项包含宠物的基本信息、症状描述和到院时间等数据,如果接口请求失败或返回数据格式不正确，则返回预设的模拟数据列表
   */
  async getQueueProfiles(): Promise<QueueItem[]> {
    try {
      const { data } = await http.get("/api/doctor/queue");
      const queueItems = unwrapList<QueueItem>(data);

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

      return reservationItems.length > 0
        ? reservationItems
        : reservationItemsMock;
    } catch (error) {
      console.warn("getReservationsProfiles fallback to mock data", error);
      return reservationItemsMock;
    }
  },

  /**
   * 获取订单记录信息
   * @returns 返回订单记录列表，每个记录包含订单的基本信息和当前状态等数据,如果接口请求失败或返回数据格式不正确，则返回预设的模拟数据列表
   */
  async getOrderRecords(): Promise<OrderRecordItem[]> {
    try {
      const { data } = await http.get("/api/doctor/orderRecord");
      const orderRecordItems = unwrapList<OrderRecordItem>(data);

      return orderRecordItems.length > 0
        ? orderRecordItems
        : orderRecordItemsMock;
    } catch (error) {
      console.warn("getOrderRecord fallback to mock data", error);
      return orderRecordItemsMock;
    }
  },

  /**
   * 获取待接诊队列信息
   * @returns 返回待接诊队列列表，每个队列项包含宠物的基本信息、症状描述和到院时间等数据,如果接口请求失败或返回数据格式不正确，则返回预设的模拟数据列表
   */
  async getQueueItems(): Promise<QueueItem[]> {
    try {
      const { data } = await http.get("/api/doctor/queue");
      const queueItems = unwrapList<QueueItem>(data);

      return queueItems.length > 0 ? queueItems : queueItemsMock;
    } catch (error) {
      console.warn("getQueue fallback to mock data", error);
      return queueItemsMock;
    }
  },

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
