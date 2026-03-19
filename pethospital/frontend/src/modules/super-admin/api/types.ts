export interface WorkTimeRecord {
  source: "online_doctors" | "work_records";
  id: number;
  user_id: number;
  name: string;
  date: string;
  check_in_time: string;
  check_out_time: string;
  status: string;
  notes: string;
  created_at: string;
  updated_at: string;
}

export interface UserRow {
  id: number;
  type_id: number;
  name: string;
  phone: string;
  email: string;
  birthday: string;
  address_id: string;
  head_image: string;
}

export interface CreateUserPayload {
  name: string;
  phone?: string;
  email?: string;
  password?: string;
  birthday?: string;
  address_id?: number;
}

export interface ApiListResponse<T> {
  success?: boolean;
  data?: T[];
  message?: string;
}
