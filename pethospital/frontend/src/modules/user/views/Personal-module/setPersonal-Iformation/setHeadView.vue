<template>
  <div class="avatar-editor">
    <div class="avatar-editor__head">
      <div>
        <p>Avatar Studio</p>
        <h3>{{ imageSrc ? "裁剪新的头像" : "更新个人头像" }}</h3>
        <span>
          {{
            imageSrc
              ? "拖入的图片会先进入裁剪区，确认后再上传到服务器。"
              : "支持拖拽或选择图片，建议使用清晰的人像或宠物陪伴照。"
          }}
        </span>
      </div>
      <button class="avatar-editor__ghost" @click="close">关闭</button>
    </div>

    <section
      v-if="!imageSrc"
      class="avatar-editor__dropzone"
      @dragover.prevent
      @drop.prevent="handleDrop"
    >
      <div class="avatar-editor__hero">
        <div class="avatar-editor__current">
          <img :src="headImage" class="avatar-editor__current-image" />
        </div>
        <div class="avatar-editor__copy">
          <small>当前头像</small>
          <strong>{{ displayName }}</strong>
          <span>拖拽一张图片到这里，或者点击下方按钮从本地相册中选择。</span>
        </div>
      </div>

      <div class="avatar-editor__actions">
        <button class="avatar-editor__primary" @click="triggerFileInput">
          选择图片
        </button>
        <span>支持 JPG、PNG 格式，大小不超过 5MB。</span>
      </div>

      <input
        ref="fileInput"
        type="file"
        accept="image/*"
        @change="handleFileSelect"
        style="display: none"
      />
    </section>

    <section v-else class="avatar-editor__cropper">
      <div class="avatar-editor__workspace">
        <div class="cropper-wrapper">
          <img
            ref="cropperImage"
            :src="imageSrc"
            alt="待裁剪图片"
            class="cropper-image"
          />
        </div>

        <div class="avatar-editor__preview">
          <small>上传预览</small>
          <img
            :src="previewImage || headImage"
            class="avatar-editor__preview-image"
          />
          <span>裁剪区域会自动限制为头像比例，方便直接使用。</span>
        </div>
      </div>

      <div class="avatar-editor__bottom">
        <p class="avatar-editor__tip">
          确认后会立即上传新头像，并自动同步到个人中心和顶部菜单。
        </p>
        <div class="avatar-editor__button-row">
          <button class="avatar-editor__ghost" @click="cancel">取消</button>
          <button
            class="avatar-editor__primary"
            @click="getcroppedImage('upload')"
          >
            确认上传
          </button>
        </div>
      </div>
    </section>
  </div>
</template>

<script setup lang="ts">
import { computed, nextTick, onBeforeUnmount, onMounted, ref } from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/store/appStore";
import Cropper from "cropperjs";
import axios from "axios";
import "cropperjs/dist/cropper.css";

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

const emit = defineEmits(["close", "crop-complete", "cancel"]);

const store = useStore(storeKey);
const headImage = ref("");
const fileInput = ref<HTMLInputElement | null>(null);
const cropperImage = ref<HTMLImageElement | null>(null);
const imageSrc = ref<string | null>(null);
const croppedImage = ref<string>();
const previewImage = ref<string>();
const cropper = ref<Cropper | null>(null);

const displayName = computed(
  () => store.getters["auth/formattedUserName"] || "当前用户"
);

const triggerFileInput = () => {
  fileInput.value?.click();
};

const handleFileSelect = (e: Event) => {
  const target = e.target as HTMLInputElement;
  const file = target.files?.[0];
  if (file) {
    loadImage(file);
  }
};

const handleDrop = (e: DragEvent) => {
  const file = e.dataTransfer?.files?.[0];
  if (file && file.type.startsWith("image/")) {
    loadImage(file);
  }
};

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

const initCropper = () => {
  if (!cropperImage.value) return;

  if (cropper.value) {
    cropper.value.destroy();
  }

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
    ready() {
      getcroppedImage();
    },
    crop() {
      getcroppedImage();
    },
  });
};

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

    if (fileName === "upload") {
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
    }

    await nextTick();
  } catch (error) {
    console.error("裁剪图片时出错:", error);
  }
};

const uploadImage = async (formData: FormData) => {
  try {
    const uploadResponse = await axios.post("/api/upload/avatar", formData, {
      headers: { "Content-Type": "multipart/form-data" },
    });

    const avatarUrl = uploadResponse.data.avatarUrl;

    store.dispatch("currentUser/updateUserField", {
      field: "userHeadImage",
      value: avatarUrl,
    });

    croppedImage.value = undefined;
    close();
  } catch (error) {
    console.error("上传失败:", error);
    alert("头像上传失败，请重试");
  }
};

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

const close = () => {
  if (croppedImage.value) {
    cancel();
    return;
  }

  imageSrc.value = null;
  cropper.value?.destroy();
  cropper.value = null;

  if (fileInput.value) {
    fileInput.value.value = "";
  }

  emit("close");
};

onMounted(() => {
  headImage.value = store.state.currentUser.userHeadImage || "";
});

onBeforeUnmount(() => {
  cropper.value?.destroy();
});
</script>

<style scoped lang="scss">
.avatar-editor {
  display: grid;
  gap: 18px;
  padding: 24px;
  border-radius: 30px;
  border: 1px solid rgba(21, 91, 92, 0.08);
  background: rgba(255, 255, 255, 0.76);
  box-shadow: 0 18px 44px rgba(24, 90, 91, 0.06);
}

.avatar-editor__head {
  display: flex;
  align-items: start;
  justify-content: space-between;
  gap: 16px;
}

.avatar-editor__head p,
.avatar-editor__copy small,
.avatar-editor__preview small {
  margin: 0;
  color: #1f8e89;
  letter-spacing: 0.14em;
  text-transform: uppercase;
  font-size: 11px;
  font-weight: 700;
}

.avatar-editor__head h3 {
  margin: 6px 0 0;
  color: #133f42;
  font-size: 32px;
}

.avatar-editor__head span,
.avatar-editor__copy span,
.avatar-editor__preview span,
.avatar-editor__tip {
  display: block;
  margin-top: 10px;
  color: #607975;
  line-height: 1.8;
  font-size: 14px;
}

.avatar-editor__dropzone,
.avatar-editor__cropper {
  display: grid;
  gap: 18px;
  padding: 22px;
  border-radius: 28px;
  border: 1px solid rgba(21, 91, 92, 0.08);
  background: linear-gradient(
    135deg,
    rgba(136, 214, 206, 0.18),
    rgba(243, 197, 155, 0.14)
  );
}

.avatar-editor__hero {
  display: grid;
  grid-template-columns: 160px minmax(0, 1fr);
  gap: 18px;
  align-items: center;
}

.avatar-editor__current {
  width: 160px;
  height: 160px;
  display: grid;
  place-items: center;
  border-radius: 42px;
  background: linear-gradient(135deg, #91ddd2, #f0c29b);
  box-shadow: 0 18px 34px rgba(28, 98, 99, 0.14);
}

.avatar-editor__current-image {
  width: 136px;
  height: 136px;
  object-fit: cover;
  border-radius: 34px;
}

.avatar-editor__copy {
  display: grid;
  gap: 6px;
}

.avatar-editor__copy strong {
  color: #143f42;
  font-size: 28px;
}

.avatar-editor__actions {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 14px;
  flex-wrap: wrap;
}

.avatar-editor__workspace {
  display: grid;
  grid-template-columns: minmax(0, 1fr) 280px;
  gap: 18px;
  align-items: start;
}

.cropper-wrapper {
  min-height: 360px;
  padding: 18px;
  border-radius: 24px;
  background: rgba(255, 255, 255, 0.74);
}

.cropper-image {
  width: 100%;
  max-height: 520px;
  object-fit: contain;
}

.avatar-editor__preview {
  display: grid;
  gap: 10px;
  padding: 20px;
  border-radius: 24px;
  background: rgba(255, 255, 255, 0.74);
  text-align: center;
}

.avatar-editor__preview-image {
  width: 148px;
  height: 148px;
  justify-self: center;
  object-fit: cover;
  border-radius: 50%;
  box-shadow: 0 18px 34px rgba(24, 90, 91, 0.1);
}

.avatar-editor__bottom {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 14px;
  flex-wrap: wrap;
}

.avatar-editor__button-row {
  display: flex;
  gap: 10px;
  flex-wrap: wrap;
}

.avatar-editor__ghost,
.avatar-editor__primary {
  border: none;
  border-radius: 999px;
  padding: 12px 16px;
  font-size: 13px;
  font-weight: 700;
  cursor: pointer;
}

.avatar-editor__ghost {
  background: rgba(20, 82, 84, 0.08);
  color: #154144;
}

.avatar-editor__primary {
  background: linear-gradient(135deg, #167f80, #2ca7a4);
  color: #fff;
  box-shadow: 0 16px 30px rgba(23, 104, 105, 0.22);
}

@media (max-width: 960px) {
  .avatar-editor__head,
  .avatar-editor__hero,
  .avatar-editor__workspace {
    grid-template-columns: 1fr;
    display: grid;
  }

  .avatar-editor__current {
    width: 124px;
    height: 124px;
    border-radius: 30px;
  }

  .avatar-editor__current-image {
    width: 104px;
    height: 104px;
    border-radius: 24px;
  }

  .cropper-wrapper {
    min-height: 280px;
  }
}
</style>
