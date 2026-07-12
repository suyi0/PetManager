<template>
  <section class="documents-page">
    <header>
      <div>
        <p>医疗档案</p>
        <h1>诊疗文书</h1>
      </div>
      <span>仅展示已定稿记录</span>
    </header>
    <p v-if="error" class="error">{{ error }}</p>
    <div v-if="loading" class="empty">正在加载...</div>
    <div v-else-if="documents.length === 0" class="empty">
      暂无可查看的诊疗文书
    </div>
    <div v-else class="document-list">
      <article v-for="item in documents" :key="item.id">
        <button class="document-id" type="button" @click="showDetail(item.id)">
          <small>{{ item.documentNo }}</small
          ><strong>{{ item.pet?.name || "宠物" }}</strong>
        </button>
        <div>
          <small>诊断</small><strong>{{ item.diagnosis || "未填写" }}</strong>
        </div>
        <div>
          <small>主治医生</small
          ><strong>{{ item.doctor?.name || "未记录" }}</strong>
        </div>
        <div>
          <small>定稿时间</small><strong>{{ item.finalizedAt }}</strong>
        </div>
        <span class="status" :class="item.status">{{
          statusLabel(item.status)
        }}</span>
        <button
          v-if="item.status !== 'voided'"
          type="button"
          @click="download(item)"
        >
          查看 PDF
        </button>
        <p v-else class="void-reason">{{ item.voidReason }}</p>
      </article>
    </div>
    <div v-if="selected" class="detail-overlay" @click.self="selected = null">
      <section class="detail-sheet">
        <header>
          <div>
            <small>{{ selected.documentNo }}</small>
            <h2>诊疗文书详情</h2>
          </div>
          <button type="button" @click="selected = null">关闭</button>
        </header>
        <dl>
          <div v-for="field in visitFields" :key="field.key">
            <dt>{{ field.label }}</dt>
            <dd>{{ textValue(selected.snapshot.visit?.[field.key]) }}</dd>
          </div>
        </dl>
        <h3>处方明细</h3>
        <table>
          <thead>
            <tr>
              <th>药品</th>
              <th>用法</th>
              <th>数量</th>
            </tr>
          </thead>
          <tbody>
            <tr
              v-for="(medicine, index) in selected.snapshot.prescription
                ?.items || []"
              :key="index"
            >
              <td>{{ medicine.medicineName }}</td>
              <td>
                {{ medicine.dosage }} {{ medicine.frequency }}
                {{ medicine.route }}
              </td>
              <td>{{ medicine.quantity }} {{ medicine.unit }}</td>
            </tr>
          </tbody>
        </table>
        <p v-if="selected.status === 'voided'" class="void-reason">
          作废原因：{{ selected.voidReason }}
        </p>
      </section>
    </div>
  </section>
</template>

<script setup lang="ts">
import { onMounted, ref } from "vue";
import { medicalDocumentApi } from "@/modules/user/api/userApi";
import type {
  UserMedicalDocumentDetail,
  UserMedicalDocumentSummary,
} from "@/modules/user/api/types";

const documents = ref<UserMedicalDocumentSummary[]>([]);
const loading = ref(true);
const error = ref("");
const selected = ref<UserMedicalDocumentDetail | null>(null);
const visitFields = [
  { key: "chiefComplaint", label: "主诉" },
  { key: "presentIllness", label: "现病史" },
  { key: "pastHistory", label: "既往史" },
  { key: "allergies", label: "过敏史" },
  { key: "physicalExam", label: "体格检查" },
  { key: "diagnosis", label: "诊断" },
  { key: "treatmentPlan", label: "治疗方案" },
  { key: "dischargeAdvice", label: "离院医嘱" },
];
const textValue = (value: unknown) =>
  typeof value === "string" && value ? value : "未记录";
const showDetail = async (id: number) => {
  try {
    selected.value = await medicalDocumentApi.get(id);
  } catch {
    error.value = "诊疗文书详情加载失败";
  }
};
const statusLabel = (status: UserMedicalDocumentSummary["status"]) =>
  ({ finalized: "已定稿", amended: "已修订", voided: "已作废" }[status]);
const download = async (item: UserMedicalDocumentSummary) => {
  try {
    const blob = await medicalDocumentApi.download(item.id);
    const url = URL.createObjectURL(blob);
    window.open(url, "_blank", "noopener,noreferrer");
    window.setTimeout(() => URL.revokeObjectURL(url), 60000);
  } catch {
    error.value = "PDF 暂时无法打开，请稍后重试";
  }
};
onMounted(async () => {
  try {
    documents.value = await medicalDocumentApi.list();
  } catch {
    error.value = "诊疗文书加载失败";
  } finally {
    loading.value = false;
  }
});
</script>

<style scoped>
.documents-page {
  display: grid;
  gap: 20px;
  padding: 28px;
  color: #1f3a36;
}
header {
  display: flex;
  justify-content: space-between;
  align-items: end;
  border-bottom: 1px solid #e2ebe7;
  padding-bottom: 18px;
}
header p,
header h1 {
  margin: 0;
}
header p,
header span,
small {
  color: #72857f;
}
header h1 {
  margin-top: 6px;
  font-size: 30px;
}
.document-list {
  display: grid;
  gap: 10px;
}
article {
  display: grid;
  grid-template-columns: 1.2fr 1.5fr 1fr 1fr auto auto;
  align-items: center;
  gap: 16px;
  padding: 17px 18px;
  background: #fff;
  border: 1px solid #e2ebe7;
  border-radius: 7px;
}
article div,
.document-id {
  display: grid;
  gap: 5px;
  min-width: 0;
}
article strong {
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}
.document-id {
  padding: 0;
  border: 0;
  background: transparent;
  color: inherit;
  text-align: left;
}
.status {
  padding: 5px 9px;
  border-radius: 4px;
  background: #e7f5f1;
  color: #1f7a6c;
  font-size: 13px;
}
.status.voided {
  background: #f8eaea;
  color: #a13d3d;
}
button {
  border: 0;
  border-radius: 5px;
  padding: 9px 12px;
  background: #2f9e8f;
  color: #fff;
  cursor: pointer;
}
.empty {
  padding: 60px;
  text-align: center;
  background: #fff;
  border: 1px solid #e2ebe7;
}
.error,
.void-reason {
  color: #a13d3d;
  margin: 0;
  font-size: 13px;
}
.detail-overlay {
  position: fixed;
  inset: 0;
  z-index: 1000;
  display: grid;
  place-items: center;
  padding: 24px;
  background: rgba(20, 44, 44, 0.48);
}
.detail-sheet {
  width: min(860px, 96vw);
  max-height: 90vh;
  overflow: auto;
  padding: 22px;
  background: #fff;
  border-radius: 8px;
}
.detail-sheet header {
  align-items: center;
}
.detail-sheet h2,
.detail-sheet h3 {
  margin: 4px 0 12px;
}
.detail-sheet dl {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 10px;
}
.detail-sheet dl div {
  padding: 12px;
  background: #f5f9f7;
}
.detail-sheet dt {
  color: #72857f;
  font-size: 13px;
}
.detail-sheet dd {
  margin: 5px 0 0;
  white-space: pre-wrap;
}
.detail-sheet table {
  width: 100%;
  border-collapse: collapse;
}
.detail-sheet th,
.detail-sheet td {
  padding: 9px;
  border: 1px solid #dce8e4;
  text-align: left;
}
@media (max-width: 980px) {
  article {
    grid-template-columns: 1fr 1fr;
  }
  header {
    align-items: flex-start;
    flex-direction: column;
    gap: 8px;
  }
}
</style>
