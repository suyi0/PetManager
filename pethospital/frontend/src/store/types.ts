import { DoctorDataItem } from "@/modules/doctor/api/types";
import { BossState } from "@/modules/boss/store/types";
import { DoctorState } from "@/modules/doctor/store/types";
import { PersonnelState } from "@/modules/personnel/store/types";
import { SuperAdminState } from "@/modules/super-admin/store/types";
import { UserPortalState } from "@/modules/user/store/types";
import { WarehouseAdminState } from "@/modules/warehouse-admin/store/types";
import { CurrentUserState } from "@/core/auth/store/currentUser/types";
import { AuthState } from "@/core/auth/store/session/types";
import { UiState } from "@/store/ui/types";

/**
 * 轻量缓存时效。
 * 页面在短时间内来回切换时直接复用缓存，停留较久后再自动重拉。
 */
const CACHE_TTL_MS = 60 * 1000;

/**
 * 判断缓存是否已经超过可复用时长。
 */
const isExpired = (lastFetchedAt: number | null): boolean =>
  lastFetchedAt === null || Date.now() - lastFetchedAt > CACHE_TTL_MS;

/**
 * 统一决定当前这份缓存是否需要重新请求。
 * force 用于手动刷新，dirty 用于业务操作后主动失效，TTL 用于长时间停留后的自动补拉。
 * meta 用于缓存元数据。
 * force、dirty、TTL 三者之间有优先级关系。 force > dirty > TTL > meta.dirty
 */
export const shouldFetch = (meta: CacheMeta, force?: boolean): boolean =>
  force === true || !meta.loaded || meta.dirty || isExpired(meta.lastFetchedAt);

/**
 * 每一类后台数据都带一份独立元信息，用来判断是否已经加载、
 * 是否被后续操作标记为脏数据，以及上次成功拉取的时间。
 */
export interface CacheMeta {
  loaded: boolean;
  dirty: boolean;
  loading: boolean;
  lastFetchedAt: number | null;
}

export interface State {
  auth: AuthState;
  boss: BossState;
  currentUser: CurrentUserState;
  doctor: DoctorState;
  personnel: PersonnelState;
  superAdmin: SuperAdminState;
  userPortal: UserPortalState;
  warehouseAdmin: WarehouseAdminState;
  ui: UiState;
}

export type { DoctorDataItem };
