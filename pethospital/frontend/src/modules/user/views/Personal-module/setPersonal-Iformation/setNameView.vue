<template>
  <div class="profile-editor">
    <div class="profile-editor__head">
      <div>
        <p>Name Studio</p>
        <h3>更新姓名展示</h3>
        <span>
          当前姓名会同步用于预约卡片、订单联系人和个人资料页展示，建议保持为你平时最常用的称呼。
        </span>
      </div>
      <button class="profile-editor__ghost" @click="close">关闭</button>
    </div>

    <section class="profile-editor__hero">
      <div class="profile-editor__badge">{{ userInitial }}</div>
      <div class="profile-editor__summary">
        <small>当前显示名</small>
        <strong>{{ previewName }}</strong>
        <span>
          支持把姓名拆成三段来维护，空白项会自动忽略，不会影响最终显示。
        </span>
      </div>
    </section>

    <form class="profile-editor__form" @submit.prevent="handleContinue">
      <label class="editor-field">
        <span>姓氏</span>
        <input
          v-model.trim="lastName"
          type="text"
          placeholder="例如：林"
          @input="isButtonActive = true"
        />
      </label>

      <label class="editor-field">
        <span>中间名</span>
        <input
          v-model.trim="middleName"
          type="text"
          placeholder="可选，例如：小"
          @input="isButtonActive = true"
        />
      </label>

      <label class="editor-field">
        <span>名字</span>
        <input
          v-model.trim="firstName"
          type="text"
          placeholder="例如：满"
          @input="isButtonActive = true"
        />
      </label>

      <div class="editor-tips">
        <article>
          <small>格式预览</small>
          <strong>{{ previewName }}</strong>
        </article>
        <article>
          <small>填写建议</small>
          <span>如果你习惯只显示一个昵称，也可以只填一个字段。</span>
        </article>
      </div>

      <div v-if="isButtonActive" class="editor-actions">
        <button type="button" class="profile-editor__ghost" @click="cancel">
          取消
        </button>
        <button type="submit" class="profile-editor__primary">保存姓名</button>
      </div>
    </form>
  </div>
</template>

<script setup lang="ts">
import { computed, onMounted, ref } from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/store/appStore";

const emit = defineEmits(["close", "submit"]);
const store = useStore(storeKey);

const isButtonActive = ref(false);
const lastName = ref("");
const middleName = ref("");
const firstName = ref("");

function parseNameString(nameString: string) {
  if (!nameString) return;

  const parts = nameString
    .split("·")
    .map((part) => part.trim())
    .filter((part) => part.length > 0);

  lastName.value = parts[0] || "";
  middleName.value = parts[1] || "";
  firstName.value = parts[2] || "";
}

const previewName = computed(() => {
  const parts = [lastName.value, middleName.value, firstName.value]
    .map((part) => part.trim())
    .filter((part) => part.length > 0);

  return parts.join("·") || "未设置姓名";
});

const userInitial = computed(() =>
  String(previewName.value || "U")
    .trim()
    .charAt(0)
    .toUpperCase()
);

function cancel() {
  isButtonActive.value = false;
  close();
}

function handleContinue() {
  isButtonActive.value = false;
  emit("submit", {
    field: "userName",
    name: previewName.value === "未设置姓名" ? "" : previewName.value,
  });
}

function close() {
  emit("close");
}

onMounted(() => {
  const savedName = store.state.currentUser.userName;
  if (savedName) {
    parseNameString(savedName);
  }
});
</script>

<style scoped lang="scss">
.profile-editor {
  display: grid;
  gap: 18px;
  padding: 24px;
  border-radius: 30px;
  border: 1px solid rgba(21, 91, 92, 0.08);
  background: rgba(255, 255, 255, 0.76);
  box-shadow: 0 18px 44px rgba(24, 90, 91, 0.06);
}

.profile-editor__head {
  display: flex;
  align-items: start;
  justify-content: space-between;
  gap: 16px;
}

.profile-editor__head p,
.profile-editor__summary small,
.editor-tips small {
  margin: 0;
  color: #1f8e89;
  letter-spacing: 0.14em;
  text-transform: uppercase;
  font-size: 11px;
  font-weight: 700;
}

.profile-editor__head h3 {
  margin: 6px 0 0;
  color: #133f42;
  font-size: 32px;
}

.profile-editor__head span,
.profile-editor__summary span,
.editor-tips span {
  display: block;
  margin-top: 10px;
  color: #607975;
  line-height: 1.8;
  font-size: 14px;
}

.profile-editor__hero {
  display: grid;
  grid-template-columns: 104px minmax(0, 1fr);
  gap: 18px;
  padding: 22px;
  border-radius: 28px;
  background: linear-gradient(
    135deg,
    rgba(136, 214, 206, 0.24),
    rgba(243, 197, 155, 0.18)
  );
}

.profile-editor__badge {
  width: 104px;
  height: 104px;
  display: grid;
  place-items: center;
  border-radius: 30px;
  background: linear-gradient(135deg, #91ddd2, #f0c29b);
  color: #15474a;
  font-family: "Rajdhani", "Noto Sans SC", sans-serif;
  font-size: 42px;
  font-weight: 700;
  box-shadow: 0 18px 34px rgba(28, 98, 99, 0.14);
}

.profile-editor__summary {
  display: grid;
  align-content: center;
  gap: 6px;
}

.profile-editor__summary strong,
.editor-tips strong {
  color: #143f42;
  font-size: 26px;
}

.profile-editor__form {
  display: grid;
  gap: 14px;
}

.editor-field {
  display: grid;
  gap: 8px;
}

.editor-field span {
  color: #24484b;
  font-size: 13px;
  font-weight: 700;
}

.editor-field input {
  width: 100%;
  padding: 13px 14px;
  border: 1px solid rgba(20, 82, 84, 0.12);
  border-radius: 16px;
  background: rgba(255, 255, 255, 0.94);
  color: #173f42;
  font-size: 14px;
}

.editor-field input:focus {
  outline: none;
  border-color: rgba(24, 128, 127, 0.5);
  box-shadow: 0 0 0 4px rgba(141, 218, 210, 0.18);
}

.editor-tips {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
}

.editor-tips article {
  padding: 16px 18px;
  border-radius: 18px;
  background: rgba(255, 249, 242, 0.95);
}

.editor-actions {
  display: flex;
  gap: 10px;
  justify-content: flex-end;
  flex-wrap: wrap;
}

.profile-editor__ghost,
.profile-editor__primary {
  border: none;
  border-radius: 999px;
  padding: 12px 16px;
  font-size: 13px;
  font-weight: 700;
  cursor: pointer;
}

.profile-editor__ghost {
  background: rgba(20, 82, 84, 0.08);
  color: #154144;
}

.profile-editor__primary {
  background: linear-gradient(135deg, #167f80, #2ca7a4);
  color: #fff;
  box-shadow: 0 16px 30px rgba(23, 104, 105, 0.22);
}

@media (max-width: 900px) {
  .profile-editor__head,
  .profile-editor__hero,
  .editor-tips {
    grid-template-columns: 1fr;
    display: grid;
  }

  .profile-editor__badge {
    width: 88px;
    height: 88px;
    border-radius: 24px;
    font-size: 36px;
  }
}
</style>
