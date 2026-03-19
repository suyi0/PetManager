import { DoctorDataItem } from "@/modules/doctor/store/types";
import { CurrentUserState } from "@/core/auth/store/currentUser/types";
import { AuthState } from "@/core/auth/store/session/types";
import { OrderState } from "@/modules/user/store/order/types";
import { ReservationModuleState } from "@/modules/user/store/reservation/types";
import { UiState } from "@/store/ui/types";

export interface State {
  auth: AuthState;
  currentUser: CurrentUserState;
  order: OrderState;
  reservation: ReservationModuleState;
  ui: UiState;
}

export type { DoctorDataItem };
