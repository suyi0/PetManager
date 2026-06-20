<template>
  <article class="card" :class="tone">
    <p>{{ label }}</p>
    <strong>{{ value }}</strong>
    <span>{{ hint }}</span>
  </article>
</template>

<script lang="ts">
import { defineComponent, PropType } from "vue";

export default defineComponent({
  name: "WarehouseStatCard",
  props: {
    label: {
      type: String,
      required: true,
    },
    value: {
      type: [String, Number],
      required: true,
    },
    hint: {
      type: String,
      default: "",
    },
    tone: {
      type: String as PropType<"neutral" | "warning" | "danger">,
      default: "neutral",
    },
  },
});
</script>

<style scoped>
.card {
  display: grid;
  gap: 6px;
  padding: 16px 18px;
  border-radius: 14px;
  border: 1px solid var(--saas-border, #e7e9ee);
  background: var(--saas-surface, #fff);
  box-shadow: 0 1px 2px rgba(16, 24, 40, 0.04);
}

.card p {
  margin: 0;
  font-size: 13px;
  color: var(--saas-muted, #64748b);
}

.card strong {
  display: block;
  font-size: 26px;
  font-weight: 800;
  letter-spacing: -0.02em;
  color: var(--saas-text, #0f172a);
  font-variant-numeric: tabular-nums;
}

.card span {
  color: var(--saas-faint, #94a3b8);
  font-size: 12px;
}

.warning {
  background: var(--saas-amber-50, #fffbeb);
  border-color: rgba(245, 158, 11, 0.28);
}

.warning strong {
  color: var(--saas-amber, #b45309);
}

.danger {
  background: var(--saas-red-50, #fef2f2);
  border-color: rgba(220, 38, 38, 0.22);
}

.danger strong {
  color: var(--saas-red, #dc2626);
}
</style>
