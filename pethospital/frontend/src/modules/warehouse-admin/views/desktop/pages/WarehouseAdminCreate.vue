<template>
  <section class="page">
    <div class="shell-grid">
      <section class="panel main-panel">
        <div class="panel-head">
          <div>
            <h3>创建新物品</h3>
            <span>填写物品基础信息，系统自动计算库存总价</span>
          </div>
          <button type="button" @click="submitForm">发布入库</button>
        </div>

        <p
          v-if="statusMessage"
          class="status-message"
          :class="`status-message--${statusType}`"
        >
          {{ statusMessage }}
        </p>

        <div class="form-grid">
          <label>
            <span>物品名称</span>
            <input
              v-model="form.item_name"
              type="text"
              placeholder="例如：宠物消毒液"
            />
          </label>
          <label>
            <span>物品类型</span>
            <input
              v-model="form.item_type"
              type="text"
              placeholder="例如：耗材 / 药品"
            />
          </label>
          <label>
            <span>数量</span>
            <input
              v-model.number="form.item_number"
              type="number"
              min="1"
              placeholder="输入入库数量"
            />
          </label>
          <label>
            <span>单价</span>
            <input
              v-model.number="form.item_price"
              type="number"
              min="0.01"
              step="0.01"
              placeholder="输入单价"
            />
          </label>
          <label>
            <span>库存总价</span>
            <input :value="`¥${totalPrice.toFixed(2)}`" type="text" readonly />
          </label>
          <label>
            <span>生产日期</span>
            <input v-model="form.item_productiondate" type="date" />
          </label>
          <label>
            <span>到期日期</span>
            <input v-model="form.item_expirationdate" type="date" />
          </label>
        </div>

        <div class="tip-bar">
          入库后库存总价由系统写入，临期和低库存状态会自动进入库存预警。
        </div>
      </section>
    </div>
  </section>
</template>

<script lang="ts">
import { computed, defineComponent, reactive, ref } from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";
import { WarehouseCreatePayload } from "@/modules/warehouse-admin/api/types";

export default defineComponent({
  name: "WarehouseAdminCreate",
  setup() {
    const store = useStore(storeKey);
    const statusMessage = ref("");
    const statusType = ref<"info" | "error">("info");
    const today = new Date().toISOString().slice(0, 10);
    const nextYear = new Date();
    nextYear.setFullYear(nextYear.getFullYear() + 1);

    const form = reactive<WarehouseCreatePayload>({
      item_name: "",
      item_type: "",
      item_productiondate: today,
      item_expirationdate: nextYear.toISOString().slice(0, 10),
      item_price: 0,
      item_number: 0,
    });

    const totalPrice = computed(
      () => Number(form.item_number) * Number(form.item_price)
    );

    const submitForm = async () => {
      try {
        await store.dispatch("warehouseAdmin/createItem", { ...form });
        statusMessage.value = "物品已成功入库";
        statusType.value = "info";
      } catch (error) {
        statusMessage.value = `入库失败：${String(
          (error as Error).message || error
        )}`;
        statusType.value = "error";
      }
    };

    return {
      form,
      totalPrice,
      statusMessage,
      statusType,
      submitForm,
    };
  },
});
</script>

<style scoped>
.page {
  display: grid;
  width: 100%;
  height: 100%;
  box-sizing: border-box;
}

.shell-grid {
  display: grid;
  grid-template-columns: 1fr;
  width: 100%;
  gap: 0;
  border-radius: 12px;
  overflow: hidden;
  border: 1px solid #dfe7df;
  background: #ffffff;
  box-shadow: 0 12px 28px rgba(35, 62, 46, 0.06);
}

.main-panel {
  width: 100%;
  box-sizing: border-box;
  padding: 18px;
}

.panel-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 18px;
}

.panel-head h3 {
  margin: 0 0 4px;
  color: #1d3429;
  font-size: 20px;
}

.panel-head span {
  color: #6d7b72;
  font-size: 12px;
}

button,
input {
  border: 1px solid #dfe7df;
  border-radius: 8px;
  background: #ffffff;
  color: #1d3429;
}

button {
  padding: 11px 16px;
  background: #245849;
  color: #ffffff;
  font-weight: 700;
  cursor: pointer;
  transition: background-color 160ms ease, box-shadow 160ms ease;
}

button:hover {
  background: #18373a;
}

.form-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
}

label {
  display: grid;
  gap: 6px;
  min-width: 0;
}

label span {
  color: #1d3429;
  font-size: 12px;
  font-weight: 700;
}

input {
  min-width: 0;
  padding: 11px 12px;
  font-size: 13px;
}

input:focus-visible,
button:focus-visible {
  outline: 3px solid rgba(36, 88, 73, 0.24);
  outline-offset: 2px;
  border-color: #245849;
}

input[readonly] {
  background: #f4f7f4;
  color: #6d7b72;
}

.tip-bar {
  margin-top: 14px;
  padding: 12px 14px;
  border-radius: 8px;
  border: 1px solid #dfe7df;
  background: #f4f7f4;
  color: #1d3429;
  font-size: 12px;
}

.status-message {
  margin: 0 0 14px;
  border-radius: 8px;
  padding: 10px 12px;
  font-size: 13px;
}

.status-message--info {
  color: #247b62;
  background: rgba(215, 242, 230, 0.86);
}

.status-message--error {
  color: #b04455;
  background: rgba(255, 229, 234, 0.9);
}

@media (max-width: 960px) {
  .shell-grid {
    grid-template-columns: 1fr;
  }

  .form-grid {
    grid-template-columns: 1fr;
  }
}
</style>
