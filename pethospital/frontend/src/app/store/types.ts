import { DoctorDataItem } from "@/modules/doctor/api/types";
import { BossState } from "@/modules/boss/store/types";
import { DoctorState } from "@/modules/doctor/store/types";
import { FinanceState } from "@/modules/finance/store/types";
import { PersonnelState } from "@/modules/personnel/store/types";
import { SuperAdminState } from "@/modules/super-admin/store/types";
import { UserPortalState } from "@/modules/user/store/types";
import { WarehouseAdminState } from "@/modules/warehouse-admin/store/types";
import { CurrentUserState } from "@/core/auth/store/currentUser/types";
import { AuthState } from "@/core/auth/store/session/types";
import { UiState } from "@/app/store/ui/types";
export type { CacheMeta } from "./cacheMeta";
export { shouldFetch } from "./cacheMeta";

export interface State {
  auth: AuthState;
  boss: BossState;
  currentUser: CurrentUserState;
  doctor: DoctorState;
  finance: FinanceState;
  personnel: PersonnelState;
  superAdmin: SuperAdminState;
  userPortal: UserPortalState;
  warehouseAdmin: WarehouseAdminState;
  ui: UiState;
}

export type { DoctorDataItem };
