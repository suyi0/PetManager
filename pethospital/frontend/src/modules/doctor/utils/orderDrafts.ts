import { SelectedMedicineItem } from "../api/types";

/**
 * 医生端开诊单草稿的完整内存数据。
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
 * 由 Vuex 中的完整草稿派生出来，避免列表页直接读取大对象细节。
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
  remainingMs: number;
};

const ORDER_DRAFT_PREFIX = "doctor:create-order:draft:";
const ORDER_DRAFT_TTL_MS = 1000 * 60 * 60 * 24;

type BuildDoctorOrderDraftKeyOptions = {
  queueId?: string;
  doctorScope?: string | number | null;
  ownerId?: string | number | null;
  petId?: string | number | null;
};

const normalizeKeyPart = (value: string | number | null | undefined) => {
  const normalizedValue = String(value ?? "").trim();
  return normalizedValue
    ? normalizedValue.replace(/[^a-zA-Z0-9_-]/g, "_")
    : "unknown";
};

export const isDoctorOrderDraftExpired = (draft: DoctorOrderDraft) => {
  const updatedAt = Number(draft.updatedAt || 0);
  return !updatedAt || Date.now() - updatedAt > ORDER_DRAFT_TTL_MS;
};

const getDraftRemainingMs = (draft: DoctorOrderDraft) => {
  const updatedAt = Number(draft.updatedAt || 0);
  if (!updatedAt) {
    return 0;
  }

  return Math.max(0, ORDER_DRAFT_TTL_MS - (Date.now() - updatedAt));
};

/**
 * 生成诊单草稿内存缓存 key。
 * 新版本按医生 + 主人 + 宠物 + 队列维度隔离，避免不同医生或不同宠物复用同一草稿。
 */
export const buildDoctorOrderDraftKey = (
  options?: string | BuildDoctorOrderDraftKeyOptions
) => {
  if (typeof options === "string") {
    const normalizedQueueId = String(options || "").trim();
    return normalizedQueueId
      ? `${ORDER_DRAFT_PREFIX}${normalizedQueueId}`
      : `${ORDER_DRAFT_PREFIX}default`;
  }

  const normalizedQueueId = String(options?.queueId || "").trim() || "default";
  const doctorScope = normalizeKeyPart(options?.doctorScope);
  const ownerId = normalizeKeyPart(options?.ownerId);
  const petId = normalizeKeyPart(options?.petId);

  return `${ORDER_DRAFT_PREFIX}${[
    `doctor-${doctorScope}`,
    `owner-${ownerId}`,
    `pet-${petId}`,
    `queue-${normalizeKeyPart(normalizedQueueId)}`,
  ].join(":")}`;
};

export const createDoctorOrderDraftSummary = (
  storageKey: string,
  draft: DoctorOrderDraft
) => {
  const rawKey = storageKey.slice(ORDER_DRAFT_PREFIX.length) || "default";
  const queueIdMatch = rawKey.match(/(?:^|:)queue-([^:]+)/);
  const queueId = queueIdMatch?.[1] || rawKey || "default";
  const normalizedQueueId = queueId === "default" ? "default" : queueId;
  const visitCode =
    normalizedQueueId === "default" ? "临时草稿" : `Q-${normalizedQueueId}`;
  const medicineCount = draft.selected.reduce(
    (sum, item) => sum + Math.max(1, item.days),
    0
  );
  const estimatedTotal = draft.selected.reduce(
    (sum, item) => sum + item.unitPrice * item.days,
    0
  );

  return {
    storageKey,
    queueId: normalizedQueueId,
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
    remainingMs: getDraftRemainingMs(draft),
  } as DoctorOrderDraftSummary;
};

/**
 * 将 Vuex 中的完整草稿 map 转为草稿列表摘要。
 */
export const listDoctorOrderDrafts = (
  drafts: Record<string, DoctorOrderDraft>
) =>
  Object.entries(drafts)
    .filter(([, draft]) => !isDoctorOrderDraftExpired(draft))
    .map(([storageKey, draft]) =>
      createDoctorOrderDraftSummary(storageKey, draft)
    )
    .sort((a, b) => b.updatedAt - a.updatedAt);
