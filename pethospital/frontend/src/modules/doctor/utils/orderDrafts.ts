import { SelectedMedicineItem } from "../api/types";

/**
 * 医生端开诊单草稿的完整本地数据。
 * 用于保存患者基础信息、已选药品以及最近更新时间。
 */
export type DoctorOrderDraft = {
  ownerId?: number;
  petId?: number;
  patientForm: {
    petName: string;
    sex: string;
    breed: string;
    age: string;
    ownerName: string;
    symptom: string;
  };
  selected: SelectedMedicineItem[];
  updatedAt: number;
};

/**
 * 草稿列表页使用的轻量摘要信息。
 * 由 localStorage 中的完整草稿派生出来，避免列表页直接读取大对象细节。
 */
export type DoctorOrderDraftSummary = {
  storageKey: string;
  queueId: string;
  ownerId?: number;
  petId?: number;
  visitCode: string;
  petName: string;
  ownerName: string;
  symptom: string;
  source: "队列接诊" | "临时新建";
  medicineCount: number;
  estimatedTotal: number;
  updatedAt: number;
};

const ORDER_DRAFT_PREFIX = "doctor:create-order:draft:";

/**
 * 根据接诊队列 id 生成草稿本地存储 key。
 * 没有关联队列时使用 default，表示临时新建的诊单草稿。
 */
export const buildDoctorOrderDraftKey = (queueId?: string) => {
  const normalizedQueueId = String(queueId || "").trim();
  return normalizedQueueId
    ? `${ORDER_DRAFT_PREFIX}${normalizedQueueId}`
    : `${ORDER_DRAFT_PREFIX}default`;
};

/**
 * 从浏览器本地存储读取指定 key 的诊单草稿。
 * 如果没有数据或数据无法解析，会返回 null；解析失败时会清理损坏的本地草稿。
 */
export const readDoctorOrderDraft = (storageKey: string) => {
  const rawDraft = localStorage.getItem(storageKey);

  if (!rawDraft) {
    return null;
  }

  try {
    return JSON.parse(rawDraft) as DoctorOrderDraft;
  } catch {
    localStorage.removeItem(storageKey);
    return null;
  }
};

/**
 * 保存医生端开诊单草稿到浏览器本地存储。
 */
export const saveDoctorOrderDraft = (
  storageKey: string,
  draft: DoctorOrderDraft
) => {
  localStorage.setItem(storageKey, JSON.stringify(draft));
};

/**
 * 删除指定 key 对应的本地诊单草稿。
 */
export const removeDoctorOrderDraft = (storageKey: string) => {
  localStorage.removeItem(storageKey);
};

/**
 * 扫描本地存储中的所有医生端诊单草稿，并转换为列表摘要。
 * 返回结果按更新时间倒序排列，最近编辑的草稿会排在最前面。
 */
export const listDoctorOrderDrafts = () => {
  const draftSummaries: DoctorOrderDraftSummary[] = [];

  for (let index = 0; index < localStorage.length; index += 1) {
    const storageKey = localStorage.key(index);

    if (!storageKey || !storageKey.startsWith(ORDER_DRAFT_PREFIX)) {
      continue;
    }

    const draft = readDoctorOrderDraft(storageKey);

    if (!draft) {
      continue;
    }

    const queueId = storageKey.slice(ORDER_DRAFT_PREFIX.length) || "default";
    const visitCode = queueId === "default" ? "临时草稿" : `Q-${queueId}`;
    const medicineCount = draft.selected.reduce(
      (sum, item) => sum + Math.max(1, item.days),
      0
    );
    const estimatedTotal = draft.selected.reduce(
      (sum, item) => sum + item.unitPrice * item.days,
      0
    );

    draftSummaries.push({
      storageKey,
      queueId,
      ownerId: draft.ownerId,
      petId: draft.petId,
      visitCode,
      petName: draft.patientForm.petName || "未命名宠物",
      ownerName: draft.patientForm.ownerName || "未填写主人",
      symptom: draft.patientForm.symptom || "暂无主诉",
      source: queueId === "default" ? "临时新建" : "队列接诊",
      medicineCount,
      estimatedTotal,
      updatedAt: Number.isFinite(draft.updatedAt) ? draft.updatedAt : 0,
    });
  }

  return draftSummaries.sort((a, b) => b.updatedAt - a.updatedAt);
};
