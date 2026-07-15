import { ActionContext, ActionTree } from "vuex";
import { State } from "@/app/store/types";
import { PersonnelState } from "./types";

type PersonnelActionContext = ActionContext<PersonnelState, State>;

// 任职与薪酬写操作由角色自有页面直接调用 personnelApi。
// Store 只保留缓存失效信号，不再暴露医生/仓库管理员硬编码 action。
export const personnelActions: ActionTree<PersonnelState, State> = {
  markUsersDirty({ commit }: PersonnelActionContext) {
    commit("markUsersDirty");
  },
};
