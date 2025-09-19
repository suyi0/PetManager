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

    <div v-show="!showPrewiew" class="setHead-container">
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
            style="display: none"
            @change="handleFileChange"
          />
        </div>
      </div>
    </div>

    <div v-show="showPrewiew" class="preview">
      <div class="preview-top" ref="previewContainer"></div>
      <p class="tips">支持 JPG、PNG 格式，大小不超过 5MB</p>
      <div class="preview-bottom">
        <button class="cancel-button" @click="cancel">取消</button>
        <button class="save-button" @click="getCroppedImage">确定</button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onBeforeUnmount } from "vue";
import { useStore } from "vuex";
import { key } from "@/store";
import Cropper from "cropperjs";

const store = useStore(key);
const emit = defineEmits(["close"]);

// 响应式数据
const headImage = ref("");
const showUpHeadImage = ref(false);
const showPrewiew = ref(false);

// 引用元素
const fileInput = ref<HTMLInputElement | null>(null);
const previewContainer = ref<HTMLDivElement | null>(null);
const cropperInstance = ref<Cropper | null>(null);

// 触发文件选择
const triggerFileInput = () => {
  if (fileInput.value) {
    fileInput.value.click();
  }
};

// 处理文件选择
const handleFileChange = (event: Event) => {
  const target = event.target as HTMLInputElement;
  if (!target.files || target.files.length === 0) return;

  const file = target.files[0];
  const reader = new FileReader();

  reader.onload = (e) => {
    if (!e.target?.result) return;

    // 销毁旧 cropper 实例
    if (cropperInstance.value) {
      try {
        cropperInstance.value.destroy();
      } catch (err) {
        console.warn("Error destroying cropper:", err);
      }
      cropperInstance.value = null;
    }

    showPrewiew.value = true;
    showUpHeadImage.value = true;

    setTimeout(() => {
      if (!previewContainer.value) return;

      // 清空容器内容，防止重复
      previewContainer.value.innerHTML = "";

      const imgElement = document.createElement("img");
      imgElement.src = e.target?.result as string;
      imgElement.alt = "预览图片";
      imgElement.style.width = "180px";
      imgElement.style.height = "300px";
      imgElement.style.objectFit = "cover";
      imgElement.style.borderRadius = "5px";
      imgElement.style.margin = "0 auto";
      imgElement.style.display = "block";

      previewContainer.value.appendChild(imgElement);

      // 初始化 Cropper 的函数
      const initCropper = () => {
        try {
          const options: Cropper.Options = {
            aspectRatio: 1,
            viewMode: 2,
            background: true,
            modal: true,
            guides: false,
            center: true,
            highlight: false,
            zoomOnTouch: false,
            zoomOnWheel: false,
            autoCropArea: 0.8,
            ready: function () {
              setTimeout(() => {
                const cropper = this as any;
                if (!cropper || !cropper.getContainerData) return;

                const containerData = cropper.getContainerData();
                const size =
                  Math.min(containerData.width, containerData.height) * 0.8;

                cropper.setCropBoxData({
                  width: size,
                  height: size,
                });

                cropper.setData({
                  x: (containerData.width - size) / 2,
                  y: (containerData.height - size) / 2,
                });
              }, 0);
            },
            cropBoxMovable: true,
            cropBoxResizable: true,
            dragMode: "move",
          };

          cropperInstance.value = new Cropper(imgElement, options);
          console.log("Cropper instance created:", cropperInstance.value);
        } catch (err) {
          console.error("Error creating cropper:", err);
        }
      };

      // 等待图片加载完成再初始化 cropper
      imgElement.addEventListener("load", initCropper, { once: true });

      // 如果图片已经加载完成，则直接初始化
      if (imgElement.complete) {
        initCropper();
      }
    }, 50);
  };
  reader.readAsDataURL(file);
};

// 获取裁剪后的图像
const getCroppedImage = async () => {
  if (!cropperInstance.value) return;

  try {
    const canvas = cropperInstance.value.getCroppedCanvas({
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
  showPrewiew.value = false;
};

// 取消操作
const cancel = () => {
  showPrewiew.value = false;
  showUpHeadImage.value = false;

  if (cropperInstance.value) {
    try {
      cropperInstance.value.destroy();
    } catch (err) {
      console.warn("Error destroying cropper:", err);
    }
    cropperInstance.value = null;
  }

  if (previewContainer.value) {
    previewContainer.value.innerHTML = "";
  }

  if (fileInput.value) {
    fileInput.value.value = "";
  }
};

// 关闭操作
const close = () => {
  if (showUpHeadImage.value) {
    cancel();
    return;
  }

  emit("close");

  if (cropperInstance.value) {
    try {
      cropperInstance.value.destroy();
    } catch (err) {
      console.warn("Error destroying cropper:", err);
    }
    cropperInstance.value = null;
  }

  if (previewContainer.value) {
    previewContainer.value.innerHTML = "";
  }

  if (fileInput.value) {
    fileInput.value.value = "";
  }
};

// 生命周期
onMounted(() => {
  if (store.state.auth.userHeadImage) {
    headImage.value = store.state.auth.userHeadImage;
  }
});

onBeforeUnmount(() => {
  if (cropperInstance.value) {
    try {
      cropperInstance.value.destroy();
    } catch (err) {
      console.warn("Error destroying cropper:", err);
    }
    cropperInstance.value = null;
  }

  if (previewContainer.value) {
    previewContainer.value.innerHTML = "";
  }

  if (fileInput.value) {
    fileInput.value.value = "";
  }
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

.preview {
  width: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  gap: 50px;
}

.preview-top {
  width: 100%;
  max-width: 300px;
  height: 300px;
  overflow: visible;
  position: relative;
  background: #f0f2f5;
  border-radius: 8px;
}

/* 修复Cropper.js容器溢出问题 */
:deep(.cropper-container) {
  position: absolute !important;
  top: 0;
  left: 0;
  width: 100% !important;
  height: 100% !important;
  z-index: 1000 !important; /* 提升层级，防止被遮挡 */
}

:deep(.cropper-crop-box),
:deep(.cropper-view-box) {
  position: absolute;
  left: 0;
  top: 0;
  border-radius: 50%;
  overflow: hidden;
  border: 2px solid #409eff !important;
  background-color: rgba(64, 158, 255, 0.1) !important;
  z-index: 1001 !important;
}
:deep(.cropper-view-box) {
  position: absolute;
}
/* 新增：确保裁剪点可见 */
:deep(.cropper-point) {
  background-color: #39f;
  width: 8px;
  height: 8px;
  opacity: 1;
  border-radius: 50%;
}

.preview-bottom {
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
