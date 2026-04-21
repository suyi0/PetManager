import { CacheMeta } from "@/store/types";

export interface CurrentUserState {
  userName: string | null;
  userBirthday: string | null;
  userEmail: string | null;
  userPhone: string | null;
  userAddressId: string | null;
  userAddress: string | null;
  userHeadImage: string | undefined;
  profileMeta: CacheMeta;
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
  userRole?: string | null;
}
