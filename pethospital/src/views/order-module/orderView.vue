<template>
  <div class="order-view">
    <div class="order-constainer">
      <div class="order-constainer-left">
        <div
          class="order-left1"
          :class="{ ' click ': activeTab === 'order' }"
          @click="switchTab('order')"
        >
          <button class="order-button1"><span>订单</span></button>
        </div>
        <div
          class="order-left2"
          :class="{ ' click ': activeTab === 'reservation' }"
          @click="switchTab('reservation')"
        >
          <button class="order-button2"><span>预约</span></button>
        </div>
      </div>
      <div class="order-right">
        <div
          v-if="activeTab === 'order'"
          class="home-order"
          ref="homeOrderTopRef"
        >
          <div class="home-order-top" :class="{ click: openSearch === true }">
            <span class="home-order-top-text">搜索</span>
            <input
              class="home-order-top-input"
              type="text"
              placeholder="请输入关键词"
              v-model="searchQuery"
              @input="handleSearch"
              @mousedown="openSearch = true"
            />
          </div>
          <div class="search-results" v-if="searchResults.length > 0">
            <div
              v-for="item in searchResults"
              :key="item.id"
              class="search-result-item"
            >
              {{ item.name }}
            </div>
          </div>
          <div class="order-list" v-else-if="!searchQuery && openSearch">
            <!-- 默认订单列表 -->
            <div v-for="order in orders" :key="order.id" class="order-item">
              {{ order.id + "." + order.name }}
            </div>
          </div>
        </div>
        <div v-if="activeTab === 'reservation'" class="reservation-order"></div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
// 1. 导入部分
import { ref, onMounted, onBeforeUnmount, computed } from "vue";
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
const activeTab = ref("order");
const searchQuery = ref("");
const openSearch = ref(false);
const orders = ref([
  { id: 1, name: "宠物美容服务订单" },
  { id: 2, name: "宠物疫苗接种订单" },
  { id: 3, name: "宠物寄养服务订单" },
  { id: 4, name: "宠物体检服务订单" },
]);
const homeOrderTopRef = ref<HTMLDivElement | null>(null); // 修改引用类型声明
// 5. 计算属性
const searchResults = computed(() => {
  if (!searchQuery.value) {
    return [];
  }

  return orders.value.filter((order) =>
    order.name.toLowerCase().includes(searchQuery.value.toLowerCase())
  );
});

// 6. 方法定义
function switchTab(tab: string) {
  activeTab.value = tab;
}

function handleSearch() {
  // 搜索逻辑已经在 computed 属性中实现
  // 这里可以添加防抖或其他额外逻辑
}
// 添加点击外部区域关闭搜索结果的方法
function handleClickOutside(event: MouseEvent) {
  if (
    homeOrderTopRef.value &&
    !homeOrderTopRef.value.contains(event.target as Node)
  ) {
    openSearch.value = false;
  }
}

// 7. 生命周期钩子
onMounted(() => {
  document.addEventListener("click", handleClickOutside);
});

onBeforeUnmount(() => {
  document.removeEventListener("click", handleClickOutside);
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
button {
  font-size: 20px;
  font-weight: 520;
  width: 100%;
  height: 100%;
}
.order-view {
  width: 100%;
  height: 100%;

  .order-constainer {
    display: flex;
    flex-direction: row;
    margin-top: 104px;

    .order-constainer-left {
      min-width: 19vw;
      font-size: 18px;
      font-weight: 520;
      display: flex;
      flex-direction: column;
      text-align: center;
      margin-top: 20px;
      margin-right: 85px;

      .order-left1 {
        border-top: 2px solid rgb(0, 0, 0);
      }
      .order-left1,
      .order-left2 {
        width: 100%;
        height: 68px;
        padding-bottom: 7px;
        padding-top: 7px;
        border-bottom: 2px solid rgb(0, 0, 0);
        border-right: 2px solid rgb(0, 0, 0);
        border-radius: 8px;
      }
      .order-left1:active,
      .order-left2:active {
        transform: scale(0.98); /*点击时轻微缩小 */
        box-shadow: 3px 3px 10px rgba(0, 0, 0, 0.3);
      }
      .order-left1.click,
      .order-left2.click {
        /* 激活状态样式 */
        background-color: rgba(80, 160, 247, 0.3);
        color: white;
        span {
          color: #42b983;
        }
      }
      .order-button1,
      .order-button2 {
        text-align: center;
      }
    }
    .order-right {
      display: flex;
      min-width: 52vw;
      max-width: 800px;
      min-height: 40vh;
      padding: 24px;
      align-items: center;
      flex-direction: column;
      box-shadow: 0px 0px 5px 2px rgba(0, 0, 0, 0.3);
      border-radius: 12px;

      .home-order {
        width: 100%;
        display: flex;
        flex-direction: column;
        justify-content: center;
        align-items: center;

        .home-order-top {
          width: 80%;
          display: flex;
          flex-direction: row;
          justify-content: center;
          align-items: center;
          box-shadow: 0px 0px 5px 2px rgba(0, 0, 0, 0.3);
          border-radius: 12px;

          .home-order-top-text {
            font-size: 25px;
            font-weight: bolder;
          }
          .home-order-top-input {
            width: 86%;
            height: 40px;
            border-radius: 12px;
            font-size: 20px;
            padding: 8px 12px;
            border: 0px;
            outline: none; /* 取消默认的蓝色边框 */
            -webkit-appearance: none; /* 移除浏览器默认样式 */
            -moz-appearance: none;
            border: none;
            background-color: transparent;
            box-shadow: none;
            transition: all 0.2s ease;
          }
        }
        .home-order-top.click {
          border-radius: 12px 12px 0 0;
          box-shadow: 0px -2px 5px 0px rgba(0, 0, 0, 0.3);
        }

        .order-list {
          width: 80%;
          padding: 10px;
          gap: 5px;
          display: flex;
          flex-direction: column;
          align-items: start;
          box-shadow: 0px 2px 5px 0px rgba(0, 0, 0, 0.3);
          border-radius: 0 0 12px 12px;
        }
      }
      .reservation-order {
      }
    }
  }
}
</style>
