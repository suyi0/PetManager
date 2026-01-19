<template>
  <div class="order-view">
    <div class="order-view-top">
      <div class="order-top">
        <div
          class="order-search-top"
          :class="{ click: openSearch === true }"
          ref="homeOrderTopRef"
        >
          <span class="order-search-top-text">搜索</span>
          <input
            class="order-search-top-input"
            type="text"
            placeholder="请输入关键词"
            v-model="searchQuery"
            @keyup.enter="confirmSearch"
            @focus="openSearch = true"
            @blur="handleInputBlur"
          />
        </div>
        <!-- 搜索结果记录列表 -->
        <div class="order-list" v-if="openSearch">
          <div class="order-list-history">
            <div class="order-list-history-header">
              <span class="order-list-history-text">搜索记录</span>
              <button
                v-if="historyOrders.length > 0"
                class="clear-history-button"
                @click="clearSearchHistory"
              >
                清除历史记录
              </button>
            </div>
            <div class="order-list-history-item">
              <div
                v-for="order in historyOrders"
                :key="order.id || 0"
                class="order-list-history-item-example"
              >
                <button
                  class="order-list-history-item-example-button"
                  @click="buttonClick(order.name)"
                >
                  {{ order.name }}
                </button>
              </div>
            </div>
          </div>
          <!-- 默认订单列表
          <template v-if="activeTab === 'order'">
            <div
              v-for="order in searchResults"
              :key="order.id"
              class="order-item"
            >
              <button
                class="order-item-button"
                @click="buttonClick(order.name)"
              >
                {{ order.id + "." + order.name }}
              </button>
            </div>
          </template>
          <template v-if="activeTab === 'reservation'">
            <div
              v-for="order in searchReservationResults"
              :key="order.id"
              class="order-item"
            >
              <button
                class="order-item-button"
                @click="buttonClick(order.name)"
              >
                {{ order.id + "." + order.name }}
              </button>
            </div>
          </template> -->
        </div>
      </div>
    </div>
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
          <button class="order-button2"><span>预约记录</span></button>
        </div>
      </div>
      <div class="order-right">
        <div class="order-right-top">
          <div class="order-right-top-timeChange">
            <button @click="changeSort('time')">
              时间
              <img
                src="@/assets/photo/排序.svg"
                alt="下拉"
                class="order-right-top-timeChange-img"
              />
            </button>
          </div>
          <div class="order-right-top-priceChange">
            <button @click="changeSort('price')">
              价格
              <img
                src="@/assets/photo/排序.svg"
                alt="下拉"
                class="order-right-top-priceChange-img"
              />
            </button>
          </div>
          <div class="order-right-top-edit">
            <button
              class="order-right-top-edit-button"
              @click="editTab = !editTab"
            >
              {{ editTab ? "完成" : "编辑" }}
            </button>
          </div>
        </div>
        <div v-if="activeTab === 'order'" class="home-order">
          <div class="search-results-order" v-if="searchResults.length > 0">
            <div
              v-for="item in searchResults"
              :key="item.id"
              class="search-result-order-item"
            >
              <router-link :to="`/user/orderDetail?tab=${activeTab}`">
                <div v-if="editTab" class="home-order-leftImg">
                  <button
                    class="home-order-leftImg-button"
                    @click="ordersButton(item.id)"
                  >
                    <img
                      v-if="choiceActive[item.id]"
                      src="@/assets/photo/钩.svg"
                      alt="钩"
                      class="home-order-leftImg-button-img"
                    />
                    <img
                      v-else
                      src="@/assets/photo/未选中.svg"
                      alt="未选中"
                      class="home-order-leftImg-button-img"
                    />
                  </button>
                </div>
                <div class="home-order-rightText">
                  <button class="home-order-rightText-button">
                    {{ item.name }}
                  </button>
                </div>
              </router-link>
            </div>
          </div>
        </div>
        <div v-if="activeTab === 'reservation'" class="reservation-order">
          <div
            class="search-results-reservation"
            v-if="searchReservationResults.length > 0"
          >
            <div
              v-for="item in searchReservationResults"
              :key="item.id"
              class="search-result-order-item"
            >
              <router-link :to="`/user/orderDetail?tab=${activeTab}`">
                <div v-if="editTab" class="home-order-leftImg">
                  <button
                    class="home-order-leftImg-button"
                    @click="ordersButton(item.id)"
                  >
                    <img
                      v-if="choiceActive[item.id]"
                      src="@/assets/photo/钩.svg"
                      alt="钩"
                      class="home-order-leftImg-button-img"
                    />
                    <img
                      v-else
                      src="@/assets/photo/未选中.svg"
                      alt="未选中"
                      class="home-order-leftImg-button-img"
                    />
                  </button>
                </div>
                <div class="reservation-order-rightText">
                  <button class="reservation-order-rightText-button">
                    {{ item.name }}
                  </button>
                </div>
              </router-link>
            </div>
          </div>
        </div>
        <div v-if="editTab" class="order-right-botton">
          <div class="order-right-botton-collect">
            <button class="order-right-botton-collect-button">移入收藏</button>
          </div>
          <div class="order-right-botton-delete">
            <button class="order-right-botton-delete-button">删除</button>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
// 1. 导入部分
import { ref, onMounted, onBeforeUnmount, computed } from "vue";
import { useStore } from "vuex";
import { key } from "@/store/userStore";
//import { useRouter, useRoute } from "vue-router";
// import type { PropType } from "vue";

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

// 3. Emit事件定义 (如果需要向父组件传递事件)
// const emit = defineEmits<{
//   (e: "update", data: any): void
//   (e: "delete", id: number): void
// }>()

// 4. 响应式数据
const activeTab = ref("order");
const editTab = ref(false);
const searchQuery = ref("");
const openSearch = ref(false);
const orders = ref<{ id: number; name: string }[]>([]); // 订单数据
const reservationOrder = ref<{ id: number; name: string }[]>([]); // 预约订单数据
const homeOrderTopRef = ref<HTMLDivElement | null>(null); // 修改引用类型声明
const choiceActive = ref<{ [key: number]: boolean }>({});
const historyOrders = ref<{ id: number; name: string }[]>([]);

const MAX_HISTORY_COUNT = 15;

// 5. 计算属性
const searchResults = computed(() => {
  if (!searchQuery.value) {
    return [];
  }

  // 注意：由于这是异步操作，可能需要检查返回的数据结构
  // 这里假设该函数返回一个包含订单数组的Promise
  return orders.value && Array.isArray(orders)
    ? orders.value.filter((order) =>
        order.name.toLowerCase().includes(searchQuery.value.toLowerCase())
      )
    : [];
});

const searchReservationResults = computed(() => {
  if (!searchQuery.value) {
    return [];
  }

  // 注意：由于这是异步操作，可能需要检查返回的数据结构
  // 这里假设该函数返回一个包含订单数组的Promise
  return reservationOrder.value && Array.isArray(reservationOrder)
    ? reservationOrder.value.filter((order) =>
        order.name.toLowerCase().includes(searchQuery.value.toLowerCase())
      )
    : [];
});

// 6. 方法定义
function switchTab(tab: string) {
  activeTab.value = tab;
  searchQuery.value = "";
}
// 确认搜索并保存历史记录
function confirmSearch() {
  if (searchQuery.value.trim()) {
    // 检查是否已存在相同的搜索词
    const existingIndex = historyOrders.value.findIndex(
      (item) => item.name === searchQuery.value.trim()
    );

    if (existingIndex !== -1) {
      // 如果已存在，移到最前面
      const [item] = historyOrders.value.splice(existingIndex, 1);
      historyOrders.value.unshift(item);
    } else {
      // 如果不存在，添加新记录
      const newItem = {
        id: Date.now(), // 使用时间戳作为唯一ID
        name: searchQuery.value.trim(),
      };

      historyOrders.value.unshift(newItem);

      // 限制历史记录数量
      if (historyOrders.value.length > MAX_HISTORY_COUNT) {
        historyOrders.value.pop();
      }
    }

    // 保存到本地存储
    localStorage.setItem("searchHistory", JSON.stringify(historyOrders.value));
  }
  // 取消input元素聚焦
  openSearch.value = false;

  // 获取input元素并失去焦点
  const inputElement = document.querySelector(".order-search-top-input");
  if (inputElement && inputElement instanceof HTMLElement) {
    inputElement.blur();
  }
}

// 清除搜索历史
function clearSearchHistory() {
  historyOrders.value = [];
  localStorage.removeItem("searchHistory");
}

// 初始化时从本地存储加载搜索历史
function loadSearchHistory() {
  const savedHistory = localStorage.getItem("searchHistory");
  if (savedHistory) {
    try {
      historyOrders.value = JSON.parse(savedHistory);
    } catch (e) {
      console.error("Failed to parse search history", e);
      historyOrders.value = [];
    }
  }
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

// 添加输入框失焦事件处理方法
function handleInputBlur() {
  // 使用 setTimeout 来延迟关闭，确保点击搜索历史记录等操作能够正常执行
  setTimeout(() => {
    openSearch.value = false;
  }, 200);
}

// 切换选择状态
function setChoiceActive(id: number) {
  choiceActive.value[id] = !choiceActive.value[id];
}

function ordersButton(id: number) {
  setChoiceActive(id);
}

function buttonClick(name: string) {
  searchQuery.value = name;
  confirmSearch();
}

function changeSort(type: string) {
  if (activeTab.value === "order") {
    if (type === "time") {
      orders.value.sort((a, b) => (a as any).time - (b as any).time);
    } else if (type === "price") {
      // 假设每个订单对象有一个 price 属性
      orders.value.sort((a, b) => (a as any).price - (b as any).price);
    }
  } else if (activeTab.value === "reservation") {
    if (type === "time") {
      reservationOrder.value.sort((a, b) => (a as any).time - (b as any).time);
    } else if (type === "price") {
      reservationOrder.value.sort(
        (a, b) => (a as any).price - (b as any).price
      );
    }
  }
}

// 7. 生命周期钩子
onMounted(() => {
  document.addEventListener("click", handleClickOutside);
  loadSearchHistory(); // 加载搜索历史

  // 初始化订单数据
  store.dispatch("auth/getOrders").then((response) => {
    if (response && response.data && response.data.data) {
      orders.value = response.data.data;
    }
  });

  // 获取预约数据
  store.dispatch("auth/getReservation").then((response) => {
    if (response && response.data && response.data.data) {
      reservationOrder.value = response.data.data;
    }
  });
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
  min-width: 100%;
  min-height: 100%;
  position: relative;

  .order-view-top {
    min-width: 100%;
    height: 80px;

    .order-top {
      min-width: 50%;
      max-width: 500px;
      position: absolute;
      left: 38%;
      top: 20px;

      .order-search-top {
        width: 100%;
        display: flex;
        flex-direction: row;
        justify-content: center;
        align-items: center;
        box-shadow: 0px 0px 5px 2px rgba(0, 0, 0, 0.3);
        border-radius: 12px;

        .order-search-top-text {
          font-size: 25px;
          font-weight: bolder;
        }
        .order-search-top-input {
          width: 86%;
          height: 40px;
          border-radius: 12px;
          font-size: 20px;
          padding: 8px 12px;
          outline: none; /* 取消默认的蓝色边框 */
          border: none;
          background-color: transparent;
          box-shadow: none;
          transition: all 0.2s ease;
        }
      }
      .order-search-top.click {
        border-radius: 12px 12px 0 0;
        box-shadow: 0px -2px 5px 0px rgba(0, 0, 0, 0.3);
      }

      .order-list {
        width: 100%;
        padding: 10px;
        gap: 5px;
        display: flex;
        flex-direction: column;
        align-items: start;
        box-shadow: 0px 2px 5px 0px rgba(0, 0, 0, 0.3);
        border-radius: 0 0 12px 12px;
        background-color: white;
        position: absolute;
        z-index: 10001;

        .order-item-button {
          font-size: 16px;
        }
        .order-list-history {
          width: 100%;
          border-bottom: 1px solid rgb(0, 0, 0);

          .order-list-history-header {
            display: flex;
            justify-content: space-between;
            align-items: center;

            .order-list-history-text {
              font-size: 12px;
              font-weight: 520;
            }

            .clear-history-button {
              width: 80px;
              height: 20px;
              font-size: 12px;
              color: #ff4d4f;
              background: none;
              border: none;
              cursor: pointer;
            }
          }

          .order-list-history-item {
            display: flex;
            flex-direction: row;
            gap: 20px;
            margin-bottom: 20px;

            .order-list-history-item-example {
              .order-list-history-item-example-button {
                font-size: 12px;
              }
            }
          }
        }
      }
    }
  }

  .order-constainer {
    display: flex;
    flex-direction: row;
    margin-top: 24px;

    .order-constainer-left {
      min-width: 19vw;
      font-size: 18px;
      font-weight: 520;
      display: flex;
      flex-direction: column;
      text-align: center;
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
      width: 100vw;
      min-height: 100vh;
      padding: 20px 10px;
      align-items: center;
      flex-direction: column;
      border-radius: 12px;
      position: relative;
      border: 2px solid rgb(0, 0, 0);

      .order-right-top {
        width: 100%;
        height: 30px;
        padding-left: 10%;
        display: flex;
        flex-direction: row;
        align-items: center;
        gap: 80px;

        .order-right-top-timeChange,
        .order-right-top-priceChange {
          font-size: 16px;

          .order-right-top-timeChange-img,
          .order-right-top-priceChange-img {
            width: 16px;
            height: 16px;
            margin-left: 5px;
          }
        }

        .order-right-top-edit {
          font-size: 16px;
          cursor: pointer;
          position: absolute;
          right: 10%;
        }
      }

      .home-order {
        width: 100%;

        .search-results-order {
          width: 100%;
          display: flex;
          flex-direction: column;
          justify-content: center;
          gap: 10px;

          .search-result-order-item {
            display: flex;
            flex-direction: row;

            .home-order-rightText {
              width: 90%;
            }
          }
        }
      }

      .reservation-order {
        width: 100%;

        .search-results-reservation {
          width: 100%;
          display: flex;
          flex-direction: column;
          justify-content: center;
          gap: 10px;

          .search-result-order-item {
            display: flex;
            flex-direction: row;
          }
        }
        .reservation-order-rightText {
          width: 90%;
        }
      }

      .home-order-leftImg {
        width: 5%;
        margin-right: 10px;

        .home-order-leftImg-button {
          width: 100%;
          display: flex;
          flex-direction: column;
          justify-content: center;
          align-items: center;
          padding: 0;

          .home-order-leftImg-button-img {
            width: 20px;
            height: 20px;
          }
        }
      }

      .order-right-botton {
        width: 100%;
        height: 60px;
        position: absolute;
        bottom: 0;
        left: 0;
        display: flex;
        flex-direction: row;
        justify-content: center;
        align-items: center;
        gap: 50px;
        border-top: 2px solid rgb(0, 0, 0);
        background-color: white;

        .order-right-botton-collect,
        .order-right-botton-delete {
          width: 150px;
          height: 40px;
          border-radius: 8px;

          .order-right-botton-collect-button {
            background-color: #52c41a;
            color: white;
            border: none;
            cursor: pointer;
          }

          .order-right-botton-delete-button {
            background-color: #ff4d4f;
            color: white;
            border: none;
            cursor: pointer;
          }
        }
      }
    }
  }
}
</style>
