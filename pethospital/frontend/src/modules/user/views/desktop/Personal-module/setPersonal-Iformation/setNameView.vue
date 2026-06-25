<template>
  <div class="pc-panel">
    <div class="pc-panel__head">
      <div>
        <h3>更新姓名展示</h3>
        <p>
          姓名会用于预约卡片、订单联系人与个人资料展示。支持拆成三段维护，空白项自动忽略。
        </p>
      </div>
      <button type="button" class="pc-btn pc-btn--ghost" @click="close">
        关闭
      </button>
    </div>

    <div class="pc-current">
      <span class="pc-current__lbl">当前显示名</span>
      <span class="pc-current__val">{{ previewDisplayName }}</span>
    </div>

    <form class="pc-form" @submit.prevent="handleContinue">
      <label class="pc-field">
        <span>姓氏</span>
        <input
          v-model.trim="lastName"
          type="text"
          placeholder="例如：林"
          @input="isButtonActive = true"
        />
      </label>

      <label class="pc-field">
        <span>中间名（可选）</span>
        <input
          v-model.trim="middleName"
          type="text"
          placeholder="可选，例如：小"
          @input="isButtonActive = true"
        />
      </label>

      <label class="pc-field">
        <span>名字</span>
        <input
          v-model.trim="firstName"
          type="text"
          placeholder="例如：满"
          @input="isButtonActive = true"
        />
      </label>

      <div class="pc-helper">
        <span class="pc-dot"></span>
        格式预览：{{ previewDisplayName }}
      </div>

      <div v-if="isButtonActive" class="pc-actions">
        <button type="button" class="pc-btn pc-btn--secondary" @click="cancel">
          取消
        </button>
        <button type="submit" class="pc-btn pc-btn--primary">保存姓名</button>
      </div>
    </form>
  </div>
</template>

<script setup lang="ts">
import { computed, onMounted, ref } from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";

const emit = defineEmits(["close", "submit"]);
const store = useStore(storeKey);

const isButtonActive = ref(false);
const lastName = ref("");
const middleName = ref("");
const firstName = ref("");

const nameParts = computed(() =>
  [lastName.value, middleName.value, firstName.value]
    .map((part) => part.trim())
    .filter((part) => part.length > 0)
);

const storageName = computed(() => {
  if (firstName.value.trim()) {
    return [lastName.value, middleName.value, firstName.value]
      .map((part) => part.trim())
      .join("·");
  }

  return nameParts.value.join("·");
});

const previewDisplayName = computed(() => {
  return nameParts.value.join("") || "未设置姓名";
});

function cancel() {
  isButtonActive.value = false;
  close();
}

function handleContinue() {
  isButtonActive.value = false;
  emit("submit", {
    field: "userNameParts",
    name: previewDisplayName.value === "未设置姓名" ? "" : storageName.value,
    lastName: lastName.value.trim(),
    middleName: middleName.value.trim(),
    firstName: firstName.value.trim(),
  });
}

function close() {
  emit("close");
}

onMounted(() => {
  lastName.value = store.state.currentUser.userLastName || "";
  middleName.value = store.state.currentUser.userMiddleName || "";
  firstName.value = store.state.currentUser.userFirstName || "";
});
</script>
