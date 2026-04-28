import { createCacheMeta } from "@/store/state";
import { PersonnelState } from "./types";

export const createPersonnelState = (): PersonnelState => ({
  users: [],
  usersMeta: createCacheMeta(),
});
