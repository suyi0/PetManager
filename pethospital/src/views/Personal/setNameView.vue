<template>
  <div class="setName">
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

    <!-- 页面标题 -->
    <div class="title-container">
      <div class="icon">
        <svg
          width="40"
          height="40"
          viewBox="0 0 40 40"
          fill="none"
          xmlns="http://www.w3.org/2000/svg"
        >
          <path
            d="M20 15C23.3137 15 26 12.3137 26 9C26 5.68629 23.3137 3 20 3C16.6863 3 14 5.68629 14 9C14 12.3137 16.6863 15 20 15Z"
            fill="#409EFF"
          />
          <path
            d="M14 19H26C27.1046 19 28 18.1046 28 17V15C28 13.8954 27.1046 13 26 13H14C12.8954 13 12 13.8954 12 15V17C12 18.1046 12.8954 19 14 19Z"
            fill="#409EFF"
          />
        </svg>
      </div>
      <h2>姓名</h2>
    </div>

    <div class="input-container">
      <!-- 姓氏输入框 -->
      <div class="input-group">
        <label class="input-label">姓氏</label>
        <input
          v-model="lastName"
          type="text"
          placeholder="请输入姓氏"
          class="input-field"
          @input="isButtonActive = true"
        />
      </div>

      <!-- 中间名输入框 -->
      <div class="input-group">
        <label class="input-label">中间名（可选）</label>
        <input
          v-model="middleName"
          type="text"
          placeholder="请输入中间名"
          class="input-field"
          @input="isButtonActive = true"
        />
      </div>

      <!-- 名字输入框 -->
      <div class="input-group">
        <label class="input-label">名字</label>
        <input
          v-model="firstName"
          type="text"
          placeholder="请输入名字"
          class="input-field"
          @input="isButtonActive = true"
        />
      </div>
    </div>
    <!-- 按钮组 -->
    <div v-show="isButtonActive" class="button-group">
      <button @click="cancel" class="cancel-button">取消</button>
      <button @click="handleContinue" class="save-button">存储</button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from "vue";
import { useStore } from "vuex";
import { key } from "../../store";

// 定义 emits
const emit = defineEmits(["close", "submit"]);

// 使用 store
const store = useStore(key);

// 响应式数据
const isButtonActive = ref(false);
const lastName = ref("");
const middleName = ref("");
const firstName = ref("");
const isValidPhoneNumber = ref(true); // 这个变量在原代码中被引用但未定义，这里添加定义

// 分离姓名字符串
function parseDateString(dateString: string) {
  if (!dateString) return;

  const [lastNameVal, middleNameVal, firstNameVal] = dateString.split("·");
  lastName.value = lastNameVal || "";
  middleName.value = middleNameVal || "";
  // （ || "" ） 确保变量始终是字符串类型
  // 提供默认值，避免 undefined 或 null 值
  // 保持代码的健壮性
  firstName.value = firstNameVal || "";
}

function cancel() {
  isButtonActive.value = false;
  close();
}

function handleContinue() {
  isButtonActive.value = false;
  if (isValidPhoneNumber.value) {
    const field = "userName";
    const name =
      lastName.value + "·" + middleName.value + "·" + firstName.value;
    emit("submit", {
      field,
      name,
    });
  }
}

// 处理关闭事件
function close() {
  emit("close");
}

// 在 mounted 钩子中执行初始化逻辑
onMounted(() => {
  const saveName = store.state.auth.userName;

  if (saveName) {
    parseDateString(saveName);
  }
});
</script>

<style scoped lang="scss">
.setName {
  width: 100%;
  max-width: 500px;
  padding: 24px;
  background-color: #ffffff;
  border-radius: 12px;
  box-shadow: 0px 0px 5px 2px rgba(0, 0, 0, 0.3);
  position: relative;
  top: 45px;

  .close-button {
    position: absolute;
    top: 12px;
    right: 16px;
    font-size: 24px;
    cursor: pointer;
    color: #333;
    z-index: 10;

    .login-cha-button {
      background: none;
      border: none;
      cursor: pointer;
      padding: 0;
    }
  }

  .title-container {
    text-align: center;
    margin-bottom: 32px;

    .icon {
      margin-bottom: 12px;
    }

    h2 {
      font-size: 24px;
      font-weight: 600;
      color: #333;
    }
  }

  .input-container {
    display: flex;
    flex-direction: column;
    align-items: center;
  }

  .input-group {
    width: 300px;
    margin-bottom: 20px;

    .input-label {
      display: block;
      margin-bottom: 8px;
      font-size: 14px;
      color: #666;
    }

    .input-field {
      width: 100%;
      padding: 12px 16px;
      border: 1px solid #dcdfe6;
      border-radius: 8px;
      font-size: 16px;
      transition: border-color 0.3s ease;

      &:focus {
        outline: none;
        border-color: #409eff;
        box-shadow: 0 0 0 2px rgba(64, 158, 255, 0.2);
      }
    }
  }
  .button-group {
    display: flex;
    justify-content: space-between;
    margin-top: 32px;

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
  }
}
</style>
