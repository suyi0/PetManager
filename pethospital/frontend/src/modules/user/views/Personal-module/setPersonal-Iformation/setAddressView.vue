<template>
  <div class="address-form">
    <div class="form-group">
      <label>街道地址:</label>
      <input type="text" v-model="address.street" maxlength="255" />
    </div>

    <div class="form-row">
      <div class="form-group">
        <label>城市:</label>
        <input type="text" v-model="address.city" maxlength="100" />
      </div>

      <div class="form-group">
        <label>省/州:</label>
        <input type="text" v-model="address.state" maxlength="100" />
      </div>
    </div>

    <div class="form-row">
      <div class="form-group">
        <label>邮政编码:</label>
        <input type="text" v-model="address.postalCode" maxlength="20" />
      </div>

      <div class="form-group">
        <label>国家:</label>
        <select v-model="address.country">
          <option value="CN">中国</option>
          <option value="US">美国</option>
          <!-- 其他国家选项 -->
        </select>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { reactive } from "vue";
import { useStore } from "vuex";
import { useRouter } from "vue-router";
import { storeKey } from "@/store/appStore";
import { defineComponent } from "vue";

const emit = defineEmits(["close", "submit"]);

defineComponent({
  name: "userSetAddress",
});

const store = useStore(storeKey);
const router = useRouter();
store;

const address = reactive({
  street: "",
  city: "",
  state: "",
  postalCode: "",
  country: "CN",
});

// 验证地址字段
function validateAddress() {
  if (!address.street || !address.city || !address.country) {
    throw new Error("请填写完整的地址信息");
  }

  // 验证邮政编码格式（以中国为例）
  if (address.country === "CN" && address.postalCode) {
    const postalCodeRegex = /^[0-9]{6}$/;
    if (!postalCodeRegex.test(address.postalCode)) {
      throw new Error("请输入有效的邮政编码");
    }
  }
}
validateAddress();

function saveAddress() {
  emit("submit", {
    field: "userAddress",
    value: address,
  });

  // 显示成功消息
  alert("地址保存成功！");

  // 返回个人资料页面
  router.push("/user/personalRight");
}
saveAddress;
</script>

<style scoped>
.address-form {
  width: 100%;
  margin-left: 55px;
}
.set-address-container {
  padding: 20px;
}

.form-group {
  margin-bottom: 15px;
}

label {
  display: block;
  margin-bottom: 5px;
  font-weight: bold;
}

input[type="text"],
select {
  width: 50%;
  padding: 8px;
  border: 1px solid #ddd;
  border-radius: 4px;
}

.save-button {
  background-color: #007bff;
  color: white;
  padding: 10px 20px;
  border: none;
  border-radius: 4px;
  cursor: pointer;
}

.save-button:hover {
  background-color: #0056b3;
}
</style>
