<template>
  <section v-if="order" class="detail-page">
    <header class="detail-hero">
      <div>
        <p class="eyebrow">医生端 / 诊单详情</p>
        <h2>{{ order.pet_name }} · {{ order.id }}</h2>
        <p class="subcopy">查看本次接诊的基础信息、诊断结论和用药明细。</p>
      </div>
      <button class="back-link" type="button" @click="goBack">
        {{ backLabel }}
      </button>
    </header>

    <section class="detail-grid">
      <article
        class="detail-card detail-card--deep-green"
        :class="{
          'detail-card detail-card--deep-red': order.order_status === '已取消',
          'detail-card detail-card--deep-yellow':
            order.order_status === '待付款',
        }"
      >
        <small>本次接诊</small>
        <strong>¥{{ order.order_totalprice.toFixed(2) }}</strong>
        <span>{{ order.order_status }} · {{ order.created_at }}</span>
      </article>

      <article class="detail-card">
        <small>主人</small>
        <strong>{{ order.owner_name || "未记录主人" }}</strong>
        <span>主人编号 {{ order.owner_id }}</span>
      </article>

      <article class="detail-card">
        <small>主治医生</small>
        <strong>{{ order.doctor_name || "未记录医生" }}</strong>
        <span>{{ order.pet_name }} · {{ order.pet_type || "宠物档案" }}</span>
      </article>
    </section>

    <section class="content-grid">
      <article class="panel">
        <div class="panel-head">
          <h3>粗略信息</h3>
        </div>
        <div class="fact-list">
          <div>
            <span>主人</span>
            <strong>{{ order.owner_name || "未记录" }}</strong>
          </div>
          <div>
            <span>宠物名</span>
            <strong>{{ order.pet_name }}</strong>
          </div>
          <div>
            <span>宠物信息</span>
            <strong>{{ petDescription }}</strong>
          </div>
          <div>
            <span>诊单时间</span>
            <strong>{{ order.created_at }}</strong>
          </div>
          <div>
            <span>诊单金额</span>
            <strong>¥{{ order.order_totalprice.toFixed(2) }}</strong>
          </div>
        </div>
      </article>

      <article class="panel">
        <div class="panel-head">
          <h3>诊断记录</h3>
        </div>
        <div class="text-block">
          <label>诊单类型</label>
          <p>{{ order.order_type || "诊疗" }}</p>
        </div>
        <div class="text-block">
          <label>诊断记录</label>
          <p>{{ order.order_data || "暂无诊断记录" }}</p>
        </div>
        <div class="text-block">
          <label>当前状态</label>
          <p>{{ order.order_status }}</p>
        </div>
      </article>
    </section>

    <section class="panel">
      <div class="panel-head">
        <h3>用药清单</h3>
      </div>
      <div class="medicine-list">
        <div
          v-for="item in order.orderMedicines"
          :key="item.id"
          class="medicine-card"
        >
          <strong>{{ item.medicine_name }}</strong>
          <span>{{ item.medicine_type || "药品" }}</span>
          <em>x{{ item.quantity }}</em>
          <b>¥{{ item.total_price.toFixed(2) }}</b>
        </div>
        <div v-if="order.orderMedicines.length === 0" class="medicine-empty">
          暂无用药记录。
        </div>
      </div>
    </section>

    <section v-if="medicalDocument" class="panel medical-panel">
      <div class="panel-head medical-head">
        <div>
          <h3>诊疗文书</h3>
          <span
            >{{ statusLabel }} · 第 {{ medicalDocument.revisionNo }} 版</span
          >
        </div>
        <div class="document-actions">
          <button type="button" @click="previewDocument">预览</button>
          <button
            v-if="
              canPrint &&
              medicalDocument.status !== 'draft' &&
              medicalDocument.status !== 'voided'
            "
            type="button"
            @click="openPdf"
          >
            打印
          </button>
          <button
            v-if="canWrite && medicalDocument.status === 'draft'"
            type="button"
            @click="saveDocument"
          >
            保存
          </button>
          <button
            v-if="canFinalize && medicalDocument.status === 'draft'"
            class="primary"
            type="button"
            @click="finalizeDocument"
          >
            定稿
          </button>
          <button
            v-if="
              canAmend &&
              (medicalDocument.status === 'finalized' ||
                medicalDocument.status === 'amended')
            "
            type="button"
            @click="amendDocument"
          >
            {{ editingAmendment ? "提交修订" : "修订" }}
          </button>
          <button
            v-if="
              canVoid &&
              (medicalDocument.status === 'finalized' ||
                medicalDocument.status === 'amended')
            "
            class="danger"
            type="button"
            @click="voidDocument"
          >
            作废
          </button>
        </div>
      </div>
      <div class="clinical-grid">
        <label v-for="field in clinicalFields" :key="field.key">
          <span>{{ field.label }}</span>
          <textarea
            v-model="medicalDocument[field.key]"
            :readonly="medicalDocument.status !== 'draft' && !editingAmendment"
            rows="4"
          />
        </label>
      </div>
      <label class="follow-up">
        <span>复诊时间</span>
        <input
          v-model="medicalDocument.followUpAt"
          :readonly="medicalDocument.status !== 'draft' && !editingAmendment"
          type="datetime-local"
        />
      </label>
      <p v-if="actionError" class="action-error">{{ actionError }}</p>
      <div v-if="versions.length" class="version-list">
        <div v-for="version in versions" :key="version.id">
          <span>第 {{ version.revisionNo }} 版</span>
          <strong>{{ version.changeReason }}</strong>
          <small>{{ version.createdAt }}</small>
          <button
            v-if="version.hasPdf"
            type="button"
            @click="openVersionPdf(version.revisionNo)"
          >
            查看归档
          </button>
        </div>
      </div>
    </section>

    <div
      v-if="previewHtml"
      class="preview-overlay"
      @click.self="previewHtml = ''"
    >
      <section class="preview-dialog">
        <header>
          <h3>诊疗单预览</h3>
          <button type="button" @click="previewHtml = ''">关闭</button>
        </header>
        <iframe title="诊疗单预览" sandbox="" :srcdoc="previewHtml" />
      </section>
    </div>

    <div
      v-if="correctionMode"
      class="preview-overlay"
      @click.self="closeCorrectionDialog"
    >
      <section class="correction-dialog">
        <header>
          <div>
            <small>{{
              correctionMode === "amend" ? "补充修订" : "作废文书"
            }}</small>
            <h3>
              {{
                correctionMode === "amend" ? "确认提交新版本" : "确认作废诊疗单"
              }}
            </h3>
          </div>
          <button type="button" @click="closeCorrectionDialog">关闭</button>
        </header>
        <p>
          {{
            correctionMode === "amend"
              ? "原版本将永久保留，本次内容会生成新的归档版本。"
              : "作废后患者不能再下载当前 PDF，历史记录仍保留用于审计。"
          }}
        </p>
        <label>
          <span>操作原因</span>
          <textarea
            v-model.trim="correctionReason"
            rows="4"
            maxlength="500"
            autofocus
          />
        </label>
        <footer>
          <button type="button" @click="closeCorrectionDialog">取消</button>
          <button
            class="primary"
            type="button"
            :disabled="!correctionReason"
            @click="confirmCorrection"
          >
            {{ correctionMode === "amend" ? "确认修订" : "确认作废" }}
          </button>
        </footer>
      </section>
    </div>
  </section>

  <section v-else class="empty-page">
    <h2>未找到诊单详情</h2>
    <button class="back-link" type="button" @click="goBackToWorkbench">
      返回工作台
    </button>
  </section>
</template>

<script lang="ts">
import { computed, defineComponent, onMounted, ref } from "vue";
import { useRoute, useRouter } from "vue-router";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";
import { doctorApi } from "@/modules/doctor/api/doctorApi";
import type {
  MedicalDocument,
  MedicalDocumentVersion,
} from "@/modules/doctor/api/types";

export default defineComponent({
  name: "DoctorOrderDetail",
  setup() {
    const store = useStore(storeKey);
    const route = useRoute();
    const router = useRouter();
    const basePath = computed(() => "/doctor");
    const orderId = computed(() => Number(route.params.orderId ?? 0));
    const loading = ref(false);
    const medicalDocument = ref<MedicalDocument | null>(null);
    const previewHtml = ref("");
    const actionError = ref("");
    const editingAmendment = ref(false);
    const correctionMode = ref<"amend" | "void" | null>(null);
    const correctionReason = ref("");
    const versions = ref<MedicalDocumentVersion[]>([]);
    const hasPermission = (permission: string) =>
      store.state.auth.permissions.includes(permission);
    const canWrite = computed(() => hasPermission("medical-record:write"));
    const canFinalize = computed(() =>
      hasPermission("medical-record:finalize")
    );
    const canPrint = computed(() => hasPermission("medical-record:print"));
    const canAmend = computed(() => hasPermission("medical-record:amend"));
    const canVoid = computed(() => hasPermission("medical-record:void"));
    const clinicalFields = [
      { key: "chiefComplaint", label: "主诉" },
      { key: "presentIllness", label: "现病史" },
      { key: "pastHistory", label: "既往史" },
      { key: "allergies", label: "过敏史" },
      { key: "physicalExam", label: "体格检查" },
      { key: "diagnosis", label: "诊断" },
      { key: "treatmentPlan", label: "治疗方案" },
      { key: "dischargeAdvice", label: "离院医嘱" },
    ] as const;

    /**
     * 诊单详情只缓存当前选中的一条完整记录。
     * 切换到新的订单 id 时会覆盖上一条详情缓存。
     */
    const loadOrderDetail = async () => {
      if (!orderId.value) {
        return;
      }

      loading.value = true;
      try {
        await store.dispatch("doctor/ensureOrderDetail", orderId.value);
        medicalDocument.value = await doctorApi.getMedicalDocument(
          orderId.value
        );
        versions.value = await doctorApi.getMedicalDocumentVersions(
          orderId.value
        );
      } finally {
        loading.value = false;
      }
    };

    const order = computed(() =>
      Number(store.state.doctor.currentOrderDetail?.id) === orderId.value
        ? store.state.doctor.currentOrderDetail
        : null
    );

    const petDescription = computed(() =>
      [order.value?.pet_type, order.value?.pet_age, order.value?.pet_sex]
        .filter(Boolean)
        .join(" · ")
    );

    const fromRecords = computed(() => route.query.from === "records");
    const backLabel = computed(() =>
      fromRecords.value ? "返回订单记录" : "返回用户档案"
    );

    const goBackToWorkbench = () => {
      router.push(`${basePath.value}/home`);
    };

    const goBack = () => {
      if (fromRecords.value) {
        router.push(`${basePath.value}/order-records`);
        return;
      }

      goBackToWorkbench();
    };

    const statusLabel = computed(
      () =>
        ({
          draft: "草稿",
          finalized: "已定稿",
          amended: "已修订",
          voided: "已作废",
        }[medicalDocument.value?.status ?? "draft"])
    );
    const documentPayload = () => {
      const document = medicalDocument.value;
      if (!document) throw new Error("诊疗单尚未加载");
      return {
        chiefComplaint: document.chiefComplaint,
        presentIllness: document.presentIllness,
        pastHistory: document.pastHistory,
        allergies: document.allergies,
        physicalExam: document.physicalExam,
        diagnosis: document.diagnosis,
        treatmentPlan: document.treatmentPlan,
        dischargeAdvice: document.dischargeAdvice,
        followUpAt: document.followUpAt,
        structuredData: document.structuredData,
        lockVersion: document.lockVersion,
        prescriptionItems: document.prescriptionItems,
      };
    };
    const runAction = async (action: () => Promise<void>) => {
      actionError.value = "";
      try {
        await action();
      } catch (error) {
        const status = (error as { response?: { status?: number } })?.response
          ?.status;
        if (status === 409) {
          actionError.value = "诊疗单已被其他人修改，已重新加载最新内容";
        } else {
          actionError.value =
            error instanceof Error ? error.message : "操作失败";
        }
        medicalDocument.value = await doctorApi.getMedicalDocument(
          orderId.value
        );
        versions.value = await doctorApi.getMedicalDocumentVersions(
          orderId.value
        );
      }
    };
    const saveDocument = () =>
      runAction(async () => {
        medicalDocument.value = await doctorApi.updateMedicalDocument(
          orderId.value,
          documentPayload()
        );
      });
    const previewDocument = () =>
      runAction(async () => {
        previewHtml.value = await doctorApi.previewMedicalDocument(
          orderId.value
        );
      });
    const finalizeDocument = () =>
      runAction(async () => {
        medicalDocument.value = await doctorApi.updateMedicalDocument(
          orderId.value,
          documentPayload()
        );
        await doctorApi.finalizeMedicalDocument(orderId.value);
        medicalDocument.value = await doctorApi.getMedicalDocument(
          orderId.value
        );
        versions.value = await doctorApi.getMedicalDocumentVersions(
          orderId.value
        );
      });
    const openPdf = () =>
      runAction(async () => {
        const blob = await doctorApi.getMedicalDocumentPdf(orderId.value);
        const url = URL.createObjectURL(blob);
        window.open(url, "_blank", "noopener,noreferrer");
        window.setTimeout(() => URL.revokeObjectURL(url), 60000);
      });
    const amendDocument = () =>
      (() => {
        if (!editingAmendment.value) {
          editingAmendment.value = true;
          return;
        }
        correctionReason.value = "";
        correctionMode.value = "amend";
      })();
    const voidDocument = () => {
      correctionReason.value = "";
      correctionMode.value = "void";
    };
    const closeCorrectionDialog = () => {
      correctionMode.value = null;
      correctionReason.value = "";
    };
    const confirmCorrection = () =>
      runAction(async () => {
        const mode = correctionMode.value;
        const reason = correctionReason.value;
        if (!mode || !reason || !medicalDocument.value) return;
        if (mode === "amend") {
          medicalDocument.value = await doctorApi.amendMedicalDocument(
            orderId.value,
            { ...documentPayload(), reason }
          );
          editingAmendment.value = false;
        } else {
          medicalDocument.value = await doctorApi.voidMedicalDocument(
            orderId.value,
            medicalDocument.value.lockVersion,
            reason
          );
        }
        versions.value = await doctorApi.getMedicalDocumentVersions(
          orderId.value
        );
        closeCorrectionDialog();
      });
    const openVersionPdf = (revisionNo: number) =>
      runAction(async () => {
        const blob = await doctorApi.getMedicalDocumentVersionPdf(
          orderId.value,
          revisionNo
        );
        const url = URL.createObjectURL(blob);
        window.open(url, "_blank", "noopener,noreferrer");
        window.setTimeout(() => URL.revokeObjectURL(url), 60000);
      });

    onMounted(() => {
      void loadOrderDetail();
    });

    return {
      loading,
      order,
      petDescription,
      backLabel,
      goBack,
      goBackToWorkbench,
      medicalDocument,
      previewHtml,
      actionError,
      editingAmendment,
      clinicalFields,
      statusLabel,
      saveDocument,
      previewDocument,
      finalizeDocument,
      openPdf,
      amendDocument,
      voidDocument,
      versions,
      openVersionPdf,
      correctionMode,
      correctionReason,
      closeCorrectionDialog,
      confirmCorrection,
      canWrite,
      canFinalize,
      canPrint,
      canAmend,
      canVoid,
    };
  },
});
</script>

<style scoped>
.detail-page {
  display: grid;
  grid-template-rows: auto auto minmax(0, 1fr);
  gap: 20px;
  min-height: var(--doctor-page-card-height, 860px);
}

.detail-hero,
.detail-card,
.panel,
.empty-page {
  border: 1px solid rgba(151, 184, 174, 0.24);
  border-radius: 28px;
  background: linear-gradient(180deg, rgba(255, 252, 246, 0.96), #f5fbf8);
  box-shadow: 0 20px 40px rgba(43, 78, 75, 0.07);
}

.medical-panel {
  display: grid;
  gap: 18px;
}
.medical-head,
.document-actions,
.preview-dialog header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}
.medical-head span,
.clinical-grid span,
.follow-up span {
  color: #6f8782;
  font-size: 13px;
}
.document-actions {
  flex-wrap: wrap;
  justify-content: flex-end;
}
.document-actions button,
.preview-dialog button {
  border: 1px solid #b9d0c9;
  background: #fff;
  color: #28555b;
  padding: 9px 14px;
  border-radius: 6px;
  cursor: pointer;
}
.document-actions .primary {
  background: #1f6159;
  border-color: #1f6159;
  color: #fff;
}
.document-actions .danger {
  color: #a93f3f;
  border-color: #d9aaaa;
}
.clinical-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 14px;
}
.clinical-grid label,
.follow-up {
  display: grid;
  gap: 7px;
}
.clinical-grid textarea,
.follow-up input {
  width: 100%;
  box-sizing: border-box;
  border: 1px solid #dbe8e3;
  border-radius: 6px;
  padding: 11px;
  background: #fff;
  color: #264b4d;
  font: inherit;
  resize: vertical;
}
.action-error {
  color: #a93f3f;
  margin: 0;
}
.version-list {
  display: grid;
  gap: 8px;
  padding-top: 14px;
  border-top: 1px solid #dbe8e3;
}
.version-list > div {
  display: grid;
  grid-template-columns: 90px minmax(0, 1fr) 170px auto;
  align-items: center;
  gap: 12px;
  padding: 10px 12px;
  background: #f7faf8;
  border-radius: 5px;
}
.version-list small {
  color: #72857f;
}
.version-list button {
  border: 1px solid #b9d0c9;
  border-radius: 5px;
  background: #fff;
  color: #28555b;
  padding: 7px 10px;
  cursor: pointer;
}
.preview-overlay {
  position: fixed;
  inset: 0;
  z-index: 1000;
  display: grid;
  place-items: center;
  padding: 24px;
  background: rgba(20, 44, 44, 0.48);
}
.preview-dialog {
  width: min(920px, 96vw);
  height: min(880px, 92vh);
  padding: 18px;
  background: #f4f8f6;
  border-radius: 8px;
  display: grid;
  grid-template-rows: auto minmax(0, 1fr);
  gap: 12px;
}
.preview-dialog h3 {
  margin: 0;
  color: #183d42;
}
.preview-dialog iframe {
  width: 100%;
  height: 100%;
  border: 1px solid #dbe8e3;
  background: #fff;
}
.correction-dialog {
  width: min(520px, 94vw);
  padding: 20px;
  background: #fff;
  border-radius: 8px;
  display: grid;
  gap: 16px;
}
.correction-dialog header,
.correction-dialog footer {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}
.correction-dialog h3,
.correction-dialog p {
  margin: 0;
}
.correction-dialog small,
.correction-dialog p,
.correction-dialog label span {
  color: #6f8782;
}
.correction-dialog label {
  display: grid;
  gap: 7px;
}
.correction-dialog textarea {
  border: 1px solid #dbe8e3;
  border-radius: 6px;
  padding: 11px;
  font: inherit;
  resize: vertical;
}
.correction-dialog button {
  border: 1px solid #b9d0c9;
  border-radius: 5px;
  padding: 9px 14px;
  background: #fff;
  color: #28555b;
  cursor: pointer;
}
.correction-dialog .primary {
  border-color: #1f6159;
  background: #1f6159;
  color: #fff;
}
.correction-dialog .primary:disabled {
  cursor: not-allowed;
  opacity: 0.5;
}

.detail-hero {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 14px;
  padding: 28px;
}

.eyebrow {
  margin: 0 0 8px;
  color: #7d958e;
  font-size: 12px;
  letter-spacing: 0.18em;
  text-transform: uppercase;
}

.detail-hero h2,
.detail-hero p,
.panel-head h3,
.empty-page h2 {
  margin: 0;
}

.detail-hero h2 {
  color: #173d42;
  font-size: 34px;
}

.subcopy {
  margin-top: 10px;
  color: #6d8681;
}

.back-link {
  border: 1px solid rgba(128, 168, 156, 0.34);
  border-radius: 999px;
  padding: 10px 16px;
  background: linear-gradient(135deg, #f8fffc, #e8f3ee);
  color: #28555b;
  font-weight: 700;
  cursor: pointer;
}

.detail-grid,
.content-grid {
  display: grid;
  gap: 18px;
}

.detail-grid {
  grid-template-columns: repeat(3, 1fr);
}

.content-grid {
  grid-template-columns: 0.95fr 1.05fr;
  min-height: 0;
  overflow: visible;
}

.detail-card,
.panel {
  padding: 22px;
}

.fact-list span,
.text-block label,
.medicine-card span {
  color: #6f8782;
}

.detail-card strong {
  display: block;
  margin: 10px 0 8px;
  color: #183d42;
  font-size: 24px;
}

.detail-card--deep-green {
  background: linear-gradient(135deg, #16ef7f, #83c683);
}
.detail-card--deep-yellow {
  background: linear-gradient(135deg, #e9b311, #f9e0a1);
}
.detail-card--deep-red {
  background: linear-gradient(135deg, #f81212, #bd7373);
}

.detail-card--deep small,
.detail-card--deep strong,
.detail-card--deep span {
  color: #f5fffb;
}

.panel-head {
  margin-bottom: 16px;
}

.panel-head h3 {
  color: #183d42;
}

.fact-list {
  display: grid;
  gap: 14px;
}

.fact-list div,
.text-block,
.medicine-card {
  display: grid;
  gap: 6px;
  padding: 16px;
  border-radius: 20px;
  background: rgba(255, 255, 255, 0.72);
  border: 1px solid rgba(223, 236, 232, 0.95);
}

.fact-list strong,
.medicine-card strong {
  color: #1a4044;
}

.text-block p {
  margin: 0;
  color: #466762;
  line-height: 1.8;
}

.medicine-list {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 12px;
}

.medicine-card em,
.medicine-card b {
  font-style: normal;
}

.medicine-card b {
  color: #1f6159;
}

.empty-page {
  min-height: 360px;
  display: grid;
  gap: 16px;
  place-items: center;
}

@media (max-width: 980px) {
  .detail-hero,
  .detail-grid,
  .content-grid,
  .medicine-list {
    grid-template-columns: 1fr;
  }

  .detail-hero {
    flex-direction: column;
  }
  .clinical-grid {
    grid-template-columns: 1fr;
  }
  .version-list > div {
    grid-template-columns: 1fr 1fr;
  }
}
</style>
