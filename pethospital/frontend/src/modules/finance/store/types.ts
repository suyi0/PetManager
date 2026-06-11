import { CacheMeta } from "@/app/store/types";
import { FinanceHomeData, SalaryManagementPayload } from "../api/types";

/**
 * 财务端业务缓存状态。
 */
export interface FinanceState {
  homeData: FinanceHomeData;
  homeDataMeta: CacheMeta;
  salaryManagement: SalaryManagementPayload;
  salaryManagementMeta: CacheMeta;
}
