<template>
  <div class="main">
    <div class="hospital-home-header">
      <div class="spinner-box">
        <div class="blue-orbit leo"></div>
        <div class="green-orbit leo"></div>
        <div class="red-orbit leo"></div>
        <div class="white-orbit w1 leo"></div>
        <div class="white-orbit w2 leo"></div>
        <div class="white-orbit w3 leo"></div>
      </div>
      <div class="hospital-name">
        <span>某某宠物医院</span>
      </div>
    </div>
    <div class="hospital-home-body">
      <div class="introduce">
        <div class="hospital-technology">
          <div class="hospital-technology-img">img</div>
          <div class="hospital-technology-body">文字</div>
        </div>
        <div class="hospital-doctor">
          <div class="hospital-doctor-body">文字</div>
          <div class="hospital-doctor-img">img</div>
        </div>
        <div class="hospital-environment">
          <div class="hospital-environment-img">img</div>
          <div class="hospital-environment-body">文字</div>
        </div>
      </div>
    </div>
    <div class="hospital-footer">
      <div class="hospital-footer-body">技术支持</div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onBeforeUnmount } from "vue";

// 响应式数据
const inputMessage = ref("");
const wsMessage = ref("");
const socket = ref<WebSocket | null>(null);
const heartbeatTimer = ref(null);

inputMessage;
wsMessage;

// 生命周期钩子
onMounted(() => {
  console.log("mounted: 实例已挂载");
  window.addEventListener("beforeunload", cleanup);
});

onBeforeUnmount(() => {
  window.removeEventListener("beforeunload", cleanup);
  cleanup(); // 正常清理

  // WebSocket连接清理
  if (socket.value) {
    if (heartbeatTimer.value) clearInterval(heartbeatTimer.value);
    socket.value.close();
    console.log("WebSocket连接已清理");
  }

  console.log("unmounted: 实例已卸载");
});

// 方法
const cleanup = () => {
  // 清理逻辑：定时器、事件监听等
  if (heartbeatTimer.value) clearInterval(heartbeatTimer.value);
  window.removeEventListener("resize", handleResize);
};

const handleResize = () => {
  // 这个方法在原代码中被引用但在片段中未定义
  // 需要根据实际需求实现
};
</script>

<style scoped>
@import "@/assets/styles/Home.css";
</style>
