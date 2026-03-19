export interface CurrentUserState {
  userName: string | null;
  userBirthday: string | null;
  userEmail: string | null;
  userPhone: string | null;
  userAddressId: string | null;
  userAddress: string | null;
  userHeadImage: string | undefined;
}

export interface SetCurrentUserPayload {
  userName: string;
  userPhone: string;
  userEmail: string;
  userBirthday: string;
  userAddress:
    | {
        street?: string;
        city?: string;
        state?: string;
        postalCode?: string;
        country?: string;
      }
    | string;
  userHeadImage: string;
  userAddressId?: string;
  userType?: number | null;
}
