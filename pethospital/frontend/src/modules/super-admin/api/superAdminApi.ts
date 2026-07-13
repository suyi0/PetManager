import http from "@/api/http";
import { unwrapList } from "@/api/response";
import { unwrapPagedList } from "@/shared/utils/pagedList";
import {
  CreateUserPayload,
  HomePageSummary,
  UserRow,
  WorkTimeRecord,
  UserLogs,
  SystemLogs,
  AuditLogItem,
  OnlineDoctorsSearchResult,
  UserSearchResult,
  UserRoleCounts,
  LogSearchResult,
  RbacDepartment,
  RbacPosition,
  PermissionTemplate,
  UserScopePayload,
  AttendanceDevice,
  AttendanceDeviceCreateResult,
  AttendanceRecord,
  AttendanceRecordQuery,
  AttendanceRecordResult,
  AttendanceSecretResult,
  PositionPermissionsPayload,
  UserRbacPermissions,
} from "./types";

export const superAdminApi = {
  async getMedicalDocumentDataContract(): Promise<{
    version: string;
    fields: Array<Record<string, unknown>>;
  }> {
    const { data } = await http.get(
      "/api/admin/report-templates/data-contracts/medical-document"
    );
    return (data?.data ?? data) as {
      version: string;
      fields: Array<Record<string, unknown>>;
    };
  },

  async getReportTemplates(): Promise<Array<Record<string, unknown>>> {
    const { data } = await http.get("/api/admin/report-templates");
    return unwrapList<Record<string, unknown>>(data);
  },

  async getReportTemplateVersions(
    templateId: number
  ): Promise<Array<Record<string, unknown>>> {
    const { data } = await http.get(
      `/api/admin/report-templates/${templateId}/versions`
    );
    return unwrapList<Record<string, unknown>>(data);
  },

  async getReportTemplateVersion(
    templateId: number,
    versionId: number
  ): Promise<Record<string, unknown>> {
    const { data } = await http.get(
      `/api/admin/report-templates/${templateId}/versions/${versionId}`
    );
    return (data?.data ?? data) as Record<string, unknown>;
  },

  async previewReportTemplate(
    templateId: number,
    templateContent: string
  ): Promise<string> {
    const { data } = await http.post(
      `/api/admin/report-templates/${templateId}/previews`,
      { templateContent }
    );
    return String((data?.data ?? data)?.html ?? "");
  },

  async createReportTemplateVersion(
    templateId: number,
    templateContent: string
  ): Promise<void> {
    await http.post(`/api/admin/report-templates/${templateId}/versions`, {
      templateContent,
    });
  },

  async publishReportTemplateVersion(
    templateId: number,
    versionId: number
  ): Promise<void> {
    await http.post(`/api/admin/report-templates/${templateId}/publish`, {
      versionId,
    });
  },
  /**
   * 刷新管理员会话
   * @returns 新的访问令牌字符串，或者在请求失败或原令牌无效时抛出错误。
   */
  refreshAdminSession() {
    return http.post("/api/admins/session-renewals");
  },

  /**
   * 获取考勤记录列表。
   * @returns 获取到的考勤记录列表；接口为空时返回空列表。
   */
  async getWorkTimeRecord(): Promise<WorkTimeRecord[]> {
    const { data } = await http.get("/api/admins/work-time-records");
    return unwrapList<WorkTimeRecord>(data);
  },

  async changeDoctorWorkTime(params: {
    user_id: number;
    date: string;
    identifier: "check_in_time" | "check_out_time";
  }): Promise<void> {
    await http.post("/api/admins/doctor-work-time-changes", params);
  },

  async changeDoctorWorkStatus(params: {
    doctorId: number;
    status: "online" | "offline";
  }): Promise<void> {
    await http.post("/api/admins/doctor-work-status-changes", params);
  },

  async getAttendanceRecords(
    params: AttendanceRecordQuery
  ): Promise<AttendanceRecordResult> {
    const { data } = await http.get("/api/admin/attendance/records", {
      params,
    });
    const payload = data?.data ?? data;

    return {
      items: unwrapList<AttendanceRecord>(
        payload?.items ?? payload?.records ?? payload
      ),
      total: Number(payload?.total ?? payload?.items?.length ?? 0),
    };
  },

  async manualAttendancePunch(payload: {
    user_id: number;
    punched_at: string;
    verify_mode?: string;
    reason: string;
  }): Promise<void> {
    await http.post("/api/admin/attendance/manual-punch", payload);
  },

  async closeAttendanceDay(workDate: string): Promise<number> {
    const { data } = await http.post("/api/admin/attendance/close-day", {
      work_date: workDate,
    });
    const payload = data?.data ?? data;
    return Number(payload?.absent_records_created ?? 0);
  },

  async getAttendanceDevices(): Promise<AttendanceDevice[]> {
    const { data } = await http.get("/api/admin/attendance/devices");
    return unwrapList<AttendanceDevice>(
      data?.data?.devices ?? data?.devices ?? data
    );
  },

  async createAttendanceDevice(payload: {
    name: string;
    device_key?: string;
    vendor?: string;
    location?: string;
    branch_id?: number | null;
  }): Promise<AttendanceDeviceCreateResult> {
    const { data } = await http.post("/api/admin/attendance/devices", payload);
    const result = data?.data ?? data;

    return {
      id: Number(result?.id ?? 0),
      device_key: String(result?.device_key ?? ""),
      secret: String(result?.secret ?? ""),
    };
  },

  async rotateAttendanceDeviceSecret(
    deviceId: number
  ): Promise<AttendanceSecretResult> {
    const { data } = await http.post(
      `/api/admin/attendance/devices/${deviceId}/rotate-secret`
    );
    const result = data?.data ?? data;

    return {
      device_id: Number(result?.device_id ?? deviceId),
      secret: String(result?.secret ?? ""),
    };
  },

  async disableAttendanceDevice(deviceId: number): Promise<void> {
    await http.post(`/api/admin/attendance/devices/${deviceId}/disable`);
  },

  /**
   * 获取用户列表。
   * @returns 获取到的用户列表；接口为空时返回空列表。
   */
  async getUsers(): Promise<UserRow[]> {
    const { data } = await http.get("/api/admins/users");
    return unwrapList<UserRow>(data);
  },

  async searchUsers(params: {
    keyword: string;
    role?: string;
    page: number;
    pageSize: number;
    includeCounts?: boolean;
  }): Promise<UserSearchResult> {
    const { data } = await http.post("/api/admins/users/search", params);
    const paged = unwrapPagedList<UserRow>(data, params);
    const source = (data as { data?: unknown })?.data ?? data;
    const rc = (source as { roleCounts?: Partial<UserRoleCounts> })?.roleCounts;

    return {
      ...paged,
      roleCounts: rc
        ? {
            all: Number(rc.all ?? paged.total),
            normal: Number(rc.normal ?? 0),
            medical: Number(rc.medical ?? 0),
            admin: Number(rc.admin ?? 0),
          }
        : undefined,
    };
  },

  async searchOnlineDoctors(params: {
    keyword: string;
    page: number;
    pageSize: number;
  }): Promise<OnlineDoctorsSearchResult> {
    const { data } = await http.post(
      "/api/admins/online-doctors/search",
      params
    );
    const payload = data?.data ?? data;
    const paged = unwrapPagedList<UserRow>(data, params);

    return {
      ...paged,
      records: unwrapList<WorkTimeRecord>(payload?.records),
    };
  },

  async createUser(payload: CreateUserPayload): Promise<void> {
    await http.post("/api/admins/users", payload);
  },

  async deleteUser(userID: number): Promise<void> {
    await http.post("/api/admins/user-deletions", { user_id: userID });
  },

  /**
   * 获取用户日志和系统日志。
   * @returns 包含用户日志和系统日志的对象；接口为空时返回空列表。
   */
  async getLogs(): Promise<{ userLogs: UserLogs[]; systemLogs: SystemLogs[] }> {
    const { data } = await http.get("/api/admins/logs");
    const logsPayload = data?.data ?? data;

    return {
      userLogs: unwrapList<UserLogs>(logsPayload?.userLogs),
      systemLogs: unwrapList<SystemLogs>(logsPayload?.systemLogs),
    };
  },

  async searchLogs(params: {
    majorTab: "user" | "system";
    role: string;
    keyword: string;
    module?: string;
    result?: string;
    startDate?: string;
    endDate?: string;
    page: number;
    pageSize: number;
    includeCounts?: boolean;
  }): Promise<LogSearchResult> {
    const { data } = await http.post("/api/admins/logs/search", params);
    const paged = unwrapPagedList<AuditLogItem>(data, params);
    const source = (data as { data?: unknown })?.data ?? data;
    const counts = source as {
      userLogCount?: unknown;
      systemLogCount?: unknown;
    };

    return {
      ...paged,
      userLogCount:
        counts?.userLogCount === undefined
          ? undefined
          : Number(counts.userLogCount),
      systemLogCount:
        counts?.systemLogCount === undefined
          ? undefined
          : Number(counts.systemLogCount),
    };
  },

  /**
   * 获取首页摘要数据，包括用户总数、在线医生总数和日志总数。
   * @returns 包含首页摘要数据的对象；接口失败时抛出错误。
   */
  async homePageGetData(): Promise<HomePageSummary> {
    const { data } = await http.get("/api/admins/home-data");
    const summary = data?.data ?? data;

    return {
      dailyExpense: Number(summary?.dailyExpense ?? 0),
      dailyCost: Number(summary?.dailyCost ?? 0),
      dailySales: Number(summary?.dailySales ?? 0),
      dailyProfit: Number(summary?.dailyProfit ?? 0),
      userCount: Number(summary?.userCount ?? 0),
      onlineDoctorCount: Number(summary?.onlineDoctorCount ?? 0),
      allLogCount: Number(summary?.allLogCount ?? 0),
      userLogCount: Number(summary?.userLogCount ?? 0),
      systemLogCount: Number(summary?.systemLogCount ?? 0),
    };
  },

  async getRbacDepartments(): Promise<RbacDepartment[]> {
    const { data } = await http.get("/api/admin/org/departments");
    return unwrapList<RbacDepartment>(
      data?.data?.departments ?? data?.departments ?? data
    );
  },

  async createRbacDepartment(payload: {
    name: string;
    branch_id?: number;
    description?: string;
    business_domain?: string;
  }): Promise<void> {
    await http.post("/api/admin/org/departments", payload);
  },

  async updateRbacDepartment(
    departmentId: number,
    payload: { name?: string; business_domain?: string }
  ): Promise<void> {
    await http.put(`/api/admin/org/departments/${departmentId}`, payload);
  },

  async deleteRbacDepartment(departmentId: number): Promise<void> {
    await http.delete(`/api/admin/org/departments/${departmentId}`);
  },

  async getRbacPositions(): Promise<RbacPosition[]> {
    const { data } = await http.get("/api/admin/org/positions");
    return unwrapList<RbacPosition>(
      data?.data?.positions ?? data?.positions ?? data
    );
  },

  async createRbacPosition(payload: {
    department_id: number;
    name: string;
    staff_kind: string;
    description?: string;
  }): Promise<void> {
    await http.post("/api/admin/org/positions", payload);
  },

  async deleteRbacPosition(positionId: number): Promise<void> {
    await http.delete(`/api/admin/org/positions/${positionId}`);
  },

  async getPermissionCatalog(): Promise<string[]> {
    const { data } = await http.get("/api/admin/rbac/permissions/catalog");
    return unwrapList<string>(
      data?.data?.permissions ?? data?.permissions ?? data
    );
  },

  async getPositionPermissions(positionId: number): Promise<PositionPermissionsPayload> {
    const { data } = await http.get(
      `/api/admin/rbac/positions/${positionId}/permissions`
    );
    const payload = data?.data ?? data;
    return {
      permissions: unwrapList<string>(payload?.permissions ?? payload),
      grantableKeys: unwrapList<string>(payload?.grantableKeys),
    };
  },

  async updatePositionPermissions(
    positionId: number,
    permissions: string[]
  ): Promise<void> {
    await http.put(`/api/admin/rbac/positions/${positionId}/permissions`, {
      permissions,
    });
  },

  async resetPositionDefaults(positionId: number): Promise<void> {
    await http.post(`/api/admin/rbac/positions/${positionId}/reset-defaults`);
  },

  async getUserRbacPermissions(userId: number): Promise<UserRbacPermissions> {
    const { data } = await http.get(`/api/admin/rbac/users/${userId}/permissions`);
    const payload = data?.data ?? data;
    return {
      positionId: Number(payload?.positionId ?? 0),
      positionName: String(payload?.positionName ?? ""),
      departmentName: String(payload?.departmentName ?? ""),
      positionPermissions: unwrapList<string>(payload?.positionPermissions),
      personalPermissions: unwrapList<string>(payload?.personalPermissions),
      grantableKeys: unwrapList<string>(payload?.grantableKeys),
    };
  },

  async updateUserRbacPermissions(userId: number, permissions: string[]): Promise<void> {
    await http.put(`/api/admin/rbac/users/${userId}/permissions`, { permissions });
  },

  async applyPermissionTemplate(
    positionId: number,
    templateId: number
  ): Promise<void> {
    await http.post(`/api/admin/rbac/positions/${positionId}/apply-template`, {
      template_id: templateId,
    });
  },

  async getPermissionTemplates(): Promise<PermissionTemplate[]> {
    const { data } = await http.get("/api/admin/rbac/permission-templates");
    return unwrapList<PermissionTemplate>(
      data?.data?.templates ?? data?.templates ?? data
    );
  },

  async updateUserPosition(userId: number, positionId: number | null) {
    await http.put(`/api/admin/users/${userId}/position`, {
      position_id: positionId,
    });
  },

  async getUserScopes(userId: number): Promise<UserScopePayload> {
    const { data } = await http.get(`/api/admin/users/${userId}/scopes`);
    const payload = data?.data ?? data;
    return {
      user_id: Number(payload?.user_id ?? userId),
      branch_ids: Array.isArray(payload?.branch_ids)
        ? payload.branch_ids.map(Number)
        : [],
      department_ids: Array.isArray(payload?.department_ids)
        ? payload.department_ids.map(Number)
        : [],
    };
  },

  async updateUserScopes(
    userId: number,
    payload: { branch_ids: number[]; department_ids: number[] }
  ): Promise<void> {
    await http.put(`/api/admin/users/${userId}/scopes`, payload);
  },
};
