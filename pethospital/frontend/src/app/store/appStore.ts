import { InjectionKey } from "vue";
import { createStore, Store } from "vuex";
import { currentUserActions } from "@/core/auth/store/currentUser/actions";
import { currentUserMutations } from "@/core/auth/store/currentUser/mutations";
import { createCurrentUserState } from "@/core/auth/store/currentUser/state";
import { CurrentUserState } from "@/core/auth/store/currentUser/types";
import { bossActions } from "@/modules/boss/store/actions";
import { bossMutations } from "@/modules/boss/store/mutations";
import { createBossState } from "@/modules/boss/store/state";
import { BossState } from "@/modules/boss/store/types";
import { authActions } from "@/core/auth/store/session/actions";
import { authGetters } from "@/core/auth/store/session/getters";
import { authMutations } from "@/core/auth/store/session/mutations";
import { createAuthState } from "@/core/auth/store/session/state";
import { AuthState } from "@/core/auth/store/session/types";
import { State } from "@/app/store/types";
import { uiMutations } from "@/app/store/ui/mutations";
import { createUiState } from "@/app/store/ui/state";
import { UiState } from "@/app/store/ui/types";
import { doctorActions } from "@/modules/doctor/store/actions";
import { doctorMutations } from "@/modules/doctor/store/mutations";
import { createDoctorState } from "@/modules/doctor/store/state";
import { DoctorState } from "@/modules/doctor/store/types";
import { financeActions } from "@/modules/finance/store/actions";
import { financeMutations } from "@/modules/finance/store/mutations";
import { createFinanceState } from "@/modules/finance/store/state";
import { FinanceState } from "@/modules/finance/store/types";
import { personnelActions } from "@/modules/personnel/store/actions";
import { personnelMutations } from "@/modules/personnel/store/mutations";
import { createPersonnelState } from "@/modules/personnel/store/state";
import { PersonnelState } from "@/modules/personnel/store/types";
import { superAdminActions } from "@/modules/super-admin/store/actions";
import { superAdminMutations } from "@/modules/super-admin/store/mutations";
import { createSuperAdminState } from "@/modules/super-admin/store/state";
import { SuperAdminState } from "@/modules/super-admin/store/types";
import { userPortalActions } from "@/modules/user/store/actions";
import { userPortalMutations } from "@/modules/user/store/mutations";
import { createUserPortalState } from "@/modules/user/store/state";
import { UserPortalState } from "@/modules/user/store/types";
import { warehouseAdminActions } from "@/modules/warehouse-admin/store/action";
import { warehouseAdminMutations } from "@/modules/warehouse-admin/store/mutation";
import { createWarehouseAdminState } from "@/modules/warehouse-admin/store/state";
import { WarehouseAdminState } from "@/modules/warehouse-admin/store/types";

export type { AuthState } from "@/core/auth/store/session/types";
export type { CurrentUserState } from "@/core/auth/store/currentUser/types";
export type { State } from "@/app/store/types";
export type { LoginGrade, UiState } from "@/app/store/ui/types";

export const storeKey: InjectionKey<Store<State>> = Symbol();

export const appStore = createStore<State>({
  modules: {
    auth: {
      namespaced: true,
      state: createAuthState() as AuthState,
      mutations: authMutations,
      actions: authActions,
      getters: authGetters,
    },
    boss: {
      namespaced: true,
      state: createBossState() as BossState,
      mutations: bossMutations,
      actions: bossActions,
    },
    currentUser: {
      namespaced: true,
      state: createCurrentUserState() as CurrentUserState,
      mutations: currentUserMutations,
      actions: currentUserActions,
    },
    doctor: {
      namespaced: true,
      state: createDoctorState() as DoctorState,
      mutations: doctorMutations,
      actions: doctorActions,
    },
    finance: {
      namespaced: true,
      state: createFinanceState() as FinanceState,
      mutations: financeMutations,
      actions: financeActions,
    },
    personnel: {
      namespaced: true,
      state: createPersonnelState() as PersonnelState,
      mutations: personnelMutations,
      actions: personnelActions,
    },
    superAdmin: {
      namespaced: true,
      state: createSuperAdminState() as SuperAdminState,
      mutations: superAdminMutations,
      actions: superAdminActions,
    },
    userPortal: {
      namespaced: true,
      state: createUserPortalState() as UserPortalState,
      mutations: userPortalMutations,
      actions: userPortalActions,
    },
    warehouseAdmin: {
      namespaced: true,
      state: createWarehouseAdminState() as WarehouseAdminState,
      mutations: warehouseAdminMutations,
      actions: warehouseAdminActions,
    },
    ui: {
      namespaced: true,
      state: createUiState() as UiState,
      mutations: uiMutations,
    },
  },
});
