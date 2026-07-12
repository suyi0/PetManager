import { computed } from "vue";
import type { Router } from "vue-router";
import { authStorage } from "./authStorage";

// 返回按钮标签按来源端动态显示：从超管「快捷入口」跳入他端时回到管理员端，
// 从总裁端跳入时回到总裁端。判据只看存储的 returnTo 路径前缀，不新增存储字段。
const labelForReturnTo = (returnTo: string): string => {
  if (returnTo.startsWith("/super-admin")) return "返回管理员端";
  if (returnTo.startsWith("/boss")) return "返回总裁端";
  return "返回上级端";
};

export const useBossPortalReturn = (router: Router) => {
  const bossPortalReturn = computed(() => authStorage.loadBossPortalReturn());
  const showBossReturn = computed(() => Boolean(bossPortalReturn.value));
  const returnLabel = computed(() =>
    labelForReturnTo(bossPortalReturn.value?.returnTo || "")
  );

  const returnToBossPortal = async () => {
    const target = bossPortalReturn.value?.returnTo || "/boss/overview";
    authStorage.clearBossPortalReturn();
    await router.push(target);
  };

  return {
    showBossReturn,
    returnLabel,
    returnToBossPortal,
  };
};
