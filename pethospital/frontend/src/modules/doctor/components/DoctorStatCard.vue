<template>
  <article class="card">
    <span class="label">{{ label }}</span>
    <strong class="value">{{ value }}</strong>
    <small class="hint">{{ hint }}</small>
  </article>
</template>

<script lang="ts">
import { defineComponent } from "vue";

export default defineComponent({
  name: "DoctorStatCard",
  props: {
    label: { type: String, required: true },
    value: { type: [String, Number], required: true },
    hint: { type: String, required: true },
  },
});
</script>

<style scoped>
.card {
  position: relative;
  display: grid;
  gap: 10px;
  padding: 22px 20px 18px;
  border-radius: 24px;
  border: 1px solid rgba(159, 188, 179, 0.28);
  background: linear-gradient(180deg, rgba(255, 253, 250, 0.98), #f4fbf7);
  box-shadow: 0 18px 34px rgba(38, 71, 66, 0.06);
}

.card::after {
  content: "";
  position: absolute;
  right: 18px;
  top: 18px;
  width: 42px;
  height: 42px;
  border-radius: 16px;
  background: linear-gradient(135deg, rgba(213, 236, 229, 0.95), #c9e6dc);
}

.label {
  font-size: 12px;
  color: #708783;
  letter-spacing: 0.08em;
  text-transform: uppercase;
}

.value {
  position: relative;
  z-index: 1;
  font-size: 34px;
  line-height: 1;
  color: #17393d;
}

.hint {
  font-size: 12px;
  color: #8b9f9b;
}
</style>
