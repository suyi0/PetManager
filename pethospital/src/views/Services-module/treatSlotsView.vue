<template>
  <div class="reservation-slots">
    <div class="reservation-slots-top">
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
      <div class="title-container">
        <span>选择医生</span>
      </div>
    </div>
    <div class="reservation-slots-middle">
      <div class="reservation-slots-middle-div">
        <img
          class="reservation-slots-middle-div-img"
          src="@/assets/photo/background.jpeg"
          alt="头像"
        />
        <span class="reservation-slots-middle-div-span">医生介绍:</span>
      </div>
      <div class="reservation-slots-middle-div">
        <img
          class="reservation-slots-middle-div-img"
          src="@/assets/photo/background.jpeg"
          alt="头像"
        />
        <span class="reservation-slots-middle-div-span">医生介绍:</span>
      </div>
      <div class="reservation-slots-middle-div">
        <img
          class="reservation-slots-middle-div-img"
          src="@/assets/photo/background.jpeg"
          alt="头像"
        />
        <span class="reservation-slots-middle-div-span">医生介绍:</span>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
// 1. 导入部分
import { ref, onMounted, onBeforeUnmount } from "vue";
import { useStore } from "vuex";
import { key } from "@/store";
// import { useRouter, useRoute } from "vue-router";
// import type { PropType } from "vue";

const store = useStore(key);
store;

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

// 3. Emit事件定义 (如果需要向父组件传递事件)
const emit = defineEmits(["close", "submit"]);

// 4. 响应式数据
const count = ref(0);
count;

// 5. 计算属性

// 6. 方法定义
function close() {
  emit("close");
}

// 7. 生命周期钩子
onMounted(() => {
  // 可以在这里执行初始化操作
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
.reservation-slots {
  width: 100%;
  height: 100%;
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 24px;
  box-shadow: 0px 0px 5px 2px rgba(0, 0, 0, 0.3);
  border-radius: 12px;
  position: relative;

  .reservation-slots-top {
    width: 100%;
    height: 50px;
    border-bottom: 1px solid #e0e0e0;
    margin-bottom: 20px;

    .close-button {
      position: absolute;
      top: 12px;
      right: 16px;
      font-size: 24px;
      cursor: pointer;
      color: #333;
      z-index: 10;
    }
    .title-container {
      width: 100%;
      height: 100%;
      display: flex;
      justify-content: center;
      align-items: center;
      font-size: 30px;
      font-weight: 520;
    }
  }
  .reservation-slots-middle {
    width: 100%;
    height: 100%;
    display: flex;
    gap: 50px;
    flex-flow: row wrap;
  }
  .reservation-slots-middle-div {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 10px;
    .reservation-slots-middle-div-img {
      width: 200px;
      height: 250px;
      object-fit: cover;
      border-radius: 12px;
      box-shadow: 0px 0px 5px 2px rgba(0, 0, 0, 0.3);
    }
    .reservation-slots-middle-div-span {
      width: 250px;
      height: 300px;
      font-size: 15px;
      font-weight: 520;
      display: flex;
      align-items: start;
      justify-content: left;
      flex-direction: row;
    }
  }
}
</style>
