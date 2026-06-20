<template>
  <section class="stat-card">
    <p class="label">{{ label }}</p>
    <p class="value">{{ value }}</p>
    <p class="hint">{{ hint }}</p>
  </section>
</template>

<script lang="ts">
import { defineComponent } from "vue";

export default defineComponent({
  name: "StatCard",
  props: {
    label: { type: String, required: true },
    value: { type: [String, Number], required: true },
    hint: { type: String, required: true },
  },
});
</script>

<style scoped>
.stat-card {
  padding: 16px 18px;
  border-radius: 14px;
  border: 1px solid var(--saas-border, #e7e9ee);
  background: var(--saas-surface, #fff);
  box-shadow: 0 1px 2px rgba(16, 24, 40, 0.04);
}

.label {
  margin: 0;
  font-size: 13px;
  color: var(--saas-muted, #64748b);
}

.value {
  margin: 6px 0;
  font-size: 28px;
  color: var(--saas-text, #0f172a);
  font-weight: 800;
  letter-spacing: -0.02em;
  font-variant-numeric: tabular-nums;
}

.hint {
  margin: 0;
  font-size: 12px;
  color: var(--saas-faint, #94a3b8);
}
</style>
