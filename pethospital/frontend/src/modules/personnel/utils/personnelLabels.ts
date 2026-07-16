import type { PersonnelEmployee } from "@/shared/types/employmentWorkflow";

export const employmentStatusLabel = (employee: PersonnelEmployee) => {
  if (!employee.employmentStatus)
    return employee.accountType === "customer" ? "待入职" : "档案待同步";
  return (
    (
      {
        draft: "草稿",
        onboarding: "入职处理中",
        probation: "试用期",
        regularization_pending: "待正式薪酬",
        active: "正式在职",
        rejected: "已拒绝",
        separated: "已离职",
      } as Record<string, string>
    )[employee.employmentStatus] || employee.employmentStatus
  );
};

export const statusTone = (status: string) =>
  status === "active"
    ? "success"
    : status === "separated" || status === "rejected"
    ? "danger"
    : status === "probation"
    ? "warning"
    : "neutral";

export const assignmentPolicyLabel = (policy: string) =>
  policy === "direct"
    ? "可直接生效"
    : policy === "approval_required"
    ? "需要主管审批"
    : "不可由人事派岗";

export const phaseLabel = (phase: string) =>
  ((
    {
      probation: "试用期薪酬",
      regular: "正式薪酬",
      adjustment: "在职调薪",
    } as Record<string, string>
  )[phase] || phase);

export const proposalStatusLabel = (status: string) =>
  ((
    {
      draft: "草稿",
      submitted: "待管理审批",
      management_approved: "待财务激活",
      returned: "已退回",
      finance_confirmed: "财务已确认",
      active: "已生效",
      cancelled: "已取消",
    } as Record<string, string>
  )[status] || status);

export const proposalStatusTone = (status: string) =>
  status === "active"
    ? "success"
    : status === "returned" || status === "cancelled"
    ? "danger"
    : status === "draft"
    ? "neutral"
    : "warning";

export const initialOf = (name: string) => name.trim().charAt(0) || "?";

export const today = () => new Date().toISOString().slice(0, 10);
