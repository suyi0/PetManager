<template>
  <section class="page">
    <header class="topbar">
      <div class="page-intro">
        <div>
          <p class="intro-copy">医生端 / 值班工作区</p>
          <h3>医生首页工作台</h3>
        </div>
      </div>
      <div class="topbar-actions">
        <span class="date-pill">{{ time }}</span>
        <div class="status" :class="{ 'status--offline': !isDoctorOnline }">
          <span class="dot"></span>
          <span>{{ statusText }}</span>
          <button class="logout-button" @click="logout">登出</button>
        </div>
      </div>
    </header>
    <div class="page-head">
      <section class="panel">
        <div class="panel-contain">
          <div class="panel-head">
            <h4>待接诊队列</h4>
            <span>优先展示即将到号、已到院、等待医生处理的患者。</span>
          </div>
          <ul class="list queue-list">
            <li v-for="item in priorityQueue" :key="`focus-${item.id}`">
              <strong
                >{{ item.petName }} / {{ item.ownerName }} /
                {{ item.symptom }}</strong
              >
              <span>{{ item.arrivedAt }} · 等待中 · 预约ID {{ item.id }}</span>
              <button class="row-action" @click="goToCreateOrder(item)">
                就医
              </button>
            </li>
          </ul>
        </div>
      </section>
      <section class="hero">
        <div class="hero-copy">
          <div class="hero-information">
            <p>医生名称: {{ doctorName }}</p>
            <p>联系方式: {{ doctorPhone }}</p>
            <div class="hero-actions-inline">
              <button
                class="soft-action"
                :disabled="
                  !dutyStatusLoaded || dutyActionLoading || isDoctorOnline
                "
                @click="online"
              >
                {{
                  dutyActionLoading && !isDoctorOnline
                    ? "签到中..."
                    : "设为在线"
                }}
              </button>
              <button
                class="soft-action danger"
                :disabled="
                  !dutyStatusLoaded || dutyActionLoading || !isDoctorOnline
                "
                @click="offline"
              >
                {{
                  dutyActionLoading && isDoctorOnline ? "签退中..." : "结束接诊"
                }}
              </button>
            </div>
          </div>
          <div
            class="hero-status"
            :class="{ 'status--offline': !isDoctorOnline }"
          >
            <small>当前状态</small>
            <strong>{{ isDoctorOnline ? "在线" : "离线" }}</strong>
            <span>{{ dutyStatusHint }}</span>
          </div>
        </div>
      </section>
    </div>
    <section class="detail-grid">
      <section class="panel">
        <div class="panel-contain">
          <div class="panel-head">
            <h4>搜索用户</h4>
          </div>
          <div class="search-div">
            <label class="search-box">
              <span class="search-icon" aria-hidden="true">
                <svg viewBox="0 0 24 24" fill="none">
                  <circle
                    cx="11"
                    cy="11"
                    r="6.5"
                    stroke="currentColor"
                    stroke-width="2"
                  />
                  <path
                    d="M16 16L21 21"
                    stroke="currentColor"
                    stroke-width="2"
                    stroke-linecap="round"
                  />
                </svg>
              </span>
              <input
                type="text"
                class="search-form"
                placeholder="输入用户名 / 手机号码"
                v-model.trim="searchKeyword"
                @input="handleUserSearch"
              />
            </label>
            <div class="search-results">
              <button
                v-for="item in searchResults"
                :key="item.id"
                type="button"
                class="result-tag"
                @click="goToUserProfile(item.id)"
              >
                <strong>{{ item.ownerName }}</strong>
                <span>{{ item.phone }}</span>
                <em>{{ item.petNames.join(" / ") }}</em>
              </button>
              <p v-if="!searchResults.length" class="search-empty">
                暂无匹配结果，试试输入宠物名、主人名或手机号码。
              </p>
            </div>
          </div>
        </div>
      </section>
      <section class="stats-grid">
        <article
          v-for="item in stats"
          :key="item.label"
          class="summary-card"
          :class="{ 'summary-card--deep': item.label === '已开诊单' }"
        >
          <small>{{ item.label }}</small>
          <strong>{{ item.value }}</strong>
          <span>{{ item.hint }}</span>
        </article>

        <article class="summary-card summary-card--action">
          <div class="panel-head">
            <h4>创建新医订单</h4>
            <span>快速入口</span>
          </div>
          <p>核心步骤字段：宠物、药品、数量。</p>
          <RouterLink class="search-link" :to="`${basePath}/create-order`">
            一键进入开单
          </RouterLink>
        </article>
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
import { storeKey } from "@/store/appStore";
import { doctorApi } from "../../api/doctorApi";
import { QueueItem } from "../../api/types";
import { doctorWorkbenchStats } from "../../api/doctorMock";
import {
  DoctorOrderDraftSummary,
  listDoctorOrderDrafts,
} from "../../utils/orderDrafts";

type SearchUserItem = {
  id: string;
  ownerName: string;
  phone: string;
  petNames: string[];
};

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
    const lastDutyActionAt = ref("");
    const searchKeyword = ref("");
    const searchSource = computed<SearchUserItem[]>(() =>
      store.state.doctor.userProfiles.map((item) => ({
        id: item.id,
        ownerName: item.ownerName,
        phone: item.phone,
        petNames: item.pets.map((pet) => pet.name),
      }))
    );
    const searchResults = ref<SearchUserItem[]>([]);
    const now = ref(new Date());
    let timer: number | undefined;
    const syncDrafts = () => {
      draftSummaries.value = listDoctorOrderDrafts();
    };

    onMounted(() => {
      syncDrafts();
      timer = window.setInterval(() => {
        now.value = new Date();
      }, 1000);
      window.addEventListener("focus", syncDrafts);
      window.addEventListener("storage", syncDrafts);

      void store
        .dispatch("doctor/ensureWorkbenchData")
        .then(() => {
          const status = store.state.doctor.dutyStatus;
          isDoctorOnline.value = status.is_online;
          lastDutyActionAt.value = status.is_online
            ? status.check_in_time?.slice(0, 5) || ""
            : status.check_out_time?.slice(0, 5) ||
              status.check_in_time?.slice(0, 5) ||
              "";
          searchResults.value = searchSource.value.slice(0, 6);
        })
        .catch((error) => {
          console.error("获取医生值班状态失败:", error);
        })
        .finally(() => {
          dutyStatusLoaded.value = true;
        });
    });

    onBeforeUnmount(() => {
      if (timer) {
        window.clearInterval(timer);
      }
      window.removeEventListener("focus", syncDrafts);
      window.removeEventListener("storage", syncDrafts);
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
    const basePath = computed(() =>
      route.path.startsWith("/preview/doctor") ? "/preview/doctor" : "/doctor"
    );
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
      try {
        const message = await doctorApi.online();
        store.commit("doctor/markDutyStatusDirty");
        await store.dispatch("doctor/refreshDutyStatus");
        isDoctorOnline.value = store.state.doctor.dutyStatus.is_online;
        lastDutyActionAt.value =
          store.state.doctor.dutyStatus.check_in_time?.slice(0, 5) ||
          formatDutyTime(new Date());
        window.alert(message);
      } catch (error) {
        window.alert(resolveErrorMessage(error));
      } finally {
        dutyActionLoading.value = false;
      }
    };

    const offline = async () => {
      if (dutyActionLoading.value || !isDoctorOnline.value) {
        return;
      }

      dutyActionLoading.value = true;
      try {
        const message = await doctorApi.offline();
        store.commit("doctor/markDutyStatusDirty");
        await store.dispatch("doctor/refreshDutyStatus");
        isDoctorOnline.value = store.state.doctor.dutyStatus.is_online;
        lastDutyActionAt.value =
          store.state.doctor.dutyStatus.check_out_time?.slice(0, 5) ||
          formatDutyTime(new Date());
        window.alert(message);
      } catch (error) {
        window.alert(resolveErrorMessage(error));
      } finally {
        dutyActionLoading.value = false;
      }
    };

    const handleUserSearch = () => {
      const keyword = searchKeyword.value.trim().toLowerCase();

      if (!keyword) {
        searchResults.value = searchSource.value.slice(0, 6);
        return;
      }

      searchResults.value = searchSource.value.filter((item) =>
        [item.ownerName, item.phone, item.petNames.join(" ")]
          .join(" ")
          .toLowerCase()
          .includes(keyword)
      );
    };

    const goToUserProfile = (userId: string) => {
      router.push(`${basePath.value}/users/${userId}`);
    };

    const goToCreateOrder = (item: QueueItem) => {
      router.push({
        path: `${basePath.value}/create-order/${item.id}`,
        query: {
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
        query: route.path.startsWith("/preview/doctor")
          ? undefined
          : { redirect: route.fullPath },
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
      statusText,
      dutyStatusHint,
      basePath,
      online,
      offline,
      searchKeyword,
      searchResults,
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
  display: grid;
  gap: 18px;
}

.topbar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
  padding: 18px 22px;
  border-radius: 28px;
  border: 1px solid rgba(147, 178, 169, 0.22);
  background: rgba(255, 253, 249, 0.66);
  box-shadow: 0 18px 40px rgba(57, 87, 83, 0.07);
}

.page-intro {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 16px;
}

.intro-copy {
  margin: 0 0 4px;
  font-size: 12px;
  color: #8ba09b;
}

.page-intro h3 {
  margin: 0;
  font-size: 28px;
  color: #21464b;
}

.topbar-actions {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 10px;
  flex-wrap: wrap;
}

.date-pill {
  padding: 10px 16px;
  border-radius: 14px;
  background: #edf7f3;
  color: #5f8e84;
  font-size: 13px;
  font-weight: 700;
}

.status {
  display: inline-flex;
  align-items: center;
  gap: 10px;
  padding: 11px 16px;
  border-radius: 999px;
  background: linear-gradient(135deg, rgba(247, 253, 250, 0.9), #e7f3ee);
  border: 1px solid rgba(133, 176, 162, 0.28);
  color: #315b57;
  font-size: 13px;
  font-weight: 600;
}

.status--offline {
  background: linear-gradient(135deg, rgba(255, 247, 239, 0.9), #f8ede3);
  border-color: rgba(197, 156, 125, 0.24);
  color: #8c6244;
}

.dot {
  width: 10px;
  height: 10px;
  border-radius: 50%;
  background: #56bb87;
  box-shadow: 0 0 0 6px rgba(86, 187, 135, 0.12);
}

.status--offline .dot {
  background: #d7a36e;
  box-shadow: 0 0 0 6px rgba(215, 163, 110, 0.14);
}

.logout-button {
  border: 1px solid rgba(143, 204, 180, 0.42);
  border-radius: 999px;
  padding: 8px 16px;
  background: linear-gradient(135deg, #effaf5, #dff2ea);
  color: #27575d;
  font-size: 12px;
  font-weight: 700;
  letter-spacing: 0.02em;
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.7),
    0 8px 16px rgba(49, 91, 87, 0.08);
  cursor: pointer;
  transition: transform 0.2s ease, box-shadow 0.2s ease, border-color 0.2s ease;
}

.logout-button:hover {
  transform: translateY(-1px);
  border-color: rgba(65, 132, 112, 0.42);
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.85),
    0 12px 20px rgba(49, 91, 87, 0.12);
}

.page-head {
  width: 100%;
  height: 250px;
  display: grid;
  grid-template-columns: 1.05fr 0.95fr;
  gap: 20px;
}

.hero,
.summary-card,
.panel {
  border: 1px solid #d8eae6;
  border-radius: 28px;
  background: linear-gradient(180deg, rgba(255, 255, 255, 0.96), #f8fcfb);
  box-shadow: 0 14px 30px rgba(67, 102, 96, 0.05);
}

.panel {
  width: 100%;
  height: 100%;
  overflow-y: auto; /* 垂直方向滚动 */
  overflow-x: hidden; /* 隐藏水平滚动 */
  /* 隐藏滚动条但保持滚动功能 */
  &::-webkit-scrollbar {
    display: none; /* 隐藏滚动条 */
  }

  .panel-contain {
    padding: 22px;
  }
}

.hero {
  width: 100%;
  height: 100%;
  gap: 18px;
  padding: 0;
  overflow: hidden;
  background: linear-gradient(135deg, #255e67, #21545c 65%, #21474f);
}

.hero-copy {
  display: flex;
  align-items: center;
  flex-direction: row;
  justify-content: space-between;
  gap: 10px;
  padding: 36px 32px 28px;
  color: #fff;

  .hero-information {
    width: 50%;
    height: 100%;

    p {
      font-size: 12px;
    }
  }
}

.hero-copy h4,
.panel h4 {
  margin: 0;
}

.hero-copy h4 {
  font-size: 56px;
  line-height: 1.02;
  letter-spacing: 0.02em;
}

.hero-copy p {
  margin: 10px 0 0;
  font-size: 24px;
  color: rgba(255, 255, 255, 0.88);
}

.hero-actions-inline {
  display: flex;
  gap: 18px;
  margin-top: 34px;
}

.soft-action {
  width: 40%;
  border: 0;
  border-radius: 14px;
  padding: 13px 20px;
  background: #90f0bf;
  color: #25555c;
  font-weight: 700;
  cursor: pointer;
}

.soft-action:disabled {
  cursor: not-allowed;
  opacity: 0.62;
  box-shadow: none;
}

.soft-action.danger {
  margin-left: auto;
  background: #91e9c4;
  color: #ff3a2f;
}

.hero-status {
  display: grid;
  align-content: stretch;
  gap: 10px;
  padding: 28px 24px;
  background: linear-gradient(180deg, #22d126, #26d22d);
  color: #fff;
  border-radius: 20px;
}

.status--offline {
  color: #21464b;
  background: linear-gradient(135deg, #878787, #f7f7f7);
}

.hero-status small {
  font-size: 12px;
  opacity: 0.9;
}

.hero-status strong {
  font-size: 58px;
  line-height: 1;
}

.hero-status span {
  font-size: 13px;
  opacity: 0.95;
}

.stats-grid,
.detail-grid {
  display: grid;
  gap: 20px;
}

.stats-grid {
  grid-template-columns: 1fr 1fr;
  grid-template-rows: 1fr 1fr;
}

.detail-grid {
  grid-template-columns: 1.05fr 0.95fr;
}

.summary-card {
  padding: 22px;
}

.summary-card small,
.summary-card span {
  display: block;
  color: #8a9f9b;
}

.summary-card small {
  font-size: 12px;
}

.summary-card strong {
  display: block;
  margin: 10px 0 8px;
  font-size: 22px;
  color: #21464b;
}

.summary-card span {
  font-size: 12px;
}

.summary-card--deep {
  background: linear-gradient(135deg, #1f5158, #23474e);
}

.summary-card--deep small,
.summary-card--deep span,
.summary-card--deep strong {
  color: #f3fffd;
}

.summary-card--action p {
  margin: 0 0 18px;
  color: #7c918d;
  font-size: 13px;
}

.panel-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 14px;
  gap: 12px;
}

.panel-head h4 {
  color: #24464b;
}

.panel-head span {
  font-size: 12px;
  color: #809692;
}

.search-link {
  display: inline-flex;
  justify-content: center;
  align-items: center;
  min-height: 44px;
  padding: 0 16px;
  border-radius: 12px;
  background: #173f46;
  color: #fff;
  text-decoration: none;
  font-weight: 700;
}

.search-link--full {
  width: 30%;
}

.list {
  list-style: none;
  margin: 0;
  padding: 0;
  display: grid;
  gap: 12px;
}

.list li {
  display: grid;
  gap: 6px;
  padding: 14px 16px;
  border-radius: 16px;
  border: 1px solid rgba(223, 237, 233, 0.92);
  background: #fbfefd;
}

@media (max-width: 960px) {
  .topbar {
    align-items: flex-start;
    gap: 12px;
    flex-direction: column;
  }

  .page-intro {
    flex-direction: column;
    align-items: flex-start;
  }
}

.list span,
.list em {
  font-size: 13px;
  color: #5f7873;
  font-style: normal;
}

.list strong {
  color: #18383b;
}

.row-action {
  justify-self: end;
  border: 0;
  border-radius: 999px;
  padding: 7px 14px;
  background: #e6f5ef;
  color: #4f8b76;
  font-size: 12px;
  font-weight: 700;
}

.search-div {
  display: grid;
  gap: 12px;

  .search-box {
    display: flex;
    align-items: center;
    gap: 10px;
    min-height: 40px;
    padding: 10px 14px;
    border-radius: 12px;
    border: 1px solid rgba(150, 181, 172, 0.34);
    background: rgba(255, 255, 255, 0.96);
    color: #829792;
    font-size: 13px;
    box-sizing: border-box;
    cursor: text;

    .search-icon {
      display: inline-flex;
      width: 18px;
      height: 18px;
      color: #79908a;
      flex: 0 0 auto;
    }

    .search-icon svg {
      width: 100%;
      height: 100%;
    }

    .search-form {
      height: 30px;
    }
  }
  .search-box input {
    width: 100%;
    min-width: 0;
    border: 0;
    background: transparent;
    color: #21464b;
    font-size: 13px;
    outline: none;
  }

  .search-box input::placeholder {
    color: #829792;
  }

  .search-results {
    width: 100%;
    display: flex;
    flex-wrap: wrap;
    gap: 10px;
  }

  .result-tag {
    display: grid;
    gap: 4px;
    min-width: 178px;
    padding: 12px 14px;
    border: 1px solid rgba(120, 165, 153, 0.2);
    border-radius: 16px;
    background: linear-gradient(180deg, #f8fffc, #eef8f4);
    box-shadow: 0 10px 18px rgba(67, 102, 96, 0.06);
    text-align: left;
    cursor: pointer;
    transition: transform 0.2s ease, box-shadow 0.2s ease,
      border-color 0.2s ease;
  }

  .result-tag:hover {
    transform: translateY(-2px);
    border-color: rgba(72, 142, 120, 0.38);
    box-shadow: 0 14px 24px rgba(67, 102, 96, 0.1);
  }

  .result-tag strong {
    color: #1f4a4f;
    font-size: 14px;
  }

  .result-tag span,
  .result-tag em {
    color: #6f8782;
    font-size: 12px;
    font-style: normal;
  }

  .search-empty {
    margin: 0;
    padding: 14px 16px;
    border-radius: 14px;
    background: rgba(241, 248, 245, 0.9);
    color: #7c938e;
    font-size: 13px;
  }
}

.fake-row {
  display: grid;
  grid-template-columns: 1fr 156px;
  gap: 12px;
}

@media (max-width: 960px) {
  .hero,
  .stats-grid,
  .detail-grid {
    grid-template-columns: 1fr;
  }

  .hero {
    gap: 0;
  }

  .hero-copy {
    padding: 28px 22px 24px;
  }

  .hero-copy h4 {
    font-size: 42px;
  }

  .soft-action.danger {
    margin-left: 0;
  }

  .fake-row {
    grid-template-columns: 1fr;
  }
}
</style>
