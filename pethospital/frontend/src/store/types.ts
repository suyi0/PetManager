import { DoctorDataItem } from "@/modules/doctor/api/types";
import { CurrentUserState } from "@/core/auth/store/currentUser/types";
import { AuthState } from "@/core/auth/store/session/types";
import { UiState } from "@/store/ui/types";

export interface State {
  auth: AuthState;
  currentUser: CurrentUserState;
  ui: UiState;
}

export type { DoctorDataItem };
