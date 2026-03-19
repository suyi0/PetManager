<template>
  <section class="panel">
    <div class="panel-head">
      <div>
        <h3>预约订单</h3>
        <p>以预约卡片快速处理待确认、已确认和到院状态。</p>
      </div>
      <button>批量确认</button>
    </div>

    <div class="cards">
      <article v-for="item in items" :key="item.id" class="card">
        <div class="card-top">
          <strong>{{ item.petName }}</strong>
          <span class="status">{{ item.status }}</span>
        </div>
        <p>{{ item.ownerName }} · {{ item.project }}</p>
        <small>{{ item.schedule }} · {{ item.doctorName }}</small>
      </article>
    </div>
  </section>
</template>

<script lang="ts">
import { defineComponent } from "vue";
import { reservationItems } from "../../api/doctorMock";

export default defineComponent({
  name: "DoctorReservations",
  setup() {
    return { items: reservationItems };
  },
});
</script>

<style scoped>
.panel {
  display: grid;
  gap: 18px;
  border: 1px solid rgba(157, 188, 178, 0.24);
  border-radius: 28px;
  background: linear-gradient(180deg, rgba(255, 253, 248, 0.96), #f6fbf8);
  padding: 22px;
  box-shadow: 0 20px 38px rgba(49, 82, 77, 0.06);
}

.panel-head {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
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

button {
  border: 1px solid rgba(144, 175, 166, 0.24);
  border-radius: 16px;
  padding: 11px 16px;
  background: linear-gradient(135deg, #29565a, #7d5348);
  color: #fffdfb;
  cursor: pointer;
  box-shadow: 0 12px 24px rgba(49, 82, 87, 0.12);
}

.cards {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 16px;
}

.card {
  position: relative;
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

.card small {
  margin-top: 8px;
}

@media (max-width: 960px) {
  .panel-head {
    flex-direction: column;
    align-items: flex-start;
  }

  .cards {
    grid-template-columns: 1fr;
  }
}
</style>
