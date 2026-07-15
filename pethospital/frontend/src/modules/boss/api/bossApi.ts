import http from "@/api/http";
import { unwrapList } from "@/api/response";
import { unwrapPagedList } from "@/shared/utils/pagedList";
import {
  AssignmentApproval,
  CompensationProposal,
  WorkflowPage,
} from "@/shared/types/employmentWorkflow";
import {
  normalizeCompensationProposal,
  positiveNumberOrNull,
  workflowPayloadOf,
  workflowRecordOf,
} from "@/shared/utils/employmentWorkflow";
import { BossStockDistribution, BossStockItem } from "./types";

const normalizeBossStockItem = (
  item: Partial<BossStockItem>,
  index: number
): BossStockItem => ({
  id: Number(item.id ?? index + 1),
  type:
    item.type === "total" || item.type === "remaining" || item.type === "holder"
      ? item.type
      : "holder",
  holder: String(item.holder ?? ""),
  share: Number(item.share ?? 0),
  percentage: Number(item.percentage ?? 0),
});

export const bossApi = {
  async getStock(): Promise<BossStockDistribution> {
    const { data } = await http.get("/api/bosses/stocks");
    const payload = data?.data ?? data;
    const decisionStocks = unwrapList<BossStockItem>(
      payload?.decisionStocks ?? payload?.decision_stocks
    ).map(normalizeBossStockItem);
    const dividendStocks = unwrapList<BossStockItem>(
      payload?.dividendStocks ?? payload?.dividend_stocks
    ).map(normalizeBossStockItem);

    return {
      decisionStocks,
      dividendStocks,
    };
  },

  async listEmploymentApprovals(
    params: {
      status?: string;
      page?: number;
      pageSize?: number;
    } = {}
  ): Promise<WorkflowPage<AssignmentApproval>> {
    const fallback = {
      page: params.page ?? 1,
      pageSize: params.pageSize ?? 50,
    };
    const { data } = await http.get(
      "/api/bosses/employment-assignment-approvals",
      {
        params: { status: params.status ?? "pending", ...fallback },
      }
    );
    const page = unwrapPagedList<unknown>(data, fallback);
    return {
      ...page,
      items: page.items.map((value) => {
        const item = workflowRecordOf(value);
        return {
          id: Number(item.id ?? 0),
          employmentId: Number(item.employment_id ?? item.employmentId ?? 0),
          userId: Number(item.user_id ?? item.userId ?? 0),
          userName: String(item.user_name ?? item.userName ?? ""),
          branchId: Number(item.branch_id ?? item.branchId ?? 0),
          branchName: String(item.branch_name ?? item.branchName ?? ""),
          departmentId: Number(item.department_id ?? item.departmentId ?? 0),
          departmentName: String(
            item.department_name ?? item.departmentName ?? ""
          ),
          fromPositionId: positiveNumberOrNull(
            item.from_position_id ?? item.fromPositionId
          ),
          fromPositionName: String(
            item.from_position_name ?? item.fromPositionName ?? ""
          ),
          toPositionId: positiveNumberOrNull(
            item.to_position_id ?? item.toPositionId
          ),
          toPositionName: String(
            item.to_position_name ?? item.toPositionName ?? ""
          ),
          action: String(
            item.action ?? "onboard"
          ) as AssignmentApproval["action"],
          status: String(item.status ?? "pending"),
          effectiveFrom: String(
            item.effective_from ?? item.effectiveFrom ?? ""
          ),
          reason: String(item.reason ?? ""),
          requestedBy: positiveNumberOrNull(
            item.requested_by ?? item.requestedBy
          ),
          reviewedBy: positiveNumberOrNull(item.reviewed_by ?? item.reviewedBy),
          expectedEmploymentRowVersion: Number(
            item.expected_employment_row_version ??
              item.expectedEmploymentRowVersion ??
              1
          ),
          rowVersion: Number(item.row_version ?? item.rowVersion ?? 1),
          createdAt: String(item.created_at ?? item.createdAt ?? ""),
          employmentStatus: String(
            item.employment_status ?? item.employmentStatus ?? ""
          ) as AssignmentApproval["employmentStatus"],
          currentPositionId: Number(
            item.current_position_id ?? item.currentPositionId ?? 0
          ),
        };
      }),
    };
  },

  async decideEmploymentApproval(
    approvalId: number,
    payload: {
      action: "approve" | "reject";
      reason: string;
      expectedRowVersion: number;
    }
  ): Promise<void> {
    await http.post(
      `/api/bosses/employment-assignment-approvals/${approvalId}/decision`,
      payload
    );
  },

  async listCompensationApprovals(
    params: {
      status?: string;
      page?: number;
      pageSize?: number;
    } = {}
  ): Promise<WorkflowPage<CompensationProposal>> {
    const fallback = {
      page: params.page ?? 1,
      pageSize: params.pageSize ?? 50,
    };
    const { data } = await http.get("/api/bosses/compensation-approvals", {
      params: { status: params.status ?? "submitted", ...fallback },
    });
    const page = unwrapPagedList<unknown>(data, fallback);
    return { ...page, items: page.items.map(normalizeCompensationProposal) };
  },

  async decideCompensationApproval(
    proposalId: number,
    payload: {
      action: "approve" | "return";
      reason: string;
      expectedRowVersion: number;
    }
  ): Promise<CompensationProposal> {
    const { data } = await http.post(
      `/api/bosses/compensation-approvals/${proposalId}/decision`,
      payload
    );
    return normalizeCompensationProposal(workflowPayloadOf(data));
  },
};
