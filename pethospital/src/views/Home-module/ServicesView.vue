<template>
  <div class="services-view">
    <div class="services-container">
      <div class="services-container-left">
        <div class="services-container-left1">
          <button
            class="services-container-left1-button"
            @click="switchTab('reservation')"
            :class="{ ' click ': activeTab === 'reservation' }"
          >
            <span>预约服务</span>
          </button>
        </div>
        <div class="services-container-left2">
          <button
            class="services-container-left2-button"
            @click="switchTab('afterSale')"
            :class="{ ' click ': activeTab === 'afterSale' }"
          >
            <span>售后服务</span>
          </button>
        </div>
      </div>
      <div class="services-container-right">
        <div
          v-if="activeTab === 'reservation'"
          class="services-container-right-reservation"
        >
          <div class="services-container-right-reservation-text1">
            <button class="services-container-right-reservation-text1-button">
              <img src="@/assets/photo/background.jpeg" alt="图片" />
              <span>宠物医治</span>
            </button>
          </div>
          <div class="services-container-right-reservation-text1">
            <button class="services-container-right-reservation-text1-button">
              <img src="@/assets/photo/background.jpeg" alt="图片" />
              <span>宠物绝育</span>
            </button>
          </div>
          <div class="services-container-right-reservation-text1">
            <button class="services-container-right-reservation-text1-button">
              <img src="@/assets/photo/background.jpeg" alt="图片" />
              <span>宠物美容</span>
            </button>
          </div>
          <div class="services-container-right-reservation-text1">
            <button class="services-container-right-reservation-text1-button">
              <img src="@/assets/photo/background.jpeg" alt="图片" />
              <span>宠物SPA</span>
            </button>
          </div>
        </div>
        <div
          v-if="activeTab === 'afterSale'"
          class="services-container-right-afterSale"
        ></div>
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
// const emit = defineEmits<{
//   (e: "update", data: any): void
//   (e: "delete", id: number): void
// }>()

// 4. 响应式数据
// const schedule = ref("");
const activeTab = ref("reservation");
// 5. 计算属性

// 6. 方法定义
const switchTab = (tab: string) => {
  activeTab.value = tab;
};

// 7. 生命周期钩子
onMounted(() => {
  // store.dispatch("auth/scheduleTime").then((result: any) => {
  //   if (result && typeof result === "string") {
  //     schedule.value = result;
  //   }
  // });
});

onBeforeUnmount(() => {
  // 可以在这里执行清理操作
});

// 8. 监听器 (如需要)

// 9. 路由相关 (如需要)
</script>

<style scoped lang="scss">
span {
  font-size: 20px;
  font-weight: 520;
}
.services-view {
  width: 100%;
  height: 100%;
}
.services-container {
  width: 100%;
  height: 100%;
  display: flex;
  flex-direction: row;
}
.services-container-left {
  min-width: 19vw;
  display: flex;
  flex-direction: column;
  text-align: center;
  margin-top: 104px;
  margin-right: 85px;
  position: sticky;
  z-index: 1001;
  .services-container-left1-button {
    border-top: 2px solid rgb(0, 0, 0);
  }
  .services-container-left1-button,
  .services-container-left2-button {
    width: 100%;
    height: 68px;
    padding-bottom: 7px;
    padding-top: 7px;
    border-bottom: 2px solid rgb(0, 0, 0);
    border-right: 2px solid rgb(0, 0, 0);
    border-radius: 8px;
  }
  .services-container-left1-button:active,
  .services-container-left2-button:active {
    transform: scale(0.98); /*点击时轻微缩小 */
    box-shadow: 3px 3px 10px rgba(0, 0, 0, 0.3); /*添加阴影效果 */
  }
  .services-container-left1-button.click,
  .services-container-left2-button.click {
    background-color: rgba(80, 160, 247, 0.3);
    color: white;
    span {
      color: #42b983;
    }
  }
}
.services-container-right {
  min-width: 52vw;
  margin-top: 104px;
  .services-container-right-reservation {
    width: 100%;
    display: flex;
    flex-flow: row wrap;
    .services-container-right-reservation-text1 {
      width: 450px;
      height: 250px;
      text-align: center;
      box-shadow: 0px 0px 5px 2px rgba(0, 0, 0, 0.3);
      border-radius: 15px;
      margin: 0px 20px 20px 20px;
      position: relative;
      .services-container-right-reservation-text1-button {
        width: 100%;
        height: 100%;
        padding: 3px;
        img {
          width: 100%;
          height: 100%;
          object-fit: cover;
          border-radius: 15px;
        }
        span {
          font-size: 30px;
          position: absolute;
          top: 20px;
          left: 170px;
          color: white;
        }
      }
    }
  }
}
</style>
