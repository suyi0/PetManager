import { GetterTree } from "vuex";
import { State } from "@/store/types";
import { AuthState } from "./types";

export const authGetters: GetterTree<AuthState, State> = {
  // 处理用户名使其标准化
  formattedUserName(_, _getters, rootState) {
    const fullName = rootState.currentUser.userName;
    if (!fullName) return "";

    const parts = fullName.split("·");
    let result = "";

    if (parts[0] && parts[0].trim() !== "") {
      result += parts[0];
    }

    if (parts[1] && parts[1].trim() !== "") {
      result += parts[1];
    }

    if (parts[2] && parts[2].trim() !== "") {
      result += parts[2];
    }

    return result;
  },
};
