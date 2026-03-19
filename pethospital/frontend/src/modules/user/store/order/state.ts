import { OrderState } from "./types";

export const createOrderState = (): OrderState => {
  return {
    orders: [],
    orderList: [],
    orderDetails: {},
    loading: false,
    error: null,
  };
};
