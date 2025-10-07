<template>
  <div class="about-container">
    <div class="about-container-middle">
      <div class="about-container-left">
        <div
          class="about-left1"
          :class="{ ' click ': activeTab === 'environment' }"
          @click="switchTab('environment')"
        >
          <button class="about-left1-button">
            <span> 医院环境 </span>
          </button>
        </div>
        <div
          class="about-left2"
          :class="{ ' click ': activeTab === 'doctor' }"
          @click="switchTab('doctor')"
        >
          <button class="about-left2-button"><span>医生团队</span></button>
        </div>
        <div
          class="about-left3"
          :class="{ ' click ': activeTab === 'machine' }"
          @click="switchTab('machine')"
        >
          <button class="about-left3-button"><span>设备优势</span></button>
        </div>
        <div
          class="about-left4"
          :class="{ ' click ': activeTab === 'advantage' }"
          @click="switchTab('advantage')"
        >
          <button class="about-left4-button"><span>服务优势</span></button>
        </div>
        <div
          class="about-left5"
          :class="{ ' click ': activeTab === 'consultingService' }"
          @click="switchTab('consultingService')"
        >
          <button class="about-left5-button"><span>免费咨询</span></button>
        </div>
        <div
          class="about-left6"
          :class="{ ' click ': activeTab === 'contact' }"
          @click="switchTab('contact')"
        >
          <button class="about-left6-button"><span>联系我们</span></button>
        </div>
      </div>
      <div class="about-container-right">
        <div
          v-if="activeTab === 'environment'"
          class="about-container-right-environment"
          :style="{ backgroundImage: `url(${backgroundImage})` }"
        >
          <div></div>
        </div>
        <div
          v-if="activeTab === 'doctor'"
          class="about-container-right-doctor"
        ></div>
        <div
          v-if="activeTab === 'machine'"
          class="about-container-right-machine"
        ></div>
        <div
          v-if="activeTab === 'advantage'"
          class="about-container-right-advantage"
        ></div>
        <div
          v-if="activeTab === 'consultingService'"
          class="about-container-right-consultingService"
        ></div>
        <div
          v-if="activeTab === 'contact'"
          class="about-container-right-contact"
        >
          <div class="contact-container">
            <h2 class="contact-container-h2">联系方式</h2>
            <span class="contact-container-span1"
              >(如果有什么问题请通过以下方式联系)</span
            >
            <span class="contact-container-span2">电话: 123-456-7890</span>
            <span class="contact-container-span3">邮箱: *************</span>
            <div class="contact-container-WeChat">
              <img src="" alt="QR" />
              <span class="contact-container-span4">微信公众号</span>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
// 1. 导入部分
import { ref, onMounted, onBeforeUnmount } from "vue";
import backgroundImage from "@/assets/photo/background.jpeg";
//import { useStore } from "vuex";
//import { key } from "@/store";
// import { useRouter, useRoute } from "vue-router"
// import type { PropType } from "vue"

//const store = useStore(key);

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
const activeTab = ref("environment");

// 5. 计算属性

// 6. 方法定义
const switchTab = (tab: string) => {
  activeTab.value = tab;
};

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
@import "@/assets/styles/About.css";
</style>
