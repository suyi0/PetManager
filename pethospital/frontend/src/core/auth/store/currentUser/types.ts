import { CacheMeta } from "@/app/store/types";

export interface CurrentUserState {
  userName: string | null;
  userLastName: string | null;
  userMiddleName: string | null;
  userFirstName: string | null;
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
  userLastName?: string;
  userMiddleName?: string;
  userFirstName?: string;
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
