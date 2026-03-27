import { DoctorDataItem } from "@/modules/doctor/store/types";
import { ActionContext, ActionTree } from "vuex";
import { reservationApi } from "@/modules/user/api/reservationApi";
import { authStorage } from "@/core/auth/utils/authStorage";
import { State } from "@/store/types";
import { ReservationModuleState, ReservateState } from "./types";

type ReservationActionContext = ActionContext<ReservationModuleState, State>;

// 预约时间表项
type ReservationScheduleItem = {
  year: string | number;
  date: string;
  weekday: string;
  time_slots?: Record<string, unknown>;
};

// 定时更新预约数据的定时器
let dailyUpdateTimer: ReturnType<typeof setTimeout> | null = null;

export const reservationActions: ActionTree<ReservationModuleState, State> = {
  // 获取预约时间
  scheduleTime({ commit }: ReservationActionContext) {
    reservationApi.getScheduleData().then((response) => {
      if (response.status === 200 && response.data.success) {
        if (response && response.data && typeof response.data === "object") {
          if (response.data.data && Array.isArray(response.data.data)) {
            const year: string[] = [];
            const month: string[] = [];
            const day: string[] = [];
            const weekday: string[] = [];
            const slots: string[][] = [];

            for (const item of response.data
              .data as ReservationScheduleItem[]) {
              year.push(item.year.toString());
              const dateParts = item.date.split("-");
              month.push(dateParts[0]);
              day.push(dateParts[1]);
              weekday.push(item.weekday);

              const timeSlotsArray: string[] = [];
              if (item.time_slots) {
                Object.values(item.time_slots).forEach((slot) => {
                  timeSlotsArray.push(String(slot));
                });
              }
              slots.push(timeSlotsArray);
            }

            commit("setReservate", { year, month, day, weekday, slots });
          }
        }
      }
    });
  },

  // 获取预约医生表
  scheduleDoctor({ commit }: ReservationActionContext) {
    reservationApi.getDoctors().then((response) => {
      if (response.status === 200 && response.data.success) {
        if (response.data.data && Array.isArray(response.data.data)) {
          commit(
            "setDoctorData",
            response.data.data.map((item: DoctorDataItem) => ({
              id: item.id,
              name: item.name,
              specialty: item.specialty,
              status: item.status,
            }))
          );
        }
      }
    });
  },

  // 创建预约记录
  createReservationRecord(
    { rootState }: ReservationActionContext,
    payload: {
      upDoctorId: number;
      upYear: string;
      upMonth: string;
      upDay: string;
      upSlot: string;
    }
  ) {
    return reservationApi
      .createReservationRecord({
        name: rootState.currentUser.userName,
        phone: rootState.currentUser.userPhone,
        email: rootState.currentUser.userEmail,
        doctorId: payload.upDoctorId,
        date: payload.upYear + "-" + payload.upMonth + "-" + payload.upDay,
        slot: payload.upSlot,
      })
      .then((response) => {
        if (response.status === 200 && response.data.success) {
          return response;
        }
        if (response.status === 400) {
          return response;
        }
        return response;
      })
      .catch((error) => {
        if (error.response && error.response.status) {
          return error.response.status;
        }
        if (error.request) {
          console.error("No response received:", error.request);
          return null;
        }
        console.error("Error:", error.message);
        return null;
      });
  },

  // 初始化预约数据
  initReservateData({ commit }: ReservationActionContext) {
    const reservateData = authStorage.loadReservate();

    if (
      reservateData.year.length > 0 ||
      reservateData.month.length > 0 ||
      reservateData.day.length > 0 ||
      reservateData.weekday.length > 0 ||
      reservateData.slots.length > 0
    ) {
      commit(
        "setReservate",
        reservateData as Omit<ReservateState, "doctorData"> // as Omit<ReservateState, "doctorData"> -- 忽略 "doctorData" 属性
      );
    }

    if (reservateData.doctorData.length > 0) {
      commit("setDoctorData", reservateData.doctorData);
    }
  },

  // 定时更新预约数据
  scheduleDailyUpdate({ dispatch, rootState }: ReservationActionContext) {
    if (dailyUpdateTimer) {
      clearTimeout(dailyUpdateTimer);
    }

    if (rootState.auth.isLoggedIn) {
      dispatch("scheduleTime");
    }

    const scheduleNextUpdate = () => {
      const current = new Date();
      const nextMidnight = new Date();
      nextMidnight.setHours(24, 0, 0, 0);

      dailyUpdateTimer = setTimeout(() => {
        if (rootState.auth.isLoggedIn) {
          dispatch("scheduleTime");
        }
        scheduleNextUpdate();
      }, nextMidnight.getTime() - current.getTime());
    };

    scheduleNextUpdate();
  },

  // 获取预约记录
  getReservation({ rootState }: ReservationActionContext) {
    return reservationApi
      .getReservations({
        name: rootState.currentUser.userName,
        phone: rootState.currentUser.userPhone,
        email: rootState.currentUser.userEmail,
      })
      .then((response) => {
        if (response.status === 200 && response.data.success) {
          return response;
        }
        return response;
      });
  },
};
