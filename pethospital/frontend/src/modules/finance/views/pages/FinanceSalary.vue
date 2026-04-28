<template>
  <SuperAdminSalary />
</template>

<script lang="ts">
import { defineComponent } from "vue";
import SuperAdminSalary from "@/modules/super-admin/views/pages/SuperAdminSalary.vue";

export default defineComponent({
  name: "FinanceSalary",
  components: {
    SuperAdminSalary,
  },
});
</script>
