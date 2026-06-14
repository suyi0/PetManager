<template>
  <section class="page">
    <div class="panel">
      <div class="panel-head">
        <div>
          <h3>待接诊队列</h3>
          <p>按照到院时间和优先等级快速排序，保持桌面式临床清单感。</p>
        </div>
        <div class="panel-head__actions">
          <AppPager
            :page="page"
            :total-pages="totalPages"
            @update:page="page = $event"
          />
          <button @click="loadQueueItems">刷新队列</button>
        </div>
      </div>

      <div class="table-shell">
        <table>
          <thead>
            <tr>
              <th>宠物</th>
              <th>主人</th>
              <th>主诉</th>
              <th>优先级</th>
              <th>到院时间</th>
              <th class="action-column-th">操作</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="item in visibleItems" :key="item.id">
              <td>{{ item.petName }}</td>
              <td>{{ item.ownerName }}</td>
              <td>{{ item.symptom }}</td>
              <td>
                <span class="tag" :class="item.level">{{ item.level }}</span>
              </td>
              <td>{{ item.arrivedAt }}</td>
              <td class="action-column-td">
                <button
                  type="button"
                  class="action-button"
                  @click="goToCreateOrder(item)"
                >
                  就医
                </button>
              </td>
            </tr>
            <tr
              v-for="placeholder in placeholderRows"
              :key="`placeholder-${placeholder}`"
              class="placeholder-row"
            >
              <td colspan="6"></td>
            </tr>
            <tr v-if="visibleItems.length === 0">
              <td colspan="6" class="empty-cell">当前页暂无待接诊记录。</td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>
  </section>
</template>

<script lang="ts">
import { computed, defineComponent, ref, watch, onMounted } from "vue";
import { useRouter } from "vue-router";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";
import AppPager from "@/shared/components/AppPager.vue";
import { QueueItem } from "@/modules/doctor/api/types";

export default defineComponent({
  name: "DoctorQueue",
  components: { AppPager },
  setup() {
    const store = useStore(storeKey);
    const router = useRouter();
    /**
     * 待接诊队列数据列表
     */
    const queueItems = computed<QueueItem[]>(
      () => store.state.doctor.queueItems
    );
    const page = ref(1);
    const pageSize = 10;
    const basePath = computed(() => "/doctor");

    /**
     * 进入队列页时通过 RESTful 获取一次最新队列。
     */
    const loadQueueItems = async () => {
      await store.dispatch("doctor/ensureQueueItems", { force: true });
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

    const totalPages = computed(() =>
      Math.max(1, Math.ceil(queueItems.value.length / pageSize))
    );

    const visibleItems = computed(() => {
      const start = (page.value - 1) * pageSize;
      return queueItems.value.slice(start, start + pageSize);
    });

    const placeholderRows = computed(() =>
      visibleItems.value.length === 0
        ? []
        : Array.from(
            { length: Math.max(0, pageSize - visibleItems.value.length) },
            (_, index) => index + 1
          )
    );

    watch(totalPages, (value) => {
      if (page.value > value) {
        page.value = value;
      }
    });

    onMounted(() => {
      void loadQueueItems();
    });

    return {
      page,
      totalPages,
      visibleItems,
      placeholderRows,
      loadQueueItems,
      goToCreateOrder,
    };
  },
});
</script>

<style scoped>
.page {
  display: grid;
}

.panel {
  display: grid;
  grid-template-rows: auto minmax(0, 1fr);
  border: 1px solid rgba(157, 188, 178, 0.24);
  border-radius: 28px;
  background: linear-gradient(180deg, rgba(255, 253, 248, 0.96), #f6fbf8);
  padding: 22px;
  max-height: min(100vh - 140px, 760px);
  box-shadow: 0 20px 38px rgba(49, 82, 77, 0.06);
  box-sizing: border-box;
  overflow: hidden;
}

.panel-head {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  margin-bottom: 18px;
  gap: 12px;
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

button {
  border: 1px solid rgba(144, 175, 166, 0.24);
  border-radius: 16px;
  padding: 11px 16px;
  background: linear-gradient(135deg, #29565a, #7d5348);
  color: #fffdfb;
  cursor: pointer;
  box-shadow: 0 12px 24px rgba(49, 82, 87, 0.12);
}

.action-button {
  padding: 7px 12px;
  border-radius: 12px;
  background: linear-gradient(135deg, #ecf8f3, #d7ebe4);
  color: #214f4b;
  box-shadow: none;
}

button:disabled {
  opacity: 0.52;
  cursor: not-allowed;
  box-shadow: none;
}

.table-shell {
  min-height: 0;
  overflow: hidden;
}

table {
  width: 100%;
  border-collapse: collapse;
  overflow: hidden;
  table-layout: fixed;
}

th,
td {
  text-align: left;
  padding: 14px 12px;
  border-bottom: 1px solid rgba(226, 236, 232, 0.92);
  font-size: 13px;
}

th {
  color: #6f8582;
  font-weight: 600;
}

td {
  color: #19383b;
}

.action-column-th {
  width: 110px;
  padding: 14px 24px 14px 12px;
  text-align: right;
}

.action-column-td {
  width: 110px;
  text-align: right;
}

tbody tr {
  background: rgba(255, 255, 255, 0.46);
}

.placeholder-row td {
  height: 52px;
  background: rgba(255, 255, 255, 0.32);
}

.empty-cell {
  height: 120px;
  color: #708682;
  text-align: center;
}

.tag {
  display: inline-flex;
  padding: 7px 11px;
  border-radius: 999px;
  font-size: 12px;
  font-weight: 700;
}

.tag.普通 {
  background: #edf5f1;
  color: #355a53;
}

.tag.优先 {
  background: #fff1d8;
  color: #96611d;
}

.tag.紧急 {
  background: #ffe2df;
  color: #b14739;
}

@media (max-width: 960px) {
  .panel-head {
    flex-direction: column;
    align-items: flex-start;
  }

  .panel-head__actions {
    width: 100%;
    flex-wrap: wrap;
  }

  .panel {
    overflow: auto;
  }

  table {
    min-width: 680px;
  }
}
</style>
