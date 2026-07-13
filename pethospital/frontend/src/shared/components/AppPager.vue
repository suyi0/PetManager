<template>
  <div class="pager">
    <span v-if="totalItems >= 0" class="pager-count">
      共 <b>{{ totalItems }}</b> 条<template v-if="pageSize > 0"> · 每页 <b>{{ pageSize }}</b> 条</template>
    </span>

    <button
      type="button"
      class="pager-button pager-button--nav"
      :disabled="page <= 1"
      @click="$emit('update:page', page - 1)"
    >
      ‹ 上一页
    </button>

    <template v-for="(item, index) in pageList" :key="`${item}-${index}`">
      <span v-if="item === ELLIPSIS" class="pager-dots">…</span>
      <button
        v-else
        type="button"
        class="pager-button pager-button--num"
        :class="{ 'is-active': item === page }"
        @click="$emit('update:page', item)"
      >
        {{ item }}
      </button>
    </template>

    <button
      type="button"
      class="pager-button pager-button--nav"
      :disabled="page >= totalPages"
      @click="$emit('update:page', page + 1)"
    >
      下一页 ›
    </button>

    <label class="pager-jump">
      <span>跳至</span>
      <input
        v-model="localPageInput"
        type="number"
        min="1"
        :max="totalPages"
        @keyup.enter="submitPage"
        @blur="submitPage"
      />
      <span>页</span>
    </label>
  </div>
</template>

<script lang="ts">
import { computed, defineComponent, ref, watch } from "vue";

const ELLIPSIS = "…";

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
    // 可选：显式传入（含 0）即展示「共 N 条」；默认 -1 表示未传，保持旧版精简形态，向后兼容既有调用点。
    totalItems: {
      type: Number,
      default: -1,
    },
    pageSize: {
      type: Number,
      default: 0,
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

    // 页码窗口：总页数 ≤ 7 时全展开，否则首尾常驻、当前页左右各一页，中间用省略号占位。
    const pageList = computed<(number | string)[]>(() => {
      const total = props.totalPages;
      const current = props.page;
      if (total <= 7) {
        return Array.from({ length: total }, (_, index) => index + 1);
      }
      const items: (number | string)[] = [1];
      const left = Math.max(2, current - 1);
      const right = Math.min(total - 1, current + 1);
      if (left > 2) {
        items.push(ELLIPSIS);
      }
      for (let p = left; p <= right; p += 1) {
        items.push(p);
      }
      if (right < total - 1) {
        items.push(ELLIPSIS);
      }
      items.push(total);
      return items;
    });

    const submitPage = () => {
      const nextPage = Number(localPageInput.value);
      if (!Number.isFinite(nextPage)) {
        localPageInput.value = String(props.page);
        return;
      }
      const clamped = Math.min(
        props.totalPages,
        Math.max(1, Math.floor(nextPage))
      );
      localPageInput.value = String(clamped);
      if (clamped !== props.page) {
        emit("update:page", clamped);
      }
    };

    return {
      ELLIPSIS,
      localPageInput,
      pageList,
      submitPage,
    };
  },
});
</script>

<style scoped>
.pager {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  flex-wrap: wrap;
  color: #64748b;
  font-size: 13px;
}

.pager-count {
  margin-right: 8px;
  font-size: 12px;
  color: #64748b;
}

.pager-count b {
  color: #334155;
  font-variant-numeric: tabular-nums;
}

.pager-button {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 34px;
  height: 34px;
  padding: 0 11px;
  border: 1px solid #e5e7eb;
  border-radius: 9px;
  background: #ffffff;
  color: #374151;
  font-size: 13px;
  font-weight: 600;
  font-variant-numeric: tabular-nums;
  cursor: pointer;
  transition: background 0.15s ease, border-color 0.15s ease,
    color 0.15s ease, box-shadow 0.15s ease;
}

.pager-button--num {
  padding: 0 8px;
}

.pager-button:hover:not(:disabled):not(.is-active) {
  border-color: #2f7dcb;
  color: #2f7dcb;
}

.pager-button.is-active {
  border-color: #2f7dcb;
  background: #2f7dcb;
  color: #ffffff;
  box-shadow: 0 4px 12px rgba(47, 125, 203, 0.28);
  cursor: default;
}

.pager-button:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.pager-dots {
  padding: 0 2px;
  color: #94a3b8;
  user-select: none;
}

.pager-jump {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  margin-left: 4px;
  font-size: 12px;
  color: #64748b;
}

.pager-jump input {
  width: 54px;
  height: 34px;
  padding: 0 8px;
  border: 1px solid #e5e7eb;
  border-radius: 9px;
  background: #ffffff;
  color: #374151;
  font-size: 13px;
  font-variant-numeric: tabular-nums;
  text-align: center;
  -moz-appearance: textfield;
  appearance: textfield;
}

.pager-jump input::-webkit-outer-spin-button,
.pager-jump input::-webkit-inner-spin-button {
  -webkit-appearance: none;
  margin: 0;
}

.pager-jump input:focus {
  outline: none;
  border-color: #2f7dcb;
}

@media (max-width: 960px) {
  .pager {
    width: 100%;
  }
  .pager-count {
    width: 100%;
    margin-bottom: 4px;
  }
}
</style>
