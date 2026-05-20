import { computed } from "vue";
import type { Router } from "vue-router";
import { authStorage } from "./authStorage";

export const useBossPortalReturn = (router: Router) => {
  const bossPortalReturn = computed(() => authStorage.loadBossPortalReturn());
  const showBossReturn = computed(() => Boolean(bossPortalReturn.value));

  const returnToBossPortal = async () => {
    const target = bossPortalReturn.value?.returnTo || "/boss/overview";
    authStorage.clearBossPortalReturn();
    await router.push(target);
  };

  return {
    showBossReturn,
    returnToBossPortal,
  };
};
