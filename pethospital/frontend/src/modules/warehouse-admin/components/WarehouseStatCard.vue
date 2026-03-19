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
  border-radius: 18px;
  padding: 16px 18px;
  border: 1px solid rgba(147, 196, 255, 0.3);
  background: radial-gradient(
      circle at top right,
      rgba(106, 190, 255, 0.24),
      transparent 36%
    ),
    linear-gradient(
      180deg,
      rgba(224, 237, 255, 0.98),
      rgba(197, 216, 244, 0.98)
    );
  box-shadow: inset 0 0 0 1px rgba(255, 255, 255, 0.28),
    0 10px 28px rgba(70, 127, 205, 0.16);
}

.card::after {
  content: "";
  position: absolute;
  inset: 0;
  border-top: 1px solid rgba(169, 216, 255, 0.28);
  pointer-events: none;
}

.card p {
  margin: 0;
  font-size: 12px;
  color: #476489;
}

.card strong {
  display: block;
  margin: 8px 0 6px;
  font-size: 20px;
  font-family: "Rajdhani", "Noto Sans SC", sans-serif;
  color: #15385e;
}

.card span {
  color: #4a678d;
  font-size: 12px;
}

.warning strong,
.warning span {
  color: #cc8f2f;
}

.danger strong,
.danger span {
  color: #d65f76;
}
</style>
