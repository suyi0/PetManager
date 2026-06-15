import http from "@/api/http";
import {
  PetProfile,
  OrderDetail,
  ReservationOrderRecordItem,
  ReservationSummary,
  OrderSummary,
  ReservationScheduleState,
  ReservationScheduleResponseItem,
} from "@/modules/user/api/types";
import { DoctorDataItem } from "@/modules/doctor/api/types";

/**
 * 从常见的 `{ data: { data: [] } }` 响应结构中提取数组数据。
 */
const unwrapListData = <T>(response: unknown) => {
  const responseData = response as {
    data?: {
      data?: unknown;
    };
  };

  const rows = responseData?.data?.data;
  return Array.isArray(rows) ? (rows as T[]) : [];
};

/**
 * 从订单列表接口响应中提取订单摘要列表，并进行类型断言。
 * @param response 接受一个未知类型的响应对象，尝试从中提取订单摘要列表。
 * @returns  返回订单摘要列表，如果响应数据格式不正确则返回空数组。
 */
const unwrapOrderList = (response: unknown) => {
  const responseData = response as {
    data?: unknown | { data?: unknown };
  };

  if (Array.isArray(responseData?.data)) {
    return responseData.data as OrderSummary[];
  }

  const nestedRows =
    responseData?.data &&
    typeof responseData.data === "object" &&
    "data" in responseData.data
      ? (responseData.data as { data?: unknown }).data
      : null;

  return Array.isArray(nestedRows) ? (nestedRows as OrderSummary[]) : [];
};

/**
 * 从订单详情接口响应中提取完整订单信息。
 */
const unwrapOrderDetail = (response: unknown) => {
  const responseData = response as {
    data?: unknown | { data?: unknown };
  };

  const detail =
    responseData?.data &&
    typeof responseData.data === "object" &&
    "data" in responseData.data
      ? (responseData.data as { data?: unknown }).data
      : responseData?.data;

  return detail && typeof detail === "object" ? (detail as OrderDetail) : null;
};

const normalizeReservationSummaries = (
  rows: ReservationSummary[]
): ReservationSummary[] =>
  rows.map((item) => ({
    ...item,
    schedule:
      item.schedule ||
      [item.date, item.time_slot].filter(Boolean).join(" ") ||
      "",
  }));

/**
 * 订单列表摘要接口请求函数。
 * @returns 返回订单摘要列表；接口为空或失败时返回空列表。
 */
const fetchGetOrderSummaryResponse = () => http.get("/api/users/me/orders");

/**
 * 订单完整信息接口请求函数。
 */
const fetchGetOrderInformationResponse = (orderId: number) =>
  http.get(`/api/users/me/orders/${orderId}`);

/**
 * 将后端预约时间表结构转换成预约页面可直接使用的拆分字段。
 */
const normalizeScheduleData = (
  rows: ReservationScheduleResponseItem[]
): Omit<ReservationScheduleState, "doctorData"> => {
  const year: string[] = [];
  const month: string[] = [];
  const day: string[] = [];
  const weekday: string[] = [];
  const slots: string[][] = [];

  for (const item of rows) {
    year.push(item.year.toString());

    const dateParts = item.date.split("-");
    month.push(dateParts[0] ?? "");
    day.push(dateParts[1] ?? "");
    weekday.push(item.weekday);

    const timeSlotsArray = item.time_slots
      ? Object.values(item.time_slots).map((slot) => String(slot))
      : [];
    slots.push(timeSlotsArray);
  }

  return { year, month, day, weekday, slots };
};

/**
 * 请求预约时间表。
 */
const fetchGetDateResponse = () => http.get("/api/users/me/reservation-dates");

/**
 * 请求预约医生列表。
 */
const fetchGetDoctorResponse = () =>
  http.get("/api/users/me/reservation-doctors");

/**
 * 请求当前登录用户的预约记录，具体用户范围由后端登录态判断。
 */
const fetchReservationsSummaryResponse = () =>
  http.get("/api/users/me/reservations");

/**
 * 按关键词搜索当前用户的订单或预约摘要。
 */
const fetchSearchKeywordResponse = (
  searchType: "orders" | "reservations",
  searchByKeyword: string
) =>
  http.post("/api/users/me/search-keyword", {
    searchType,
    searchByKeyword,
  });

const updateSearchHistoryResponse = (searchText: string) =>
  http.post("/api/users/me/search-history", { searchText });

/**
 * 请求当前选中的完整预约记录。
 */
const fetchReservationInformationResponse = (reservationId: number) =>
  http.get(`/api/users/me/reservations/${reservationId}`);

/**
 * 用户档案相关接口函数集合，包含保存用户基础资料和上传用户头像等功能。
 */
export const profileApi = {
  /**
   * 保存用户基础资料。
   */
  saveUserData(payload: {
    name: string;
    phone?: string | null;
    email?: string | null;
    birthday?: string | null;
    address?: string | null;
    headImage?: string | null | undefined;
  }) {
    return http.put("/api/users/me/profile", payload);
  },

  /**
   * 使用邮箱验证码校验通过后的短期凭证更新登录邮箱。
   */
  updateEmail(payload: { email: string; ticket: string }) {
    return http.patch("/api/users/me/email", payload);
  },

  /**
   * 上传用户头像文件。
   */
  uploadAvatar(formData: FormData) {
    return http.post("/api/users/me/avatar", formData, {
      headers: { "Content-Type": "multipart/form-data" },
    });
  },
};

/**
 * 订单相关接口函数集合，包含获取订单摘要列表和完整订单详情等功能。
 */
export const orderApi = {
  /**
   * 获取订单摘要列表，供订单详情页或列表页按编号继续查询使用。
   */
  async getOrderSummary(): Promise<OrderSummary[]> {
    const response = await fetchGetOrderSummaryResponse();
    return unwrapOrderList(response);
  },

  /**
   * 按订单编号获取完整订单信息。
   */
  async getOrderInformation(orderId: number): Promise<OrderDetail | null> {
    const response = await fetchGetOrderInformationResponse(orderId);
    return unwrapOrderDetail(response);
  },

  /**
   * 按关键词搜索订单摘要，searchType 固定为 orders。
   */
  async searchOrderSummaries(searchByKeyword: string): Promise<OrderSummary[]> {
    const response = await fetchSearchKeywordResponse(
      "orders",
      searchByKeyword
    );

    return unwrapOrderList(response);
  },

  /**
   * 更新用户搜索历史记录。
   */
  async updateSearchHistory(searchText: string): Promise<void> {
    await updateSearchHistoryResponse(searchText);
  },
};

/**
 * 预约相关接口函数集合，包含获取预约时间表和医生列表、提交预约记录和获取用户预约记录等功能。
 */
export const reservationApi = {
  /**
   * 获取预约日期与时段，并转换成预约页面可直接使用的结构。
   */
  async getDate(): Promise<Omit<ReservationScheduleState, "doctorData">> {
    const response = await fetchGetDateResponse();
    const rows = unwrapListData<ReservationScheduleResponseItem>(response);
    return normalizeScheduleData(rows);
  },

  /**
   * 获取预约医生列表。
   */
  async getDoctor(): Promise<DoctorDataItem[]> {
    const response = await fetchGetDoctorResponse();
    return unwrapListData<DoctorDataItem>(response);
  },

  /**
   * 提交用户预约记录。
   */
  record(payload: {
    name?: string | null;
    phone?: string | null;
    email?: string | null;
    doctorId: number;
    petId: number;
    reservationType: string;
    date: string;
    slot: string;
  }) {
    return http.post("/api/users/me/reservations", payload);
  },

  /**
   * 获取当前用户的预约记录列表。
   */
  async getReservationsSummary(): Promise<ReservationSummary[]> {
    const response = await fetchReservationsSummaryResponse();
    return normalizeReservationSummaries(
      unwrapListData<ReservationSummary>(response)
    );
  },

  /**
   * 按关键词搜索预约摘要，searchType 固定为 reservations。
   */
  async searchReservationSummaries(
    searchByKeyword: string
  ): Promise<ReservationSummary[]> {
    const response = await fetchSearchKeywordResponse(
      "reservations",
      searchByKeyword
    );

    return normalizeReservationSummaries(
      unwrapListData<ReservationSummary>(response)
    );
  },

  /**
   * 按预约编号获取完整预约详情。
   */
  async reservationInformation(
    reservationId: number
  ): Promise<ReservationOrderRecordItem | null> {
    const response = await fetchReservationInformationResponse(reservationId);
    const detail = (response.data?.data ||
      response.data) as ReservationOrderRecordItem | null;

    if (!detail || typeof detail !== "object") {
      return null;
    }

    return {
      ...detail,
      schedule:
        detail.schedule ||
        [detail.date, detail.time_slot].filter(Boolean).join(" ") ||
        "",
      price: typeof detail.price === "number" ? detail.price : 0,
    };
  },

  /**
   * 删除当前用户的一条预约记录。
   */
  async deleterecord(reservationId: number): Promise<void> {
    await http.delete(
      `/api/users/me/reservations/${reservationId}/cancellation`
    );
  },
};

export const petApi = {
  async getPetProfiles(): Promise<PetProfile[]> {
    const response = await http.get("/api/users/me/pet-profiles");
    return unwrapListData<PetProfile>(response);
  },

  async createPetProfile(payload: Omit<PetProfile, "id">): Promise<PetProfile> {
    const response = await http.post("/api/users/me/pet-profiles", payload);
    return (response.data?.data || response.data) as PetProfile;
  },

  async updatePetProfile(payload: PetProfile): Promise<PetProfile> {
    const response = await http.put(
      `/api/users/me/pet-profiles/${payload.id}`,
      payload
    );
    return (response.data?.data || response.data) as PetProfile;
  },

  async deletePetProfile(petId: string): Promise<void> {
    await http.delete(`/api/users/me/pet-profiles/${petId}`);
  },
};
