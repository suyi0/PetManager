<template>
  <div class="pager">
    <button
      type="button"
      class="pager-button"
      :disabled="page <= 1"
      @click="$emit('update:page', page - 1)"
    >
      上一页
    </button>
    <span>{{ page }} / {{ totalPages }}</span>
    <button
      type="button"
      class="pager-button"
      :disabled="page >= totalPages"
      @click="$emit('update:page', page + 1)"
    >
      下一页
    </button>
    <label class="pager-jump">
      <span>跳转</span>
      <input
        v-model="localPageInput"
        type="number"
        min="1"
        :max="totalPages"
        @keyup.enter="submitPage"
      />
    </label>
    <button
      type="button"
      class="pager-button pager-button--ghost"
      @click="submitPage"
    >
      确定
    </button>
  </div>
</template>

<script lang="ts">
import { defineComponent, ref, watch } from "vue";

export default defineComponent({
  name: "AppPager",
  props: {
    page: {
      type: Number,
      required: true,
    },
    totalPages: {
      type: Number,
      required: true,
    },
  },
  emits: ["update:page"],
  setup(props, { emit }) {
    const localPageInput = ref(String(props.page));

    watch(
      () => props.page,
      (value) => {
        localPageInput.value = String(value);
      },
      { immediate: true }
    );

    const submitPage = () => {
      const nextPage = Number(localPageInput.value);
      if (!Number.isFinite(nextPage)) {
        localPageInput.value = String(props.page);
        return;
      }

      emit(
        "update:page",
        Math.min(props.totalPages, Math.max(1, Math.floor(nextPage)))
      );
    };

    return {
      localPageInput,
      submitPage,
    };
  },
});
</script>

<style scoped>
.pager {
  display: inline-flex;
  align-items: center;
  gap: 8px;
  color: #64748b;
  font-size: 13px;
}

.pager-button,
.pager-button--ghost {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  height: 36px;
  padding: 0 12px;
  border: 1px solid #e5e7eb;
  border-radius: 8px;
  background: #ffffff;
  color: #374151;
  font-size: 13px;
  font-weight: 600;
  cursor: pointer;
  transition: background 0.15s ease, border-color 0.15s ease, color 0.15s ease;
}

.pager-button:hover:not(:disabled),
.pager-button--ghost:hover {
  background: #f6f7f9;
  border-color: #cbd5e1;
}

.pager-button:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.pager-jump {
  display: inline-flex;
  align-items: center;
  gap: 6px;
}

.pager-jump input {
  width: 56px;
  height: 36px;
  padding: 0 10px;
  border: 1px solid #e5e7eb;
  border-radius: 8px;
  background: #ffffff;
  color: #374151;
  font-size: 13px;
}

.pager-jump input:focus {
  outline: none;
  border-color: #94a3b8;
}

@media (max-width: 960px) {
  .pager {
    width: 100%;
    flex-wrap: wrap;
  }
}
</style>
