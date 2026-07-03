<template>
  <section class="online-page">
    <section class="online-hero">
      <div>
        <p class="online-hero__eyebrow">Live Doctor Desk</p>
        <h3>在线医生观察台</h3>
        <span>
          聚合当前正在接诊的医生卡片，快速查看联系方式、签到状态与最近值班轨迹，便于在用户管理与日志审计之间完成实时巡检。
        </span>
      </div>

      <div class="online-hero__metrics">
        <article>
          <strong>{{ onlineDoctors.length }}</strong>
          <span>当前在线医生</span>
        </article>
        <article>
          <strong>{{ readyCount }}</strong>
          <span>已签到待接诊</span>
        </article>
        <article>
          <strong>{{ activeRecordsCount }}</strong>
          <span>今日在线记录</span>
        </article>
        <article>
          <strong>{{ averageShiftHours }}</strong>
          <span>平均在线时长</span>
        </article>
      </div>
    </section>

    <section class="online-shell">
      <section class="online-panel">
        <div class="online-panel__head">
          <div>
            <h4>医生卡片目录</h4>
          </div>
          <div class="online-panel__actions">
            <input
              v-model.trim="keywordInput"
              type="text"
              class="online-search"
              placeholder="搜索医生姓名 / 手机号 / 邮箱"
              @keyup.enter="applySearch"
            />
            <button class="online-ghost" type="button" @click="refreshDoctors">
              刷新列表
            </button>
          </div>
        </div>

        <div v-if="filteredDoctors.length > 0" class="doctor-grid">
          <button
            v-for="item in pagedDoctors"
            :key="item.id"
            type="button"
            class="doctor-card"
            :class="{ 'doctor-card--active': selectedDoctor?.id === item.id }"
            @click="selectedDoctorId = item.id"
          >
            <div class="doctor-card__top">
              <div class="doctor-card__avatar">
                <img
                  v-if="item.head_image"
                  :src="item.head_image"
                  :alt="item.name"
                />
                <span v-else>{{ item.name.slice(0, 1) || "D" }}</span>
              </div>

              <div class="doctor-card__title">
                <small>Doctor #{{ item.id }}</small>
                <strong>{{ item.name || "未命名医生" }}</strong>
                <span>{{ item.email || item.phone || "暂无联系方式" }}</span>
              </div>

              <em class="doctor-status doctor-status--online">在线</em>
            </div>

            <div class="doctor-card__stats">
              <article>
                <span>今日签到</span>
                <strong>{{
                  getCurrentShift(item.id)?.check_in_time || "未记录"
                }}</strong>
              </article>
              <article>
                <span>预计离岗</span>
                <strong>{{ getDisplayCheckout(item.id) }}</strong>
              </article>
              <article>
                <span>本周记录</span>
                <strong>{{ getRecentRecords(item.id).length }} 条</strong>
              </article>
            </div>

            <p class="doctor-card__note">
              {{ getShiftSummary(item.id) }}
            </p>
          </button>
        </div>

        <div v-else class="online-empty">
          <strong>当前没有符合条件的在线医生</strong>
          <span>可以尝试清空搜索词，或稍后刷新查看最新接诊状态。</span>
        </div>

        <div v-if="filteredDoctors.length > 0" class="online-footer">
          <span class="online-footer__summary">
            共 {{ filteredDoctors.length }} 位在线医生，当前第 {{ page }} /
            {{ totalPages }} 页
          </span>
          <AppPager
            :page="page"
            :total-pages="totalPages"
            @update:page="page = $event"
          />
        </div>
      </section>

      <aside class="doctor-detail">
        <div class="doctor-detail__head">
          <small>Doctor Detail</small>
          <h4>医生详情</h4>
        </div>

        <div v-if="selectedDoctor" class="doctor-detail__content">
          <section class="doctor-detail__hero">
            <div class="doctor-detail__avatar">
              <img
                v-if="selectedDoctor.head_image"
                :src="selectedDoctor.head_image"
                :alt="selectedDoctor.name"
              />
              <span v-else>{{ selectedDoctor.name.slice(0, 1) || "D" }}</span>
            </div>

            <div>
              <strong>{{ selectedDoctor.name || "未命名医生" }}</strong>
              <p>#{{ selectedDoctor.id }} · 医生 · 当前在线</p>
            </div>

            <button
              class="online-ghost"
              type="button"
              @click="goToDoctorDetail"
            >
              查看完整资料
            </button>
          </section>

          <section class="doctor-detail__grid">
            <article>
              <span>手机号</span>
              <strong>{{ selectedDoctor.phone || "未填写" }}</strong>
            </article>
            <article>
              <span>邮箱</span>
              <strong>{{ selectedDoctor.email || "未填写" }}</strong>
            </article>
            <article>
              <span>生日</span>
              <strong>{{ selectedDoctor.birthday || "未填写" }}</strong>
            </article>
          </section>

          <section class="doctor-detail__shift">
            <div class="doctor-detail__block-head">
              <h5>当前值班状态</h5>
              <span>{{
                currentShift ? currentShift.date : "今日暂无排班记录"
              }}</span>
            </div>

            <div class="doctor-detail__shift-grid">
              <article>
                <span>签到时间</span>
                <strong>{{ currentShift?.check_in_time || "未记录" }}</strong>
              </article>
              <article>
                <span>签退时间</span>
                <strong>{{ getDisplayCheckout(selectedDoctor.id) }}</strong>
              </article>
              <article>
                <span>状态</span>
                <strong>{{ currentShift?.status || "online" }}</strong>
              </article>
              <article>
                <span>记录说明</span>
                <strong>{{
                  currentShift?.notes || "当前在线，可继续接诊"
                }}</strong>
              </article>
            </div>
          </section>

          <section class="doctor-detail__timeline">
            <div class="doctor-detail__block-head">
              <h5>最近值班轨迹</h5>
              <span>{{ recentSelectedRecords.length }} 条记录</span>
            </div>

            <div v-if="recentSelectedRecords.length > 0" class="timeline-list">
              <article
                v-for="record in recentSelectedRecords"
                :key="`${record.source}-${record.id}-${record.date}`"
                class="timeline-item"
              >
                <strong>{{ record.date }}</strong>
                <span>
                  {{ record.check_in_time || "--:--:--" }} -
                  {{ record.check_out_time || "进行中" }}
                </span>
                <p>{{ record.notes || "系统同步生成的值班记录" }}</p>
              </article>
            </div>

            <div v-else class="online-empty online-empty--inline">
              <strong>暂无更多值班记录</strong>
              <span>等医生产生新的签到或工时记录后，这里会自动同步展示。</span>
            </div>
          </section>
        </div>

        <div v-else class="online-empty online-empty--detail">
          <strong>请选择一位在线医生</strong>
          <span>点击左侧医生卡片后，这里会展示更完整的值班与联系信息。</span>
        </div>
      </aside>
    </section>
  </section>
</template>

<script lang="ts">
import { computed, defineComponent, onMounted, ref, watch } from "vue";
import { useRouter } from "vue-router";
import AppPager from "@/shared/components/AppPager.vue";
import { calculateTotalPages } from "@/shared/utils/pagination";
import { UserRow, WorkTimeRecord } from "../../api/types";
import { superAdminApi } from "../../api/superAdminApi";

const formatDateTimeKey = (record: WorkTimeRecord) =>
  `${record.date}T${record.check_in_time || "00:00:00"}`;

export default defineComponent({
  name: "SuperAdminOnlineDoctors",
  components: { AppPager },
  setup() {
    const router = useRouter();
    const keywordInput = ref("");
    const keyword = ref("");
    const page = ref(1);
    const pageSize = 10;
    const selectedDoctorId = ref<number | null>(null);
    const total = ref(0);

    const onlineDoctors = ref<UserRow[]>([]);
    const workRecords = ref<WorkTimeRecord[]>([]);

    const recordsByDoctorId = computed(() => {
      const map = new Map<number, WorkTimeRecord[]>();

      workRecords.value.forEach((record) => {
        const bucket = map.get(record.user_id) || [];
        bucket.push(record);
        map.set(record.user_id, bucket);
      });

      map.forEach((rows) => {
        rows.sort((left, right) =>
          formatDateTimeKey(right).localeCompare(formatDateTimeKey(left))
        );
      });

      return map;
    });

    const getRecentRecords = (doctorId: number) =>
      (recordsByDoctorId.value.get(doctorId) || []).slice(0, 3);

    const getCurrentShift = (doctorId: number) =>
      (recordsByDoctorId.value.get(doctorId) || []).find(
        (record) => record.status === "online"
      ) ||
      (recordsByDoctorId.value.get(doctorId) || [])[0] ||
      null;

    const getDisplayCheckout = (doctorId: number) => {
      const record = getCurrentShift(doctorId);
      if (!record) {
        return "未记录";
      }
      return record.check_out_time || "待下线";
    };

    const getShiftSummary = (doctorId: number) => {
      const record = getCurrentShift(doctorId);
      if (!record) {
        return "当前仅检测到在线标记，暂未同步具体排班信息。";
      }
      if (record.notes) {
        return record.notes;
      }
      return record.check_out_time
        ? `已记录完整值班时间段，最近签到于 ${record.check_in_time}。`
        : `当前正在接诊中，最近签到时间为 ${
            record.check_in_time || "未记录"
          }。`;
    };

    const filteredDoctors = computed(() => onlineDoctors.value);

    const totalPages = computed(() =>
      calculateTotalPages(total.value, pageSize)
    );

    const pagedDoctors = computed(() => filteredDoctors.value);

    const selectedDoctor = computed(
      () =>
        filteredDoctors.value.find(
          (item) => item.id === selectedDoctorId.value
        ) ||
        pagedDoctors.value[0] ||
        null
    );

    const currentShift = computed(() =>
      selectedDoctor.value ? getCurrentShift(selectedDoctor.value.id) : null
    );

    const recentSelectedRecords = computed(() =>
      selectedDoctor.value ? getRecentRecords(selectedDoctor.value.id) : []
    );

    const readyCount = computed(
      () =>
        onlineDoctors.value.filter((item) => Boolean(getCurrentShift(item.id)))
          .length
    );

    const activeRecordsCount = computed(
      () =>
        workRecords.value.filter((item) => item.source === "online_doctors")
          .length
    );

    const averageShiftHours = computed(() => {
      const hours = onlineDoctors.value
        .map((item) => getCurrentShift(item.id))
        .filter((item): item is WorkTimeRecord => Boolean(item))
        .map((item) => {
          if (!item.check_in_time) {
            return 0;
          }

          const start = item.check_in_time.split(":").map(Number);
          const end = (item.check_out_time || "18:00:00")
            .split(":")
            .map(Number);
          const startHours =
            (start[0] || 0) + (start[1] || 0) / 60 + (start[2] || 0) / 3600;
          const endHours =
            (end[0] || 0) + (end[1] || 0) / 60 + (end[2] || 0) / 3600;
          return Math.max(0, endHours - startHours);
        });

      if (!hours.length) {
        return "0h";
      }

      const average = hours.reduce((sum, item) => sum + item, 0) / hours.length;
      return `${average.toFixed(1)}h`;
    });

    const loadOnlineDoctors = async () => {
      const result = await superAdminApi.searchOnlineDoctors({
        keyword: keyword.value.trim(),
        page: page.value,
        pageSize,
      });
      onlineDoctors.value = result.items;
      workRecords.value = result.records;
      total.value = result.total;
    };

    const refreshDoctors = async () => {
      keywordInput.value = "";
      keyword.value = "";
      page.value = 1;
      await loadOnlineDoctors();
    };

    const applySearch = () => {
      keyword.value = keywordInput.value.trim();
      page.value = 1;
      void loadOnlineDoctors();
    };

    const goToDoctorDetail = () => {
      if (!selectedDoctor.value) {
        return;
      }

      void router.push({
        name: "superAdminUserDetail",
        params: { userId: selectedDoctor.value.id },
      });
    };

    watch(filteredDoctors, (rows) => {
      if (page.value > totalPages.value) {
        page.value = totalPages.value;
      }

      if (!rows.some((item) => item.id === selectedDoctorId.value)) {
        selectedDoctorId.value = rows[0]?.id ?? null;
      }
    });

    watch(page, () => {
      void loadOnlineDoctors();
      if (
        !pagedDoctors.value.some((item) => item.id === selectedDoctorId.value)
      ) {
        selectedDoctorId.value = pagedDoctors.value[0]?.id ?? null;
      }
    });

    onMounted(async () => {
      await loadOnlineDoctors();
      selectedDoctorId.value = filteredDoctors.value[0]?.id ?? null;
    });

    return {
      keywordInput,
      keyword,
      page,
      totalPages,
      onlineDoctors,
      filteredDoctors,
      pagedDoctors,
      selectedDoctorId,
      selectedDoctor,
      currentShift,
      recentSelectedRecords,
      readyCount,
      activeRecordsCount,
      averageShiftHours,
      getCurrentShift,
      getDisplayCheckout,
      getRecentRecords,
      getShiftSummary,
      refreshDoctors,
      applySearch,
      goToDoctorDetail,
    };
  },
});
</script>

<style scoped>
.online-page {
  display: grid;
  grid-template-rows: auto minmax(0, 1fr);
  gap: 14px;
  height: 100%;
  min-height: 0;
}

.online-hero,
.online-panel,
.doctor-detail {
  border: 1px solid #e7e9ee;
  border-radius: 14px;
  background: #ffffff;
  box-shadow: 0 1px 2px rgba(16, 24, 40, 0.04);
}

.online-hero {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 18px;
  padding: 14px 18px;
  background: #ffffff;
}

.online-hero__eyebrow,
.doctor-detail__head small,
.doctor-card__title small {
  margin: 0;
  color: #31859c;
  letter-spacing: 0.14em;
  text-transform: uppercase;
  font-size: 11px;
  font-weight: 700;
}

.online-hero h3,
.online-panel__head h4,
.doctor-detail__head h4,
.doctor-detail__block-head h5 {
  margin: 0;
  color: #1c3159;
}

.online-hero h3 {
  font-size: 18px;
}

.online-hero > div:first-child span {
  display: none;
}

.online-hero span,
.online-panel__head p,
.timeline-item p {
  color: #627494;
  line-height: 1.6;
  font-size: 13px;
}

.online-hero__metrics {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 10px;
  min-width: 460px;
}

.online-hero__metrics article {
  display: grid;
  gap: 2px;
  padding: 8px 12px;
  border-radius: 10px;
  border: 1px solid #e7e9ee;
  background: #f8fafc;
}

.online-hero__metrics strong {
  color: #0f172a;
  font-size: 20px;
  font-weight: 800;
}

.online-hero__metrics span {
  color: #64748b;
  font-size: 13px;
}

.online-shell {
  display: grid;
  grid-template-columns: minmax(0, 1.8fr) 380px;
  gap: 14px;
  min-height: 0;
  height: 100%;
}

.online-panel,
.doctor-detail {
  padding: 14px;
  min-width: 0;
  min-height: 0;
}

.online-panel {
  display: grid;
  grid-template-rows: auto minmax(0, 1fr) auto;
  gap: 10px;
}

.doctor-detail {
  overflow-y: auto;
}

.doctor-grid {
  overflow-y: auto;
}

.online-panel__head,
.doctor-detail__hero,
.doctor-detail__block-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}

.online-panel__actions {
  display: grid;
  grid-template-columns: minmax(260px, 320px) auto;
  gap: 10px;
}

.online-search {
  width: 100%;
  min-height: 40px;
  padding: 9px 12px;
  border: 1px solid #e7e9ee;
  border-radius: 14px;
  background: #fff;
  color: #1f3257;
  font-size: 13px;
}

.doctor-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
  align-content: start;
  justify-content: start;
  align-items: start;
}

.doctor-card {
  border: 1px solid #e7e9ee;
  border-radius: 18px;
  padding: 14px;
  background: linear-gradient(180deg, #ffffff, #f8fafc);
  text-align: left;
  color: #314566;
  cursor: pointer;
  transition: transform 0.18s ease, box-shadow 0.18s ease,
    border-color 0.18s ease;
}

.doctor-card:hover {
  transform: translateY(-2px);
  box-shadow: 0 16px 30px rgba(16, 24, 40, 0.1);
}

.doctor-card--active {
  border-color: #a5b4fc;
  background: linear-gradient(180deg, #eef2ff, #f8fafc);
  box-shadow: 0 18px 34px rgba(79, 70, 229, 0.14);
}

.doctor-card__top {
  display: grid;
  grid-template-columns: 46px minmax(0, 1fr) auto;
  gap: 10px;
  align-items: center;
}

.doctor-card__avatar,
.doctor-detail__avatar {
  width: 46px;
  height: 46px;
  border-radius: 14px;
  overflow: hidden;
  display: grid;
  place-items: center;
  background: linear-gradient(135deg, #a5b4fc, #a5b4fc);
  color: #18345a;
  font-size: 20px;
  font-weight: 800;
}

.doctor-detail__avatar {
  width: 64px;
  height: 64px;
  border-radius: 22px;
}

.doctor-card__avatar img,
.doctor-detail__avatar img {
  width: 100%;
  height: 100%;
  object-fit: cover;
}

.doctor-card__title {
  display: grid;
  gap: 2px;
  min-width: 0;
}

.doctor-card__title strong,
.doctor-detail__hero strong {
  color: #183158;
  font-size: 16px;
}

.doctor-card__title span,
.doctor-detail__hero p,
.doctor-detail__block-head span,
.doctor-detail__grid span,
.doctor-detail__shift-grid span,
.timeline-item span,
.timeline-item strong,
.doctor-card__stats span,
.doctor-card__note {
  color: #6d7d9f;
}

.doctor-card__stats {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 8px;
  margin-top: 12px;
}

.doctor-card__stats article,
.doctor-detail__grid article,
.doctor-detail__shift-grid article {
  display: grid;
  gap: 6px;
  padding: 10px 11px;
  border-radius: 14px;
  background: rgba(255, 255, 255, 0.9);
  border: 1px solid rgba(79, 70, 229, 0.1);
}

.doctor-card__stats strong,
.doctor-detail__grid strong,
.doctor-detail__shift-grid strong {
  color: #132b50;
  font-size: 14px;
}

.doctor-card__note {
  margin: 10px 0 0;
  line-height: 1.6;
  font-size: 12px;
}

.doctor-status {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 56px;
  padding: 6px 10px;
  border-radius: 999px;
  font-style: normal;
  font-size: 11px;
  font-weight: 700;
}

.doctor-status--online {
  background: rgba(16, 185, 129, 0.14);
  color: #0f8f66;
}

.doctor-detail {
  display: grid;
  gap: 16px;
}

.doctor-detail__content {
  display: grid;
  gap: 16px;
}

.doctor-detail__hero {
  align-items: center;
}

.doctor-detail__grid,
.doctor-detail__shift-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
}

.doctor-detail__timeline {
  display: grid;
  gap: 12px;
}

.timeline-list {
  display: grid;
  gap: 10px;
}

.timeline-item {
  padding: 14px;
  border-radius: 18px;
  border: 1px solid #e7e9ee;
  background: linear-gradient(180deg, #ffffff, #f8fafc);
}

.timeline-item strong,
.timeline-item span {
  display: block;
}

.timeline-item span {
  margin-top: 4px;
  font-size: 13px;
}

.timeline-item p {
  margin: 8px 0 0;
}

.online-ghost {
  border: 0;
  min-height: 40px;
  padding: 8px 12px;
  border-radius: 999px;
  background: #eef2ff;
  color: #2e6598;
  cursor: pointer;
  font-weight: 700;
  font-size: 12px;
}

.online-footer {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  padding-top: 2px;
}

.online-footer__summary {
  color: #6e80a2;
  font-size: 12px;
  line-height: 1.4;
}

.online-footer :deep(.pager) {
  gap: 8px;
  font-size: 12px;
}

.online-footer :deep(.pager-button),
.online-footer :deep(.pager-button--ghost) {
  padding: 7px 10px;
  border-radius: 12px;
  font-size: 11px;
  box-shadow: 0 8px 16px rgba(16, 24, 40, 0.1);
}

.online-footer :deep(.pager-jump) {
  gap: 6px;
}

.online-footer :deep(.pager-jump input) {
  width: 54px;
  min-height: 30px;
  padding: 0 8px;
  border-radius: 10px;
}

.online-empty {
  display: grid;
  gap: 8px;
  align-content: center;
  min-height: 220px;
  padding: 24px;
  border: 1px dashed #e7e9ee;
  border-radius: 22px;
  background: linear-gradient(180deg, #f8fafc, #f8fafc);
  text-align: center;
}

.online-empty strong {
  color: #1a335c;
}

.online-empty span {
  color: #64748b;
  line-height: 1.7;
}

.online-empty--detail,
.online-empty--inline {
  min-height: auto;
}

@media (max-width: 1280px) {
  .online-shell {
    grid-template-columns: 1fr;
  }

  .doctor-detail {
    order: -1;
  }
}

@media (max-width: 960px) {
  .online-hero,
  .online-panel__head,
  .doctor-detail__hero,
  .doctor-detail__block-head,
  .online-footer {
    flex-direction: column;
    align-items: stretch;
  }

  .online-hero__metrics,
  .doctor-grid,
  .doctor-detail__grid,
  .doctor-detail__shift-grid,
  .doctor-card__stats {
    grid-template-columns: 1fr;
  }

  .online-panel__actions {
    grid-template-columns: 1fr;
  }

  .doctor-card__top {
    grid-template-columns: 56px minmax(0, 1fr);
  }

  .doctor-status {
    grid-column: 1 / -1;
    justify-self: start;
  }
}
</style>
