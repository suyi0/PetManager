import { UiState } from "./types";

export const createUiState = (): UiState => ({
  personal: false,
  isLogoutButtonActive: false,
  isAcountLoginButtonActive: false,
  isWeChatLoginButtonActive: false,
  isPhoneLoginButtonActive: false,
  LoginGrade: 1,
  showRegister: false,
  choiceActive: false,
  isinitLoginActive: true,
  isLoginButtonActive: false,
});
