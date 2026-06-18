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
  position: relative;
  overflow: hidden;
  border-radius: 12px;
  padding: 14px 16px;
  border: 1px solid #dfe7df;
  background: #ffffff;
  box-shadow: 0 12px 28px rgba(35, 62, 46, 0.06);
}

.card p {
  margin: 0;
  font-size: 12px;
  color: #6d7b72;
  font-weight: 700;
}

.card strong {
  display: block;
  margin: 8px 0 6px;
  font-size: 20px;
  color: #1d3429;
  letter-spacing: 0;
}

.card span {
  color: #6d7b72;
  font-size: 12px;
}

.warning {
  background: #fffaf0;
  border-color: rgba(155, 104, 23, 0.22);
}

.warning strong {
  color: #9b6817;
}

.danger {
  background: #fff6f7;
  border-color: rgba(176, 68, 85, 0.22);
}

.danger strong {
  color: #b04455;
}
</style>
