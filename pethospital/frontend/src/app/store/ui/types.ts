export type LoginGrade = 1 | 2 | 3 | 4 | 5;

export interface UiState {
  personal: boolean;
  LoginGrade: LoginGrade | null;
  isLogoutButtonActive: boolean;
  isAcountLoginButtonActive: boolean;
  isWeChatLoginButtonActive: boolean;
  isPhoneLoginButtonActive: boolean;
  showRegister: boolean;
  choiceActive: boolean;
  isinitLoginActive: boolean;
  isLoginButtonActive: boolean;
}
