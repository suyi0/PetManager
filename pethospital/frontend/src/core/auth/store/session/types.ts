export interface AuthState {
  userType: number | null;
  userRole: string | null;
  token: string | null;
  isLoggedIn: boolean;
}
