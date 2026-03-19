<template>
  <section class="page">
    <section class="workspace">
      <section class="top-panel">
        <div class="search-strip">
          <label class="search-box">
            <span class="search-icon" aria-hidden="true">
              <svg viewBox="0 0 24 24" fill="none">
                <circle
                  cx="11"
                  cy="11"
                  r="6.5"
                  stroke="currentColor"
                  stroke-width="2"
                />
                <path
                  d="M16 16L21 21"
                  stroke="currentColor"
                  stroke-width="2"
                  stroke-linecap="round"
                />
              </svg>
            </span>
            <input
              v-model.trim="searchQuery"
              type="text"
              placeholder="输入药品名称 / 编码 / 类型"
            />
          </label>
          <div class="strip-actions">
            <button class="primary" type="button">搜索</button>
            <button class="ghost" type="button" @click="resetSearch">
              重置筛选
            </button>
          </div>
        </div>

        <div class="medicine-board">
          <div class="board-head">
            <span>选择</span>
            <span>药品名</span>
            <span>类型</span>
            <span>价格</span>
            <span>库存</span>
          </div>
          <div class="board-scroll scroll-hidden">
            <div
              v-for="item in filteredMedicines"
              :key="item.id"
              class="board-row"
            >
              <span>
                <button
                  type="button"
                  class="selector-button"
                  @click="toggleMedicine(item)"
                >
                  <i class="selector" :class="{ active: item.selected }"></i>
                </button>
              </span>
              <span>{{ item.name }}</span>
              <span>{{ item.type }}</span>
              <span>¥{{ item.price.toFixed(2) }}</span>
              <span>{{ item.stock }}</span>
            </div>
            <div v-if="filteredMedicines.length === 0" class="board-empty">
              没有匹配的药品，请调整搜索关键词。
            </div>
          </div>
        </div>
      </section>

      <section class="sheet">
        <div class="sheet-title">诊单</div>

        <div class="info-grid">
          <div class="info-row">
            <label>姓名：</label>
            <input type="text" value="可乐" />
          </div>
          <div class="info-row">
            <label>性别：</label>
            <input type="text" value="母" />
          </div>
          <div class="info-row">
            <label>品种：</label>
            <input type="text" value="英短" />
          </div>
          <div class="info-row">
            <label>年龄：</label>
            <input type="text" value="4 岁 2 月" />
          </div>
        </div>

        <div class="sheet-section">
          <h4>药品列表：</h4>
          <div class="prescription-table">
            <div class="prescription-head">
              <span></span>
              <span>药品名</span>
              <span>类型</span>
              <span>价格</span>
              <span>个数</span>
              <span>总价</span>
              <span>库存</span>
            </div>
            <div class="prescription-scroll scroll-hidden">
              <div
                v-for="item in selected"
                :key="item.id"
                class="prescription-row"
              >
                <button
                  type="button"
                  class="remove-cell"
                  @click="removeSelected(item.id)"
                >
                  ⊖
                </button>
                <span>{{ item.name }}</span>
                <span>{{ medicineTypeMap[item.name] }}</span>
                <span>¥{{ item.unitPrice.toFixed(2) }}</span>
                <label class="quantity-cell">
                  <input
                    :value="item.days"
                    type="number"
                    min="1"
                    :max="medicineStockMap[item.name]"
                    @input="updateQuantity(item.id, $event)"
                  />
                </label>
                <span>¥{{ (item.unitPrice * item.days).toFixed(2) }}</span>
                <span>{{ medicineStockMap[item.name] }}</span>
              </div>
            </div>
          </div>
        </div>

        <div class="sheet-section notes">
          <h4>病历描述：</h4>
          <div class="notes-box"></div>
        </div>

        <div class="print-row">
          <button type="button" class="print-button">打印诊单</button>
        </div>

        <div class="sheet-footer">
          <div class="visit-meta">林安 医师 · 内科门诊 · 编号 ZD-0310-08</div>
          <div class="total">
            <span>总费用</span>
            <strong>¥{{ total.toFixed(2) }}</strong>
          </div>
        </div>
      </section>
    </section>
  </section>
</template>

<script lang="ts">
import { computed, defineComponent, ref } from "vue";
import { medicineSearchItems } from "../../api/doctorMock";
import { MedicineSearchItem, SelectedMedicineItem } from "../../api/types";

export default defineComponent({
  name: "DoctorCreateOrder",
  setup() {
    const searchQuery = ref("");
    const medicines = ref<MedicineSearchItem[]>(
      medicineSearchItems.map((item) => ({ ...item, selected: false }))
    );
    const selected = ref<SelectedMedicineItem[]>([]);

    const total = computed(() =>
      selected.value.reduce((sum, item) => sum + item.unitPrice * item.days, 0)
    );

    const medicineTypeMap: Record<string, string> = {
      胃复安片: "止吐类",
      蒙脱石散: "肠胃保护",
      益生菌粉: "驱虫",
    };

    const medicineStockMap: Record<string, number> = {
      胃复安片: 28,
      蒙脱石散: 44,
      益生菌粉: 20,
    };

    const filteredMedicines = computed(() => {
      const keyword = searchQuery.value.toLowerCase();
      if (!keyword) return medicines.value;

      return medicines.value.filter((item) => {
        const searchableText = [
          String(item.id),
          item.name,
          item.type,
          item.spec,
        ]
          .join(" ")
          .toLowerCase();
        return searchableText.includes(keyword);
      });
    });

    const toggleMedicine = (item: MedicineSearchItem) => {
      const existingIndex = selected.value.findIndex(
        (selectedItem) => selectedItem.id === item.id
      );

      if (existingIndex >= 0) {
        selected.value.splice(existingIndex, 1);
        item.selected = false;
        return;
      }

      selected.value.push({
        id: item.id,
        name: item.name,
        dosage: "1次 / 日",
        days: 1,
        unitPrice: item.price,
        subtotal: item.price,
      });
      item.selected = true;
    };

    const removeSelected = (id: number) => {
      const targetIndex = selected.value.findIndex((item) => item.id === id);
      if (targetIndex >= 0) {
        selected.value.splice(targetIndex, 1);
      }

      const targetMedicine = medicines.value.find((item) => item.id === id);
      if (targetMedicine) {
        targetMedicine.selected = false;
      }
    };

    const updateQuantity = (id: number, event: Event) => {
      const target = event.target as HTMLInputElement;
      const nextValue = Number(target.value);
      const selectedItem = selected.value.find((item) => item.id === id);

      if (!selectedItem) {
        return;
      }

      const stockLimit =
        medicineStockMap[selectedItem.name] ?? Number.MAX_SAFE_INTEGER;
      const safeValue = Math.min(
        stockLimit,
        Math.max(1, Number.isFinite(nextValue) ? Math.floor(nextValue) : 1)
      );

      selectedItem.days = safeValue;
      selectedItem.subtotal = selectedItem.unitPrice * safeValue;
      target.value = String(safeValue);
    };

    const resetSearch = () => {
      searchQuery.value = "";
    };

    return {
      searchQuery,
      medicines,
      filteredMedicines,
      selected,
      total,
      medicineTypeMap,
      medicineStockMap,
      toggleMedicine,
      removeSelected,
      updateQuantity,
      resetSearch,
    };
  },
});
</script>

<style scoped>
.page {
  display: grid;
  min-height: 100%;
}

.workspace {
  display: grid;
  grid-template-rows: auto minmax(0, 1fr);
  gap: 0;
  overflow: hidden;
  max-height: min(100vh - 24px, 900px);
  border-radius: 30px;
  border: 1px solid rgba(157, 188, 178, 0.28);
  background: linear-gradient(180deg, rgba(255, 254, 250, 0.95), #f6fbf8);
  box-shadow: 0 22px 46px rgba(47, 77, 73, 0.08);
}

.top-panel {
  display: grid;
  grid-template-rows: auto auto;
  min-height: 0;
  background: linear-gradient(180deg, rgba(244, 251, 247, 0.92), #f8fcfb);
}

.search-strip {
  display: grid;
  grid-template-columns: minmax(0, 1fr) auto;
  gap: 14px;
  align-items: center;
  padding: 12px 14px;
  background: linear-gradient(135deg, #f3fbf5, #eef7f2);
  border-bottom: 1px solid rgba(172, 196, 188, 0.28);
}

.search-box {
  display: flex;
  align-items: center;
  gap: 10px;
  min-height: 36px;
  padding: 8px 12px;
  border-radius: 12px;
  border: 1px solid rgba(150, 181, 172, 0.34);
  background: rgba(255, 255, 255, 0.96);
  color: #829792;
  font-size: 13px;
  box-sizing: border-box;
  cursor: text;
}

.search-icon {
  display: inline-flex;
  width: 18px;
  height: 18px;
  color: #79908a;
  flex: 0 0 auto;
}

.search-icon svg {
  width: 100%;
  height: 100%;
}

.search-box input {
  width: 100%;
  min-width: 0;
  border: 0;
  background: transparent;
  color: #21464b;
  font-size: 13px;
  outline: none;
}

.search-box input::placeholder {
  color: #829792;
}

.strip-actions {
  display: flex;
  gap: 10px;
}

button {
  border-radius: 12px;
  padding: 8px 14px;
  font-weight: 700;
  cursor: pointer;
}

.primary {
  border: 1px solid rgba(144, 175, 166, 0.24);
  background: linear-gradient(135deg, #29565a, #7d5348);
  color: #fffdfb;
  box-shadow: 0 12px 24px rgba(49, 82, 87, 0.12);
}

.ghost {
  border: 1px solid rgba(175, 198, 190, 0.34);
  background: #f3f9f5;
  color: #214f4b;
}

.medicine-board {
  display: grid;
  grid-template-rows: auto auto;
  min-height: 0;
  border-bottom: 1px solid rgba(172, 196, 188, 0.28);
}

.board-scroll {
  min-height: 0;
  max-height: 98px;
  overflow-y: auto;
  background-color: rgba(244, 234, 158, 0.8);
}

.board-head,
.board-row {
  display: grid;
  grid-template-columns: 84px 1.6fr 1fr 0.8fr 0.8fr;
  align-items: center;
  gap: 12px;
  padding: 8px 14px;
}

.board-head {
  background: linear-gradient(180deg, #f6fcf8, #edf7f2);
  color: #67807b;
  font-size: 12px;
  font-weight: 700;
}

.board-row {
  border-top: 1px solid rgba(227, 238, 234, 0.95);
  background: rgba(255, 255, 255, 0.74);
  color: #21464b;
  font-size: 12px;
}

.board-empty {
  padding: 16px 14px 20px;
  color: #7b918c;
  font-size: 12px;
  text-align: center;
}

.selector-button {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 28px;
  height: 28px;
  padding: 0;
  border: 0;
  background: transparent;
  box-shadow: none;
}

.selector {
  width: 18px;
  height: 18px;
  border-radius: 6px;
  border: 1px solid #86a8a1;
  background: #fff;
}

.selector.active {
  background: #214f4b;
  box-shadow: inset 0 0 0 4px #eaf5f2;
}

.sheet {
  margin-top: 10px;
  min-height: 0;
  overflow: hidden;
  padding: 0;
  background: linear-gradient(180deg, #fffefd, #fbfcfb);
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.94);
}

.top-panel,
.sheet {
  align-content: start;
}

.sheet-title {
  padding: 8px 18px 5px;
  border-bottom: 1px solid rgba(176, 199, 191, 0.34);
  text-align: center;
  font-size: 20px;
  font-weight: 800;
  color: #15393d;
  letter-spacing: 0.06em;
}

.info-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 0 32px;
  padding: 10px 18px 4px;
}

.info-row {
  display: flex;
  gap: 6px;
  min-height: 34px;
  align-items: center;
  border-bottom: 1px solid rgba(223, 233, 229, 0.92);
  color: #1d3e42;
}

.info-row label {
  font-weight: 700;
  flex: 0 0 auto;
}

.info-row input {
  width: 100%;
  min-width: 0;
  border: 0;
  background: transparent;
  color: #1d3e42;
  font-size: 14px;
  outline: none;
}

.sheet-section {
  padding: 8px 18px 0;
}

.sheet-section h4 {
  margin: 0 0 8px;
  font-size: 16px;
  color: #193b3f;
}

.prescription-table {
  display: grid;
  border-top: 1px solid rgba(176, 199, 191, 0.4);
}

.prescription-head {
  display: grid;
  grid-template-columns: 66px repeat(6, minmax(0, 1fr));
  align-items: center;
  text-align: center;
  gap: 12px;
  min-height: 32px;
  border-bottom: 1px solid rgba(223, 233, 229, 0.92);
  color: #738983;
  font-size: 12px;
  font-weight: 700;
}

.prescription-scroll {
  min-height: 180px;
  max-height: 180px;
  overflow-y: auto;
}

.prescription-row {
  display: grid;
  grid-template-columns: 66px repeat(6, minmax(0, 1fr));
  align-items: center;
  text-align: center;
  gap: 12px;
  min-height: 28px;
  border-bottom: 1px solid rgba(223, 233, 229, 0.92);
  color: #23464a;
}

.prescription-row {
  font-size: 12px;
}

.quantity-cell {
  display: flex;
  align-items: center;
  justify-content: center;
}

.quantity-cell input {
  width: 56px;
  min-width: 0;
  min-height: 28px;
  padding: 0 8px;
  border: 1px solid rgba(178, 197, 191, 0.8);
  border-radius: 8px;
  background: #fff;
  color: #21464b;
  font-size: 12px;
  text-align: center;
  outline: none;
}

.remove-cell {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 24px;
  height: 24px;
  padding: 0;
  border: 0;
  background: transparent;
  color: #ff3131;
  font-size: 18px;
  line-height: 1;
  text-align: center;
  box-shadow: none;
}

.notes {
  margin-top: 10px;
}

.notes-box {
  min-height: 120px;
  border-top: 1px solid rgba(176, 199, 191, 0.4);
  border-bottom: 1px solid rgba(176, 199, 191, 0.4);
  background: linear-gradient(
    180deg,
    rgba(255, 255, 255, 0.42),
    rgba(247, 250, 249, 0.72)
  );
}

.print-row {
  display: flex;
  justify-content: center;
  padding: 14px 18px 6px;
}

.print-button {
  min-width: 160px;
  border: 1px solid rgba(144, 175, 166, 0.24);
  background: linear-gradient(135deg, #29565a, #7d5348);
  color: #fffdfb;
  box-shadow: 0 12px 24px rgba(49, 82, 87, 0.12);
}

.sheet-footer {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
  padding: 14px 18px 18px;
}

.visit-meta {
  color: #67807b;
  font-size: 13px;
}

.total {
  display: flex;
  align-items: baseline;
  gap: 10px;
}

.total span {
  color: #78908b;
  font-size: 12px;
}

.total strong {
  color: #173b38;
  font-size: 22px;
}

.scroll-hidden {
  scrollbar-width: none;
  -ms-overflow-style: none;
}

.scroll-hidden::-webkit-scrollbar {
  width: 0;
  height: 0;
}

@media (max-width: 960px) {
  .workspace {
    max-height: none;
  }

  .search-strip {
    grid-template-columns: 1fr;
  }

  .sheet-footer {
    flex-direction: column;
    align-items: flex-start;
  }

  .strip-actions {
    width: 100%;
  }

  .strip-actions button {
    flex: 1;
  }

  .board-head,
  .board-row {
    min-width: 720px;
  }

  .prescription-row {
    min-width: 760px;
  }

  .info-grid {
    grid-template-columns: 1fr;
  }
}
</style>
