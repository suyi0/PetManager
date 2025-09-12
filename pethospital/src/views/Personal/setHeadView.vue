<template>
  <div class="setHead">
    <!-- 关闭按钮 -->
    <div class="close-button" @click="close">
      <svg
        width="18"
        height="18"
        viewBox="0 0 18 18"
        fill="none"
        xmlns="http://www.w3.org/2000/svg"
      >
        <path
          d="M14 2L2 14M14 14L2 2"
          stroke="#333"
          stroke-width="2"
          stroke-linecap="round"
          stroke-linejoin="round"
        />
      </svg>
    </div>
    <div class="setHead-top">
      {{ headImage }}
    </div>
    <div class="setHead-bottom">
      <div class="setHead-bottom1">
        <button class="setHead-bottom1-button">
          <span class="setHead-bottom1-button-span">上传新头像</span>
          <div class="ImageSVG">
            <svg
              width="36"
              height="36"
              viewBox="0 0 36 36"
              xmlns="http://www.w3.org/2000/svg"
            >
              <!-- 头部 -->
              <circle cx="18" cy="13" r="9" fill="#e0e0e0" />

              <!-- 身体 -->
              <path d="M10 23 L26 23 L26 31 L10 31 Z" fill="#e0e0e0" />

              <!-- 领口 -->
              <path
                d="M12 23 L18 17 L24 23"
                stroke="#e0e0e0"
                stroke-width="1.5"
                fill="none"
              />
            </svg>
          </div>
        </button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
// 1. 导入部分
import { ref, onMounted, onBeforeUnmount } from "vue";
import { useStore } from "vuex";
import { key } from "@/store";
// import { useRouter, useRoute } from "vue-router"
// import type { PropType } from "vue"

const store = useStore(key);

// 2. Props定义 (如果需要接收父组件传递的数据)
// const props = defineProps({
//   title: {
//     type: String,
//     default: ''
//   },
//   dataList: {
//     type: Array as PropType<any[]>,
//     default: () => []
//   }
// })

//3. Emit事件定义 (如果需要向父组件传递事件)
const emit = defineEmits(["close"]);

// 4. 响应式数据
const headImage = ref("");
const showUpHeadImage = ref(false);
const isButtonActive = ref(false);

// 5. 计算属性

// 6. 方法定义

// 取消操作
const cancel = () => {
  showUpHeadImage.value = false;
  isButtonActive.value = false;
};
function close() {
  if (showUpHeadImage.value === true) {
    cancel();
    return;
  } else {
    emit("close");
  }
}

// 7. 生命周期钩子
onMounted(() => {
  // 如果有个人头像图片则加载头像图片
  if (store.state.auth.userHeadImage) {
    headImage.value = store.state.auth.userHeadImage;
  }
});

onBeforeUnmount(() => {
  // 可以在这里执行清理操作
});

// 8. 监听器 (如需要)
// import { watch, watchEffect } from "vue"
// watch(() => formData.name, (newVal, oldVal) => {
//   console.log("Name changed:", newVal)
// });

// 9. 路由相关 (如需要)
// const router = useRouter()
// const route = useRoute()
// const goToDetail = (id: number) => {
//   router.push(`/detail/id`)
// };
</script>

<style scoped lang="scss">
.setHead {
  width: 500px;
  max-width: 500px;
  padding: 32px;
  background-color: #ffffff;
  border-radius: 12px;
  box-shadow: 0px 0px 5px 2px rgba(0, 0, 0, 0.3);
  position: relative;
  text-align: center;
  top: 45px;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 30px;
}

.close-button {
  position: absolute;
  top: 12px;
  right: 16px;
  font-size: 24px;
  cursor: pointer;
  background: none;
  border: none;
  color: #000;
  font-weight: bold;
  padding: 4px;
  border-radius: 50%;
}

.setHead-top {
  width: 120px;
  height: 120px;
  border-radius: 50%;
  margin-top: 40px;
  margin-bottom: 40px;
  background-color: #000;
}

.setHead-bottom {
  width: 80%;
}
.setHead-bottom1-button {
  display: flex;
  justify-content: space-between;
}
.setHead-bottom1-button-span {
  display: flex;
  align-items: center;
  width: 100px;
  height: 36px;
  font-size: 20px;
}
.ImageSVG {
  width: 36px;
  height: 36px;
}
</style>
