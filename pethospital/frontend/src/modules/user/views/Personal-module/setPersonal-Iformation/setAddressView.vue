<template>
  <div class="address-editor">
    <div class="address-editor__head">
      <div>
        <p>Address Studio</p>
        <h3>维护配送地址</h3>
        <span>保存后会回到个人面板，并同步更新当前用户端展示。</span>
      </div>
      <button class="address-editor__ghost" @click="close">关闭</button>
    </div>

    <section class="address-editor__hero">
      <article>
        <small>当前地址预览</small>
        <strong>{{ addressPreview }}</strong>
      </article>
      <article>
        <small>使用说明</small>
        <span
          >建议把常用配送地址整理完整，后续药品寄送和服务确认会直接复用。</span
        >
      </article>
    </section>

    <form class="address-form" @submit.prevent="saveAddress">
      <label class="address-form__wide">
        <span>街道地址</span>
        <input
          v-model.trim="address.street"
          type="text"
          maxlength="255"
          placeholder="例如：徐汇区漕溪北路 18 号 1203 室"
        />
      </label>

      <label>
        <span>城市</span>
        <input
          v-model.trim="address.city"
          type="text"
          maxlength="100"
          placeholder="例如：上海"
        />
      </label>

      <label>
        <span>省 / 州</span>
        <input
          v-model.trim="address.state"
          type="text"
          maxlength="100"
          placeholder="例如：上海市"
        />
      </label>

      <label>
        <span>邮政编码</span>
        <input
          v-model.trim="address.postalCode"
          type="text"
          maxlength="20"
          placeholder="例如：200030"
        />
      </label>

      <label>
        <span>国家 / 地区</span>
        <select v-model="address.country">
          <option value="中国">中国</option>
          <option value="美国">美国</option>
          <option value="其他">其他</option>
        </select>
      </label>

      <div class="address-form__actions">
        <button type="button" class="address-form__ghost" @click="fillCurrent">
          使用当前资料
        </button>
        <button type="submit" class="address-form__primary">保存地址</button>
      </div>
    </form>
  </div>
</template>

<script setup lang="ts">
import { computed, onMounted, reactive } from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/store/appStore";

const emit = defineEmits(["close", "submit"]);
const store = useStore(storeKey);

const address = reactive({
  street: "",
  city: "",
  state: "",
  postalCode: "",
  country: "中国",
});

const splitAddress = (value: string) => {
  const [
    street = "",
    city = "",
    state = "",
    postalCode = "",
    country = "中国",
  ] = value.split(",").map((part) => part.trim());

  address.street = street;
  address.city = city;
  address.state = state;
  address.postalCode = postalCode;
  address.country = country || "中国";
};

const buildAddressString = () =>
  [
    address.street,
    address.city,
    address.state,
    address.postalCode,
    address.country,
  ]
    .filter((part) => part && part.trim().length > 0)
    .join(", ");

const addressPreview = computed(
  () => buildAddressString() || "暂未填写完整地址"
);

const close = () => {
  emit("close");
};

const fillCurrent = () => {
  splitAddress(store.state.currentUser.userAddress || "");
};

const saveAddress = () => {
  emit("submit", {
    field: "userAddress",
    address: buildAddressString(),
  });
};

onMounted(() => {
  fillCurrent();
});
</script>

<style scoped lang="scss">
.address-editor {
  display: grid;
  gap: 18px;
  padding: 24px;
  border-radius: 30px;
  border: 1px solid rgba(21, 91, 92, 0.08);
  background: rgba(255, 255, 255, 0.74);
  box-shadow: 0 18px 44px rgba(24, 90, 91, 0.06);
}

.address-editor__head {
  display: flex;
  align-items: start;
  justify-content: space-between;
  gap: 16px;
}

.address-editor__head p {
  margin: 0;
  color: #1f8e89;
  letter-spacing: 0.14em;
  text-transform: uppercase;
  font-size: 11px;
  font-weight: 700;
}

.address-editor__head h3 {
  margin: 6px 0 0;
  color: #133f42;
  font-size: 32px;
}

.address-editor__head span {
  display: block;
  margin-top: 10px;
  color: #607975;
  line-height: 1.8;
  font-size: 14px;
}

.address-form__ghost,
.address-editor__ghost,
.address-form__primary {
  border: none;
  border-radius: 999px;
  padding: 12px 16px;
  font-size: 13px;
  font-weight: 700;
  cursor: pointer;
}

.address-editor__ghost,
.address-form__ghost {
  background: rgba(20, 82, 84, 0.08);
  color: #154144;
}

.address-editor__hero {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
}

.address-editor__hero article {
  padding: 16px 18px;
  border-radius: 18px;
  background: linear-gradient(
    135deg,
    rgba(136, 214, 206, 0.18),
    rgba(243, 197, 155, 0.14)
  );
}

.address-editor__hero small {
  color: #1f8e89;
  letter-spacing: 0.14em;
  text-transform: uppercase;
  font-size: 11px;
  font-weight: 700;
}

.address-editor__hero strong {
  display: block;
  margin-top: 8px;
  color: #143f42;
  font-size: 22px;
  line-height: 1.4;
}

.address-editor__hero span {
  display: block;
  margin-top: 8px;
  color: #607975;
  line-height: 1.8;
  font-size: 14px;
}

.address-form__primary {
  background: linear-gradient(135deg, #167f80, #2ca7a4);
  color: #fff;
  box-shadow: 0 16px 30px rgba(23, 104, 105, 0.22);
}

.address-form {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 14px;
}

.address-form label {
  display: grid;
  gap: 8px;
}

.address-form span {
  color: #24484b;
  font-size: 13px;
  font-weight: 700;
}

.address-form input,
.address-form select {
  width: 100%;
  padding: 13px 14px;
  border: 1px solid rgba(20, 82, 84, 0.12);
  border-radius: 16px;
  background: rgba(255, 255, 255, 0.94);
  color: #173f42;
  font-size: 14px;
}

.address-form input:focus,
.address-form select:focus {
  outline: none;
  border-color: rgba(24, 128, 127, 0.5);
  box-shadow: 0 0 0 4px rgba(141, 218, 210, 0.18);
}

.address-form__wide,
.address-form__actions {
  grid-column: 1 / -1;
}

.address-form__actions {
  display: flex;
  gap: 10px;
  justify-content: flex-end;
  flex-wrap: wrap;
}

@media (max-width: 900px) {
  .address-editor__head,
  .address-editor__hero,
  .address-form {
    grid-template-columns: 1fr;
    display: grid;
  }
}
</style>
