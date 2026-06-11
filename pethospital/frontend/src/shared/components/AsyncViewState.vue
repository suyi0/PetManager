<template>
  <div class="async-state" :class="`async-state--${tone}`">
    <div class="async-state__mark">
      <span v-if="loading" class="async-state__spinner"></span>
      <span v-else>!</span>
    </div>
    <div class="async-state__content">
      <strong>{{ title }}</strong>
      <p>{{ message }}</p>
    </div>
    <button
      v-if="showRetry"
      type="button"
      class="async-state__button"
      @click="$emit('retry')"
    >
      重试
    </button>
  </div>
</template>

<script lang="ts">
import { computed, defineComponent } from "vue";

export default defineComponent({
  name: "AsyncViewState",
  emits: ["retry"],
  props: {
    loading: {
      type: Boolean,
      default: false,
    },
    error: {
      type: String,
      default: "",
    },
    loadingText: {
      type: String,
      default: "正在同步数据",
    },
    errorTitle: {
      type: String,
      default: "数据加载失败",
    },
    retryable: {
      type: Boolean,
      default: true,
    },
  },
  setup(props) {
    const tone = computed(() => (props.loading ? "loading" : "error"));
    const title = computed(() =>
      props.loading ? "正在加载" : props.errorTitle
    );
    const message = computed(() =>
      props.loading ? props.loadingText : props.error || "请稍后重试。"
    );
    const showRetry = computed(() => !props.loading && props.retryable);

    return {
      tone,
      title,
      message,
      showRetry,
    };
  },
});
</script>

<style scoped>
.async-state {
  display: flex;
  align-items: center;
  gap: 14px;
  border-radius: 18px;
  border: 1px solid rgba(94, 134, 138, 0.18);
  padding: 14px 16px;
  background: rgba(248, 252, 249, 0.92);
  color: #173f45;
}

.async-state--error {
  border-color: rgba(180, 65, 83, 0.18);
  background: rgba(255, 238, 241, 0.92);
}

.async-state__mark {
  display: grid;
  place-items: center;
  flex: 0 0 auto;
  width: 34px;
  height: 34px;
  border-radius: 50%;
  background: rgba(31, 112, 119, 0.12);
  color: #1f7077;
  font-weight: 900;
}

.async-state--error .async-state__mark {
  background: rgba(180, 65, 83, 0.12);
  color: #a23c4a;
}

.async-state__spinner {
  width: 16px;
  height: 16px;
  border-radius: 50%;
  border: 2px solid rgba(31, 112, 119, 0.22);
  border-top-color: #1f7077;
  animation: async-state-spin 0.8s linear infinite;
}

.async-state__content {
  display: grid;
  gap: 4px;
  min-width: 0;
}

.async-state__content strong,
.async-state__content p {
  margin: 0;
}

.async-state__content p {
  color: #647f82;
  line-height: 1.6;
}

.async-state__button {
  flex: 0 0 auto;
  border: 0;
  border-radius: 12px;
  padding: 10px 14px;
  background: linear-gradient(135deg, #1f7077, #6d5348);
  color: #ffffff;
  font-weight: 800;
  cursor: pointer;
}

@keyframes async-state-spin {
  to {
    transform: rotate(360deg);
  }
}
</style>
