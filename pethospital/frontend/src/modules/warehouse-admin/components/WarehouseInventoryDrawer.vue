<!-- Provides the inventory create/edit drawer, stock movement controls, and delete confirmation. -->
<template>
  <div class="drawer-backdrop" @click.self="$emit('close')">
    <aside class="inventory-drawer">
      <div class="drawer-head">
        <div>
          <small>{{ editingItem ? "EDIT ITEM" : "CREATE ITEM" }}</small>
          <h3>{{ editingItem ? editingItem.item_name : "新增入库" }}</h3>
        </div>
        <button type="button" class="icon-button" @click="$emit('close')">
          ×
        </button>
      </div>

      <div class="form-grid">
        <label>
          <span>物品名称</span>
          <input
            :value="editForm.item_name"
            type="text"
            @input="updateEditFormField('item_name', $event)"
          />
        </label>
        <label>
          <span>物品类型</span>
          <input
            :value="editForm.item_type"
            type="text"
            @input="updateEditFormField('item_type', $event)"
          />
        </label>
        <label>
          <span>数量</span>
          <input
            :value="editForm.item_number"
            type="number"
            min="1"
            @input="updateEditFormNumberField('item_number', $event)"
          />
        </label>
        <label>
          <span>单价</span>
          <input
            :value="editForm.item_price"
            type="number"
            min="0.01"
            step="0.01"
            @input="updateEditFormNumberField('item_price', $event)"
          />
        </label>
        <label>
          <span>总价</span>
          <input type="text" :value="editSummaryTotal" readonly />
        </label>
        <label>
          <span>库存记录</span>
          <input
            type="text"
            :value="editingItem ? '系统已建档' : '新增后生成'"
            readonly
          />
        </label>
        <label>
          <span>生产日期</span>
          <input
            :value="editForm.item_productiondate"
            type="date"
            @input="updateEditFormField('item_productiondate', $event)"
          />
        </label>
        <label>
          <span>到期日期</span>
          <input
            :value="editForm.item_expirationdate"
            type="date"
            @input="updateEditFormField('item_expirationdate', $event)"
          />
        </label>
      </div>

      <div class="meta-strip">
        <span>{{
          editingItem ? "当前库存 " + editingItem.item_number : "新增库存记录"
        }}</span>
        <span>{{
          editingItem ? "修改后 " + editForm.item_number : "待入库"
        }}</span>
        <span>总价自动更新</span>
      </div>

      <div class="movement-box">
        <input
          :value="movementQuantity"
          type="number"
          min="1"
          placeholder="入库 / 出库数量"
          @input="updateMovementQuantity"
        />
        <button
          class="ghost"
          :disabled="!editingItem || Number(movementQuantity) <= 0"
          @click="$emit('stock-in')"
        >
          入库
        </button>
        <button
          class="ghost"
          :disabled="!editingItem || Number(movementQuantity) <= 0"
          @click="$emit('stock-out')"
        >
          出库
        </button>
      </div>

      <div class="panel-actions">
        <button class="ghost" @click="$emit('close')">取消</button>
        <button @click="$emit('save-edit')">
          {{ editingItem ? "保存修改" : "新增入库" }}
        </button>
      </div>

      <section class="delete-panel">
        <div class="section-title">
          <div>
            <h3>删除确认</h3>
            <span>危险操作</span>
          </div>
        </div>

        <template v-if="deletingItem">
          <p class="delete-title">即将删除 {{ deletingItem.item_name }}</p>
          <small
            >ID {{ deletingItem.id }} ·
            删除后将从库存列表和统计面板中移除</small
          >
          <input
            :value="deleteConfirmText"
            type="text"
            :placeholder="`输入 ${deletingItem.item_name} 确认删除`"
            @input="updateDeleteConfirmText"
          />
          <button
            class="danger-btn"
            :disabled="deleteConfirmText !== deletingItem.item_name"
            @click="$emit('confirm-delete')"
          >
            确认删除
          </button>
        </template>
        <template v-else>
          <p class="delete-title">新增模式不可删除</p>
          <small>保存为库存记录后，才能执行删除操作。</small>
        </template>
      </section>
    </aside>
  </div>
</template>

<script lang="ts">
import { defineComponent, PropType } from "vue";
import {
  WarehouseCreatePayload,
  WarehouseItem,
} from "@/modules/warehouse-admin/api/types";

const looseToNumber = (value: string) => {
  const parsed = parseFloat(value);
  return Number.isNaN(parsed) ? value : parsed;
};

export default defineComponent({
  name: "WarehouseInventoryDrawer",
  props: {
    editingItem: {
      type: Object as PropType<WarehouseItem | null>,
      default: null,
    },
    deletingItem: {
      type: Object as PropType<WarehouseItem | null>,
      default: null,
    },
    editForm: {
      type: Object as PropType<WarehouseCreatePayload>,
      required: true,
    },
    editSummaryTotal: {
      type: String,
      required: true,
    },
    movementQuantity: {
      type: [Number, String] as PropType<number | string>,
      required: true,
    },
    deleteConfirmText: {
      type: String,
      required: true,
    },
  },
  emits: [
    "update-edit-form-field",
    "update:movementQuantity",
    "update:deleteConfirmText",
    "close",
    "save-edit",
    "stock-in",
    "stock-out",
    "confirm-delete",
  ],
  methods: {
    updateEditFormField(field: keyof WarehouseCreatePayload, event: Event) {
      const target = event.target as HTMLInputElement;
      this.$emit("update-edit-form-field", field, target.value);
    },
    updateEditFormNumberField(
      field: keyof WarehouseCreatePayload,
      event: Event
    ) {
      const target = event.target as HTMLInputElement;
      this.$emit("update-edit-form-field", field, looseToNumber(target.value));
    },
    updateMovementQuantity(event: Event) {
      const target = event.target as HTMLInputElement;
      this.$emit("update:movementQuantity", looseToNumber(target.value));
    },
    updateDeleteConfirmText(event: Event) {
      const target = event.target as HTMLInputElement;
      this.$emit("update:deleteConfirmText", target.value.trim());
    },
  },
});
</script>

<style scoped>
.drawer-backdrop {
  position: fixed;
  inset: 0;
  z-index: 50;
  display: flex;
  justify-content: flex-end;
  background: rgba(24, 35, 30, 0.28);
}

.inventory-drawer {
  width: min(520px, 100%);
  height: 100%;
  box-sizing: border-box;
  overflow-y: auto;
  padding: 20px;
  background: #ffffff;
  box-shadow: -18px 0 44px rgba(24, 42, 32, 0.18);
}

.drawer-head {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 12px;
  margin-bottom: 18px;
}

.drawer-head small {
  color: #64748b;
  font-size: 11px;
  letter-spacing: 0.12em;
}

.drawer-head h3 {
  margin: 4px 0 0;
  color: #0f172a;
  font-size: 20px;
}

.icon-button {
  display: grid;
  place-items: center;
  width: 34px;
  height: 34px;
  border: 1px solid #e7e9ee;
  border-radius: 8px;
  background: #ffffff;
  color: #0f172a;
  font-size: 22px;
  line-height: 1;
  cursor: pointer;
}

.form-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 10px;
}

.form-grid label {
  display: grid;
  gap: 6px;
  min-width: 0;
}

.form-grid label span {
  color: #64748b;
  font-size: 12px;
}

.inventory-drawer input,
.delete-panel input {
  min-width: 0;
  width: 100%;
  box-sizing: border-box;
  border: 1px solid #e7e9ee;
  border-radius: 8px;
  padding: 11px 12px;
  background: #ffffff;
  color: #0f172a;
  outline: none;
}

.inventory-drawer input::placeholder,
.delete-panel input::placeholder {
  color: #94a3b8;
}

.meta-strip {
  display: flex;
  gap: 8px;
  flex-wrap: wrap;
  margin: 14px 0;
}

.meta-strip span {
  padding: 8px 12px;
  border-radius: 8px;
  background: #f1f2f5;
  color: #64748b;
  font-size: 12px;
}

.movement-box {
  display: grid;
  grid-template-columns: minmax(0, 1fr) auto auto;
  gap: 8px;
  margin: 10px 0 14px;
}

.movement-box button {
  border: 1px solid #e7e9ee;
  border-radius: 8px;
  padding: 0 14px;
  background: #fafbff;
  color: #4f46e5;
  font-weight: 800;
  cursor: pointer;
}

.movement-box button:disabled {
  color: #9dafb1;
  cursor: not-allowed;
  opacity: 0.68;
}

.panel-actions {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 10px;
}

.panel-actions button,
.danger-btn {
  width: 100%;
  border: 1px solid #e7e9ee;
  border-radius: 8px;
  padding: 10px 12px;
  background: #ffffff;
  color: #4f46e5;
  font-weight: 700;
  cursor: pointer;
}

.panel-actions .ghost {
  background: #f1f2f5;
}

.panel-actions button:last-child {
  border-color: #4f46e5;
  background: #4f46e5;
  color: #ffffff;
  font-weight: 700;
}

.delete-panel {
  margin-top: 16px;
  border-radius: 12px;
  border: 1px solid #e7e9ee;
  padding: 14px;
  background: #fef2f2;
  box-shadow: 0 12px 28px rgba(16, 24, 40, 0.06);
}

.section-title {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 14px;
  gap: 12px;
}

.section-title h3 {
  margin: 0 0 4px;
  color: #0f172a;
  font-size: 16px;
}

.section-title span,
.delete-panel small {
  color: #64748b;
  font-size: 12px;
}

.delete-title {
  margin: 0;
  font-size: 16px;
  color: #0f172a;
}

.delete-panel small {
  display: block;
  margin: 6px 0 12px;
}

.danger-btn {
  margin-top: 12px;
  color: #dc2626;
  border-color: rgba(220, 38, 38, 0.24);
  background: linear-gradient(
    180deg,
    rgba(254, 242, 242, 0.86),
    rgba(254, 242, 242, 0.92)
  );
}

.danger-btn:disabled {
  opacity: 0.62;
  cursor: not-allowed;
}

.danger-btn.muted {
  color: #b7969e;
}

@media (max-width: 1080px) {
  .form-grid {
    grid-template-columns: 1fr;
  }
}

@media (max-width: 720px) {
  .delete-panel {
    border-radius: 12px;
    padding-left: 12px;
    padding-right: 12px;
  }

  .section-title {
    flex-direction: column;
    align-items: flex-start;
    gap: 6px;
  }

  .panel-actions {
    grid-template-columns: 1fr;
    width: 100%;
  }
}

@media (max-width: 640px) {
  .movement-box {
    grid-template-columns: 1fr;
  }

  .inventory-drawer {
    padding: 16px;
  }
}
</style>
