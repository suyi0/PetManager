<template>
  <div class="pc-panel">
    <div class="pc-panel__head">
      <div>
        <h3>{{ imageSrc ? "裁剪新的头像" : "更新个人头像" }}</h3>
        <p>
          {{
            imageSrc
              ? "拖入的图片会先进入裁剪区，确认后再上传。"
              : "支持拖拽或选择图片，建议使用清晰的人像或宠物陪伴照。支持 JPG、PNG，不超过 5MB。"
          }}
        </p>
      </div>
      <button type="button" class="pc-btn pc-btn--ghost" @click="close">
        关闭
      </button>
    </div>

    <section
      v-if="!imageSrc"
      class="avatar-drop"
      @dragover.prevent
      @drop.prevent="handleDrop"
    >
      <div class="avatar-current">
        <img v-if="headImage" :src="headImage" alt="当前头像" />
        <span v-else>{{ displayName.charAt(0) }}</span>
      </div>
      <div class="avatar-drop__copy">
        <div class="avatar-drop__name">{{ displayName }}</div>
        <p>拖拽一张图片到这里，或点击下方按钮从本地选择。</p>
        <button
          type="button"
          class="pc-btn pc-btn--primary"
          @click="triggerFileInput"
        >
          选择图片
        </button>
      </div>

      <input
        ref="fileInput"
        type="file"
        accept="image/*"
        @change="handleFileSelect"
        style="display: none"
      />
    </section>

    <section v-else>
      <div class="avatar-workspace">
        <div class="cropper-wrapper">
          <img
            ref="cropperImage"
            :src="imageSrc"
            alt="待裁剪图片"
            class="cropper-image"
          />
        </div>

        <div class="avatar-preview">
          <span class="avatar-preview__lbl">上传预览</span>
          <img :src="previewImage || headImage" alt="预览" />
          <p>裁剪区域已锁定为头像比例。</p>
        </div>
      </div>

      <div class="pc-actions">
        <button type="button" class="pc-btn pc-btn--secondary" @click="cancel">
          取消
        </button>
        <button
          type="button"
          class="pc-btn pc-btn--primary"
          @click="getcroppedImage('upload')"
        >
          确认上传
        </button>
      </div>
    </section>
  </div>
</template>

<script setup lang="ts">
import { computed, nextTick, onBeforeUnmount, onMounted, ref } from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";
import Cropper from "cropperjs";
import { profileApi } from "@/modules/user/api/userApi";
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
    const uploadResponse = await profileApi.uploadAvatar(formData);

    const avatarUrl =
      uploadResponse.data?.data?.avatarUrl || uploadResponse.data?.avatarUrl;

    if (!avatarUrl) {
      throw new Error("Avatar upload response is missing avatarUrl");
    }

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

<style scoped>
.avatar-drop {
  display: grid;
  grid-template-columns: 96px minmax(0, 1fr);
  gap: 18px;
  align-items: center;
  padding: 20px;
  border: 1px dashed var(--pc-border, #dfe7df);
  border-radius: 10px;
  background: var(--pc-panel, #f4f7f4);
}

.avatar-current {
  width: 96px;
  height: 96px;
  border-radius: 10px;
  overflow: hidden;
  display: grid;
  place-items: center;
  background: #fff;
  border: 1px solid var(--pc-border, #dfe7df);
  color: var(--pc-primary, #245849);
  font-size: 34px;
  font-weight: 700;
}

.avatar-current img {
  width: 100%;
  height: 100%;
  object-fit: cover;
}

.avatar-drop__copy {
  display: grid;
  gap: 8px;
  justify-items: start;
}

.avatar-drop__name {
  font-size: 16px;
  font-weight: 700;
  color: var(--pc-primary-deep, #18373a);
}

.avatar-drop__copy p {
  margin: 0;
  font-size: 13px;
  color: var(--pc-muted, #6d7b72);
}

.avatar-workspace {
  display: grid;
  grid-template-columns: minmax(0, 1fr) 240px;
  gap: 16px;
  align-items: start;
}

.cropper-wrapper {
  min-height: 340px;
  padding: 14px;
  border-radius: 10px;
  border: 1px solid var(--pc-border, #dfe7df);
  background: #fff;
}

.cropper-image {
  width: 100%;
  max-height: 480px;
  object-fit: contain;
}

.avatar-preview {
  display: grid;
  gap: 10px;
  padding: 18px;
  border-radius: 10px;
  border: 1px solid var(--pc-border, #dfe7df);
  background: var(--pc-panel, #f4f7f4);
  text-align: center;
}

.avatar-preview__lbl {
  font-size: 12px;
  font-weight: 600;
  color: var(--pc-muted, #6d7b72);
}

.avatar-preview img {
  width: 132px;
  height: 132px;
  justify-self: center;
  object-fit: cover;
  border-radius: 50%;
  border: 1px solid var(--pc-border, #dfe7df);
}

.avatar-preview p {
  margin: 0;
  font-size: 12px;
  color: var(--pc-muted, #6d7b72);
}

@media (max-width: 760px) {
  .avatar-workspace {
    grid-template-columns: 1fr;
  }
}
</style>
