<template>
  <section v-if="user" class="page">
    <button class="link-back" @click="goBack">
      <svg
        viewBox="0 0 24 24"
        fill="none"
        stroke="currentColor"
        stroke-width="2"
      >
        <path d="M15 18l-6-6 6-6" />
      </svg>
      返回用户列表
    </button>

    <!-- 档案头卡 -->
    <div class="profile-card">
      <div class="profile-id">
        <div class="avatar" :class="`avatar--${roleKind}`">{{ initial }}</div>
        <div class="profile-id__text">
          <div class="profile-id__name">
            {{ user.name || "未命名用户" }}
            <span
              v-if="isDoctor"
              class="status-chip"
              :class="online ? 'status-chip--online' : 'status-chip--offline'"
            >
              <span class="dot"></span>{{ online ? "在线" : "离线" }}
            </span>
          </div>
          <div class="profile-id__meta">
            <span class="pill" :class="`pill--${roleKind}`">{{
              roleName
            }}</span>
            <span class="muted">用户编号 #{{ user.id }}</span>
          </div>
        </div>
      </div>

      <div class="balance">
        <span class="balance__label">账户余额</span>
        <strong class="balance__value">¥{{ accountBalance }}</strong>
      </div>
    </div>

    <!-- 内容两栏 -->
    <div class="detail-grid">
      <section class="card">
        <div class="card__head">
          <h3>基础资料</h3>
        </div>
        <dl class="info-list">
          <div class="info-row">
            <dt>用户 ID</dt>
            <dd>{{ user.id }}</dd>
          </div>
          <div class="info-row">
            <dt>角色</dt>
            <dd>{{ roleName }}</dd>
          </div>
          <div class="info-row">
            <dt>手机号</dt>
            <dd>{{ user.phone || "未填写" }}</dd>
          </div>
          <div class="info-row">
            <dt>邮箱</dt>
            <dd>{{ user.email || "未填写" }}</dd>
          </div>
          <div class="info-row">
            <dt>生日</dt>
            <dd>{{ user.birthday || "未填写" }}</dd>
          </div>
        </dl>
      </section>

      <div class="side-col">
        <section v-if="isDoctor" class="card">
          <div class="card__head">
            <h3>接诊状态</h3>
          </div>
          <p class="card__desc">切换医生在线 / 离线，决定其是否可接诊。</p>

          <div
            class="status-now"
            :class="online ? 'status-now--online' : 'status-now--offline'"
          >
            <span class="dot"></span>
            <strong>{{
              online ? "在线 · 当前可接诊" : "离线 · 当前未接诊"
            }}</strong>
          </div>

          <div class="seg">
            <button
              class="seg__btn"
              :class="{ 'seg__btn--active': online }"
              :disabled="statusUpdating || online"
              @click="updateDoctorStatus('online')"
            >
              {{
                statusUpdating && pendingStatus === "online"
                  ? "切换中…"
                  : "设为在线"
              }}
            </button>
            <button
              class="seg__btn"
              :class="{ 'seg__btn--active': !online }"
              :disabled="statusUpdating || !online"
              @click="updateDoctorStatus('offline')"
            >
              {{
                statusUpdating && pendingStatus === "offline"
                  ? "切换中…"
                  : "设为离线"
              }}
            </button>
          </div>

          <p class="hint">{{ statusMessage }}</p>
        </section>

        <section class="card card--danger">
          <div class="card__head">
            <h3>危险操作</h3>
          </div>
          <p class="card__desc">
            删除后该账号将从用户列表移除，关联数据按后端约束处理。此操作不可撤销。
          </p>
          <button
            class="btn-danger"
            :disabled="deleteLoading"
            @click="deleteCurrentUser"
          >
            {{ deleteLoading ? "删除中…" : "删除账号" }}
          </button>
          <p class="hint">{{ deleteMessage }}</p>
        </section>
      </div>
    </div>
  </section>

  <section v-else class="page page--empty">
    <div class="card empty-state">
      <h3>未找到该用户</h3>
      <p>请返回列表页重新选择用户。</p>
      <button class="btn-primary" @click="goBack">返回用户列表</button>
    </div>
  </section>
</template>

<script lang="ts">
import { computed, defineComponent, onMounted, ref } from "vue";
import { useStore } from "vuex";
import { useRoute, useRouter } from "vue-router";
import {
  isSuperAdminPortalRole,
  resolveRoleName,
} from "@/core/auth/utils/roleUtils";
import { storeKey } from "@/app/store";

type RoleKind = "boss" | "doctor" | "warehouse" | "user";

export default defineComponent({
  name: "SuperAdminUserDetail",
  setup() {
    const getErrorDetails = (error: unknown) => {
      const responseError = error as {
        response?: {
          data?: {
            error?: {
              details?: string;
            };
          };
        };
      };

      return responseError.response?.data?.error?.details;
    };

    const store = useStore(storeKey);
    const route = useRoute();
    const router = useRouter();
    const statusUpdating = ref(false);
    const pendingStatus = ref<"online" | "offline" | null>(null);
    const statusMessage = ref("等待操作");
    const deleteLoading = ref(false);
    const deleteMessage = ref("谨慎操作");

    const userId = computed(() => Number(route.params.userId));
    const user = computed(
      () =>
        store.state.superAdmin.users.find((item) => item.id === userId.value) ||
        null
    );
    const roleName = computed(() =>
      user.value
        ? resolveRoleName(user.value.type_name, user.value.type_id) ||
          "未知角色"
        : "未知角色"
    );
    const isDoctor = computed(() => roleName.value === "医生");
    const online = computed(() => user.value?.status === "online");
    const roleKind = computed<RoleKind>(() => {
      const role = roleName.value;
      if (isSuperAdminPortalRole(role)) return "boss";
      if (role === "医生" || role === "护士") return "doctor";
      if (role === "仓库管理员") return "warehouse";
      return "user";
    });
    const initial = computed(() => {
      const name = (user.value?.name || "").trim();
      return name ? name.charAt(0) : "?";
    });
    const accountBalance = computed(() => {
      const rawBalance = (user.value as { balance?: number | string } | null)
        ?.balance;
      const value = Number(rawBalance ?? 0);
      return Number.isFinite(value) ? value.toFixed(2) : "0.00";
    });

    /**
     * 用户详情页依赖用户列表缓存定位当前用户。
     */
    const loadUser = async () => {
      await store.dispatch("superAdmin/ensureUsers", { force: true });
    };

    const goBack = () => {
      void router.push({ name: "superAdminUsers" });
    };

    const updateDoctorStatus = async (status: "online" | "offline") => {
      if (!user.value || !isDoctor.value) return;

      statusUpdating.value = true;
      pendingStatus.value = status;
      try {
        await store.dispatch("superAdmin/changeDoctorWorkStatus", {
          doctorId: user.value.id,
          status,
        });
        statusMessage.value = `医生状态已更新为${
          status === "online" ? "在线" : "离线"
        }`;
      } catch (error: unknown) {
        statusMessage.value =
          getErrorDetails(error) || "状态修改失败，请稍后重试";
      } finally {
        statusUpdating.value = false;
        pendingStatus.value = null;
      }
    };

    const deleteCurrentUser = async () => {
      if (!user.value || deleteLoading.value) return;

      const confirmed = window.confirm(
        `确认删除账号「${user.value.name || user.value.id}」吗？`
      );
      if (!confirmed) {
        return;
      }

      deleteLoading.value = true;
      try {
        await store.dispatch("superAdmin/deleteUser", user.value.id);
        deleteMessage.value = "账号已删除";
        goBack();
      } catch (error: unknown) {
        deleteMessage.value = getErrorDetails(error) || "删除失败，请稍后重试";
      } finally {
        deleteLoading.value = false;
      }
    };

    onMounted(loadUser);

    return {
      user,
      roleName,
      roleKind,
      initial,
      isDoctor,
      online,
      accountBalance,
      statusUpdating,
      pendingStatus,
      statusMessage,
      deleteLoading,
      deleteMessage,
      goBack,
      updateDoctorStatus,
      deleteCurrentUser,
    };
  },
});
</script>

<style scoped>
.page {
  display: grid;
  align-content: start;
  gap: 14px;
  height: 100%;
  min-height: 0;
  overflow-y: auto;
  color: #0f172a;
}

.page--empty {
  place-content: center;
}

/* 返回 */
.link-back {
  justify-self: start;
  display: inline-flex;
  align-items: center;
  gap: 6px;
  border: 0;
  padding: 4px 6px;
  background: transparent;
  color: #64748b;
  font-size: 13px;
  font-weight: 600;
  cursor: pointer;
}

.link-back:hover {
  color: #4f46e5;
}

.link-back svg {
  width: 16px;
  height: 16px;
}

/* 卡片基底 */
.card,
.profile-card {
  border: 1px solid #e7e9ee;
  border-radius: 14px;
  background: #ffffff;
  box-shadow: 0 1px 2px rgba(16, 24, 40, 0.04);
}

/* 档案头卡 */
.profile-card {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 18px;
  padding: 18px 20px;
  flex-wrap: wrap;
}

.profile-id {
  display: flex;
  align-items: center;
  gap: 14px;
  min-width: 0;
}

.avatar {
  width: 56px;
  height: 56px;
  flex: 0 0 auto;
  border-radius: 16px;
  display: grid;
  place-items: center;
  font-size: 22px;
  font-weight: 800;
}

.avatar--boss {
  background: #eef2ff;
  color: #4f46e5;
}

.avatar--doctor {
  background: #ecfdf5;
  color: #059669;
}

.avatar--warehouse {
  background: #fff7ed;
  color: #c2410c;
}

.avatar--user {
  background: #f1f5f9;
  color: #475569;
}

.profile-id__name {
  display: flex;
  align-items: center;
  gap: 10px;
  font-size: 20px;
  font-weight: 700;
  letter-spacing: -0.01em;
}

.profile-id__meta {
  display: flex;
  align-items: center;
  gap: 10px;
  margin-top: 6px;
}

.muted {
  color: #94a3b8;
  font-size: 13px;
  font-variant-numeric: tabular-nums;
}

.pill {
  display: inline-flex;
  align-items: center;
  height: 22px;
  padding: 0 9px;
  border-radius: 7px;
  font-size: 12px;
  font-weight: 600;
}

.pill--boss {
  background: #eef2ff;
  color: #4338ca;
}

.pill--doctor {
  background: #ecfdf5;
  color: #047857;
}

.pill--warehouse {
  background: #fff7ed;
  color: #b45309;
}

.pill--user {
  background: #f1f5f9;
  color: #475569;
}

.status-chip {
  display: inline-flex;
  align-items: center;
  gap: 5px;
  height: 22px;
  padding: 0 9px;
  border-radius: 999px;
  font-size: 12px;
  font-weight: 700;
}

.status-chip .dot {
  width: 6px;
  height: 6px;
  border-radius: 50%;
  background: currentColor;
}

.status-chip--online {
  background: #ecfdf5;
  color: #059669;
}

.status-chip--offline {
  background: #f1f5f9;
  color: #64748b;
}

.balance {
  display: grid;
  gap: 4px;
  padding: 12px 18px;
  border-radius: 12px;
  background: linear-gradient(135deg, #4f46e5, #6366f1);
  color: #fff;
  min-width: 180px;
  text-align: right;
}

.balance__label {
  font-size: 12px;
  font-weight: 600;
  color: rgba(255, 255, 255, 0.82);
}

.balance__value {
  font-size: 26px;
  font-weight: 800;
  letter-spacing: -0.02em;
  font-variant-numeric: tabular-nums;
}

/* 两栏 */
.detail-grid {
  display: grid;
  grid-template-columns: 1.1fr 0.9fr;
  gap: 14px;
  align-items: start;
}

.side-col {
  display: grid;
  gap: 14px;
}

.card {
  padding: 18px 20px;
  display: grid;
  gap: 14px;
  align-content: start;
}

.card__head h3 {
  margin: 0;
  font-size: 15px;
  font-weight: 700;
  color: #0f172a;
}

.card__desc {
  margin: 0;
  color: #64748b;
  font-size: 13px;
  line-height: 1.6;
}

/* 基础资料：key-value 行 */
.info-list {
  margin: 0;
  display: grid;
}

.info-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
  padding: 11px 0;
  border-bottom: 1px solid #f1f2f4;
}

.info-row:last-child {
  border-bottom: 0;
}

.info-row dt {
  color: #64748b;
  font-size: 13px;
}

.info-row dd {
  margin: 0;
  color: #0f172a;
  font-size: 14px;
  font-weight: 600;
  text-align: right;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

/* 接诊状态 */
.status-now {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 10px 12px;
  border-radius: 10px;
  font-size: 13px;
}

.status-now .dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  background: currentColor;
}

.status-now--online {
  background: #ecfdf5;
  color: #059669;
}

.status-now--offline {
  background: #f1f5f9;
  color: #64748b;
}

.seg {
  display: inline-flex;
  padding: 4px;
  gap: 2px;
  background: #f1f2f5;
  border-radius: 10px;
}

.seg__btn {
  flex: 1;
  height: 34px;
  border: 0;
  border-radius: 8px;
  background: transparent;
  color: #64748b;
  font-size: 13px;
  font-weight: 600;
  cursor: pointer;
}

.seg__btn--active {
  background: #fff;
  color: #4f46e5;
  box-shadow: 0 1px 2px rgba(16, 24, 40, 0.08);
}

.seg__btn:disabled {
  cursor: default;
}

.seg__btn:not(.seg__btn--active):not(:disabled):hover {
  color: #0f172a;
}

.hint {
  margin: 0;
  color: #94a3b8;
  font-size: 12px;
}

/* 危险区 */
.card--danger {
  border-color: #f3d6da;
  background: #fffafa;
}

.btn-danger {
  justify-self: start;
  height: 38px;
  padding: 0 18px;
  border: 1px solid #f3c9cd;
  border-radius: 9px;
  background: #fff;
  color: #dc2626;
  font-size: 13px;
  font-weight: 700;
  cursor: pointer;
}

.btn-danger:hover:not(:disabled) {
  background: #fef2f2;
}

.btn-danger:disabled {
  cursor: not-allowed;
  opacity: 0.6;
}

.btn-primary {
  height: 38px;
  padding: 0 18px;
  border: 0;
  border-radius: 9px;
  background: #4f46e5;
  color: #fff;
  font-size: 13px;
  font-weight: 700;
  cursor: pointer;
}

.btn-primary:hover {
  background: #4338ca;
}

/* 空态 */
.empty-state {
  text-align: center;
  gap: 10px;
  justify-items: center;
  padding: 40px 24px;
}

.empty-state h3 {
  margin: 0;
  font-size: 16px;
}

.empty-state p {
  margin: 0;
  color: #64748b;
  font-size: 13px;
}

@media (max-width: 960px) {
  .detail-grid {
    grid-template-columns: 1fr;
  }

  .profile-card {
    flex-direction: column;
    align-items: stretch;
  }

  .balance {
    text-align: left;
  }
}
</style>
