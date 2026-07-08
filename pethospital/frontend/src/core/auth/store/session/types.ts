export interface AuthState {
  userType: number | null;
  userRole: string | null;
  accountType: string | null;
  positionId: number | null;
  staffKind: string | null;
  permissions: string[];
  token: string | null;
  isLoggedIn: boolean;
}
