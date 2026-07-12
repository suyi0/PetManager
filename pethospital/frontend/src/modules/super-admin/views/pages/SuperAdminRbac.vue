<template>
  <section class="rbac-page">
    <header class="page-head">
      <div>
        <p>权限组织</p>
        <h2>权限组织</h2>
        <span>先维护组织结构，再选择管理对象进入对应权限工作台。</span>
      </div>
    </header>

    <div v-if="error" class="banner banner--error">{{ error }}</div>
    <div v-if="message" class="banner">{{ message }}</div>

    <!-- ① 创建部门与职位（置顶）：左=两个创建表单上下排，右=已有信息 -->
    <section class="panel">
      <div class="panel-title">创建部门与职位</div>

      <div class="create-layout">
        <!-- 左列：部门（新增 + 已有） -->
        <div class="col">
          <form class="create-form" @submit.prevent="createDepartment">
            <strong class="create-form__title">新增部门</strong>
            <div class="create-form__fields">
              <select v-model.number="departmentBranchId">
                <option :value="0">默认分院</option>
                <option v-for="branch in branches" :key="branch.id" :value="branch.id">
                  {{ branch.name }}
                </option>
              </select>
              <select v-model="departmentDomain" title="业务域决定该部门下能创建哪些工种的职位">
                <option v-for="d in businessDomains" :key="d.value" :value="d.value">
                  {{ d.label }}
                </option>
              </select>
              <input v-model.trim="departmentName" placeholder="新部门名称" />
              <button type="submit" class="button">新增部门</button>
            </div>
            <p class="create-form__hint">
              业务域决定该部门下能创建哪些工种（如财务部只能建财务/普通员工职位）。
            </p>
          </form>

          <div class="existing__block">
            <div class="existing__head">
              已有部门 <span class="count">{{ departments.length }}</span>
            </div>
            <div class="existing__list">
              <div
                v-for="department in departments"
                :key="department.id"
                class="existing__item"
              >
                <strong>{{ department.name }}</strong>
                <span class="existing__meta">
                  <small>
                    {{ department.branch_name || "默认分院" }} ·
                    {{ domainLabel(department.business_domain) }}域
                  </small>
                  <button
                    type="button"
                    class="edit-link"
                    :disabled="department.is_system"
                    :title="department.is_system ? '系统内置部门不可编辑' : '编辑名称与业务域'"
                    @click="openEditDepartment(department)"
                  >
                    编辑
                  </button>
                </span>
              </div>
              <p v-if="departments.length === 0" class="existing__empty">
                暂无部门，请先在左侧新增。
              </p>
            </div>
          </div>
        </div>

        <!-- 右列：职位（新增 + 已有） -->
        <div class="col">
          <form class="create-form" @submit.prevent="createPosition">
            <strong class="create-form__title">新增职位</strong>
            <div class="create-form__fields create-form__fields--position">
              <select v-model.number="positionForm.department_id">
                <option :value="0">选择部门</option>
                <option
                  v-for="department in departments"
                  :key="department.id"
                  :value="department.id"
                >
                  {{ department.name }}（{{ domainLabel(department.business_domain) }}）
                </option>
              </select>
              <select
                v-model="positionForm.staff_kind"
                :disabled="!positionForm.department_id"
                :title="!positionForm.department_id ? '请先选择部门' : '工种已按部门业务域过滤'"
              >
                <option v-if="!positionForm.department_id" value="general_staff">
                  请先选择部门
                </option>
                <option
                  v-for="k in availableStaffKinds"
                  :key="k.value"
                  :value="k.value"
                >
                  {{ k.label }}
                </option>
              </select>
              <input v-model.trim="positionForm.name" placeholder="新职位名称" />
              <button type="submit" class="button" :disabled="!positionForm.department_id">
                新增职位
              </button>
            </div>
            <p class="create-form__hint">
              工种按所选部门的业务域过滤——如管理部只能建「管理 / 普通员工」职位。
            </p>
          </form>

          <div class="existing__block">
            <div class="existing__head">
              已有职位 <span class="count">{{ positions.length }}</span>
            </div>
            <div class="existing__list">
              <div
                v-for="position in positions"
                :key="position.id"
                class="existing__item"
              >
                <strong>{{ position.name }}</strong>
                <small>
                  {{ position.department_name || "未分部门" }} ·
                  {{ staffKindLabel(position.staff_kind) }}
                </small>
              </div>
              <p v-if="positions.length === 0" class="existing__empty">
                暂无职位，请先在左侧新增。
              </p>
            </div>
          </div>
        </div>
      </div>
    </section>

    <!-- ② 管理入口分流 -->
    <div class="entry-grid">
      <router-link class="entry-card" to="/super-admin/rbac/positions">
        <span class="entry-card__index">01</span>
        <div>
          <h3>职位权限管理</h3>
          <p>维护职位默认最低权限，影响该职位所有职工</p>
        </div>
        <b>进入管理 →</b>
      </router-link>
      <router-link class="entry-card" to="/super-admin/rbac/users">
        <span class="entry-card__index">02</span>
        <div>
          <h3>职工个人授权</h3>
          <p>给具体职工在职位默认之上单独加授，只影响一人</p>
        </div>
        <b>进入管理 →</b>
      </router-link>
    </div>

    <!-- 编辑部门弹窗：改名称与业务域；域冲突时后端列出阻塞职位，原文显示 -->
    <div v-if="editOpen" class="dialog-backdrop" @click.self="editOpen = false">
      <div class="dialog">
        <div class="dialog__head">
          <h3>编辑部门</h3>
          <button type="button" class="button button--ghost" @click="editOpen = false">
            关闭
          </button>
        </div>
        <form class="dialog__form" @submit.prevent="saveEditDepartment">
          <label>
            <span>部门名称</span>
            <input v-model.trim="editForm.name" placeholder="部门名称" />
          </label>
          <label>
            <span>业务域</span>
            <select v-model="editForm.business_domain">
              <option v-for="d in businessDomains" :key="d.value" :value="d.value">
                {{ d.label }}
              </option>
            </select>
          </label>
          <p class="dialog__hint">
            改域要求该部门下所有职位的工种都属于新域，存在冲突职位时会被拒绝并列出，需先调整或删除它们。
          </p>
          <p v-if="editError" class="dialog__error">{{ editError }}</p>
          <div class="dialog__actions">
            <button type="button" class="button button--ghost" @click="editOpen = false">
              取消
            </button>
            <button type="submit" class="button" :disabled="editSaving">
              {{ editSaving ? "保存中..." : "保存" }}
            </button>
          </div>
        </form>
      </div>
    </div>
  </section>
</template>

<script lang="ts">
import { computed, defineComponent, onMounted, reactive, ref, watch } from "vue";
import { superAdminApi } from "@/modules/super-admin/api/superAdminApi";
import type {
  RbacDepartment,
  RbacPosition,
} from "@/modules/super-admin/api/types";

const staffKinds = [
  { value: "general_staff", label: "普通员工" },
  { value: "management", label: "管理" },
  { value: "finance", label: "财务" },
  { value: "personnel", label: "人事" },
  { value: "doctor", label: "医生" },
  { value: "nurse", label: "护士" },
  { value: "warehouse", label: "仓储" },
];

// 部门业务域（与后端 domainKey 一致）。新增部门时选定，决定该部门下可创建的工种。
const businessDomains = [
  { value: "general", label: "通用（仅普通员工）" },
  { value: "management", label: "管理" },
  { value: "finance", label: "财务" },
  { value: "personnel", label: "人事" },
  { value: "medical", label: "医疗" },
  { value: "warehouse", label: "仓储" },
];

// 域 → 该域下允许的工种（普通员工在任何部门可选）。须与后端 staffKindsForDomain 保持一致。
const KINDS_BY_DOMAIN: Record<string, string[]> = {
  general: ["general_staff"],
  management: ["general_staff", "management"],
  finance: ["general_staff", "finance"],
  personnel: ["general_staff", "personnel"],
  medical: ["general_staff", "doctor", "nurse"],
  warehouse: ["general_staff", "warehouse"],
};

const domainLabel = (domain?: string) =>
  businessDomains.find((d) => d.value === (domain || "general"))?.label.replace(
    "（仅普通员工）",
    ""
  ) || "通用";

export default defineComponent({
  name: "SuperAdminRbac",
  setup() {
    const departments = ref<RbacDepartment[]>([]);
    const positions = ref<RbacPosition[]>([]);
    const departmentBranchId = ref(0);
    const departmentName = ref("");
    const departmentDomain = ref("general");
    const error = ref("");
    const message = ref("");
    const positionForm = reactive({
      department_id: 0,
      name: "",
      staff_kind: "general_staff",
    });

    // 选中部门的业务域；未选部门时为空 → 工种禁用。
    const selectedDeptDomain = computed(() =>
      positionForm.department_id
        ? departments.value.find((d) => d.id === positionForm.department_id)
            ?.business_domain || "general"
        : ""
    );

    // 该部门业务域下允许的工种；未选部门时为空数组（下拉禁用且无可选项）。
    const availableStaffKinds = computed(() => {
      if (!selectedDeptDomain.value) return [];
      const allowed = KINDS_BY_DOMAIN[selectedDeptDomain.value] || ["general_staff"];
      return staffKinds.filter((k) => allowed.includes(k.value));
    });

    // 切换部门后，若当前工种不在新部门域内，回落到该域第一个合法工种。
    watch(availableStaffKinds, (kinds) => {
      if (kinds.length === 0) {
        positionForm.staff_kind = "general_staff";
      } else if (!kinds.some((k) => k.value === positionForm.staff_kind)) {
        positionForm.staff_kind = kinds[0].value;
      }
    });

    const branches = computed(() => {
      const map = new Map<number, string>();
      departments.value.forEach((d) => {
        if (d.branch_id > 0) {
          map.set(d.branch_id, d.branch_name || `分院 ${d.branch_id}`);
        }
      });
      return Array.from(map, ([id, name]) => ({ id, name }));
    });

    const staffKindLabel = (kind?: string) =>
      staffKinds.find((k) => k.value === kind)?.label || kind || "普通员工";

    const load = async () => {
      try {
        const [nextDepartments, nextPositions] = await Promise.all([
          superAdminApi.getRbacDepartments(),
          superAdminApi.getRbacPositions(),
        ]);
        departments.value = nextDepartments;
        positions.value = nextPositions;
      } catch (e) {
        error.value = e instanceof Error ? e.message : "加载组织数据失败";
      }
    };

    const createDepartment = async () => {
      if (!departmentName.value) {
        error.value = "部门名称不能为空";
        return;
      }
      try {
        await superAdminApi.createRbacDepartment({
          name: departmentName.value,
          branch_id: departmentBranchId.value || undefined,
          business_domain: departmentDomain.value,
        });
        departmentName.value = "";
        message.value = "部门已创建";
        error.value = "";
        await load();
      } catch (e) {
        error.value = e instanceof Error ? e.message : "创建部门失败";
      }
    };

    const createPosition = async () => {
      if (!positionForm.department_id || !positionForm.name) {
        error.value = "请选择部门并填写职位名称";
        return;
      }
      try {
        await superAdminApi.createRbacPosition({ ...positionForm });
        positionForm.name = "";
        message.value = "职位已创建";
        error.value = "";
        await load();
      } catch (e) {
        error.value = e instanceof Error ? e.message : "创建职位失败";
      }
    };

    // 编辑部门（名称 + 业务域）。系统内置部门入口已禁用，后端同样拒绝。
    const editOpen = ref(false);
    const editSaving = ref(false);
    const editError = ref("");
    const editForm = reactive({ id: 0, name: "", business_domain: "general" });

    const openEditDepartment = (department: RbacDepartment) => {
      editForm.id = department.id;
      editForm.name = department.name;
      editForm.business_domain = department.business_domain || "general";
      editError.value = "";
      editOpen.value = true;
    };

    const saveEditDepartment = async () => {
      if (!editForm.name) {
        editError.value = "部门名称不能为空";
        return;
      }
      editSaving.value = true;
      try {
        await superAdminApi.updateRbacDepartment(editForm.id, {
          name: editForm.name,
          business_domain: editForm.business_domain,
        });
        editOpen.value = false;
        message.value = "部门已更新";
        error.value = "";
        await load();
      } catch (e) {
        // 域冲突时后端返回被阻塞的职位清单，原文展示便于直接处理
        editError.value = e instanceof Error ? e.message : "保存失败，请重试";
      } finally {
        editSaving.value = false;
      }
    };

    onMounted(() => void load());

    return {
      departments,
      positions,
      branches,
      staffKinds,
      businessDomains,
      staffKindLabel,
      domainLabel,
      availableStaffKinds,
      selectedDeptDomain,
      departmentBranchId,
      departmentName,
      departmentDomain,
      positionForm,
      error,
      message,
      createDepartment,
      createPosition,
      editOpen,
      editSaving,
      editError,
      editForm,
      openEditDepartment,
      saveEditDepartment,
    };
  },
});
</script>

<style scoped>
.rbac-page {
  height: 100%;
  min-height: 0;
  overflow: hidden;
  box-sizing: border-box;
  padding: 22px;
  display: flex;
  flex-direction: column;
  gap: 16px;
  color: #14263f;
  background: linear-gradient(180deg, #f6faff 0, #f8fafc 260px);
}
.page-head {
  flex: 0 0 auto;
}

.page-head p {
  margin: 0;
  color: #2f7dd0;
  font-size: 12px;
  font-weight: 800;
}
.page-head h2 {
  margin: 3px 0;
  font-size: 24px;
}
.page-head span {
  color: #5c7085;
  font-size: 13px;
}

/* 创建卡：占据剩余高度，内部列表自己滚动 → 整页不滚动 */
.panel {
  flex: 1 1 auto;
  min-height: 0;
  display: flex;
  flex-direction: column;
  padding: 18px;
  background: #fff;
  border: 1px solid #d7e3f2;
  border-radius: 16px;
  box-shadow: 0 4px 14px rgba(20, 38, 63, 0.05);
}
.panel-title {
  flex: 0 0 auto;
  padding-bottom: 12px;
  border-bottom: 1px solid #d7e3f2;
  font-weight: 800;
}

/* 创建区：左列=部门（新增+已有），右列=职位（新增+已有），中间竖线分隔 */
.create-layout {
  flex: 1 1 auto;
  min-height: 0;
  display: grid;
  grid-template-columns: minmax(0, 1fr) minmax(0, 1fr);
  gap: 28px;
  margin-top: 14px;
}
.col {
  display: flex;
  flex-direction: column;
  gap: 12px;
  min-height: 0;
}
/* 左列右侧一条竖分隔线，把「部门列」与「职位列」明确分开 */
.col:first-child {
  padding-right: 28px;
  border-right: 1px solid #e6eff9;
  margin-right: -28px;
}
.create-form {
  flex: 0 0 auto;
  padding: 12px 14px;
  background: #f7fafd;
  border: 1px solid #d7e3f2;
  border-radius: 11px;
}
.create-form__title {
  display: block;
  margin-bottom: 9px;
  font-size: 13px;
  color: #3f5b7e;
  font-weight: 800;
}
.create-form__hint {
  margin: 8px 0 0;
  color: #8a99ab;
  font-size: 12px;
  line-height: 1.5;
}
/* 两个表单均为 4 项，2×2 排布：分院|域 / 名称|按钮（职位为 部门|工种 / 名称|按钮） */
.create-form__fields {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 8px;
  align-items: center;
}

input,
select {
  height: 36px;
  min-width: 0;
  padding: 0 10px;
  border: 1px solid #cfe0f5;
  border-radius: 8px;
  color: #14263f;
  background: #fff;
}
.button {
  height: 36px;
  padding: 0 16px;
  border: 1px solid #245fa5;
  border-radius: 8px;
  background: #2f7dd0;
  color: #fff;
  font-size: 13px;
  font-weight: 700;
  cursor: pointer;
}
.button:hover:not(:disabled) {
  background: #245fa5;
}
.button--ghost {
  border-color: #cfe0f5;
  background: #e2eefb;
  color: #1e5aa0;
}
.button--ghost:hover:not(:disabled) {
  background: #d3e5f8;
}
select:disabled,
.button:disabled {
  background: #eef2f7;
  border-color: #dbe4f0;
  color: #9aa7b6;
  cursor: not-allowed;
}

/* 已有部门 / 已有职位：在各自列内占满剩余高度，列表自身滚动 */
.existing__block {
  flex: 1 1 auto;
  display: flex;
  flex-direction: column;
  min-height: 0;
  border: 1px solid #dbe6f2;
  border-radius: 11px;
  background: #fff;
  overflow: hidden;
}
.existing__head {
  padding: 9px 12px;
  border-bottom: 1px solid #e6eff9;
  background: #f2f8ff;
  font-size: 13px;
  font-weight: 800;
  color: #3f5b7e;
}
.existing__head .count {
  display: inline-block;
  margin-left: 6px;
  padding: 0 8px;
  border-radius: 999px;
  background: #e2eefb;
  color: #2263ad;
  font-size: 12px;
}
.existing__list {
  flex: 1 1 auto;
  display: flex;
  flex-direction: column;
  gap: 6px;
  padding: 10px;
  min-height: 0;
  overflow-y: auto;
}
.existing__item {
  display: flex;
  align-items: baseline;
  justify-content: space-between;
  gap: 10px;
  padding: 7px 10px;
  border: 1px solid #eef3fa;
  border-radius: 8px;
  background: #fbfdff;
}
.existing__item strong {
  font-size: 13px;
}
.existing__item small {
  color: #5c7085;
  font-size: 12px;
  white-space: nowrap;
}
.existing__empty {
  margin: 4px 2px;
  color: #8a99ab;
  font-size: 12px;
}
.existing__meta {
  display: inline-flex;
  align-items: center;
  gap: 10px;
  flex-shrink: 0;
}
.edit-link {
  border: 1px solid #cfe0f5;
  border-radius: 7px;
  padding: 3px 9px;
  background: #fff;
  color: #2263ad;
  font-size: 12px;
  font-weight: 700;
  cursor: pointer;
}
.edit-link:hover:not(:disabled) {
  background: #e2eefb;
  border-color: #2f7dd0;
}
.edit-link:disabled {
  color: #9aa7b6;
  cursor: not-allowed;
  background: #f3f6fa;
}

/* 编辑部门弹窗 */
.dialog-backdrop {
  position: fixed;
  inset: 0;
  z-index: 30;
  display: grid;
  place-items: center;
  padding: 20px;
  background: rgba(18, 35, 58, 0.32);
}
.dialog {
  width: min(460px, 100%);
  border: 1px solid #d7e3f2;
  border-radius: 16px;
  background: #fff;
  box-shadow: 0 24px 60px rgba(20, 38, 63, 0.2);
  padding: 20px;
}
.dialog__head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}
.dialog__head h3 {
  margin: 0;
  font-size: 18px;
  color: #14263f;
}
.dialog__form {
  display: grid;
  gap: 12px;
  margin-top: 14px;
}
.dialog__form label {
  display: grid;
  gap: 6px;
  color: #3f5b7e;
  font-size: 13px;
  font-weight: 700;
}
.dialog__hint {
  margin: 0;
  color: #8a99ab;
  font-size: 12px;
  line-height: 1.6;
}
.dialog__error {
  margin: 0;
  padding: 9px 11px;
  border: 1px solid #fecaca;
  border-radius: 8px;
  background: #fef2f2;
  color: #991b1b;
  font-size: 13px;
  line-height: 1.6;
}
.dialog__actions {
  display: flex;
  justify-content: flex-end;
  gap: 10px;
  margin-top: 2px;
}

/* 入口卡：固定在底部，不参与纵向拉伸，保证整页不滚动 */
.entry-grid {
  flex: 0 0 auto;
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 16px;
}
.entry-card {
  display: grid;
  grid-template-columns: auto 1fr;
  gap: 8px 16px;
  padding: 16px 20px;
  background: #fff;
  border: 1px solid #d7e3f2;
  border-radius: 16px;
  box-shadow: 0 6px 18px rgba(20, 38, 63, 0.06);
  color: #14263f;
  text-decoration: none;
  transition: 0.18s ease;
}
.entry-card:hover {
  transform: translateY(-2px);
  border-color: #2f7dd0;
  box-shadow: 0 10px 24px rgba(47, 125, 208, 0.12);
}
.entry-card__index {
  display: grid;
  place-items: center;
  width: 44px;
  height: 44px;
  border-radius: 12px;
  background: #eaf3fd;
  color: #2f7dd0;
  font-weight: 900;
}
.entry-card h3 {
  margin: 0 0 4px;
  font-size: 18px;
}
.entry-card p {
  margin: 0;
  color: #5c7085;
  line-height: 1.6;
}
.entry-card b {
  grid-column: 2;
  color: #2f7dd0;
  font-size: 13px;
}

.banner {
  padding: 10px 12px;
  border: 1px solid #cfe0f5;
  border-radius: 9px;
  background: #eaf3fd;
  color: #1e5aa0;
}
.banner--error {
  border-color: #fecaca;
  background: #fef2f2;
  color: #991b1b;
}

@media (max-width: 1000px) {
  /* 窄屏放开整页滚动：两列改为上下堆叠，去掉竖分隔线 */
  .rbac-page {
    overflow-y: auto;
  }
  .panel,
  .create-layout,
  .existing__block {
    flex: 0 0 auto;
  }
  .entry-grid {
    grid-template-columns: 1fr;
  }
  .create-layout {
    grid-template-columns: 1fr;
    gap: 16px;
  }
  .col:first-child {
    padding-right: 0;
    border-right: none;
    margin-right: 0;
  }
  .existing__list {
    max-height: 200px;
  }
}
</style>
