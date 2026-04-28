import { CacheMeta } from "@/store/types";
import { UserRow } from "@/modules/super-admin/api/types";

export interface PersonnelState {
  users: UserRow[];
  usersMeta: CacheMeta;
}
