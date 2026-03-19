import { MutationTree } from "vuex";
import { LoginGrade, UiState } from "./types";

export const uiMutations: MutationTree<UiState> = {
  upDataLoginButtonActive(state, payload: Partial<UiState>) {
    Object.assign(state, payload);
  },

  setLoginGradeActive(state, grades: LoginGrade) {
    state.LoginGrade = grades;
  },

  clearLoginGrade(state) {
    state.LoginGrade = 1;
  },

  openPersonal(state) {
    state.personal = true;
  },

  closePersonal(state) {
    state.personal = false;
  },

  closeLogin(state) {
    state.isLoginButtonActive = false;
    state.isinitLoginActive = false;
    state.isAcountLoginButtonActive = false;
    state.isWeChatLoginButtonActive = false;
    state.isPhoneLoginButtonActive = false;
    state.showRegister = false;
  },
};
