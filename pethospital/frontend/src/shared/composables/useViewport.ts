import { computed, onBeforeUnmount, onMounted, ref } from "vue";

const MOBILE_BREAKPOINT = 768;

export const useViewport = () => {
  const width = ref(
    typeof window === "undefined" ? MOBILE_BREAKPOINT + 1 : window.innerWidth
  );

  const updateWidth = () => {
    width.value = window.innerWidth;
  };

  onMounted(() => {
    updateWidth();
    window.addEventListener("resize", updateWidth, { passive: true });
  });

  onBeforeUnmount(() => {
    window.removeEventListener("resize", updateWidth);
  });

  const isMobile = computed(() => width.value <= MOBILE_BREAKPOINT);

  return {
    width,
    isMobile,
    isDesktop: computed(() => !isMobile.value),
  };
};
