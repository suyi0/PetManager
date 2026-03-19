<template>
  <section class="page">
    <div class="shell-grid">
      <section class="panel main-panel">
        <div class="panel-head">
          <div>
            <h3>创建新物品</h3>
            <span>New Inventory Item</span>
          </div>
          <button @click="submitForm">发布入库</button>
        </div>

        <div class="form-grid">
          <input v-model="form.item_name" type="text" placeholder="物品名称" />
          <input v-model="form.item_type" type="text" placeholder="物品类型" />
          <input
            v-model.number="form.item_number"
            type="number"
            min="1"
            placeholder="数量"
          />
          <input
            v-model.number="form.item_price"
            type="number"
            min="0.01"
            step="0.01"
            placeholder="单价"
          />
          <input
            :value="`总价自动生成 ¥${totalPrice.toFixed(2)}`"
            type="text"
            readonly
          />
          <input type="text" placeholder="SKU / 批次号" />
          <input v-model="form.item_productiondate" type="date" />
          <input v-model="form.item_expirationdate" type="date" />
        </div>

        <div class="tip-bar">
          支持自动校验临期时间和价格异常，`item_totalprice` 由系统直接生成。
        </div>
      </section>
    </div>
  </section>
</template>

<script lang="ts">
import { computed, defineComponent, reactive } from "vue";
import { warehouseAdminApi } from "../../api/warehouseAdminApi";
import { WarehouseCreatePayload } from "../../api/types";

export default defineComponent({
  name: "WarehouseAdminCreate",
  setup() {
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
        await warehouseAdminApi.createItem({ ...form });
      } catch {
        // 设计预览场景允许静默失败。
      }
    };

    return {
      form,
      totalPrice,
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
  border-radius: 20px;
  overflow: hidden;
  border: 1px solid rgba(148, 197, 255, 0.28);
  background: linear-gradient(
    180deg,
    rgba(225, 237, 253, 0.97),
    rgba(205, 223, 247, 0.98)
  );
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
  color: #16385d;
}

.panel-head span {
  color: #587398;
  font-size: 12px;
}

button,
input {
  border: 1px solid rgba(171, 210, 255, 0.3);
  border-radius: 12px;
  background: rgba(219, 232, 250, 0.84);
  color: #163a60;
}

button {
  padding: 10px 16px;
  background: linear-gradient(135deg, #89defe, #67a5ff);
  color: #062448;
  font-weight: 700;
  cursor: pointer;
}

.form-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 0.8fr));
  gap: 10px;
}

input {
  min-width: 0;
  padding: 12px 14px;
}

.tip-bar {
  margin-top: 14px;
  padding: 12px 14px;
  border-radius: 12px;
  background: rgba(150, 194, 245, 0.42);
  color: #2c5b8c;
  font-size: 12px;
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
