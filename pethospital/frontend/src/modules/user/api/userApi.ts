import http from "@/api/http";
import {
  reservationDoctorsMock,
  reservationScheduleMock,
} from "@/modules/user/api/userMock";
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
 * 生成一个与 axios 成功响应结构兼容的 Promise。
 */
const createSuccessResponse = <T>(data: T) =>
  Promise.resolve({
    status: 200,
    data,
  });

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
const fetchGetOrderSummaryResponse = () =>
  http.get("/api/user/orders").catch(() => createSuccessResponse([]));

/**
 * 订单完整信息接口请求函数。
 */
const fetchGetOrderInformationResponse = (orderId: number) =>
  http.get(`/api/user/orders/${orderId}`);

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
 * 请求预约时间表，并在接口为空或失败时回退到预约时间 mock 数据。
 */
const fetchGetDateResponse = () =>
  http
    .get("/api/user/reservations/dates")
    .then((response) => {
      const rows = Array.isArray(response?.data?.data)
        ? response.data.data
        : [];
      return rows.length
        ? response
        : createSuccessResponse({
            success: true,
            data: reservationScheduleMock,
          });
    })
    .catch(() =>
      createSuccessResponse({
        success: true,
        data: reservationScheduleMock,
      })
    );

/**
 * 请求预约医生列表，并在接口为空或失败时回退到医生 mock 数据。
 */
const fetchGetDoctorResponse = () =>
  http
    .get("/api/user/reservations/doctors")
    .then((response) => {
      const rows = Array.isArray(response?.data?.data)
        ? response.data.data
        : [];
      return rows.length
        ? response
        : createSuccessResponse({
            success: true,
            data: reservationDoctorsMock,
          });
    })
    .catch(() =>
      createSuccessResponse({
        success: true,
        data: reservationDoctorsMock,
      })
    );

/**
 * 请求当前登录用户的预约记录，具体用户范围由后端登录态判断。
 */
const fetchReservationsSummaryResponse = () =>
  http.get("/api/user/reservations").catch(() =>
    createSuccessResponse({
      success: true,
      data: [],
    })
  );

/**
 * 请求当前选中的完整预约记录。
 */
const fetchReservationInformationResponse = (reservationId: number) =>
  http.get(`/api/user/reservations/${reservationId}`);

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
    address_id?: string | null;
    address?: string | null;
    headImage?: string | null | undefined;
  }) {
    return http.put("/api/user/profile", payload);
  },

  /**
   * 上传用户头像文件。
   */
  uploadAvatar(formData: FormData) {
    return http.post("/api/user/avatar", formData, {
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
    return normalizeScheduleData(rows.length ? rows : reservationScheduleMock);
  },

  /**
   * 获取预约医生列表，并在接口不可用时回退到 mock 数据。
   */
  async getDoctor(): Promise<DoctorDataItem[]> {
    const response = await fetchGetDoctorResponse();
    const rows = unwrapListData<DoctorDataItem>(response);
    return rows.length ? rows : reservationDoctorsMock;
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
    return http.post("/api/user/reservations", payload);
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
    await http.delete(`/api/user/reservations/${reservationId}`);
  },
};

export const petApi = {
  async getPetProfiles(): Promise<PetProfile[]> {
    const response = await http.get("/api/user/petProfiles");
    return unwrapListData<PetProfile>(response);
  },

  async createPetProfile(payload: Omit<PetProfile, "id">): Promise<PetProfile> {
    const response = await http.post("/api/user/petProfiles", payload);
    return (response.data?.data || response.data) as PetProfile;
  },

  async updatePetProfile(payload: PetProfile): Promise<PetProfile> {
    const response = await http.put(
      `/api/user/petProfiles/${payload.id}`,
      payload
    );
    return (response.data?.data || response.data) as PetProfile;
  },

  async deletePetProfile(petId: string): Promise<void> {
    await http.delete(`/api/user/petProfiles/${petId}`);
  },
};
