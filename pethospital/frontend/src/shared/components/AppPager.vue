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
  display: flex;
  align-items: center;
  gap: 10px;
  color: #5c7470;
  font-size: 13px;
}

.pager-button,
.pager-button--ghost {
  padding: 9px 12px;
  border: 1px solid rgba(144, 175, 166, 0.24);
  border-radius: 14px;
  background: linear-gradient(135deg, #29565a, #7d5348);
  color: #fffdfb;
  font-size: 12px;
  font-weight: 700;
  cursor: pointer;
  box-shadow: 0 12px 24px rgba(49, 82, 87, 0.12);
}

.pager-button--ghost {
  background: rgba(241, 248, 244, 0.96);
  color: #214f4b;
}

.pager-button:disabled {
  opacity: 0.52;
  cursor: not-allowed;
  box-shadow: none;
}

.pager-jump {
  display: inline-flex;
  align-items: center;
  gap: 8px;
}

.pager-jump input {
  width: 62px;
  min-height: 34px;
  padding: 0 10px;
  border: 1px solid rgba(144, 175, 166, 0.24);
  border-radius: 12px;
  background: rgba(255, 255, 255, 0.96);
  color: #214f4b;
}

@media (max-width: 960px) {
  .pager {
    width: 100%;
    flex-wrap: wrap;
  }
}
</style>
