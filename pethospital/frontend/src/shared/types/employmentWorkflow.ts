export type EmploymentStatus =
  | "draft"
  | "onboarding"
  | "probation"
  | "regularization_pending"
  | "active"
  | "rejected"
  | "separated"
  | "";

export type AssignmentAction =
  | "onboard"
  | "transfer"
  | "regularize"
  | "offboard";

export interface PersonnelEmployee {
  id: number;
  name: string;
  phone: string;
  email: string;
  accountType: string;
  positionId: number | null;
  positionName: string;
  departmentId: number | null;
  departmentName: string;
  staffKind: string;
  assignmentPolicy: string;
  permissionSummary: string[];
  employmentId: number | null;
  employmentStatus: EmploymentStatus;
  employmentRowVersion: number | null;
  hireDate: string;
  probationWaived: boolean;
}

export interface DepartmentOption {
  id: number;
  branchId: number;
  branchName: string;
  name: string;
  description: string;
  businessDomain: string;
}

export interface PositionOption {
  id: number;
  departmentId: number | null;
  departmentName: string;
  name: string;
  systemKey: string;
  staffKind: string;
  description: string;
  assignmentPolicy:
    | "direct"
    | "approval_required"
    | "super_admin_only"
    | string;
  status: string;
  permissionSummary: string[];
}

export interface AssignmentResult {
  userId: number;
  positionId: number | null;
  accountType: string;
  assignmentStatus: "pending" | "effective" | string;
  assignmentId: number;
  employmentId: number;
  message: string;
}

export interface AssignmentApproval {
  id: number;
  employmentId: number;
  userId: number;
  userName: string;
  branchId: number;
  branchName: string;
  departmentId: number;
  departmentName: string;
  fromPositionId: number | null;
  fromPositionName: string;
  toPositionId: number | null;
  toPositionName: string;
  action: AssignmentAction;
  status: string;
  effectiveFrom: string;
  reason: string;
  requestedBy: number | null;
  reviewedBy: number | null;
  expectedEmploymentRowVersion: number;
  rowVersion: number;
  createdAt: string;
  employmentStatus: EmploymentStatus;
  currentPositionId: number;
}

export type CompensationPhase = "probation" | "regular" | "adjustment";
export type CompensationStatus =
  | "draft"
  | "submitted"
  | "management_approved"
  | "returned"
  | "finance_confirmed"
  | "active"
  | "cancelled";

export interface CompensationProposal {
  id: number;
  employmentId: number;
  userId: number;
  userName: string;
  positionName: string;
  branchId: number;
  branchName: string;
  departmentId: number;
  departmentName: string;
  phase: CompensationPhase;
  status: CompensationStatus;
  assigneeUserId: number;
  proposedBy: number;
  submittedBy: number | null;
  approvedBy: number | null;
  financeConfirmedBy: number | null;
  salaryProfileId: number | null;
  effectiveFrom: string;
  note?: string;
  expectedEmploymentRowVersion: number | null;
  rowVersion: number;
  employmentStatus: EmploymentStatus;
  payType?: "monthly" | "hourly";
  baseSalary?: number | null;
  hourlyRate?: number | null;
  socialInsuranceHousingFund?: number;
}

export interface CompensationDraftPayload {
  employmentId: number;
  phase: CompensationPhase;
  payType: "monthly" | "hourly";
  baseSalary: number | null;
  hourlyRate: number | null;
  socialInsuranceHousingFund: number;
  effectiveFrom: string;
  note: string;
  assigneeUserId?: number;
}

export interface WorkflowPage<T> {
  items: T[];
  total: number;
  page: number;
  pageSize: number;
}
