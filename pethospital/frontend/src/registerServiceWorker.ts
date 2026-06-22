/* eslint-disable no-console */

import { register } from "register-service-worker";

const isLocalHost = () =>
  typeof window !== "undefined" &&
  ["localhost", "127.0.0.1", "0.0.0.0"].includes(window.location.hostname);

const unregisterLocalServiceWorkers = async () => {
  if (typeof navigator === "undefined" || !("serviceWorker" in navigator)) {
    return;
  }

  const registrations = await navigator.serviceWorker.getRegistrations();
  await Promise.all(
    registrations.map((registration) => registration.unregister())
  );

  if (typeof window !== "undefined" && "caches" in window) {
    const keys = await window.caches.keys();
    await Promise.all(keys.map((key) => window.caches.delete(key)));
  }
};

if (process.env.NODE_ENV === "production" && isLocalHost()) {
  unregisterLocalServiceWorkers().catch((error) => {
    console.error("Failed to clear local service worker cache:", error);
  });
} else if (process.env.NODE_ENV === "production") {
  register(`${process.env.BASE_URL}service-worker.js`, {
    ready() {
      console.log(
        "App is being served from cache by a service worker.\n" +
          "For more details, visit https://goo.gl/AFskqB"
      );
    },
    registered() {
      console.log("Service worker has been registered.");
    },
    cached() {
      console.log("Content has been cached for offline use.");
    },
    updatefound() {
      console.log("New content is downloading.");
    },
    updated() {
      console.log("New content is available; please refresh.");
    },
    offline() {
      console.log(
        "No internet connection found. App is running in offline mode."
      );
    },
    error(error) {
      console.error("Error during service worker registration:", error);
    },
  });
}
