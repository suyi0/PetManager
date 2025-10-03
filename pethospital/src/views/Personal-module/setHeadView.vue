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

    <div
      v-if="!imageSrc"
      class="setHead-container"
      @dragover.prevent
      @drop.prevent="handleDrop"
    >
      <div class="setHead-top">
        <img :src="headImage" class="head-img" />
      </div>
      <div class="setHead-bottom">
        <div class="setHead-bottom1">
          <span class="upload-text"
            >可以把图片拖进来选择或者点击下方的选择图片</span
          >
          <div class="upload-content">
            <button @click="triggerFileInput" class="upload-btn">
              <span class="upload-span">选择图片</span>
              <div class="ImageSVG">
                <svg
                  width="36"
                  height="36"
                  viewBox="0 0 36 36"
                  xmlns="http://www.w3.org/2000/svg"
                >
                  <circle cx="18" cy="13" r="9" fill="#e0e0e0" />
                  <path d="M10 23 L26 23 L26 31 L10 31 Z" fill="#e0e0e0" />
                  <path
                    d="M12 23 L18 17 L24 23"
                    stroke="#e0e0e0"
                    stroke-width="1.5"
                    fill="none"
                  />
                </svg>
              </div>
            </button>
            <input
              ref="fileInput"
              type="file"
              accept="image/*"
              @change="handleFileSelect"
              style="display: none"
            />
          </div>
        </div>
      </div>
    </div>

    <div v-else class="cropper">
      <div class="cropper-container">
        <div class="cropper-wrapper">
          <img
            ref="cropperImage"
            :src="imageSrc"
            alt="待裁剪图片"
            class="cropper-image"
          />
        </div>
      </div>
      <div class="border-line"></div>
      <div class="preview-container">
        <div class="preview-croppedImage">
          <h3>预览</h3>
          <img :src="previewImage || headImage" class="preview-image" />
        </div>
      </div>
    </div>
    <div v-if="cropperImage" class="bottom">
      <p class="tips">支持 JPG、PNG 格式，大小不超过 5MB</p>
      <div class="cropper-bottom">
        <button class="cancel-button" @click="cancel">取消</button>
        <button class="save-button" @click="getcroppedImage('upload')">
          确定
        </button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
// eslint-disable-next-line no-unused-vars
import { ref, onBeforeUnmount, nextTick, onMounted } from "vue";
import { useStore } from "vuex";
import { key } from "@/store";
import Cropper from "cropperjs";
import axios from "axios";
import "cropperjs/dist/cropper.css";

// 定义组件属性
interface Props {
  aspectRatio?: number;
  previewWidth?: number;
  previewHeight?: number;
}

const props = withDefaults(defineProps<Props>(), {
  aspectRatio: 1,
  previewWidth: 200,
  previewHeight: 200,
});

// 定义事件
const emit = defineEmits<{
  // eslint-disable-next-line no-unused-vars
  (e: "close"): void;
  // eslint-disable-next-line no-unused-vars
  (e: "crop-complete", dataUrl: string, blob: Blob): void;
  // eslint-disable-next-line no-unused-vars
  (e: "cancel"): void;
}>();

// 响应式数据
const store = useStore(key);
const headImage = ref("");
const fileInput = ref<HTMLInputElement | null>(null);
const cropperImage = ref<HTMLImageElement | null>(null);
const imageSrc = ref<string | null>(null);
const croppedImage = ref<string>();
const previewImage = ref<string>();
const cropper = ref<Cropper | null>(null);

// 触发文件选择
const triggerFileInput = () => {
  fileInput.value?.click();
};

// 处理文件选择
const handleFileSelect = (e: Event) => {
  const target = e.target as HTMLInputElement;
  const file = target.files?.[0];
  if (file) {
    loadImage(file);
  }
};

//处理拖拽上传
const handleDrop = (e: DragEvent) => {
  const file = e.dataTransfer?.files?.[0];
  if (file && file.type.startsWith("image/")) {
    loadImage(file);
  }
};

// 加载图片
const loadImage = (file: File) => {
  const reader = new FileReader();
  reader.onload = (e) => {
    imageSrc.value = e.target?.result as string;
    nextTick(() => {
      initCropper();
    });
  };
  reader.readAsDataURL(file);
};

// 初始化裁剪器
const initCropper = () => {
  if (!cropperImage.value) return;

  // 销毁已有的裁剪器实例
  if (cropper.value) {
    cropper.value.destroy();
  }

  // 创建新的裁剪器实例
  cropper.value = new Cropper(cropperImage.value, {
    aspectRatio: props.aspectRatio,
    viewMode: 1,
    dragMode: "move",
    autoCropArea: 0.8,
    restore: false,
    guides: true,
    center: true,
    highlight: true,
    cropBoxMovable: true,
    cropBoxResizable: true,
    toggleDragModeOnDblclick: false,
    ready: function () {
      // 当裁剪器准备就绪时，先生成一次预览
      getcroppedImage();
    },
    crop: function () {
      // 当裁剪框移动或调整大小时，更新预览
      getcroppedImage();
    },
  });
};

// 获取裁剪后的图像
const getcroppedImage = async (fileName?: string) => {
  if (!cropper.value) return;

  try {
    const canvas = cropper.value.getCroppedCanvas({
      width: 180,
      height: 180,
    });

    if (!canvas) return;

    const circularCanvas = document.createElement("canvas");
    const ctx = circularCanvas.getContext("2d");
    if (!ctx) return;

    circularCanvas.width = canvas.width;
    circularCanvas.height = canvas.height;

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
    ctx.drawImage(canvas, 0, 0);

    previewImage.value = circularCanvas.toDataURL();
    croppedImage.value = circularCanvas.toDataURL();

    if (fileName == "upload") {
      circularCanvas.toBlob(
        (blob) => {
          if (blob) {
            const formData = new FormData();
            formData.append("image", blob, "cropped-image.jpg");
            uploadImage(formData);
          }
        },
        "image/jpeg",
        0.8
      );
      // 在这里添加这一行，确保预览区域能正确显示
    }
    await nextTick();
  } catch (error) {
    console.error("裁剪图片时出错:", error);
  }
};
const uploadImage = async (formData: FormData) => {
  try {
    // 第一步：上传图片到服务器
    const uploadResponse = await axios.post(
      "/api/user/upload/avatar",
      formData,
      {
        headers: { "Content-Type": "multipart/form-data" },
      }
    );

    // 假设服务器返回的是真实图片 URL，例如：
    const avatarUrl = uploadResponse.data.avatarUrl;

    // 更新本地状态
    store.dispatch("auth/updateUserField", {
      field: "userHeadImage",
      value: avatarUrl,
    });

    // 更新完 store，关闭设置界面
    croppedImage.value = undefined;
    close();
  } catch (error) {
    console.error("上传失败:", error);
    alert("头像上传失败，请重试");
  }
};

// 取消操作
const cancel = () => {
  imageSrc.value = null;
  croppedImage.value = undefined;
  cropper.value?.destroy();
  cropper.value = null;

  if (fileInput.value) {
    fileInput.value.value = "";
  }
  emit("cancel");
};

// 关闭操作
const close = () => {
  if (croppedImage.value) {
    cancel();
    return;
  } else {
    imageSrc.value = null;
    cropper.value?.destroy();
    cropper.value = null;
    emit("close");
  }

  if (fileInput.value) {
    fileInput.value.value = "";
  }
};

// 组件初始化
onMounted(() => {
  headImage.value = store.state.auth.userHeadImage || "";
});

// 组件卸载前清理
onBeforeUnmount(() => {
  cropper.value?.destroy();
});
</script>

<style scoped lang="scss">
.setHead {
  min-width: 500px;
  padding: 32px;
  background-color: #ffffff;
  border-radius: 12px;
  box-shadow: 0px 0px 5px 2px rgba(0, 0, 0, 0.3);
  position: relative;
  text-align: center;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
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

.setHead-container {
  width: 100%;
  display: flex;
  flex-direction: column;
  align-items: center;
}

.setHead-top {
  width: 180px;
  height: 180px;
  border-radius: 50%;
  margin-top: 40px;
  margin-bottom: 40px;
  background-color: #000;
}
.head-img {
  width: 100%;
  height: 100%;
  border-radius: 90px;
}

.setHead-bottom1 {
  width: 180px;
  display: flex;
  justify-content: center;
  align-items: center;
  flex-direction: column;
  gap: 20px;
}

.image-cropper {
  max-width: 100%;
  font-family: "Helvetica Neue", Helvetica, Arial, sans-serif;
}

.upload-area {
  width: 100%;
  height: 100%;
  left: 0px;
  top: 0px;
  position: relative;
  border-radius: 12px;
  z-index: 1;

  &:hover {
    border-color: #409eff;
  }
}

.upload-content {
  display: flex;
  justify-content: center;
  align-items: center;
  background-color: rgba(255, 255, 255, 0.8);
  border-radius: 8px;
  cursor: pointer;
  transition: background-color 0.3s;
  .upload-icon {
    font-size: 48px;
    margin-bottom: 16px;
  }

  p {
    font-size: 16px;
    color: #666;
    margin-bottom: 20px;
  }
}
.upload-text {
  width: 250px;
  font-size: 16px;
  background: rgba(130, 190, 19, 0.1);
  color: #000000;
}

.upload-btn {
  background-color: #409eff;
  color: white;
  border: none;
  display: flex;
  justify-content: space-between;
  padding: 10px 20px;
  border-radius: 4px;
  cursor: pointer;
  font-size: 14px;
  transition: background-color 0.3s;

  &:hover {
    background-color: #66b1ff;
  }
}
.upload-span {
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

.cropper {
  display: flex;
  align-items: center;
  justify-content: center;
  margin-bottom: 40px;
}
.cropper-container {
  display: flex;
  flex-direction: column;
  align-items: center;
  margin-right: 40px;
}

.cropper-wrapper {
  width: 300px;
  height: 300px;
  margin-bottom: 20px;

  .cropper-image {
    height: 300px;
    object-fit: contain;
  }
}

.border-line {
  height: 300px;
  width: 3px;
  background: #e5e9ef;
  float: left;
  position: relative;
  left: 20px;
}

.preview-container {
  width: 300px;
  height: 300px;
  text-align: center;

  h3 {
    font-size: 30px;
    font-weight: 900;
    margin-bottom: 10px;
    color: #333;
  }

  .preview-image {
    width: 137px;
    height: 137px;
    border-radius: 50%;
    position: relative;
    top: 25px;
    background-color: rgb(0, 0, 0);
    box-shadow: 0 2px 12px 0 rgba(0, 0, 0, 0.1);
  }
}

.tips {
  display: block;
  font-size: 16px;
  line-height: 1.4; /* 明确设置行高 */
  min-height: 24px; /* 强制最小高度 */
  margin-top: 10px;
  margin-bottom: 20px;
}

.cropper-bottom {
  width: 100%;
  display: flex;
  justify-content: space-between;
}

.cancel-button {
  padding: 12px 24px;
  border: 1px solid #409eff;
  border-radius: 8px;
  background-color: transparent;
  color: #409eff;
  font-size: 16px;
  cursor: pointer;
  transition: all 0.3s ease;
  width: 35%;

  &:hover {
    background-color: rgba(64, 158, 255, 0.1);
  }
}

.save-button {
  padding: 12px 24px;
  border: none;
  border-radius: 8px;
  background-color: #409eff;
  color: white;
  font-size: 16px;
  cursor: pointer;
  transition: all 0.3s ease;
  width: 35%;

  &:hover {
    background-color: #3a8ee6;
  }
}
</style>
