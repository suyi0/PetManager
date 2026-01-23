<template>
  <div class="services-view">
    <div class="services-container">
      <div class="services-container-left">
        <div
          class="services-container-left1"
          @click="switchTab('reservation')"
          :class="{
            ' click ': isReservationTab(activeTab),
          }"
        >
          <button class="services-container-left1-button">
            <span>预约服务</span>
          </button>
        </div>
        <div
          class="services-container-left2"
          @click="switchTab('afterSale')"
          :class="{ ' click ': activeTab === 'afterSale' }"
        >
          <button class="services-container-left2-button">
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
            <button
              class="services-container-right-reservation-text1-button"
              @click="switchTab('reservation-treatSlots')"
            >
              <img src="@/assets/photo/background.jpeg" alt="图片" />
              <span>宠物医治</span>
            </button>
          </div>
          <div class="services-container-right-reservation-text1">
            <button
              class="services-container-right-reservation-text1-button"
              @click="switchTab('reservation-sterilizateSlots')"
            >
              <img src="@/assets/photo/background.jpeg" alt="图片" />
              <span>宠物绝育</span>
            </button>
          </div>
          <div class="services-container-right-reservation-text1">
            <button
              class="services-container-right-reservation-text1-button"
              @click="switchTab('reservation-beautySlots')"
            >
              <img src="@/assets/photo/background.jpeg" alt="图片" />
              <span>宠物美容</span>
            </button>
          </div>
          <div class="services-container-right-reservation-text1">
            <button
              class="services-container-right-reservation-text1-button"
              @click="switchTab('reservation-SPASlots')"
            >
              <img src="@/assets/photo/background.jpeg" alt="图片" />
              <span>宠物SPA</span>
            </button>
          </div>
        </div>
        <div
          v-if="activeTab === 'afterSale'"
          class="services-container-right-afterSale"
        ></div>
        <treatSlots
          v-if="
            activeTab === 'reservation-treatSlots' || activeTab === 'showSlots'
          "
          :active-tab="activeTab"
          :switchTab="switchTab"
          @close="close"
          @cancle="cancle"
          @submit-success="handleSubmitSuccess"
        />
      </div>
    </div>
    <div v-if="submitAfter" class="submit-mask"></div>
  </div>
</template>

<script setup lang="ts">
// 1. 导入部分
import { ref, onMounted, onBeforeUnmount } from "vue";
import { useStore } from "vuex";
import { key } from "@/store/userStore";
import treatSlots from "@/views/UserDashboard/Services-module/treatSlots.vue";
import { defineComponent } from "vue";
// import { useRouter, useRoute } from "vue-router";
// import type { PropType } from "vue";

defineComponent({
  name: "userHomeNavServices",
});

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
const activeTab = ref("reservation");
const submitAfter = ref(false);

// 5. 计算属性

// 6. 方法定义
const isReservationTab = (tab: string) => {
  const reservationTabs = [
    "reservation",
    "reservation-treatSlots",
    "reservation-sterilizateSlots",
    "reservation-beautySlots",
    "reservation-SPASlots",
    "showSlots",
  ];
  return reservationTabs.includes(tab);
};

const switchTab = (tab: string) => {
  activeTab.value = tab;
};

const handleSubmitSuccess = () => {
  submitAfter.value = true;
};
const cancle = () => {
  activeTab.value = "reservation-treatSlots";
};

const close = () => {
  if (activeTab.value === "showSlots") {
    activeTab.value = "reservation-treatSlots";
  } else if (activeTab.value === "reservation-treatSlots") {
    activeTab.value = "reservation";
  }
};

// 7. 生命周期钩子
onMounted(() => {
  // store
  //   .dispatch("auth/scheduleTime")
  //   .then((response) => {
  //     if (response && response.data && typeof response.data === "object") {
  //       // response.data 包含一个名为 'data' 的属性，其中是数组
  //       if (response.data.data && Array.isArray(response.data.data)) {
  //         for (const item of response.data.data) {
  //           // 将每个日期的数据添加到对应的数组中
  //           year.value.push(item.year.toString());
  //           // 提取月份和日期部分
  //           const dateParts = item.date.split("-");
  //           month.value.push(dateParts[0]);
  //           day.value.push(dateParts[1]);
  //           weekday.value.push(item.weekday);
  //           // 处理 time_slots 字段
  //           const timeSlotsArray: string[] = [];
  //           if (item.time_slots) {
  //             // 如果 time_slots 是对象，提取其值
  //             Object.values(item.time_slots).forEach((slot) => {
  //               timeSlotsArray.push(String(slot));
  //             });
  //           }
  //           slots.value.push(timeSlotsArray);
  //         }
  //       }
  //     }
  //   })
  //   .catch((error) => {
  //     console.error("获取预约时间表失败:", error);
  //     // 清空数组而不是重新赋值
  //     store.state.auth.reservate.year.length = 0;
  //     store.state.auth.reservate.month.length = 0;
  //     store.state.auth.reservate.day.length = 0;
  //     store.state.auth.reservate.weekday.length = 0;
  //     store.state.auth.reservate.slots.length = 0;
  //   });
});

onBeforeUnmount(() => {
  // 可以在这里执行清理操作
});

// 8. 监听器 (如需要)

// 9. 路由相关 (如需要)
</script>

<style scoped lang="scss">
button {
  font-size: 20px;
  font-weight: 520;
  width: 100%;
  height: 100%;
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
  .services-container-left {
    min-width: 19vw;
    display: flex;
    flex-direction: column;
    text-align: center;
    margin-top: 104px;
    margin-right: 85px;
    position: sticky;
    z-index: 1001;

    .services-container-left1 {
      border-top: 2px solid rgb(0, 0, 0);
    }
    .services-container-left1,
    .services-container-left2 {
      width: 100%;
      height: 68px;
      padding-bottom: 7px;
      padding-top: 7px;
      border-bottom: 2px solid rgb(0, 0, 0);
      border-right: 2px solid rgb(0, 0, 0);
      border-radius: 8px;
    }
    .services-container-left1:active,
    .services-container-left2:active {
      transform: scale(0.98); /*点击时轻微缩小 */
      box-shadow: 3px 3px 10px rgba(0, 0, 0, 0.3); /*添加阴影效果 */
    }
    .services-container-left1.click,
    .services-container-left2.click {
      background-color: rgba(80, 160, 247, 0.3);
      color: white;
      span {
        color: #42b983;
      }
    }
    .services-container-left1-button,
    .services-container-left2-button {
      text-align: center;
    }
  }

  .services-container-right {
    min-width: 52vw;
    max-width: 100%;
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
}
.submit-mask {
  min-width: 100vw;
  min-height: 100vh;
  background: rgba(0, 0, 0, 0.5);
  z-index: 999;
  position: absolute;
  left: 0;
  top: 0;
}
</style>
