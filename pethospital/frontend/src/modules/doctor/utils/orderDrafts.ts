import { SelectedMedicineItem } from "../api/types";

export type DoctorOrderDraft = {
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

export type DoctorOrderDraftSummary = {
  storageKey: string;
  queueId: string;
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

export const buildDoctorOrderDraftKey = (queueId?: string) => {
  const normalizedQueueId = String(queueId || "").trim();
  return normalizedQueueId
    ? `${ORDER_DRAFT_PREFIX}${normalizedQueueId}`
    : `${ORDER_DRAFT_PREFIX}default`;
};

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

export const saveDoctorOrderDraft = (
  storageKey: string,
  draft: DoctorOrderDraft
) => {
  localStorage.setItem(storageKey, JSON.stringify(draft));
};

export const removeDoctorOrderDraft = (storageKey: string) => {
  localStorage.removeItem(storageKey);
};

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
