import http from "@/api/http";
import { unwrapList } from "@/api/response";
import { unwrapPagedList } from "@/shared/utils/pagedList";
import {
  normalizeCompensationProposal,
  positiveNumberOrNull,
} from "@/shared/utils/employmentWorkflow";
import {
  AssignmentResult,
  CompensationDraftPayload,
  CompensationProposal,
  DepartmentOption,
  PersonnelEmployee,
  PositionOption,
  WorkflowPage,
} from "@/shared/types/employmentWorkflow";

type UnknownRecord = Record<string, unknown>;

const recordOf = (value: unknown): UnknownRecord =>
  value && typeof value === "object" ? (value as UnknownRecord) : {};

const payloadOf = (value: unknown): UnknownRecord => {
  const source = recordOf(value);
  return recordOf(source.data ?? source);
};

const numberOrNull = (value: unknown) => {
  return positiveNumberOrNull(value);
};

const normalizeEmployee = (value: unknown): PersonnelEmployee => {
  const item = recordOf(value);
  return {
    id: Number(item.id ?? 0),
    name: String(item.name ?? ""),
    phone: String(item.phone ?? ""),
    email: String(item.email ?? ""),
    accountType: String(item.account_type ?? item.accountType ?? ""),
    positionId: numberOrNull(item.position_id ?? item.positionId),
    positionName: String(item.position_name ?? item.positionName ?? ""),
    departmentId: numberOrNull(item.department_id ?? item.departmentId),
    departmentName: String(item.department_name ?? item.departmentName ?? ""),
    staffKind: String(item.staff_kind ?? item.staffKind ?? ""),
    assignmentPolicy: String(
      item.assignment_policy ?? item.assignmentPolicy ?? ""
    ),
    permissionSummary: Array.isArray(item.permission_summary)
      ? item.permission_summary.map(String)
      : [],
    employmentId: numberOrNull(item.employment_id ?? item.employmentId),
    employmentStatus: String(
      item.employment_status ?? item.employmentStatus ?? ""
    ) as PersonnelEmployee["employmentStatus"],
    employmentRowVersion: numberOrNull(
      item.employment_row_version ?? item.employmentRowVersion
    ),
    hireDate: String(item.hire_date ?? item.hireDate ?? ""),
    probationWaived: Boolean(
      item.probation_waived ?? item.probationWaived ?? false
    ),
  };
};

const normalizeAssignmentResult = (value: unknown): AssignmentResult => {
  const item = payloadOf(value);
  return {
    userId: Number(item.user_id ?? item.userId ?? 0),
    positionId: numberOrNull(item.position_id ?? item.positionId),
    accountType: String(item.account_type ?? item.accountType ?? ""),
    assignmentStatus: String(
      item.assignment_status ?? item.assignmentStatus ?? ""
    ),
    assignmentId: Number(item.assignment_id ?? item.assignmentId ?? 0),
    employmentId: Number(item.employment_id ?? item.employmentId ?? 0),
    message: String(item.message ?? ""),
  };
};

export const personnelApi = {
  refreshSession() {
    return http.post("/api/admins/session-renewals");
  },

  async searchEmployees(params: {
    keyword: string;
    page: number;
    pageSize: number;
  }): Promise<WorkflowPage<PersonnelEmployee>> {
    const { data } = await http.post("/api/personnel/employees/search", params);
    const page = unwrapPagedList<unknown>(data, params);
    return { ...page, items: page.items.map(normalizeEmployee) };
  },

  async getEmployee(employeeId: number): Promise<PersonnelEmployee> {
    const { data } = await http.get(`/api/personnel/employees/${employeeId}`);
    return normalizeEmployee(payloadOf(data));
  },

  async getDepartments(): Promise<DepartmentOption[]> {
    const { data } = await http.get("/api/personnel/org/departments");
    return unwrapList<unknown>(payloadOf(data).departments).map((value) => {
      const item = recordOf(value);
      return {
        id: Number(item.id ?? 0),
        branchId: Number(item.branch_id ?? item.branchId ?? 0),
        branchName: String(item.branch_name ?? item.branchName ?? ""),
        name: String(item.name ?? ""),
        description: String(item.description ?? ""),
        businessDomain: String(
          item.business_domain ?? item.businessDomain ?? "general"
        ),
      };
    });
  },

  async getPositions(departmentId?: number): Promise<PositionOption[]> {
    const { data } = await http.get("/api/personnel/org/positions", {
      params: { departmentId, assignableOnly: true },
    });
    return unwrapList<unknown>(payloadOf(data).positions).map((value) => {
      const item = recordOf(value);
      return {
        id: Number(item.id ?? 0),
        departmentId: numberOrNull(item.department_id ?? item.departmentId),
        departmentName: String(
          item.department_name ?? item.departmentName ?? ""
        ),
        name: String(item.name ?? ""),
        systemKey: String(item.system_key ?? item.systemKey ?? ""),
        staffKind: String(item.staff_kind ?? item.staffKind ?? ""),
        description: String(item.description ?? ""),
        assignmentPolicy: String(
          item.assignment_policy ?? item.assignmentPolicy ?? "super_admin_only"
        ),
        status: String(item.status ?? "published"),
        permissionSummary: Array.isArray(item.permission_summary)
          ? item.permission_summary.map(String)
          : [],
      };
    });
  },

  async updateAssignment(
    employeeId: number,
    payload: {
      action: "onboard" | "transfer";
      positionId: number;
      expectedCurrentPositionId: number;
      effectiveFrom: string;
      reason: string;
    }
  ): Promise<AssignmentResult> {
    const { data } = await http.put(
      `/api/personnel/employees/${employeeId}/assignment`,
      {
        action: payload.action,
        position_id: payload.positionId,
        expected_current_position_id: payload.expectedCurrentPositionId,
        effective_from: payload.effectiveFrom,
        reason: payload.reason,
      }
    );
    return normalizeAssignmentResult(data);
  },

  async regularize(
    employeeId: number,
    payload: {
      expectedCurrentPositionId: number;
      effectiveFrom: string;
      reason: string;
    }
  ): Promise<AssignmentResult> {
    const { data } = await http.post(
      `/api/personnel/employees/${employeeId}/regularization`,
      {
        expected_current_position_id: payload.expectedCurrentPositionId,
        effective_from: payload.effectiveFrom,
        reason: payload.reason,
      }
    );
    return normalizeAssignmentResult(data);
  },

  async offboard(
    employeeId: number,
    payload: {
      expectedCurrentPositionId: number;
      effectiveFrom: string;
      reason: string;
    }
  ): Promise<AssignmentResult> {
    const { data } = await http.post(
      `/api/personnel/employees/${employeeId}/offboarding`,
      {
        expected_current_position_id: payload.expectedCurrentPositionId,
        effective_from: payload.effectiveFrom,
        reason: payload.reason,
      }
    );
    return normalizeAssignmentResult(data);
  },

  async listCompensationProposals(
    params: {
      status?: string;
      phase?: string;
      page?: number;
      pageSize?: number;
    } = {}
  ): Promise<WorkflowPage<CompensationProposal>> {
    const fallback = {
      page: params.page ?? 1,
      pageSize: params.pageSize ?? 100,
    };
    const { data } = await http.get("/api/personnel/compensation-proposals", {
      params: { ...params, ...fallback },
    });
    const page = unwrapPagedList<unknown>(data, fallback);
    return { ...page, items: page.items.map(normalizeCompensationProposal) };
  },

  async createCompensationProposal(
    payload: CompensationDraftPayload
  ): Promise<CompensationProposal> {
    const { data } = await http.post(
      "/api/personnel/compensation-proposals",
      payload
    );
    return normalizeCompensationProposal(payloadOf(data));
  },

  async updateCompensationProposal(
    proposalId: number,
    payload: Omit<
      CompensationDraftPayload,
      "employmentId" | "phase" | "assigneeUserId"
    > & {
      expectedRowVersion: number;
    }
  ): Promise<CompensationProposal> {
    const { data } = await http.put(
      `/api/personnel/compensation-proposals/${proposalId}`,
      payload
    );
    return normalizeCompensationProposal(payloadOf(data));
  },

  async submitCompensationProposal(
    proposalId: number,
    expectedRowVersion: number,
    reason: string
  ): Promise<CompensationProposal> {
    const { data } = await http.post(
      `/api/personnel/compensation-proposals/${proposalId}/submit`,
      { expectedRowVersion, reason }
    );
    return normalizeCompensationProposal(payloadOf(data));
  },

  async reassignCompensationProposal(
    proposalId: number,
    assigneeUserId: number,
    expectedRowVersion: number,
    reason: string
  ): Promise<CompensationProposal> {
    const { data } = await http.post(
      `/api/personnel/compensation-proposals/${proposalId}/reassign`,
      { assigneeUserId, expectedRowVersion, reason }
    );
    return normalizeCompensationProposal(payloadOf(data));
  },
};
