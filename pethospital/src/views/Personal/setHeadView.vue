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
        <button class="setHead-bottom1-button" @click="triggerFileInput">
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

        <!-- 隐藏的文件输入框 -->
        <input
          ref="fileInput"
          type="file"
          accept="image/*"
          style="display: none"
          @change="handleFileChange"
        />
        <button @click="getCroppedImage">确定</button>
      </div>
    </div>
    <div ref="previewContainer" style="display: none">
      <img ref="previewImage" alt="预览图片" style="max-width: 100%" />
    </div>
  </div>
</template>

<script setup lang="ts">
// 1. 导入部分
import { ref, onMounted, onBeforeUnmount } from "vue";
import { useStore } from "vuex";
import { key } from "@/store";
import Cropper from "cropperjs";
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

// 引用文件输入框
const fileInput = ref<HTMLInputElement | null>(null);

// 引用预览图像和容器
const previewContainer = ref<HTMLDivElement | null>(null);
const previewImage = ref<HTMLImageElement | null>(null); // 预览图元素

const cropperInstance = ref<Cropper | null>(null); // 存储 cropper 实例
// 5. 计算属性

// 6. 方法定义
// 触发文件选择对话框
const triggerFileInput = () => {
  if (fileInput.value) {
    fileInput.value.click();
  }
};

// 处理文件选择后的逻辑
// 处理文件选择后的逻辑
const handleFileChange = (event: Event) => {
  const target = event.target as HTMLInputElement;
  if (target.files && target.files.length > 0) {
    const file = target.files[0];
    const reader = new FileReader();

    reader.onload = (e) => {
      if (!e.target?.result) return;

      const img = new Image();
      img.onload = () => {
        // 确保 previewImage 元素存在
        if (!previewImage.value) return;

        // 设置预览图
        previewImage.value.src = e.target?.result as string;
        previewImage.value.style.display = "block";

        // 显示预览容器
        if (previewContainer.value) {
          previewContainer.value.style.display = "block";
        }

        // 清理现有的裁剪器实例
        if (cropperInstance.value) {
          cropperInstance.value = null;
          cropperInstance.value = null;
        }

        const cropperOptions: any = {
          aspectRatio: 1, // 固定 1:1 比例
          viewMode: 1,
          background: false,
          autoCropArea: 0.8,
          cropBoxResizable: false,
          modal: true,
          guides: false,
          center: false,
          highlight: false,
          zoomOnTouch: false,
          zoomOnWheel: false,
          cropBoxWidth: 180,
          cropBoxHeight: 180,
          ready: function () {
            const cropper = this as any;
            const container = cropper.container;

            // 移除默认裁剪框边框
            const cropBox = container.querySelector(".cropper-crop-box");
            if (cropBox) {
              cropBox.style.border = "none";
              cropBox.style.outline = "none";
            }

            // 创建圆形裁剪框（覆盖原生裁剪框）
            const circularFrame = document.createElement("div");
            circularFrame.className = "circular-frame";
            circularFrame.style.position = "absolute";
            circularFrame.style.top = "50%";
            circularFrame.style.left = "50%";
            circularFrame.style.transform = "translate(-50%, -50%)";
            circularFrame.style.width = "180px";
            circularFrame.style.height = "180px";
            circularFrame.style.borderRadius = "50%";
            circularFrame.style.border = "2px solid #409eff";
            circularFrame.style.boxShadow = "0 0 0 5000px rgba(0, 0, 0, 0.5)";
            circularFrame.style.pointerEvents = "none"; // 不影响交互
            circularFrame.style.zIndex = "1000";
            circularFrame.style.boxSizing = "border-box";

            container.appendChild(circularFrame);

            // 添加虚线内圈提示（可选）
            const innerCircle = document.createElement("div");
            innerCircle.className = "inner-circle";
            innerCircle.style.position = "absolute";
            innerCircle.style.top = "50%";
            innerCircle.style.left = "50%";
            innerCircle.style.transform = "translate(-50%, -50%)";
            innerCircle.style.width = "176px";
            innerCircle.style.height = "176px";
            innerCircle.style.borderRadius = "50%";
            innerCircle.style.border = "1px dashed #ccc";
            innerCircle.style.pointerEvents = "none";
            innerCircle.style.zIndex = "999";

            container.appendChild(innerCircle);
          },
        };
        // 使用 previewImage.value 创建 cropper 实例
        cropperInstance.value = new Cropper(previewImage.value, cropperOptions);
      };
      img.src = e.target.result as string;
    };
    reader.readAsDataURL(file);
  }
};

// 裁剪并获取结果
const getCroppedImage = async () => {
  if (!cropperInstance.value) return;

  // 获取裁剪后的画布
  try {
    // 获取裁剪后的画布 (使用正确的API)
    // 修复：在cropperjs 2.0中，getCroppedCanvas已被替换为getCropperSelection().$toCanvas()
    const selection = cropperInstance.value.getCropperSelection();
    if (!selection) return;

    const canvas = await selection.$toCanvas({
      width: 180,
      height: 180,
    });

    if (!canvas) return;

    // 将裁剪后的图像转换为圆形
    const circularCanvas = document.createElement("canvas");
    const ctx = circularCanvas.getContext("2d");
    if (!ctx) return;

    circularCanvas.width = canvas.width;
    circularCanvas.height = canvas.height;

    // 创建圆形路径
    ctx.beginPath();
    ctx.arc(
      canvas.width / 2,
      canvas.height / 2,
      Math.min(canvas.width, canvas.height) / 2,
      0,
      Math.PI * 2
    );
    ctx.closePath();
    ctx.clip();

    // 绘制裁剪后的图像到圆形画布上
    ctx.drawImage(canvas, 0, 0);

    // 将圆形画布转换为blob
    circularCanvas.toBlob(
      (blob) => {
        if (blob) {
          const formData = new FormData();
          formData.append("image", blob, "cropped-image.jpg");

          // 上传到服务器
          uploadImage(formData);
        }
      },
      "image/jpeg",
      0.8
    );
  } catch (error) {
    console.error("裁剪图片时出错:", error);
  }
};

// 上传裁剪后的图片
const uploadImage = (formData: FormData) => {
  console.log("上传裁剪后的图片", formData);
  // 这里可以添加实际的上传逻辑
};
// 取消操作
const cancel = () => {
  showUpHeadImage.value = false;
  isButtonActive.value = false;
};
// 关闭操作
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
  width: 180px;
  height: 180px;
  border-radius: 50%;
  margin-top: 40px;
  margin-bottom: 40px;
  background-color: #000;
}

.setHead-bottom {
  width: 45%;
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

.cropper-container .circular-frame {
  border: 2px solid #409eff;
  border-radius: 50%;
  box-shadow: 0 0 0 5000px rgba(0, 0, 0, 0.5);
}

.cropper-container .inner-circle {
  border: 1px dashed #ccc;
  border-radius: 50%;
}
</style>
