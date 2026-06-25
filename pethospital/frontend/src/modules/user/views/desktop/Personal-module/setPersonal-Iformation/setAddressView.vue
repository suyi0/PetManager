<template>
  <div class="pc-panel">
    <div class="pc-panel__head">
      <div>
        <h3>维护配送地址</h3>
        <p>
          地址用于药品配送、护理用品寄送与服务范围确认。保存后会同步回个人中心总览。
        </p>
      </div>
      <button type="button" class="pc-btn pc-btn--ghost" @click="close">
        关闭
      </button>
    </div>

    <div class="pc-current">
      <span class="pc-current__lbl">当前地址</span>
      <span
        class="pc-current__val"
        :class="{ 'pc-current__val--empty': !buildAddressString() }"
      >
        {{ addressPreview }}
      </span>
    </div>

    <form class="pc-form" @submit.prevent="saveAddress">
      <label class="pc-field">
        <span>街道地址</span>
        <input
          v-model.trim="address.street"
          type="text"
          maxlength="255"
          placeholder="例如：徐汇区漕溪北路 18 号 1203 室"
        />
      </label>

      <div class="addr-grid">
        <label class="pc-field">
          <span>城市</span>
          <input
            v-model.trim="address.city"
            type="text"
            maxlength="100"
            placeholder="例如：上海"
          />
        </label>

        <label class="pc-field">
          <span>省 / 州</span>
          <input
            v-model.trim="address.state"
            type="text"
            maxlength="100"
            placeholder="例如：上海市"
          />
        </label>

        <label class="pc-field">
          <span>邮政编码</span>
          <input
            v-model.trim="address.postalCode"
            type="text"
            maxlength="20"
            placeholder="例如：200030"
          />
        </label>

        <label class="pc-field">
          <span>国家 / 地区</span>
          <select v-model="address.country">
            <option value="中国">中国</option>
            <option value="美国">美国</option>
            <option value="其他">其他</option>
          </select>
        </label>
      </div>

      <div class="pc-actions">
        <button
          type="button"
          class="pc-btn pc-btn--secondary"
          @click="fillCurrent"
        >
          使用当前资料
        </button>
        <button type="submit" class="pc-btn pc-btn--primary">保存地址</button>
      </div>
    </form>
  </div>
</template>

<script setup lang="ts">
import { computed, onMounted, reactive } from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";

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

<style scoped>
.addr-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 14px;
}
@media (max-width: 640px) {
  .addr-grid {
    grid-template-columns: 1fr;
  }
}
</style>
