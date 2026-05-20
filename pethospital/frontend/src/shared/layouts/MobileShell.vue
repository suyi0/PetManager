<template>
  <div class="mobile-shell">
    <header v-if="$slots.header" class="mobile-shell__header">
      <slot name="header" />
    </header>

    <main class="mobile-shell__content">
      <slot />
    </main>

    <nav v-if="$slots.tabbar" class="mobile-shell__tabbar">
      <slot name="tabbar" />
    </nav>
  </div>
</template>

<style scoped>
.mobile-shell {
  box-sizing: border-box;
  min-height: 100vh;
  color: var(--pm-color-text);
  background: var(--pm-color-page);
}

.mobile-shell__header {
  position: sticky;
  top: 0;
  z-index: 20;
  min-height: var(--pm-mobile-header-height);
  background: var(--pm-color-surface);
  border-bottom: 1px solid var(--pm-color-border);
}

.mobile-shell__content {
  box-sizing: border-box;
  min-height: calc(100vh - var(--pm-mobile-header-height));
  padding: var(--pm-mobile-page-padding);
  padding-bottom: calc(var(--pm-mobile-tabbar-height) + 18px);
}

.mobile-shell__tabbar {
  position: fixed;
  right: 0;
  bottom: 0;
  left: 0;
  z-index: 30;
  min-height: var(--pm-mobile-tabbar-height);
  background: var(--pm-color-surface);
  border-top: 1px solid var(--pm-color-border);
}
</style>
