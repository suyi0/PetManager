import { State as RootState } from "@/store/types";

export interface OrderRecordItem {
  id: number;
  name: string;
  time?: number;
  price?: number;
}

export interface OrderSummary {
  id: number;
  pet_id: number;
  pet_name: string;
  doctor_id: number;
  order_type: string;
  order_data: string;
  order_status: string;
  order_totalprice: number;
  created_at: string;
}

export interface OrderMedicine {
  id: number;
  medicine_id: number;
  quantity: number;
  price: number;
  total_price: number;
  medicine_name: string;
  medicine_type: string;
}

export interface OrderDetail extends OrderSummary {
  orderMedicines: OrderMedicine[];
}

export interface OrderState {
  orders: OrderRecordItem[];
  orderList: OrderSummary[];
  orderDetails: Record<number, OrderDetail>;
  loading: boolean;
  error: string | null;
}

export interface OrderRootState extends RootState {
  order: OrderState;
}
