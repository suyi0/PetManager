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

    <div v-if="!imageSrc" class="setHead-container">
      <div class="setHead-top">
        {{ headImage }}
      </div>
      <div class="setHead-bottom">
        <div class="setHead-bottom1">
          <div
            v-if="!imageSrc"
            class="upload-area"
            @dragover.prevent
            @drop.prevent="handleDrop"
          ></div>
          <div class="upload-content">
            <button @click="triggerFileInput" class="upload-btn">
              选择图片
            </button>
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

    <div v-else class="cropper-container">
      <div class="cropper-wrapper">
        <img
          ref="cropperImage"
          :src="imageSrc"
          alt="待裁剪图片"
          class="cropper-image"
        />
      </div>
      <p class="tips">支持 JPG、PNG 格式，大小不超过 5MB</p>
      <div class="cropper-bottom">
        <button class="cancel-button" @click="cancel">取消</button>
        <button class="save-button" @click="getcroppedImage">确定</button>
      </div>
    </div>

    <div v-if="croppedImage" class="preview-container">
      <h3>预览</h3>
      <img :src="croppedImage" alt="裁剪预览" class="preview-image" />
    </div>
  </div>
</template>

<script setup lang="ts">
// eslint-disable-next-line no-unused-vars
import { ref, onBeforeUnmount, nextTick } from "vue";
import Cropper from "cropperjs";
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
const headImage = ref("");
const fileInput = ref<HTMLInputElement | null>(null);
const cropperImage = ref<HTMLImageElement | null>(null);
const imageSrc = ref<string | null>(null);
const croppedImage = ref<string | null>(null);
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
  });
};

// 获取裁剪后的图像
const getcroppedImage = async () => {
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

    croppedImage.value = circularCanvas.toDataURL();

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
  } catch (error) {
    console.error("裁剪图片时出错:", error);
  }
};

// 上传图片
const uploadImage = (formData: FormData) => {
  console.log("上传裁剪后的图片", formData);
};

// // 旋转图片
// const rotateImage = () => {
//   cropper.value?.rotate(90);
// };

// // 缩放图片
// const zoomImage = (isZoomIn: boolean) => {
//   if (isZoomIn) {
//     cropper.value?.zoom(0.1);
//   } else {
//     cropper.value?.zoom(-0.1);
//   }
// };

// // 重置图片
// const resetImage = () => {
//   cropper.value?.reset();
// };
// 取消操作
const cancel = () => {
  imageSrc.value = null;
  croppedImage.value = null;
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
  }

  emit("close");

  if (fileInput.value) {
    fileInput.value.value = "";
  }
};

// 组件卸载前清理
onBeforeUnmount(() => {
  cropper.value?.destroy();
});
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

.setHead-bottom {
  width: 180px;
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

.cropper {
  width: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  gap: 50px;
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
  position: absolute;
  border-radius: 12px;
  z-index: 1;

  &:hover {
    border-color: #409eff;
  }
}

.upload-content {
  z-index: 2;
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

.upload-btn {
  background-color: #409eff;
  color: white;
  border: none;
  padding: 10px 20px;
  border-radius: 4px;
  cursor: pointer;
  font-size: 14px;
  transition: background-color 0.3s;

  &:hover {
    background-color: #66b1ff;
  }
}

.cropper-container {
  display: flex;
  flex-direction: column;
  align-items: center;
}

.cropper-wrapper {
  max-width: 100%;
  margin-bottom: 20px;

  .cropper-image {
    height: 300px;
    object-fit: contain;
  }
}

.cropper-actions {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
  justify-content: center;
  margin-bottom: 20px;

  .action-btn {
    padding: 8px 16px;
    border: 1px solid #dcdfe6;
    border-radius: 4px;
    background-color: white;
    cursor: pointer;
    transition: all 0.3s;

    &:hover {
      background-color: #f5f7fa;
      border-color: #409eff;
      color: #409eff;
    }

    &.confirm-btn {
      background-color: #67c23a;
      color: white;
      border-color: #67c23a;

      &:hover {
        background-color: #85ce61;
        border-color: #85ce61;
      }
    }

    &.cancel-btn {
      background-color: #f56c6c;
      color: white;
      border-color: #f56c6c;

      &:hover {
        background-color: #f78989;
        border-color: #f78989;
      }
    }
  }
}

.preview-container {
  text-align: center;

  h3 {
    margin-bottom: 10px;
    color: #333;
  }

  .preview-image {
    max-width: 100%;
    border-radius: 8px;
    box-shadow: 0 2px 12px 0 rgba(0, 0, 0, 0.1);
  }
}

@media (max-width: 768px) {
  .cropper-actions {
    flex-direction: column;
    align-items: center;

    .action-btn {
      width: 80%;
    }
  }
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
