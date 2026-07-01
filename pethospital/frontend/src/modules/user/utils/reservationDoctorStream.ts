import { authStorage } from "@/core/auth/utils/authStorage";

type DoctorListMessage = {
  event?: string;
  version?: number;
  sentAt?: string;
};

const RETRY_DELAYS = [10000, 30000, 60000, 120000];

const createWebSocketUrl = (path: string) => {
  const protocol = window.location.protocol === "https:" ? "wss:" : "ws:";
  return `${protocol}//${window.location.host}${path}`;
};

export const subscribeReservationDoctors = (
  onDoctorListChanged: () => void
) => {
  let socket: WebSocket | null = null;
  let retryTimer: number | null = null;
  let retryCount = 0;
  let closedByClient = false;

  const connect = () => {
    const token = authStorage.getToken();
    if (!token) {
      closedByClient = true;
      return;
    }

    socket = new WebSocket(
      createWebSocketUrl(
        `/realtime/users/reservation-doctors?token=${encodeURIComponent(token)}`
      )
    );

    socket.onmessage = (event) => {
      try {
        const message = JSON.parse(event.data) as DoctorListMessage;

        if (message.event === "doctorList") {
          retryCount = 0;
          onDoctorListChanged();
        }
      } catch {
        // Ignore malformed realtime messages and wait for the next signal.
      }
    };

    socket.onerror = () => {
      socket?.close();
    };

    socket.onclose = () => {
      socket = null;

      if (!closedByClient) {
        retryCount += 1;
        const retryDelay =
          RETRY_DELAYS[Math.min(retryCount - 1, RETRY_DELAYS.length - 1)];
        retryTimer = window.setTimeout(connect, retryDelay);
      }
    };
  };

  connect();

  return () => {
    closedByClient = true;

    if (retryTimer !== null) {
      window.clearTimeout(retryTimer);
      retryTimer = null;
    }

    socket?.close();
    socket = null;
  };
};
