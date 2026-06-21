<template>
  <section class="page">
    <!-- 值班状态 -->
    <section class="duty">
      <div class="duty__profile">
        <div class="duty__avatar">{{ doctorName.charAt(0) }}</div>
        <div>
          <div class="duty__name">{{ doctorName }}</div>
          <div class="duty__meta">{{ doctorPhone }} · {{ time }}</div>
        </div>
      </div>

      <div
        class="duty__state"
        :class="{ 'duty__state--online': isDoctorOnline }"
      >
        <span class="duty__dot"></span>
        <div>
          <div class="duty__label">
            {{ isDoctorOnline ? "在线接诊中" : "离线休诊" }}
          </div>
          <div class="duty__hint">{{ dutyStatusHint }}</div>
        </div>
      </div>

      <div class="duty__actions">
        <div class="duty__actions__btns">
          <button
            class="btn btn--primary"
            :disabled="!dutyStatusLoaded || dutyActionLoading || isDoctorOnline"
            @click="online"
          >
            {{
              dutyActionLoading && !isDoctorOnline ? "签到中..." : "打卡上线"
            }}
          </button>
          <button
            class="btn btn--danger"
            :disabled="
              !dutyStatusLoaded || dutyActionLoading || !isDoctorOnline
            "
            @click="offline"
          >
            {{ dutyActionLoading && isDoctorOnline ? "签退中..." : "结束接诊" }}
          </button>
        </div>
        <p
          v-if="dutyMessage"
          class="duty__msg"
          :class="`duty__msg--${dutyMessage.type}`"
        >
          {{ dutyMessage.text }}
        </p>
      </div>
    </section>

    <!-- 统计 + 快捷开单 -->
    <section class="stats">
      <article v-for="item in stats" :key="item.label" class="stat">
        <small>{{ item.label }}</small>
        <strong>{{ item.value }}</strong>
        <span>{{ item.hint }}</span>
      </article>
      <RouterLink class="stat stat--action" :to="`${basePath}/create-order`">
        <small>快速入口</small>
        <strong>创建新医订单</strong>
        <span>宠物 · 药品 · 数量，一键开单 →</span>
      </RouterLink>
    </section>

    <!-- 待接诊队列 + 搜索用户 -->
    <section class="grid2">
      <section class="panel">
        <div class="panel__head">
          <h4>待接诊队列</h4>
          <span>即将到号 / 已到院、等待处理的患者</span>
        </div>
        <p v-if="queueMessage" class="msg msg--error">
          {{ queueMessage.text }}
        </p>
        <ul class="queue">
          <li
            v-for="item in priorityQueue"
            :key="`focus-${item.id}`"
            class="queue__item"
          >
            <div class="queue__info">
              <strong>{{ item.petName }} · {{ item.ownerName }}</strong>
              <span>{{ item.symptom }}</span>
              <small
                >{{ item.arrivedAt }} · 等待中 · 预约ID {{ item.id }}</small
              >
            </div>
            <button
              class="btn btn--primary btn--sm"
              @click="goToCreateOrder(item)"
            >
              就医
            </button>
          </li>
          <li v-if="!priorityQueue.length" class="queue__empty">
            暂无待接诊患者
          </li>
        </ul>
      </section>

      <section class="panel">
        <div class="panel__head">
          <h4>搜索用户</h4>
          <span>按用户名 / 手机号查询并查看档案</span>
        </div>
        <label class="search">
          <svg
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="2"
          >
            <circle cx="11" cy="11" r="7" />
            <path d="m21 21-4.3-4.3" />
          </svg>
          <input
            type="text"
            placeholder="输入用户名 / 手机号码后按 Enter"
            v-model.trim="searchKeyword"
            @input="handleSearchInput"
            @keyup.enter="handleUserSearch"
          />
        </label>
        <div class="results">
          <button
            v-for="item in searchResults"
            :key="item.id"
            type="button"
            class="result"
            @click="goToUserProfile(item.id)"
          >
            <strong>{{ item.name || "未命名用户" }}</strong>
            <span>
              {{ item.phone || "未登记手机号" }} ·
              {{ item.email || "未登记邮箱" }}
            </span>
            <em>{{ formatPetNames(item) }}</em>
          </button>
          <p v-if="searchLoading" class="results__empty">正在查询用户摘要...</p>
          <p
            v-else-if="searchHasSearched && !searchResults.length"
            class="results__empty"
          >
            暂无匹配结果，试试输入主人名或手机号码。
          </p>
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
  ref,
  onBeforeUnmount,
} from "vue";
import { useRoute, useRouter } from "vue-router";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";
import { doctorApi } from "@/modules/doctor/api/doctorApi";
import {
  DoctorDutyStatus,
  DoctorUserSummary,
  QueueItem,
} from "@/modules/doctor/api/types";
import { doctorWorkbenchStats } from "@/modules/doctor/api/doctorMock";
import { DoctorOrderDraftSummary } from "@/modules/doctor/utils/orderDrafts";

export default defineComponent({
  name: "DoctorWorkbench",
  setup() {
    const store = useStore(storeKey);
    const router = useRouter();
    const route = useRoute();
    const draftSummaries = ref<DoctorOrderDraftSummary[]>([]);
    const stats = computed(() =>
      doctorWorkbenchStats.map((item, index) =>
        index === 0
          ? {
              label: "诊单草稿",
              value: draftSummaries.value.length,
              hint: "可继续编辑未提交诊单",
            }
          : item
      )
    );
    const priorityQueue = computed(() =>
      store.state.doctor.queueItems.slice(0, 2)
    );
    const doctorName = computed(
      () => store.getters["auth/formattedUserName"] || "当前值班医生"
    );
    const doctorPhone = computed(
      () => store.state.currentUser.userPhone || "未绑定联系方式"
    );
    const isDoctorOnline = ref(false);
    const dutyStatusLoaded = ref(false);
    const dutyActionLoading = ref(false);
    const dutyMessage = ref<{
      type: "success" | "error";
      text: string;
    } | null>(null);
    const queueMessage = ref<{
      type: "error";
      text: string;
    } | null>(null);
    const lastDutyActionAt = ref("");
    const searchKeyword = ref("");
    const searchLoading = ref(false);
    const searchHasSearched = ref(false);
    const searchResults = ref<DoctorUserSummary[]>([]);
    const now = ref(new Date());
    let timer: number | undefined;
    let searchRequestId = 0;
    const syncDrafts = async () => {
      draftSummaries.value = (await store.dispatch(
        "doctor/listOrderDrafts"
      )) as DoctorOrderDraftSummary[];
    };

    const handleFocus = () => {
      void syncDrafts();
    };

    onMounted(() => {
      void syncDrafts();
      timer = window.setInterval(() => {
        now.value = new Date();
      }, 1000);
      window.addEventListener("focus", handleFocus);

      void store
        .dispatch("doctor/ensureDutyStatus")
        .then(() => {
          const status = store.state.doctor.dutyStatus;
          isDoctorOnline.value = status.is_online;
          lastDutyActionAt.value = status.is_online
            ? status.check_in_time?.slice(0, 5) || ""
            : status.check_out_time?.slice(0, 5) ||
              status.check_in_time?.slice(0, 5) ||
              "";
        })
        .catch((error) => {
          dutyMessage.value = {
            type: "error",
            text: resolveErrorMessage(error),
          };
        })
        .finally(() => {
          dutyStatusLoaded.value = true;
        });

      void store
        .dispatch("doctor/ensureQueueItems", { force: true })
        .then(() => {
          queueMessage.value = null;
        })
        .catch((error) => {
          queueMessage.value = {
            type: "error",
            text: resolveErrorMessage(error),
          };
        });
    });

    onBeforeUnmount(() => {
      if (timer) {
        window.clearInterval(timer);
      }
      window.removeEventListener("focus", handleFocus);
    });
    const time = computed(() => {
      const current = now.value;
      const hours = String(current.getHours()).padStart(2, "0");
      const minutes = String(current.getMinutes()).padStart(2, "0");
      const seconds = String(current.getSeconds()).padStart(2, "0");
      const week = (date: Date) => {
        switch (date.getDay()) {
          case 0:
            return "日";
          case 1:
            return "一";
          case 2:
            return "二";
          case 3:
            return "三";
          case 4:
            return "四";
          case 5:
            return "五";
          case 6:
            return "六";
        }
        return "";
      };
      let weekMap = "星期" + week(current);
      return `${hours}:${minutes}:${seconds} ` + weekMap;
    });
    const basePath = computed(() => "/doctor");
    const statusText = computed(() =>
      isDoctorOnline.value ? "在线接诊中" : "未签到"
    );
    const dutyStatusHint = computed(() => {
      if (!dutyStatusLoaded.value) {
        return "正在同步今日接诊状态...";
      }

      if (!lastDutyActionAt.value) {
        return isDoctorOnline.value
          ? "已签到 · 可接诊"
          : "点击设为在线后开始接诊";
      }

      return isDoctorOnline.value
        ? `${lastDutyActionAt.value} 已签到 · 可接诊`
        : `${lastDutyActionAt.value} 已签退 · 今日休诊`;
    });

    const formatDutyTime = (date: Date) => {
      const hours = String(date.getHours()).padStart(2, "0");
      const minutes = String(date.getMinutes()).padStart(2, "0");
      return `${hours}:${minutes}`;
    };

    const resolveErrorMessage = (error: unknown) => {
      const errorWithResponse = error as {
        response?: {
          data?: {
            error?: string;
            message?: string;
          };
        };
      };

      if (errorWithResponse.response?.data) {
        const responseData = errorWithResponse.response.data;
        return (
          responseData.error || responseData.message || "操作失败，请稍后重试。"
        );
      }

      if (error instanceof Error) {
        return error.message;
      }

      return "操作失败，请稍后重试。";
    };

    const online = async () => {
      if (dutyActionLoading.value || isDoctorOnline.value) {
        return;
      }

      dutyActionLoading.value = true;
      dutyMessage.value = null;
      try {
        const { message, dutyStatus } = (await store.dispatch(
          "doctor/changeDutyStatus",
          "online"
        )) as {
          message: string;
          dutyStatus: DoctorDutyStatus;
        };
        isDoctorOnline.value = dutyStatus.is_online;
        const checkInTime = dutyStatus.check_in_time?.slice(0, 5);
        lastDutyActionAt.value = checkInTime || formatDutyTime(new Date());
        dutyMessage.value = {
          type: "success",
          text: message,
        };
      } catch (error) {
        dutyMessage.value = {
          type: "error",
          text: resolveErrorMessage(error),
        };
      } finally {
        dutyActionLoading.value = false;
      }
    };

    const offline = async () => {
      if (dutyActionLoading.value || !isDoctorOnline.value) {
        return;
      }

      dutyActionLoading.value = true;
      dutyMessage.value = null;
      try {
        const { message, dutyStatus } = (await store.dispatch(
          "doctor/changeDutyStatus",
          "offline"
        )) as {
          message: string;
          dutyStatus: DoctorDutyStatus;
        };
        isDoctorOnline.value = dutyStatus.is_online;
        const checkOutTime = dutyStatus.check_out_time?.slice(0, 5);
        lastDutyActionAt.value = checkOutTime || formatDutyTime(new Date());
        dutyMessage.value = {
          type: "success",
          text: message,
        };
      } catch (error) {
        dutyMessage.value = {
          type: "error",
          text: resolveErrorMessage(error),
        };
      } finally {
        dutyActionLoading.value = false;
      }
    };

    const formatPetNames = (item: DoctorUserSummary) => {
      const petNames = item.pets
        .map((pet) => pet.pet_name)
        .filter(Boolean)
        .slice(0, 2);

      return petNames.length > 0 ? petNames.join(" / ") : "暂无宠物档案";
    };

    const handleSearchInput = () => {
      searchResults.value = [];
      searchHasSearched.value = false;
      searchLoading.value = false;
    };

    const handleUserSearch = async () => {
      const keyword = searchKeyword.value.trim();
      const requestId = ++searchRequestId;

      if (!keyword) {
        searchResults.value = [];
        searchLoading.value = false;
        searchHasSearched.value = false;
        return;
      }

      searchLoading.value = true;
      try {
        const users = await doctorApi.getUserList(keyword);

        if (requestId === searchRequestId) {
          searchResults.value = users;
          searchHasSearched.value = true;
        }
      } finally {
        if (requestId === searchRequestId) {
          searchLoading.value = false;
        }
      }
    };

    const goToUserProfile = (userId: number) => {
      router.push(`${basePath.value}/users/${userId}`);
    };

    const goToCreateOrder = (item: QueueItem) => {
      router.push({
        path: `${basePath.value}/create-order/${item.id}`,
        query: {
          ownerId: item.ownerId,
          petId: item.petId,
          petName: item.petName,
          sex: item.sex || "",
          breed: item.breed || "",
          age: item.age || "",
          ownerName: item.ownerName,
          symptom: item.symptom,
        },
      });
    };

    const logout = () => {
      store.dispatch("auth/logout");
      router.push({
        name: "PetHospital",
        query: { redirect: route.fullPath },
      });
    };
    return {
      time,
      stats,
      priorityQueue,
      doctorName,
      doctorPhone,
      isDoctorOnline,
      dutyStatusLoaded,
      dutyActionLoading,
      dutyMessage,
      queueMessage,
      statusText,
      dutyStatusHint,
      basePath,
      online,
      offline,
      searchKeyword,
      searchLoading,
      searchHasSearched,
      searchResults,
      formatPetNames,
      handleSearchInput,
      handleUserSearch,
      goToUserProfile,
      goToCreateOrder,
      logout,
    };
  },
});
</script>

<style scoped>
.page {
  --indigo: #4f46e5;
  --green: #16a34a;
  --red: #dc2626;
  --text: #0f172a;
  --muted: #64748b;
  --faint: #94a3b8;
  --border: #e7e9ee;

  display: grid;
  grid-template-rows: auto auto minmax(0, 1fr);
  gap: 14px;
  height: 100%;
  min-height: 0;
  color: var(--text);
}

.duty,
.stat,
.panel {
  background: #fff;
  border: 1px solid var(--border);
  border-radius: 14px;
  box-shadow: 0 1px 2px rgba(16, 24, 40, 0.04);
}

/* 值班 hero */
.duty {
  display: flex;
  align-items: center;
  gap: 18px;
  padding: 16px 20px;
  flex-wrap: wrap;
}

.duty__profile {
  display: flex;
  align-items: center;
  gap: 12px;
}

.duty__avatar {
  width: 48px;
  height: 48px;
  border-radius: 50%;
  background: linear-gradient(135deg, #6366f1, #4f46e5);
  color: #fff;
  display: grid;
  place-items: center;
  font-weight: 800;
  font-size: 20px;
}

.duty__name {
  font-size: 18px;
  font-weight: 700;
}

.duty__meta {
  font-size: 13px;
  color: var(--muted);
  margin-top: 2px;
  font-variant-numeric: tabular-nums;
}

.duty__state {
  display: flex;
  align-items: center;
  gap: 10px;
  margin-left: auto;
  padding: 8px 14px;
  border-radius: 10px;
  background: #f1f5f9;
  border: 1px solid var(--border);
}

.duty__dot {
  width: 9px;
  height: 9px;
  border-radius: 50%;
  background: #94a3b8;
}

.duty__state--online {
  background: #ecfdf5;
  border-color: #bbf7d0;
}

.duty__state--online .duty__dot {
  background: #16a34a;
  box-shadow: 0 0 0 3px rgba(22, 163, 74, 0.18);
}

.duty__label {
  font-size: 14px;
  font-weight: 700;
}

.duty__state--online .duty__label {
  color: #15803d;
}

.duty__hint {
  font-size: 12px;
  color: var(--muted);
}

.duty__actions {
  display: flex;
  align-items: center;
  gap: 10px;
  flex-direction: column;
}

.duty__actions__btns {
  display: flex;
  align-items: center;
  gap: 10px;
  flex-wrap: wrap;
}

.duty__msg {
  margin: 0;
  font-size: 12px;
  font-weight: 600;
}

.duty__msg--success {
  color: var(--green);
}

.duty__msg--error {
  color: var(--red);
}

/* 按钮 */
.btn {
  height: 38px;
  padding: 0 16px;
  border: 1px solid transparent;
  border-radius: 9px;
  font-size: 13px;
  font-weight: 700;
  cursor: pointer;
}

.btn--sm {
  height: 32px;
  padding: 0 14px;
}

.btn--primary {
  background: var(--indigo);
  color: #fff;
}

.btn--primary:hover:not(:disabled) {
  background: #4338ca;
}

.btn--danger {
  background: #fff;
  color: var(--red);
  border-color: #f3c9cd;
}

.btn--danger:hover:not(:disabled) {
  background: #fef2f2;
}

.btn:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

/* 统计 */
.stats {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 14px;
}

.stat {
  padding: 14px 16px;
  display: grid;
  gap: 4px;
  text-decoration: none;
  color: inherit;
}

.stat small {
  font-size: 12px;
  color: var(--muted);
}

.stat strong {
  font-size: 24px;
  font-weight: 800;
  letter-spacing: -0.02em;
  color: var(--text);
}

.stat span {
  font-size: 12px;
  color: var(--faint);
}

.stat--action {
  background: linear-gradient(135deg, #eef2ff, #fff);
  border-color: #c7d2fe;
}

.stat--action strong {
  color: var(--indigo);
  font-size: 16px;
}

.stat--action span {
  color: var(--indigo);
}

/* 底部两栏 */
.grid2 {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 14px;
  min-height: 0;
}

.panel {
  display: flex;
  flex-direction: column;
  min-height: 0;
  padding: 16px 18px;
}

.panel__head {
  display: flex;
  align-items: baseline;
  gap: 8px;
  margin-bottom: 12px;
}

.panel__head h4 {
  margin: 0;
  font-size: 15px;
  font-weight: 700;
}

.panel__head span {
  font-size: 12px;
  color: var(--muted);
}

.msg {
  margin: 0 0 10px;
  font-size: 13px;
}

.msg--error {
  color: var(--red);
}

/* 队列 */
.queue {
  list-style: none;
  margin: 0;
  padding: 0;
  display: grid;
  gap: 10px;
  align-content: start;
  overflow-y: auto;
  min-height: 0;
}

.queue__item {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  padding: 12px 14px;
  border: 1px solid var(--border);
  border-radius: 10px;
  background: #fafbfc;
}

.queue__info {
  display: grid;
  gap: 3px;
  min-width: 0;
}

.queue__info strong {
  font-size: 14px;
  font-weight: 700;
}

.queue__info span {
  font-size: 13px;
  color: var(--muted);
}

.queue__info small {
  font-size: 12px;
  color: var(--faint);
}

.queue__empty {
  color: var(--faint);
  font-size: 13px;
  text-align: center;
  padding: 24px;
}

/* 搜索 */
.search {
  display: flex;
  align-items: center;
  gap: 8px;
  height: 40px;
  padding: 0 12px;
  border: 1px solid var(--border);
  border-radius: 9px;
  background: #fff;
  margin-bottom: 12px;
}

.search:focus-within {
  border-color: var(--indigo);
  box-shadow: 0 0 0 3px #eef2ff;
}

.search svg {
  width: 16px;
  height: 16px;
  color: var(--faint);
  flex: 0 0 auto;
}

.search input {
  border: 0;
  outline: 0;
  width: 100%;
  font-size: 13px;
  color: var(--text);
  background: transparent;
}

.results {
  display: grid;
  gap: 8px;
  align-content: start;
  overflow-y: auto;
  min-height: 0;
}

.result {
  display: grid;
  gap: 3px;
  text-align: left;
  cursor: pointer;
  padding: 10px 12px;
  border: 1px solid var(--border);
  border-radius: 10px;
  background: #fafbfc;
}

.result:hover {
  border-color: var(--indigo);
  background: #f5f8ff;
}

.result strong {
  font-size: 14px;
  font-weight: 700;
}

.result span {
  font-size: 12px;
  color: var(--muted);
}

.result em {
  font-size: 12px;
  color: var(--indigo);
  font-style: normal;
}

.results__empty {
  margin: 0;
  color: var(--faint);
  font-size: 13px;
  padding: 16px;
  text-align: center;
}

@media (max-width: 1100px) {
  .stats {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }

  .grid2 {
    grid-template-columns: 1fr;
  }
}
</style>
