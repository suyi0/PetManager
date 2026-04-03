<template>
  <section class="panel">
    <div class="panel-head">
      <div>
        <h3>订单记录</h3>
        <p>按诊单编号、宠物、主人和费用追踪历史记录。</p>
      </div>
      <div class="panel-head__actions">
        <AppPager
          :page="page"
          :total-pages="totalPages"
          @update:page="page = $event"
        />
        <button>导出记录</button>
      </div>
    </div>

    <div class="status-filters">
      <button
        v-for="tab in statusTabs"
        :key="tab.key"
        type="button"
        class="status-filter"
        :class="{ 'status-filter--active': activeStatus === tab.key }"
        @click="activeStatus = tab.key"
      >
        <span>{{ tab.label }}</span>
        <strong>{{ tab.count }}</strong>
      </button>
    </div>

    <div class="table-shell">
      <table>
        <thead>
          <tr>
            <th>诊单编号</th>
            <th>宠物</th>
            <th>主人</th>
            <th>创建时间</th>
            <th>药品数</th>
            <th>总费用</th>
            <th>状态</th>
          </tr>
        </thead>
        <tbody>
          <tr
            v-for="item in pagedItems"
            :key="item.id"
            class="record-row"
            @click="openOrderDetail(item.id)"
          >
            <td>{{ item.id }}</td>
            <td>{{ item.petName }}</td>
            <td>{{ item.ownerName }}</td>
            <td>{{ item.createdAt }}</td>
            <td>{{ item.medicineCount }}</td>
            <td>¥{{ item.totalFee.toFixed(2) }}</td>
            <td>
              <span class="status-pill" :class="statusClassName(item.status)">
                {{ item.status }}
              </span>
            </td>
          </tr>
          <tr
            v-for="placeholder in placeholderRows"
            :key="`placeholder-${placeholder}`"
            class="placeholder-row"
          >
            <td colspan="7"></td>
          </tr>
          <tr v-if="visibleItems.length === 0">
            <td colspan="7" class="empty-cell">当前分类下暂无订单记录。</td>
          </tr>
        </tbody>
      </table>
    </div>
  </section>
</template>

<script lang="ts">
import { computed, defineComponent, ref, watch, onMounted } from "vue";
import { useRoute, useRouter } from "vue-router";
import AppPager from "../../../../components/AppPager.vue";
import { OrderRecordItem } from "../../api/types";
import { doctorApi } from "../../api/doctorApi";

export default defineComponent({
  name: "DoctorOrderRecords",
  components: { AppPager },
  setup() {
    const route = useRoute();
    const router = useRouter();
    const activeStatus = ref<"全部" | OrderRecordItem["status"]>("全部");
    const page = ref(1);
    const pageSize = 10;
    const orderRecords = ref<OrderRecordItem[]>([]);

    const basePath = computed(() =>
      route.path.startsWith("/preview/doctor") ? "/preview/doctor" : "/doctor"
    );

    const loadOrderRecords = async () => {
      orderRecords.value = await doctorApi.getOrderRecords();
    };

    /**
     * 重新排序订单记录列表，默认按照创建时间降序排列，以确保最新的订单记录显示在最前面。
      如果需要按照其他字段排序，可以在这里进行调整。
     */
    const items = computed(() =>
      [...orderRecords.value].sort((a, b) =>
        b.createdAt.localeCompare(a.createdAt)
      )
    );

    /**
     * 根据订单状态筛选出当前页需要展示的订单记录数据，如果状态为“全部”则返回所有订单记录
     */
    const visibleItems = computed(() => {
      if (activeStatus.value === "全部") {
        return items.value;
      }
      return items.value.filter((item) => item.status === activeStatus.value);
    });

    const totalPages = computed(() =>
      Math.max(1, Math.ceil(visibleItems.value.length / pageSize))
    );

    const pagedItems = computed(() => {
      const start = (page.value - 1) * pageSize;
      return visibleItems.value.slice(start, start + pageSize);
    });

    const placeholderRows = computed(() =>
      pagedItems.value.length === 0
        ? []
        : Array.from(
            { length: Math.max(0, pageSize - pagedItems.value.length) },
            (_, index) => index + 1
          )
    );

    /**
     * 状态标签
     */
    const statusTabs = computed(() => {
      const statuses: Array<"全部" | OrderRecordItem["status"]> = [
        "全部",
        "待付款",
        "已完成",
        "已取消",
      ];

      return statuses.map((status) => ({
        key: status,
        label: status,
        count:
          status === "全部"
            ? items.value.length
            : items.value.filter((item) => item.status === status).length,
      }));
    });

    const statusClassName = (status: OrderRecordItem["status"]) => {
      if (status === "待付款") return "status-pill--pending";
      if (status === "已完成") return "status-pill--done";
      return "status-pill--cancelled";
    };

    const openOrderDetail = (orderId: string) => {
      router.push({
        path: `${basePath.value}/orders/${orderId}`,
        query: { from: "records" },
      });
    };

    watch(activeStatus, () => {
      page.value = 1;
    });

    watch(totalPages, (value) => {
      if (page.value > value) {
        page.value = value;
      }
    });

    onMounted(() => {
      void loadOrderRecords();
    });

    return {
      activeStatus,
      page,
      totalPages,
      statusTabs,
      visibleItems,
      pagedItems,
      placeholderRows,
      statusClassName,
      openOrderDetail,
    };
  },
});
</script>

<style scoped>
.panel {
  display: grid;
  grid-template-rows: auto auto minmax(0, 1fr);
  border: 1px solid rgba(157, 188, 178, 0.24);
  border-radius: 28px;
  background: linear-gradient(180deg, rgba(255, 253, 248, 0.96), #f6fbf8);
  padding: 22px;
  max-height: min(100vh - 140px, 780px);
  box-shadow: 0 20px 38px rgba(49, 82, 77, 0.06);
  box-sizing: border-box;
  overflow: hidden;
}

.panel-head {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  gap: 12px;
  margin-bottom: 14px;
}

.panel-head h3,
.panel-head p {
  margin: 0;
}

.panel-head p {
  margin-top: 6px;
  color: #67807b;
  line-height: 1.6;
}

.panel-head__actions {
  display: flex;
  align-items: center;
  gap: 12px;
}

.status-filters {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 12px;
  margin-bottom: 18px;
}

.status-filter {
  display: grid;
  gap: 6px;
  justify-items: start;
  padding: 14px 16px;
  border: 1px solid rgba(155, 185, 177, 0.22);
  border-radius: 18px;
  background: rgba(255, 255, 255, 0.72);
  color: #24484b;
  box-shadow: 0 12px 26px rgba(49, 82, 77, 0.06);
}

.status-filter span {
  font-size: 13px;
  color: #69817d;
}

.status-filter strong {
  font-size: 22px;
  line-height: 1;
}

.status-filter--active {
  border-color: rgba(41, 97, 94, 0.28);
  background: linear-gradient(135deg, #edf8f3, #fff7ef);
}

button {
  border: 1px solid rgba(144, 175, 166, 0.24);
  border-radius: 16px;
  padding: 11px 16px;
  background: linear-gradient(135deg, #29565a, #7d5348);
  color: #fffdfb;
  cursor: pointer;
  box-shadow: 0 12px 24px rgba(49, 82, 87, 0.12);
}

table {
  width: 100%;
  border-collapse: collapse;
  table-layout: fixed;
}

.table-shell {
  min-height: 0;
  overflow: hidden;
}

th,
td {
  text-align: left;
  padding: 14px 12px;
  border-bottom: 1px solid rgba(226, 236, 232, 0.92);
  font-size: 13px;
  text-align: center;
}

th {
  color: #6f8582;
  font-weight: 600;
}

td {
  color: #19383b;
}

tbody tr {
  background: rgba(255, 255, 255, 0.46);
}

.record-row {
  cursor: pointer;
  transition: background 0.18s ease, transform 0.18s ease;
}

.record-row:hover {
  background: rgba(238, 247, 242, 0.95);
}

.status-pill {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 72px;
  padding: 6px 12px;
  border-radius: 999px;
  font-size: 12px;
  font-weight: 700;
}

.status-pill--pending {
  background: #fff1db;
  color: #b86c11;
}

.status-pill--done {
  background: #dff4e9;
  color: #1f8960;
}

.status-pill--cancelled {
  background: #f7e2e2;
  color: #bf4747;
}

.empty-cell {
  color: #708682;
  padding: 26px 12px;
}

.placeholder-row td {
  height: 52px;
  background: rgba(255, 255, 255, 0.32);
}

@media (max-width: 960px) {
  .panel-head {
    flex-direction: column;
    align-items: flex-start;
  }

  .panel-head__actions,
  .panel-head__actions {
    width: 100%;
    flex-wrap: wrap;
  }

  .status-filters {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }

  .panel {
    overflow: auto;
  }

  table {
    min-width: 760px;
  }
}
</style>
