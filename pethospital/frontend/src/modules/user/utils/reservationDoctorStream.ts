import { subscribeTokenRealtimeStream } from "@/shared/utils/realtimeStream";

type DoctorListMessage = {
  event?: string;
  version?: number;
  sentAt?: string;
};

export const subscribeReservationDoctors = (onDoctorListChanged: () => void) =>
  subscribeTokenRealtimeStream<DoctorListMessage>(
    "/realtime/users/reservation-doctors",
    (message) => {
      if (message.event === "doctorList") {
        onDoctorListChanged();
        return true;
      }

      return false;
    },
    { resolveTokenOnConnect: true }
  );
