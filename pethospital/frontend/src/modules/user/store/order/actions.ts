import { orderApi } from "@/modules/user/api/orderApi";
import { ActionContext, ActionTree } from "vuex";
import { State } from "@/store/types";
import { OrderRecordItem, OrderState, OrderSummary } from "./types";

type OrderActionContext = ActionContext<OrderState, State>;

export const orderActions: ActionTree<OrderState, State> = {
  getOrders({ commit, rootState }: OrderActionContext) {
    commit("setLoading", true);
    commit("setError", null);

    return orderApi
      .getOrders({
        name: rootState.currentUser.userName,
        phone: rootState.currentUser.userPhone,
        email: rootState.currentUser.userEmail,
      })
      .then((response) => {
        const orders = Array.isArray(response?.data?.data)
          ? (response.data.data as OrderRecordItem[])
          : [];

        commit("setOrders", orders);
        return response;
      })
      .catch((error) => {
        commit(
          "setError",
          error instanceof Error ? error.message : "Failed to fetch orders"
        );
        throw error;
      })
      .finally(() => {
        commit("setLoading", false);
      });
  },

  getOrderList({ commit }: OrderActionContext) {
    commit("setLoading", true);
    commit("setError", null);

    return orderApi
      .getOrderList()
      .then((response) => {
        const orderList = Array.isArray(response?.data)
          ? (response.data as OrderSummary[])
          : [];

        commit("setOrderList", orderList);
        return orderList;
      })
      .catch((error) => {
        commit(
          "setError",
          error instanceof Error ? error.message : "Failed to fetch order list"
        );
        throw error;
      })
      .finally(() => {
        commit("setLoading", false);
      });
  },
};
