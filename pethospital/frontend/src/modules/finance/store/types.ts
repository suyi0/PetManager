import { CacheMeta } from "@/app/store/types";
import { SalaryManagementPayload } from "../api/types";

/**
 * 财务端业务缓存状态。
 */
export interface FinanceState {
  salaryManagement: SalaryManagementPayload;
  salaryManagementMeta: CacheMeta;
}
