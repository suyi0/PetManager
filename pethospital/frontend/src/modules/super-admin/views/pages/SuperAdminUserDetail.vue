<template>
  <section v-if="user" class="page">
    <div class="panel hero">
      <button class="ghost back-button" @click="goBack">返回用户列表</button>
      <div class="hero__body">
        <div>
          <p class="eyebrow">User Detail</p>
          <h2>{{ user.name || "未命名用户" }}</h2>
          <p class="hero__meta">
            #{{ user.id }} · {{ roleName }}
            <span
              v-if="isDoctor"
              class="status-chip"
              :class="
                user.status === 'online'
                  ? 'status-chip--online'
                  : 'status-chip--offline'
              "
            >
              {{ user.status === "online" ? "在线" : "离线" }}
            </span>
          </p>
        </div>
        <aside class="balance-card">
          <span>账户金额</span>
          <strong>¥ {{ accountBalance }}</strong>
        </aside>
      </div>
    </div>

    <div class="detail-grid">
      <article class="panel detail-card">
        <h3>基础资料</h3>
        <div class="info-grid">
          <div>
            <span>用户 ID</span>
            <strong>{{ user.id }}</strong>
          </div>
          <div>
            <span>角色</span>
            <strong>{{ roleName }}</strong>
          </div>
          <div>
            <span>手机号</span>
            <strong>{{ user.phone || "未填写" }}</strong>
          </div>
          <div>
            <span>邮箱</span>
            <strong>{{ user.email || "未填写" }}</strong>
          </div>
          <div>
            <span>生日</span>
            <strong>{{ user.birthday || "未填写" }}</strong>
          </div>
        </div>
      </article>

      <article v-if="isDoctor" class="panel detail-card detail-card--doctor">
        <h3>医生状态管理</h3>
        <p class="detail-card__desc">
          在这里直接切换医生在线或离线状态。修改成功后，页面会自动刷新当前医生信息。
        </p>

        <div class="status-board">
          <div>
            <span>当前状态</span>
            <strong>{{ user.status === "online" ? "在线" : "离线" }}</strong>
          </div>
          <div>
            <span>状态说明</span>
            <strong>{{
              user.status === "online" ? "当前可接诊" : "当前未接诊"
            }}</strong>
          </div>
        </div>

        <div class="status-actions">
          <button
            :disabled="statusUpdating || user.status === 'online'"
            @click="updateDoctorStatus('online')"
          >
            {{
              statusUpdating && pendingStatus === "online"
                ? "切换中..."
                : "设为在线"
            }}
          </button>
          <button
            class="danger"
            :disabled="statusUpdating || user.status === 'offline'"
            @click="updateDoctorStatus('offline')"
          >
            {{
              statusUpdating && pendingStatus === "offline"
                ? "切换中..."
                : "设为离线"
            }}
          </button>
        </div>

        <p class="status-message">{{ statusMessage }}</p>
      </article>

      <article class="panel detail-card danger-zone">
        <h3>账号管理</h3>
        <p class="detail-card__desc">
          删除后该账号会从用户列表移除，关联数据将按后端约束处理。
        </p>
        <button
          class="danger"
          :disabled="deleteLoading"
          @click="deleteCurrentUser"
        >
          {{ deleteLoading ? "删除中..." : "删除账号" }}
        </button>
        <p class="status-message">{{ deleteMessage }}</p>
      </article>
    </div>
  </section>

  <section v-else class="page">
    <div class="panel empty-state">
      <h3>未找到该用户</h3>
      <p>请返回列表页重新选择用户。</p>
      <button @click="goBack">返回用户列表</button>
    </div>
  </section>
</template>

<script lang="ts">
import { computed, defineComponent, onMounted, ref } from "vue";
import { useStore } from "vuex";
import { useRoute, useRouter } from "vue-router";
import { resolveRoleName } from "@/core/auth/utils/roleUtils";
import { storeKey } from "@/app/store";

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
      isDoctor,
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
}

.panel {
  border: 1px solid #e7e9ee;
  border-radius: 14px;
  background: #ffffff;
  padding: 18px;
  box-shadow: 0 1px 2px rgba(16, 24, 40, 0.04);
}

.hero {
  display: grid;
  gap: 16px;
}

.back-button {
  justify-self: start;
}

.hero__body {
  display: flex;
  align-items: flex-end;
  justify-content: space-between;
  gap: 18px;
}

.balance-card {
  display: grid;
  align-content: center;
  gap: 12px;
  min-width: 240px;
  min-height: 120px;
  padding: 24px 28px;
  border-radius: 20px;
  border: 1px solid rgba(79, 70, 229, 0.12);
  background: rgba(255, 255, 255, 0.86);
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.76);
}

.balance-card span {
  color: #64748b;
  font-size: 16px;
  font-weight: 700;
}

.balance-card strong {
  color: #0f172a;
  font-size: 34px;
  line-height: 1.1;
}

.eyebrow {
  margin: 0 0 8px;
  font-size: 12px;
  letter-spacing: 0.24em;
  text-transform: uppercase;
  color: #64748b;
}

.hero h2 {
  margin: 0;
  font-size: 32px;
  color: #0f172a;
}

.hero__meta {
  margin: 10px 0 0;
  color: #64748b;
  display: flex;
  gap: 10px;
  align-items: center;
  flex-wrap: wrap;
}

.detail-grid {
  display: grid;
  grid-template-columns: 1.05fr 0.95fr;
  gap: 18px;
}

.detail-card {
  display: grid;
  gap: 16px;
}

.detail-card h3,
.empty-state h3 {
  margin: 0;
  color: #16284d;
}

.detail-card__desc,
.status-message,
.empty-state p {
  margin: 0;
  color: #64748b;
  line-height: 1.7;
}

.info-grid,
.status-board {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 14px;
}

.info-grid div,
.status-board div {
  padding: 16px;
  border-radius: 18px;
  background: rgba(255, 255, 255, 0.92);
  border: 1px solid rgba(79, 70, 229, 0.12);
}

.info-grid span,
.status-board span {
  display: block;
  margin-bottom: 8px;
  color: #64748b;
  font-size: 13px;
}

.info-grid strong,
.status-board strong {
  color: #15254a;
  font-size: 16px;
}

.detail-card--doctor {
  background: radial-gradient(
      circle at top right,
      rgba(16, 185, 129, 0.12),
      transparent 30%
    ),
    linear-gradient(180deg, #ffffff 0%, #f8fafc 100%);
}

.danger-zone {
  background: radial-gradient(
      circle at top right,
      rgba(220, 38, 38, 0.12),
      transparent 32%
    ),
    linear-gradient(180deg, #ffffff 0%, #fef2f2 100%);
}

.status-chip {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  padding: 6px 10px;
  border-radius: 999px;
  font-size: 12px;
  font-weight: 700;
}

.status-chip--online {
  background: #ecfdf5;
  color: #217149;
}

.status-chip--offline {
  background: #fef2f2;
  color: #dc2626;
}

.status-actions {
  display: flex;
  gap: 12px;
  flex-wrap: wrap;
}

.empty-state {
  text-align: center;
}

button {
  border: 0;
  border-radius: 14px;
  padding: 11px 16px;
  background: #4f46e5;
  color: #fff;
  cursor: pointer;
  font-weight: 700;
}

button.ghost {
  background: #ecfdf5;
  color: #2a4c92;
}

button.danger {
  background: #ef4444;
}

button:disabled {
  cursor: not-allowed;
  opacity: 0.6;
}

@media (max-width: 960px) {
  .detail-grid,
  .info-grid,
  .status-board {
    grid-template-columns: 1fr;
  }

  .hero__body {
    align-items: stretch;
    flex-direction: column;
  }
}
</style>
