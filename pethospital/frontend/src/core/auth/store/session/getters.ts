import { GetterTree } from "vuex";
import { State } from "@/app/store/types";
import { AuthState } from "./types";

export const authGetters: GetterTree<AuthState, State> = {
  // 处理用户名使其标准化
  formattedUserName(_, _getters, rootState) {
    const nameParts = [
      rootState.currentUser.userLastName,
      rootState.currentUser.userMiddleName,
      rootState.currentUser.userFirstName,
    ]
      .map((part) => String(part || "").trim())
      .filter(Boolean);

    if (nameParts.length > 0) {
      return nameParts.join("");
    }

    return String(rootState.currentUser.userName || "").replace(/·/g, "");
  },
};
