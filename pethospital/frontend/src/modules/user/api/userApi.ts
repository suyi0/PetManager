import http from "@/api/http";
import {
  reservationDoctorsMock,
  reservationScheduleMock,
} from "@/modules/user/api/userMock";
import {
  OrderRecordItem,
  OrderSummary,
  ReservationScheduleState,
  ReservationScheduleResponseItem,
} from "@/modules/user/api/types";
import { DoctorDataItem } from "@/modules/doctor/api/types";
import { PetProfile } from "@/modules/user/store/types";

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
 * 从订单记录接口响应中提取订单记录列表，并进行类型断言。
 * @param response 接受一个未知类型的响应对象，尝试从中提取订单记录列表。
 * @returns  返回订单记录列表，如果响应数据格式不正确则返回空数组。
 */
const unwrapOrderRecords = (response: unknown) => {
  const responseData = response as {
    data?: {
      data?: unknown;
    };
  };

  const rows = responseData?.data?.data;
  return Array.isArray(rows) ? (rows as OrderRecordItem[]) : [];
};

/**
 * 从订单列表接口响应中提取订单摘要列表，并进行类型断言。
 * @param response 接受一个未知类型的响应对象，尝试从中提取订单摘要列表。
 * @returns  返回订单摘要列表，如果响应数据格式不正确则返回空数组。
 */
const unwrapOrderList = (response: unknown) => {
  const responseData = response as {
    data?: unknown;
  };

  return Array.isArray(responseData?.data)
    ? (responseData.data as OrderSummary[])
    : [];
};

/**
 * 订单记录接口请求函数，接受用户标识参数并返回订单记录列表。
 * @param params 接受一个包含查询参数的对象，用于发送请求。
 * @returns 返回订单记录列表；缺少用户标识、接口为空或失败时返回空列表。
 */
const fetchOrderRecordsResponse = (params: {
  name?: string | null;
  phone?: string | null;
  email?: string | null;
}) => {
  if (!params.name && !params.phone && !params.email) {
    return createSuccessResponse({
      success: true,
      data: [],
    });
  }

  return http.get("/api/order/getrecord", { params }).catch(() =>
    createSuccessResponse({
      success: true,
      data: [],
    })
  );
};

/**
 * 订单列表摘要接口请求函数。
 * @returns 返回订单摘要列表；接口为空或失败时返回空列表。
 */
const fetchOrderSummariesResponse = () =>
  http.get("/api/order/getOrderList").catch(() => createSuccessResponse([]));

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
const fetchScheduleResponse = () =>
  http
    .get("/api/reservate/getData")
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
const fetchDoctorsResponse = () =>
  http
    .get("/api/reservate/getDoctor")
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
 * 请求用户预约记录；缺少用户标识、接口为空或失败时返回空列表。
 */
const fetchReservationRecordsResponse = (params: {
  name?: string | null;
  phone?: string | null;
  email?: string | null;
}) => {
  if (!params.name && !params.phone && !params.email) {
    return createSuccessResponse({
      success: true,
      data: [],
    });
  }

  return http.get("/api/reservate/getrecord", { params }).catch(() =>
    createSuccessResponse({
      success: true,
      data: [],
    })
  );
};

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
    return http.post("/api/upload/form", payload);
  },

  /**
   * 上传用户头像文件。
   */
  uploadAvatar(formData: FormData) {
    return http.post("/api/upload/avatar", formData, {
      headers: { "Content-Type": "multipart/form-data" },
    });
  },
};

/**
 * 订单相关接口函数集合，包含获取订单记录列表和订单摘要列表等功能。
 */
export const orderApi = {
  /**
   * 获取当前用户的订单记录列表。
   */
  async getOrderRecords(params: {
    name?: string | null;
    phone?: string | null;
    email?: string | null;
  }): Promise<OrderRecordItem[]> {
    const response = await fetchOrderRecordsResponse(params);
    return unwrapOrderRecords(response);
  },

  /**
   * 获取订单摘要列表，供订单详情页或列表页按编号继续查询使用。
   */
  async getOrderSummaries(): Promise<OrderSummary[]> {
    const response = await fetchOrderSummariesResponse();
    return unwrapOrderList(response);
  },
};

/**
 * 预约相关接口函数集合，包含获取预约时间表和医生列表、提交预约记录和获取用户预约记录等功能。
 */
export const reservationApi = {
  /**
   * 获取预约日期与时段，并转换成预约页面可直接使用的结构。
   */
  async getScheduleOptions(): Promise<
    Omit<ReservationScheduleState, "doctorData">
  > {
    const response = await fetchScheduleResponse();
    const rows = unwrapListData<ReservationScheduleResponseItem>(response);
    return normalizeScheduleData(rows.length ? rows : reservationScheduleMock);
  },

  /**
   * 获取预约医生列表，并在接口不可用时回退到 mock 数据。
   */
  async getDoctorOptions(): Promise<DoctorDataItem[]> {
    const response = await fetchDoctorsResponse();
    const rows = unwrapListData<DoctorDataItem>(response);
    return rows.length ? rows : reservationDoctorsMock;
  },

  /**
   * 提交用户预约记录。
   */
  createReservationRecord(payload: {
    name?: string | null;
    phone?: string | null;
    email?: string | null;
    doctorId: number;
    petId: number;
    date: string;
    slot: string;
  }) {
    return http.post("/api/reservate/record", payload);
  },

  /**
   * 获取当前用户的预约记录列表。
   */
  async getReservationRecords(params: {
    name?: string | null;
    phone?: string | null;
    email?: string | null;
  }): Promise<OrderRecordItem[]> {
    const response = await fetchReservationRecordsResponse(params);
    return unwrapListData<OrderRecordItem>(response);
  },

  /**
   * 删除当前用户的一条预约记录。
   */
  async deleteReservationRecord(reservationId: number): Promise<void> {
    await http.delete(`/api/reservate/deleterecord/${reservationId}`);
  },
};

export const petApi = {
  async getPetProfiles(): Promise<PetProfile[]> {
    const response = await http.get("/api/user/pets");
    return unwrapListData<PetProfile>(response);
  },

  async createPetProfile(payload: Omit<PetProfile, "id">): Promise<PetProfile> {
    const response = await http.post("/api/user/pets", payload);
    return (response.data?.data || response.data) as PetProfile;
  },

  async updatePetProfile(payload: PetProfile): Promise<PetProfile> {
    const response = await http.put(`/api/user/pets/${payload.id}`, payload);
    return (response.data?.data || response.data) as PetProfile;
  },

  async deletePetProfile(petId: string): Promise<void> {
    await http.delete(`/api/user/pets/${petId}`);
  },
};
