<template>
  <div class="image-cropper">
    <div
      v-if="!imageSrc"
      class="upload-area"
      @dragover.prevent
      @drop.prevent="handleDrop"
    >
      <div class="upload-content">
        <div class="upload-icon">📁</div>
        <p>拖拽图片到此处或点击上传</p>
        <button @click="triggerFileInput" class="upload-btn">选择图片</button>
        <input
          ref="fileInput"
          type="file"
          accept="image/*"
          @change="handleFileSelect"
          style="display: none"
        />
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

      <div class="cropper-actions">
        <button @click="cancelCrop" class="action-btn cancel-btn">取消</button>
        <button @click="rotateImage" class="action-btn">旋转</button>
        <button @click="zoomImage(true)" class="action-btn">放大</button>
        <button @click="zoomImage(false)" class="action-btn">缩小</button>
        <button @click="resetImage" class="action-btn">重置</button>
        <button @click="confirmCrop" class="action-btn confirm-btn">
          确认裁剪
        </button>
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
  (e: "crop-complete", dataUrl: string, blob: Blob): void;
  // eslint-disable-next-line no-unused-vars
  (e: "cancel"): void;
}>();

// 响应式数据
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

// 处理拖拽上传
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

// 旋转图片
const rotateImage = () => {
  cropper.value?.rotate(90);
};

// 缩放图片
const zoomImage = (isZoomIn: boolean) => {
  if (isZoomIn) {
    cropper.value?.zoom(0.1);
  } else {
    cropper.value?.zoom(-0.1);
  }
};

// 重置图片
const resetImage = () => {
  cropper.value?.reset();
};

// 确认裁剪
const confirmCrop = () => {
  if (!cropper.value) return;

  // 获取裁剪后的图片数据
  const canvas = cropper.value.getCroppedCanvas({
    width: props.previewWidth,
    height: props.previewHeight,
  });

  croppedImage.value = canvas.toDataURL();

  canvas.toBlob((blob) => {
    if (blob) {
      emit("crop-complete", croppedImage.value!, blob);
    }
  });
};

// 取消裁剪
const cancelCrop = () => {
  imageSrc.value = null;
  croppedImage.value = null;
  cropper.value?.destroy();
  cropper.value = null;
  emit("cancel");
};

// 组件卸载前清理
onBeforeUnmount(() => {
  cropper.value?.destroy();
});
</script>

<style scoped lang="scss">
.image-cropper {
  max-width: 100%;
  font-family: "Helvetica Neue", Helvetica, Arial, sans-serif;
}

.upload-area {
  border: 2px dashed #ccc;
  border-radius: 8px;
  padding: 40px;
  text-align: center;
  background-color: #f9f9f9;
  transition: border-color 0.3s ease;

  &:hover {
    border-color: #409eff;
  }
}

.upload-content {
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
</style>
