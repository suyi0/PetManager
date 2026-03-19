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
  padding: 18px;
  border-radius: 16px;
  border: 1px solid #dce7ff;
  background: linear-gradient(145deg, #ffffff 0%, #f5f8ff 100%);
}

.label {
  margin: 0;
  font-size: 13px;
  color: #5b6885;
}

.value {
  margin: 8px 0;
  font-size: 28px;
  color: #182442;
  font-weight: 700;
}

.hint {
  margin: 0;
  font-size: 12px;
  color: #3f7df2;
}
</style>
