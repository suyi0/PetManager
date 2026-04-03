<template>
  <section class="panel">
    <div class="panel-head">
      <div class="panel-head__intro">
        <h3>预约订单</h3>
        <p>以预约卡片快速处理待确认、已确认和到院状态。</p>
      </div>
      <div class="panel-head__actions">
        <AppPager
          :page="page"
          :total-pages="totalPages"
          @update:page="page = $event"
        />
        <button>批量确认</button>
      </div>
    </div>

    <div class="search-bar">
      <label class="search-field">
        <span class="search-field__icon" aria-hidden="true">
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
          v-model.trim="searchKeyword"
          type="text"
          inputmode="numeric"
          placeholder="输入完整手机号或后四位"
        />
      </label>
      <div class="search-meta">
        <span class="search-meta__count">
          {{ searchSummary }}
        </span>
        <button
          v-if="searchKeyword"
          type="button"
          class="search-meta__reset"
          @click="searchKeyword = ''"
        >
          清空
        </button>
      </div>
    </div>

    <div class="cards">
      <article v-for="item in visibleItems" :key="item.id" class="card">
        <div class="card-top">
          <strong>{{ item.petName }}</strong>
          <span class="status">{{ item.status }}</span>
        </div>
        <p>{{ item.ownerName }} · {{ item.project }}</p>
        <p class="card-phone">手机号 {{ item.phone }}</p>
        <small>{{ item.schedule }} · {{ item.doctorName }}</small>
      </article>
      <article
        v-for="placeholder in placeholderCards"
        :key="`placeholder-${placeholder}`"
        class="card card--placeholder"
      ></article>
      <div v-if="visibleItems.length === 0" class="empty-state">
        {{ emptyStateText }}
      </div>
    </div>
  </section>
</template>

<script lang="ts">
import { computed, defineComponent, ref, watch } from "vue";
import AppPager from "../../../../components/AppPager.vue";
import { reservationItemsMock } from "../../api/doctorMock";

export default defineComponent({
  name: "DoctorReservations",
  components: { AppPager },
  setup() {
    const items = reservationItemsMock;
    const page = ref(1);
    const searchKeyword = ref("");
    const pageSize = 9;

    const normalizedSearchKeyword = computed(() =>
      searchKeyword.value.replace(/\D/g, "")
    );

    const filteredItems = computed(() => {
      const keyword = normalizedSearchKeyword.value;

      if (!keyword) {
        return items;
      }

      return items.filter((item) => {
        if (keyword.length === 4) {
          return item.phone.endsWith(keyword);
        }

        if (keyword.length >= 11) {
          return item.phone === keyword;
        }

        return item.phone.includes(keyword);
      });
    });

    const totalPages = computed(() =>
      Math.max(1, Math.ceil(filteredItems.value.length / pageSize))
    );

    const visibleItems = computed(() => {
      const start = (page.value - 1) * pageSize;
      return filteredItems.value.slice(start, start + pageSize);
    });

    const placeholderCards = computed(() =>
      visibleItems.value.length === 0
        ? []
        : Array.from(
            { length: Math.max(0, pageSize - visibleItems.value.length) },
            (_, index) => index + 1
          )
    );

    const searchSummary = computed(() => {
      if (!normalizedSearchKeyword.value) {
        return `共 ${items.length} 条预约记录`;
      }

      return `匹配到 ${filteredItems.value.length} 条预约记录`;
    });

    const emptyStateText = computed(() =>
      normalizedSearchKeyword.value
        ? "没有找到符合该手机号的预约，请检查完整号码或后四位。"
        : "当前页暂无预约记录。"
    );

    watch(totalPages, (value) => {
      if (page.value > value) {
        page.value = value;
      }
    });

    watch(searchKeyword, () => {
      page.value = 1;
    });

    return {
      page,
      searchKeyword,
      totalPages,
      visibleItems,
      placeholderCards,
      searchSummary,
      emptyStateText,
    };
  },
});
</script>

<style scoped>
.panel {
  display: grid;
  grid-template-rows: auto minmax(0, 1fr);
  gap: 18px;
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
  justify-content: space-between;
  align-items: flex-start;
  gap: 12px;
}

.panel-head__intro {
  display: grid;
  gap: 6px;
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

.search-bar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 14px;
  padding: 14px 16px;
  border-radius: 22px;
  background: radial-gradient(
      circle at left top,
      rgba(215, 233, 225, 0.72),
      transparent 55%
    ),
    linear-gradient(
      135deg,
      rgba(255, 255, 255, 0.96),
      rgba(241, 249, 244, 0.88)
    );
  border: 1px solid rgba(163, 192, 184, 0.24);
}

.search-field {
  display: flex;
  align-items: center;
  gap: 12px;
  min-width: 0;
  flex: 1;
  padding: 0 2px;
}

.search-field__icon {
  width: 42px;
  height: 42px;
  border-radius: 14px;
  display: grid;
  place-items: center;
  color: #426260;
  background: rgba(223, 238, 232, 0.95);
  box-shadow: inset 0 0 0 1px rgba(163, 192, 184, 0.2);
}

.search-field__icon svg {
  width: 18px;
  height: 18px;
}

.search-field input {
  width: 100%;
  border: 0;
  outline: none;
  background: transparent;
  color: #173739;
  font-size: 18px;
  line-height: 1.5;
}

.search-field input::placeholder {
  color: #7d938d;
}

.search-meta {
  display: flex;
  align-items: center;
  gap: 12px;
}

.search-meta__count {
  font-size: 13px;
  color: #5d7671;
  white-space: nowrap;
}

.search-meta__reset {
  border: 0;
  padding: 0;
  border-radius: 0;
  background: transparent;
  color: #355c5f;
  box-shadow: none;
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

button:disabled {
  opacity: 0.52;
  cursor: not-allowed;
  box-shadow: none;
}

.cards {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  align-content: start;
  gap: 16px;
  min-height: 0;
  overflow: hidden;
}

.card {
  position: relative;
  min-height: 148px;
  padding: 18px;
  border-radius: 22px;
  background: linear-gradient(180deg, #fffefb, #f1f8f4);
  border: 1px solid rgba(166, 193, 185, 0.22);
  box-shadow: 0 16px 30px rgba(44, 76, 71, 0.05);
}

.card::after {
  content: "";
  position: absolute;
  right: 16px;
  top: 16px;
  width: 40px;
  height: 40px;
  border-radius: 14px;
  background: rgba(215, 235, 227, 0.84);
}

.card-top {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 10px;
  position: relative;
  z-index: 1;
  gap: 12px;
}

.card-top strong {
  color: #19383b;
  font-size: 18px;
}

.status {
  font-size: 12px;
  color: #49645f;
  padding: 7px 10px;
  border-radius: 999px;
  background: rgba(235, 245, 240, 0.94);
  border: 1px solid rgba(167, 193, 185, 0.22);
}

.card p,
.card small {
  display: block;
  margin: 0;
  color: #5f7773;
  position: relative;
  z-index: 1;
}

.card-phone {
  margin-top: 10px !important;
  color: #355658 !important;
  font-weight: 600;
  letter-spacing: 0.04em;
}

.card small {
  margin-top: 8px;
}

.card--placeholder {
  visibility: hidden;
  pointer-events: none;
}

.empty-state {
  display: grid;
  place-items: center;
  min-height: 148px;
  border-radius: 22px;
  border: 1px dashed rgba(160, 188, 181, 0.42);
  color: #728782;
  background: rgba(255, 255, 255, 0.42);
}

@media (max-width: 960px) {
  .panel-head {
    flex-direction: column;
    align-items: flex-start;
  }

  .search-bar,
  .panel-head__actions,
  .panel-head__actions {
    width: 100%;
    flex-wrap: wrap;
  }

  .search-bar {
    align-items: stretch;
  }

  .search-meta {
    justify-content: space-between;
  }

  .cards {
    grid-template-columns: 1fr;
  }
}
</style>
