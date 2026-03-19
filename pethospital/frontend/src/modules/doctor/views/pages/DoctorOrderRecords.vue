<template>
  <section class="panel">
    <div class="panel-head">
      <div>
        <h3>订单记录</h3>
        <p>按诊单编号、宠物、主人和费用追踪历史记录。</p>
      </div>
      <button>导出记录</button>
    </div>

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
        <tr v-for="item in items" :key="item.id">
          <td>{{ item.id }}</td>
          <td>{{ item.petName }}</td>
          <td>{{ item.ownerName }}</td>
          <td>{{ item.createdAt }}</td>
          <td>{{ item.medicineCount }}</td>
          <td>¥{{ item.totalFee.toFixed(2) }}</td>
          <td>{{ item.status }}</td>
        </tr>
      </tbody>
    </table>
  </section>
</template>

<script lang="ts">
import { defineComponent } from "vue";
import { orderRecordItems } from "../../api/doctorMock";

export default defineComponent({
  name: "DoctorOrderRecords",
  setup() {
    return { items: orderRecordItems };
  },
});
</script>

<style scoped>
.panel {
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

tbody tr {
  background: rgba(255, 255, 255, 0.46);
}

@media (max-width: 960px) {
  .panel-head {
    flex-direction: column;
    align-items: flex-start;
  }

  .panel {
    overflow: auto;
  }

  table {
    min-width: 760px;
  }
}
</style>
