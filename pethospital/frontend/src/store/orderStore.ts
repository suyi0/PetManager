import { createStore, Store, ActionContext } from "vuex";
import { InjectionKey } from "vue";
import axios from "axios";

export interface OrderSummary {
    id: number;
    pet_id: number;
    pet__name: string;
    doctor_id: number;
    order_type: string;
    order_date: string;
    order_status: string;
    order_totalpice: number;
    order_time: string;
}

export interface OrderDetail extends OrderSummary {
    id: number;
    pet_id: number;
    doctor_id: number;
    order_date: string;
    order_type: string;
    order_status: string;
    order_totalpice: number;
    order_time: string;
    orderMedicines: OrderMedicine[];  // 详情中才包含药品信息
}

export interface OrderMedicine {
    id: number;
    medicine_id: number;
    quantity: number;
    price: number;
    total_price: number;
    medicine__name: string;
    medicine__type: string;
};

export interface OrderState {
    orderList: OrderSummary[];     // 订单列表（概要信息）
    orderDetails: Record<number, OrderDetail>;  // 订单详情缓存（key 为订单 ID）
    loading: boolean;
    error: string | null;
}

