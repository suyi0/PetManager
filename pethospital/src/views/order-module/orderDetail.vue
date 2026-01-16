<template>
  <div class="order-detail">
    <div v-if="orderImg === 'magnify'" class="order-img-div">
      <img
        src=""
        alt="商品图片"
        class="order-img"
        @click="changeImg('reduce')"
      />
    </div>
    <div class="order-detail-top">
      <button class="order-detail-top-button" @click="changeImg('magnify')">
        <img src="" alt="商品图片" class="order-detail-top-img" />
      </button>
      <button class="order-detail-top-button-name">
        {{ order.orderName }}
      </button>
    </div>
    <div class="order-detail-middle-constainer">
      <div
        v-if="tabValue === 'reservation'"
        class="order-detail-middle-reservateTime"
      >
        <span>预约时间</span>
        <span>
          {{ order.reservateTime }}
        </span>
      </div>
      <div class="order-detail-middle-number">
        <span>订单编号</span>
        <span>{{ order.number }}</span>
      </div>
      <div class="order-detail-middle-createTime">
        <span>创建时间</span><span>{{ order.createTime }}</span>
      </div>
      <div class="order-detail-middle-status">
        <span>订单状态</span><span>{{ order.status }}</span>
      </div>
    </div>
    <div class="order-detail-bottom"></div>
  </div>
</template>

<script setup lang="ts">
// 1. 导入部分
import { ref, onMounted, onBeforeUnmount } from "vue";
import { useStore } from "vuex";
import { key } from "@/store";
import { useRoute } from "vue-router";
// import type { PropType } from "vue";

const store = useStore(key);
const route = useRoute();
store;

// 获取 tab 参数
const tabValue = ref<string | null>(null);

// 2. Props定义 (如果需要接收父组件传递的数据)
// const props = defineProps({
// title: {
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
const orderImg = ref("reduce");
const order = ref<{
  id: number;
  orderName: string;
  price: number;
  number: number;
  img: string;
  createTime: string;
  reservateTime: string;
  status: string;
}>({
  id: 0,
  orderName: "",
  price: 0,
  number: 0,
  img: "",
  createTime: "",
  reservateTime: "",
  status: "",
});

// 5. 计算属性

// 6. 方法定义
function changeImg(type: string) {
  if (type === "reduce") {
    // 缩小图片
    orderImg.value = "reduce";
  } else if (type === "magnify") {
    // 放大图片
    orderImg.value = "magnify";
  }
}

// 7. 生命周期钩子
onMounted(() => {
  // 可以在这里执行初始化操作

  // 获取 tab 参数值
  tabValue.value = (route.query.tab as string) || null;
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
.order-detail {
  width: 100%;
  height: 100%;
  padding: 50px;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: space-between;
  gap: 30px;

  .order-img-div {
    width: 100%;
    height: 100%;
    position: absolute;
    background-color: rgba(240, 240, 240, 0.5);

    .order-img {
      width: 80%;
      height: 60%;
      object-fit: cover;
    }
  }

  .order-detail-top {
    width: 80%;
    height: 10%;

    .order-detail-top-button {
      width: 100%;
      height: 100%;
      border: none;
      background-color: transparent;
      cursor: pointer;

      .order-detail-top-img {
        width: 100%;
        height: 100%;
        object-fit: cover;
      }
    }
  }

  .order-detail-middle-constainer {
    width: 100%;
    height: 80%;
    display: flex;
    flex-direction: column;
    align-items: flex-start;
    justify-content: center;
    gap: 10px;

    .order-detail-middle-reservateTime {
      font-size: 20px;
      font-weight: 520;
    }
  }

  .order-detail-bottom {
    width: 100%;
    height: 10%;
  }
}
</style>
