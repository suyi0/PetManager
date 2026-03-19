import { MutationTree } from "vuex";
import {
  OrderDetail,
  OrderRecordItem,
  OrderState,
  OrderSummary,
} from "./types";

export const orderMutations: MutationTree<OrderState> = {
  setOrders(state, orders: OrderRecordItem[]) {
    state.orders = orders;
  },

  setOrderList(state, orderList: OrderSummary[]) {
    state.orderList = orderList;
  },

  setOrderDetails(state, orderDetails: Record<number, OrderDetail>) {
    state.orderDetails = orderDetails;
  },

  setLoading(state, loading: boolean) {
    state.loading = loading;
  },

  setError(state, error: string | null) {
    state.error = error;
  },
};
