<template>
  <section v-if="user" class="page">
    <div class="panel hero">
      <button class="ghost back-button" @click="goBack">返回用户列表</button>
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
          <div>
            <span>地址 ID</span>
            <strong>{{ user.address_id || "未填写" }}</strong>
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
import { storeKey } from "@/store/appStore";
import { superAdminApi } from "../../api/superAdminApi";

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

    /**
     * 用户详情页依赖用户列表缓存定位当前用户。
     */
    const loadUser = async () => {
      await store.dispatch("superAdmin/ensureUsers");
    };

    const goBack = () => {
      const routeName = route.path.startsWith("/preview")
        ? "previewSuperAdminUsers"
        : "superAdminUsers";
      void router.push({ name: routeName });
    };

    const updateDoctorStatus = async (status: "online" | "offline") => {
      if (!user.value || !isDoctor.value) return;

      statusUpdating.value = true;
      pendingStatus.value = status;
      try {
        await superAdminApi.changeDoctorWorkStatus({
          doctorId: user.value.id,
          status,
        });
        // 医生状态会影响列表展示、考勤统计和审计日志。
        store.commit("superAdmin/markUsersDirty");
        store.commit("superAdmin/markWorkTimeRecordsDirty");
        store.commit("superAdmin/markLogsDirty");
        store.commit("superAdmin/markHomePageDataDirty");
        statusMessage.value = `医生状态已更新为${
          status === "online" ? "在线" : "离线"
        }`;
        await Promise.all([
          store.dispatch("superAdmin/refreshUsers"),
          store.dispatch("superAdmin/refreshWorkTimeRecords"),
        ]);
      } catch (error: unknown) {
        statusMessage.value =
          getErrorDetails(error) || "状态修改失败，请稍后重试";
      } finally {
        statusUpdating.value = false;
        pendingStatus.value = null;
      }
    };

    onMounted(loadUser);

    return {
      user,
      roleName,
      isDoctor,
      statusUpdating,
      pendingStatus,
      statusMessage,
      goBack,
      updateDoctorStatus,
    };
  },
});
</script>

<style scoped>
.page {
  display: grid;
  gap: 18px;
}

.panel {
  border: 1px solid #dce7ff;
  border-radius: 24px;
  background: radial-gradient(
      circle at top left,
      rgba(47, 111, 243, 0.12),
      transparent 34%
    ),
    linear-gradient(180deg, #ffffff 0%, #f6f9ff 100%);
  padding: 22px;
  box-shadow: 0 22px 44px rgba(34, 64, 128, 0.08);
}

.hero {
  display: grid;
  gap: 16px;
}

.back-button {
  justify-self: start;
}

.eyebrow {
  margin: 0 0 8px;
  font-size: 12px;
  letter-spacing: 0.24em;
  text-transform: uppercase;
  color: #6c7a9f;
}

.hero h2 {
  margin: 0;
  font-size: 32px;
  color: #13203a;
}

.hero__meta {
  margin: 10px 0 0;
  color: #617196;
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
  color: #6c7a9f;
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
  border: 1px solid rgba(47, 111, 243, 0.12);
}

.info-grid span,
.status-board span {
  display: block;
  margin-bottom: 8px;
  color: #7381a4;
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
      rgba(36, 190, 123, 0.12),
      transparent 30%
    ),
    linear-gradient(180deg, #ffffff 0%, #f6fbff 100%);
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
  background: #e9f8ef;
  color: #217149;
}

.status-chip--offline {
  background: #fff1f1;
  color: #b14f57;
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
  background: #2f6ff3;
  color: #fff;
  cursor: pointer;
  font-weight: 700;
}

button.ghost {
  background: #ecffd4;
  color: #2a4c92;
}

button.danger {
  background: #ea5c67;
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
}
</style>
