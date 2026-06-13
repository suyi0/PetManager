import { ActionContext, ActionTree } from "vuex";
import { State } from "@/app/store/types";
import { UiState } from "./types";

type UiActionContext = ActionContext<UiState, State>;

export const uiActions: ActionTree<UiState, State> = {
  /**
   * 打开个人中心浮层。
   */
  openPersonal({ commit }: UiActionContext) {
    commit("openPersonal");
  },

  /**
   * 关闭个人中心浮层。
   */
  closePersonal({ commit }: UiActionContext) {
    commit("closePersonal");
  },

  /**
   * 根据当前状态切换个人中心浮层。
   */
  togglePersonal({ state, commit }: UiActionContext) {
    commit(state.personal ? "closePersonal" : "openPersonal");
  },
};
