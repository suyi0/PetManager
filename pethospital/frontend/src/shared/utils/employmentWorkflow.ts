import { CompensationProposal } from "@/shared/types/employmentWorkflow";

export type UnknownRecord = Record<string, unknown>;

export const workflowRecordOf = (value: unknown): UnknownRecord =>
  value && typeof value === "object" ? (value as UnknownRecord) : {};

export const workflowPayloadOf = (value: unknown): UnknownRecord => {
  const source = workflowRecordOf(value);
  return workflowRecordOf(source.data ?? source);
};

export const positiveNumberOrNull = (value: unknown) => {
  if (value === null || value === undefined || value === "") return null;
  const parsed = Number(value);
  return Number.isFinite(parsed) && parsed > 0 ? parsed : null;
};

export const normalizeCompensationProposal = (
  value: unknown
): CompensationProposal => {
  const item = workflowRecordOf(value);
  return {
    id: Number(item.id ?? 0),
    employmentId: Number(item.employmentId ?? item.employment_id ?? 0),
    userId: Number(item.userId ?? item.user_id ?? 0),
    userName: String(item.userName ?? item.user_name ?? ""),
    positionName: String(item.positionName ?? item.position_name ?? ""),
    branchId: Number(item.branchId ?? item.branch_id ?? 0),
    branchName: String(item.branchName ?? item.branch_name ?? ""),
    departmentId: Number(item.departmentId ?? item.department_id ?? 0),
    departmentName: String(item.departmentName ?? item.department_name ?? ""),
    phase: String(item.phase ?? "probation") as CompensationProposal["phase"],
    status: String(item.status ?? "draft") as CompensationProposal["status"],
    assigneeUserId: Number(item.assigneeUserId ?? item.assignee_user_id ?? 0),
    proposedBy: Number(item.proposedBy ?? item.proposed_by ?? 0),
    submittedBy: positiveNumberOrNull(item.submittedBy ?? item.submitted_by),
    approvedBy: positiveNumberOrNull(item.approvedBy ?? item.approved_by),
    financeConfirmedBy: positiveNumberOrNull(
      item.financeConfirmedBy ?? item.finance_confirmed_by
    ),
    salaryProfileId: positiveNumberOrNull(
      item.salaryProfileId ?? item.salary_profile_id
    ),
    effectiveFrom: String(item.effectiveFrom ?? item.effective_from ?? ""),
    note: typeof item.note === "string" ? item.note : undefined,
    expectedEmploymentRowVersion: positiveNumberOrNull(
      item.expectedEmploymentRowVersion ?? item.expected_employment_row_version
    ),
    rowVersion: Number(item.rowVersion ?? item.row_version ?? 1),
    employmentStatus: String(
      item.employmentStatus ?? item.employment_status ?? ""
    ) as CompensationProposal["employmentStatus"],
    payType:
      item.payType === "hourly" || item.pay_type === "hourly"
        ? "hourly"
        : item.payType === "monthly" || item.pay_type === "monthly"
        ? "monthly"
        : undefined,
    baseSalary:
      item.baseSalary === undefined && item.base_salary === undefined
        ? undefined
        : item.baseSalary == null && item.base_salary == null
        ? null
        : Number(item.baseSalary ?? item.base_salary),
    hourlyRate:
      item.hourlyRate === undefined && item.hourly_rate === undefined
        ? undefined
        : item.hourlyRate == null && item.hourly_rate == null
        ? null
        : Number(item.hourlyRate ?? item.hourly_rate),
    socialInsuranceHousingFund:
      item.socialInsuranceHousingFund === undefined &&
      item.social_insurance_housing_fund === undefined
        ? undefined
        : Number(
            item.socialInsuranceHousingFund ??
              item.social_insurance_housing_fund ??
              0
          ),
  };
};
