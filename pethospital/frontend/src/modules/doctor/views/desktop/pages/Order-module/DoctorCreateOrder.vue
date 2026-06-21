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
              placeholder="输入药品名称或类型后按 Enter"
              @keyup.enter="searchMedicines"
            />
          </label>
          <div class="strip-actions">
            <button class="ghost" type="button" @click="resetSearch">
              重置筛选
            </button>
          </div>
        </div>

        <div class="medicine-board">
          <div class="board-head">
            <span>添加</span>
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
                  @click="addMedicine(item)"
                >
                  <i class="selector selector--add">+</i>
                </button>
              </span>
              <span>{{ item.name }}</span>
              <span>{{ item.type }}</span>
              <span>¥{{ item.price.toFixed(2) }}</span>
              <span>{{ item.stock }}</span>
            </div>
            <div v-if="filteredMedicines.length === 0" class="board-empty">
              {{ medicineSearchMessage }}
            </div>
          </div>
        </div>
      </section>

      <section class="sheet">
        <div class="sheet-title">诊单</div>

        <div class="info-grid">
          <div class="info-row">
            <label>姓名：</label>
            <input v-model="patientForm.petName" type="text" />
          </div>
          <div class="info-row">
            <label>性别：</label>
            <input v-model="patientForm.sex" type="text" />
          </div>
          <div class="info-row">
            <label>品种：</label>
            <input v-model="patientForm.breed" type="text" />
          </div>
          <div class="info-row">
            <label>年龄：</label>
            <input v-model="patientForm.age" type="text" />
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
                <span>{{ item.type || "药品" }}</span>
                <span>¥{{ item.unitPrice.toFixed(2) }}</span>
                <label class="quantity-cell">
                  <input
                    :value="item.days"
                    type="number"
                    min="1"
                    :max="getSelectedMedicineStock(item)"
                    @input="updateQuantity(item.id, $event)"
                  />
                </label>
                <span>¥{{ (item.unitPrice * item.days).toFixed(2) }}</span>
                <span>{{ formatSelectedMedicineStock(item) }}</span>
              </div>
            </div>
          </div>
        </div>

        <div class="sheet-section notes">
          <h4>病历描述：</h4>
          <div class="notes-box">
            <p v-if="patientForm.ownerName">
              主人：{{ patientForm.ownerName }}
            </p>
            <p v-if="patientForm.symptom">主诉：{{ patientForm.symptom }}</p>
          </div>
        </div>

        <div class="print-row">
          <p
            v-if="formMessage"
            class="form-message"
            :class="`form-message--${formMessage.type}`"
          >
            {{ formMessage.text }}
          </p>
          <button type="button" class="ghost-button" @click="clearDraft">
            清空草稿
          </button>
          <button
            type="button"
            class="print-button"
            :disabled="submitting"
            @click="submitOrder"
          >
            {{ submitButtonText }}
          </button>
        </div>

        <div class="sheet-footer">
          <div class="visit-meta">
            林安 医师 · 内科门诊 · 编号 {{ visitCode }}
          </div>
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
import {
  computed,
  defineComponent,
  onMounted,
  reactive,
  ref,
  watch,
} from "vue";
import { useRoute, useRouter } from "vue-router";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";
import { doctorApi } from "@/modules/doctor/api/doctorApi";
import {
  MedicineSearchItem,
  SelectedMedicineItem,
} from "@/modules/doctor/api/types";
import {
  DoctorOrderDraft,
  buildDoctorOrderDraftKey,
} from "@/modules/doctor/utils/orderDrafts";

export default defineComponent({
  name: "DoctorCreateOrder",
  setup() {
    const route = useRoute();
    const router = useRouter();
    const store = useStore(storeKey);
    const searchQuery = ref("");
    const searchLoading = ref(false);
    const searchErrorMessage = ref("");
    const submitting = ref(false);
    const formMessage = ref<{
      type: "success" | "error";
      text: string;
    } | null>(null);
    const medicines = ref<MedicineSearchItem[]>([]);
    const selected = ref<SelectedMedicineItem[]>([]);

    /**
     * 标志是否暂停保存草稿
     */
    const pauseDraftPersist = ref(false);
    const ownerId = ref(Number(route.query.ownerId || route.query.userId || 0));
    const petId = ref(Number(route.query.petId || 0));

    const initialPatientForm = {
      petName: String(route.query.petName || ""),
      sex: String(route.query.sex || ""),
      breed: String(route.query.breed || ""),
      age: String(route.query.age || ""),
      ownerName: String(route.query.ownerName || ""),
      symptom: String(route.query.symptom || ""),
    };
    /**
     * 患者信息
     */
    const patientForm = reactive({
      ...initialPatientForm,
    });

    /**
     * 订单号
     */
    const visitCode = computed(() => {
      const queueId = String(route.params.queueId || "").trim();
      return queueId ? `Q-${queueId}` : "ZD-0310-08";
    });
    const doctorDraftScope = computed(
      () =>
        store.state.currentUser.userPhone ||
        store.state.currentUser.userEmail ||
        store.state.currentUser.userName ||
        store.getters["auth/formattedUserName"] ||
        "doctor"
    );
    const draftStorageKey = computed(() => {
      const openedDraftKey = String(route.query.draftKey || "").trim();
      if (openedDraftKey) {
        return openedDraftKey;
      }

      const queueId = String(route.params.queueId || "").trim();
      return buildDoctorOrderDraftKey({
        queueId,
        doctorScope: doctorDraftScope.value,
        ownerId: ownerId.value,
        petId: petId.value,
      });
    });

    const total = computed(() =>
      selected.value.reduce((sum, item) => sum + item.unitPrice * item.days, 0)
    );

    const filteredMedicines = computed(() => medicines.value);
    const medicineSearchMessage = computed(() => {
      if (searchLoading.value) {
        return "正在搜索药品...";
      }

      return searchErrorMessage.value || "没有匹配的药品，请调整搜索关键词。";
    });
    const submitButtonText = computed(() =>
      submitting.value ? "提交中" : "提交诊单"
    );

    const searchMedicines = async () => {
      searchLoading.value = true;
      searchErrorMessage.value = "";

      try {
        medicines.value = await doctorApi.searchMedicines(searchQuery.value);
      } catch {
        medicines.value = [];
        searchErrorMessage.value = "药品搜索失败，请稍后重试。";
      } finally {
        searchLoading.value = false;
      }
    };

    const addMedicine = (item: MedicineSearchItem) => {
      const existingItem = selected.value.find(
        (selectedItem) => selectedItem.id === item.id
      );

      if (existingItem) {
        const stockLimit = item.stock || Number.MAX_SAFE_INTEGER;
        const nextCount = Math.min(stockLimit, existingItem.days + 1);
        existingItem.days = nextCount;
        existingItem.subtotal = existingItem.unitPrice * nextCount;
        existingItem.type = item.type;
        existingItem.stock = item.stock;
        return;
      }

      selected.value.push({
        id: item.id,
        name: item.name,
        type: item.type,
        stock: item.stock,
        dosage: "1次 / 日",
        days: 1,
        unitPrice: item.price,
        subtotal: item.price,
      });
    };

    const removeSelected = (id: number) => {
      const targetIndex = selected.value.findIndex((item) => item.id === id);
      if (targetIndex >= 0) {
        selected.value.splice(targetIndex, 1);
      }
    };

    const updateQuantity = (id: number, event: Event) => {
      const target = event.target as HTMLInputElement;
      const nextValue = Number(target.value);
      const selectedItem = selected.value.find((item) => item.id === id);

      if (!selectedItem) {
        return;
      }

      const stockLimit = getSelectedMedicineStock(selectedItem);
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
      void searchMedicines();
    };

    const getSelectedMedicineStock = (item: SelectedMedicineItem) => {
      return (
        item.stock ||
        medicines.value.find((medicine) => medicine.id === item.id)?.stock ||
        Number.MAX_SAFE_INTEGER
      );
    };

    const formatSelectedMedicineStock = (item: SelectedMedicineItem) => {
      const stock = getSelectedMedicineStock(item);
      return stock === Number.MAX_SAFE_INTEGER ? "未同步" : String(stock);
    };

    /**
     * 恢复草稿
     */
    const restoreDraft = async () => {
      const openedDraftKey = String(route.query.draftKey || "").trim();
      const draft = (await store.dispatch(
        "doctor/readOrderDraft",
        draftStorageKey.value
      )) as DoctorOrderDraft | null;

      if (!draft) {
        if (openedDraftKey) {
          pauseDraftPersist.value = true;
          formMessage.value = {
            type: "error",
            text: "该诊单草稿不存在或已超过 24 小时，正在返回草稿箱。",
          };
          window.setTimeout(() => {
            void router.push("/doctor/drafts");
          }, 900);
        }
        return;
      }

      Object.assign(patientForm, draft.patientForm);
      selected.value = Array.isArray(draft.selected) ? draft.selected : [];
      ownerId.value = Number(route.query.ownerId || draft.ownerId || 0);
      petId.value = Number(route.query.petId || draft.petId || 0);
    };

    /**
     * 持久化草稿
     */
    const persistDraft = () => {
      if (pauseDraftPersist.value) {
        return;
      }

      const draft: DoctorOrderDraft = {
        ownerId: ownerId.value,
        petId: petId.value,
        patientForm: {
          petName: patientForm.petName,
          sex: patientForm.sex,
          breed: patientForm.breed,
          age: patientForm.age,
          ownerName: patientForm.ownerName,
          symptom: patientForm.symptom,
        },
        selected: selected.value.map((item) => ({ ...item })),
        updatedAt: Date.now(),
      };

      void store.dispatch("doctor/saveOrderDraft", {
        draftKey: draftStorageKey.value,
        draft,
      });
    };

    /**
     * 清空指定草稿并重置表单
     */
    const clearDraft = () => {
      pauseDraftPersist.value = true;
      void store.dispatch("doctor/removeOrderDraft", draftStorageKey.value);
      Object.assign(patientForm, initialPatientForm);
      selected.value = [];
      searchQuery.value = "";
      formMessage.value = {
        type: "success",
        text: "草稿已清空。",
      };
      requestAnimationFrame(() => {
        pauseDraftPersist.value = false;
      });
    };

    /**
     * 提交订单
     */
    const submitOrder = async () => {
      if (!ownerId.value || !petId.value) {
        formMessage.value = {
          type: "error",
          text: "缺少主人或宠物信息，请从待接诊队列重新进入创建诊单页。",
        };
        return;
      }

      formMessage.value = null;
      submitting.value = true;
      try {
        const queueId = Number(route.params.queueId || 0);
        const normalizedRecord = await store.dispatch(
          "doctor/createOrderRecord",
          {
            order: {
              queueId:
                Number.isFinite(queueId) && queueId > 0 ? queueId : undefined,
              ownerId: ownerId.value,
              petId: petId.value,
              orderType: "诊疗",
              orderData: patientForm.symptom,
              orderTotalPrice: total.value,
              orderMedicines: selected.value.map((item) => ({
                medicineId: item.id,
                medicineName: item.name,
                quantity: Math.max(1, item.days),
                price: item.unitPrice,
                totalPrice: item.subtotal,
              })),
            },
            fallback: {
              pet_name: patientForm.petName || "未命名宠物",
              order_type: "诊疗",
              order_data: patientForm.symptom,
              order_totalprice: total.value,
              order_status: "待付款",
            },
          }
        );

        pauseDraftPersist.value = true;
        void store.dispatch("doctor/removeOrderDraft", draftStorageKey.value);
        formMessage.value = {
          type: "success",
          text: `诊单 ${normalizedRecord.id} 已提交，正在进入诊单记录。`,
        };
        window.setTimeout(() => {
          void router.push("/doctor/order-records");
        }, 500);
      } catch {
        formMessage.value = {
          type: "error",
          text: "订单创建失败，请检查宠物、主人和药品信息后重试。",
        };
      } finally {
        submitting.value = false;
      }
    };

    onMounted(() => {
      void searchMedicines();
      void restoreDraft();
    });

    watch(
      [patientForm, selected],
      () => {
        persistDraft();
      },
      { deep: true }
    );

    return {
      searchQuery,
      searchLoading,
      submitting,
      formMessage,
      medicines,
      filteredMedicines,
      medicineSearchMessage,
      submitButtonText,
      selected,
      patientForm,
      visitCode,
      total,
      addMedicine,
      searchMedicines,
      getSelectedMedicineStock,
      formatSelectedMedicineStock,
      removeSelected,
      updateQuantity,
      resetSearch,
      clearDraft,
      submitOrder,
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
  height: var(--doctor-page-card-height, 860px);
  overflow: hidden;
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

button:disabled {
  cursor: not-allowed;
  opacity: 0.56;
  box-shadow: none;
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
  max-height: none;
  overflow: visible;
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
  display: inline-flex;
  align-items: center;
  justify-content: center;
  color: #214f4b;
  font-size: 14px;
  font-style: normal;
  line-height: 1;
}

.selector--add {
  background: #eef7f4;
  border-color: #7ea9a0;
  font-weight: 800;
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
  background: transparent;
  border: 3px solid rgba(0, 0, 0, 0.92);
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
  max-height: none;
  overflow: visible;
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
  flex-wrap: wrap;
  justify-content: center;
  gap: 12px;
  padding: 14px 18px 6px;
}

.form-message {
  flex: 1 0 100%;
  margin: 0;
  border-radius: 10px;
  padding: 10px 12px;
  font-size: 13px;
  font-weight: 700;
  text-align: center;
}

.form-message--success {
  border: 1px solid rgba(36, 123, 98, 0.26);
  background: rgba(36, 123, 98, 0.08);
  color: #247b62;
}

.form-message--error {
  border: 1px solid rgba(176, 68, 85, 0.26);
  background: rgba(176, 68, 85, 0.08);
  color: #b04455;
}

.print-button {
  min-width: 160px;
  border: 1px solid rgba(144, 175, 166, 0.24);
  background: linear-gradient(135deg, #29565a, #7d5348);
  color: #fffdfb;
  box-shadow: 0 12px 24px rgba(49, 82, 87, 0.12);
}

.ghost-button {
  min-width: 160px;
  border: 1px solid rgba(175, 198, 190, 0.34);
  background: #f3f9f5;
  color: #214f4b;
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
