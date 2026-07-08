<template>
  <section class="page rbac-page">
    <section class="rbac-head">
      <div>
        <p class="section-label">权限组织</p>
        <h2>职位权限与组织范围</h2>
      </div>
      <button type="button" class="button button--ghost" @click="loadAll">
        刷新
      </button>
    </section>

    <div v-if="error" class="state-banner state-banner--error">
      {{ error }}
    </div>
    <div v-if="message" class="state-banner">{{ message }}</div>

    <section class="rbac-grid">
      <section class="panel rbac-panel">
        <div class="panel-title">部门与职位</div>

        <form class="inline-form" @submit.prevent="createDepartment">
          <select v-model.number="departmentBranchId">
            <option :value="0">默认分院</option>
            <option v-for="branch in branches" :key="branch.id" :value="branch.id">
              {{ branch.name }}
            </option>
          </select>
          <input v-model.trim="departmentName" placeholder="新部门名称" />
          <button type="submit" class="button">新增部门</button>
        </form>

        <form class="stack-form" @submit.prevent="createPosition">
          <div class="form-row">
            <select v-model.number="positionForm.department_id">
              <option :value="0">选择部门</option>
              <option
                v-for="department in departments"
                :key="department.id"
                :value="department.id"
              >
                {{ department.name }}
              </option>
            </select>
            <select v-model="positionForm.staff_kind">
              <option value="general_staff">普通员工</option>
              <option value="management">管理</option>
              <option value="finance">财务</option>
              <option value="personnel">人事</option>
              <option value="doctor">医生</option>
              <option value="nurse">护士</option>
              <option value="warehouse">仓储</option>
            </select>
          </div>
          <div class="form-row">
            <input v-model.trim="positionForm.name" placeholder="新职位名称" />
            <button type="submit" class="button">新增职位</button>
          </div>
        </form>

        <div class="position-list">
          <button
            v-for="position in positions"
            :key="position.id"
            type="button"
            class="position-item"
            :class="{ 'position-item--active': position.id === selectedPositionId }"
            @click="selectPosition(position.id)"
          >
            <span>{{ position.name }}</span>
            <small>{{ position.department_name || "未分部门" }}</small>
          </button>
        </div>
      </section>

      <section class="panel rbac-panel">
        <div class="panel-title">
          职位权限
          <span v-if="selectedPosition">{{ selectedPosition.name }}</span>
        </div>

        <div class="permission-tools">
          <button
            v-for="template in templates"
            :key="template.id"
            type="button"
            class="button button--ghost"
            :disabled="!selectedPositionId"
            @click="applyTemplate(template.id)"
          >
            {{ template.name }}
          </button>
        </div>

        <div class="permission-list">
          <label
            v-for="permission in permissions"
            :key="permission"
            class="permission-item"
          >
            <input
              type="checkbox"
              :checked="selectedPermissions.includes(permission)"
              :disabled="!selectedPositionId"
              @change="togglePermission(permission)"
            />
            <span>{{ permission }}</span>
          </label>
        </div>

        <button
          type="button"
          class="button permission-save"
          :disabled="!selectedPositionId || savingPermissions"
          @click="savePositionPermissions"
        >
          保存职位权限
        </button>
      </section>

      <section class="panel rbac-panel rbac-panel--wide">
        <div class="panel-title">用户派岗与范围</div>

        <div class="user-toolbar">
          <input
            v-model.trim="userKeyword"
            placeholder="搜索用户名 / 手机号 / 邮箱"
            @keyup.enter="searchUsers"
          />
          <button type="button" class="button" @click="searchUsers">搜索</button>
        </div>

        <div class="user-scope-layout">
          <div class="user-list">
            <button
              v-for="user in users"
              :key="user.id"
              type="button"
              class="user-item"
              :class="{ 'user-item--active': user.id === selectedUserId }"
              @click="selectUser(user.id)"
            >
              <strong>{{ user.name || "未命名用户" }}</strong>
              <span>{{ user.type_name || "未设置职位" }}</span>
            </button>
          </div>

          <div class="scope-editor">
            <label>
              <span>职位</span>
              <select v-model.number="selectedUserPositionId">
                <option :value="0">普通用户</option>
                <option
                  v-for="position in positions"
                  :key="position.id"
                  :value="position.id"
                >
                  {{ position.name }} / {{ position.department_name }}
                </option>
              </select>
            </label>

            <button
              type="button"
              class="button"
              :disabled="!selectedUserId"
              @click="saveUserPosition"
            >
              保存派岗
            </button>

            <div class="scope-group-title">可见分院</div>
            <div class="scope-checks">
              <label
                v-for="branch in branches"
                :key="branch.id"
                class="permission-item"
              >
                <input
                  type="checkbox"
                  :checked="selectedScopeBranchIds.includes(branch.id)"
                  :disabled="!selectedUserId"
                  @change="toggleBranchScope(branch.id)"
                />
                <span>{{ branch.name }}</span>
              </label>
            </div>

            <div class="scope-group-title">可见部门</div>
            <div class="scope-checks">
              <label
                v-for="department in departments"
                :key="department.id"
                class="permission-item"
              >
                <input
                  type="checkbox"
                  :checked="selectedScopeDepartmentIds.includes(department.id)"
                  :disabled="!selectedUserId"
                  @change="toggleDepartmentScope(department.id)"
                />
                <span>{{ department.name }}</span>
              </label>
            </div>

            <button
              type="button"
              class="button"
              :disabled="!selectedUserId"
              @click="saveUserScopes"
            >
              保存组织范围
            </button>
          </div>
        </div>
      </section>
    </section>
  </section>
</template>

<script lang="ts">
import { computed, defineComponent, onMounted, reactive, ref } from "vue";
import { superAdminApi } from "@/modules/super-admin/api/superAdminApi";
import type {
  RbacDepartment,
  RbacPosition,
  PermissionTemplate,
  UserRow,
} from "@/modules/super-admin/api/types";

interface BranchOption {
  id: number;
  name: string;
}

export default defineComponent({
  name: "SuperAdminRbac",
  setup() {
    const departments = ref<RbacDepartment[]>([]);
    const positions = ref<RbacPosition[]>([]);
    const permissions = ref<string[]>([]);
    const templates = ref<PermissionTemplate[]>([]);
    const users = ref<UserRow[]>([]);
    const selectedPermissions = ref<string[]>([]);
    const selectedScopeBranchIds = ref<number[]>([]);
    const selectedScopeDepartmentIds = ref<number[]>([]);
    const selectedPositionId = ref<number | null>(null);
    const selectedUserId = ref<number | null>(null);
    const selectedUserPositionId = ref(0);
    const departmentBranchId = ref(0);
    const departmentName = ref("");
    const userKeyword = ref("");
    const savingPermissions = ref(false);
    const error = ref("");
    const message = ref("");

    const positionForm = reactive({
      department_id: 0,
      name: "",
      staff_kind: "general_staff",
    });

    const selectedPosition = computed(() =>
      positions.value.find((item) => item.id === selectedPositionId.value)
    );

    const branches = computed<BranchOption[]>(() => {
      const branchMap = new Map<number, string>();
      departments.value.forEach((department) => {
        if (department.branch_id > 0) {
          branchMap.set(
            department.branch_id,
            department.branch_name || `分院 ${department.branch_id}`
          );
        }
      });
      return Array.from(branchMap.entries()).map(([id, name]) => ({ id, name }));
    });

    const setNotice = (text: string) => {
      message.value = text;
      error.value = "";
    };

    const setError = (text: string) => {
      error.value = text;
      message.value = "";
    };

    const loadAll = async () => {
      try {
        const [nextDepartments, nextPositions, nextPermissions, nextTemplates] =
          await Promise.all([
            superAdminApi.getRbacDepartments(),
            superAdminApi.getRbacPositions(),
            superAdminApi.getPermissionCatalog(),
            superAdminApi.getPermissionTemplates(),
          ]);
        departments.value = nextDepartments;
        positions.value = nextPositions;
        permissions.value = nextPermissions;
        templates.value = nextTemplates;
        if (!selectedPositionId.value && nextPositions.length > 0) {
          await selectPosition(nextPositions[0].id);
        }
      } catch (err) {
        setError(err instanceof Error ? err.message : "加载权限数据失败");
      }
    };

    const createDepartment = async () => {
      if (!departmentName.value) {
        setError("部门名称不能为空");
        return;
      }
      await superAdminApi.createRbacDepartment({
        name: departmentName.value,
        branch_id: departmentBranchId.value > 0 ? departmentBranchId.value : undefined,
      });
      departmentName.value = "";
      setNotice("部门已创建");
      await loadAll();
    };

    const createPosition = async () => {
      if (!positionForm.department_id || !positionForm.name) {
        setError("请选择部门并填写职位名称");
        return;
      }
      await superAdminApi.createRbacPosition({ ...positionForm });
      positionForm.name = "";
      setNotice("职位已创建");
      await loadAll();
    };

    const selectPosition = async (positionId: number) => {
      selectedPositionId.value = positionId;
      selectedPermissions.value =
        await superAdminApi.getPositionPermissions(positionId);
    };

    const togglePermission = (permission: string) => {
      selectedPermissions.value = selectedPermissions.value.includes(permission)
        ? selectedPermissions.value.filter((item) => item !== permission)
        : [...selectedPermissions.value, permission];
    };

    const savePositionPermissions = async () => {
      if (!selectedPositionId.value) return;
      savingPermissions.value = true;
      try {
        await superAdminApi.updatePositionPermissions(
          selectedPositionId.value,
          selectedPermissions.value
        );
        setNotice("职位权限已保存");
      } catch (err) {
        setError(err instanceof Error ? err.message : "保存职位权限失败");
      } finally {
        savingPermissions.value = false;
      }
    };

    const applyTemplate = async (templateId: number) => {
      if (!selectedPositionId.value) return;
      await superAdminApi.applyPermissionTemplate(selectedPositionId.value, templateId);
      await selectPosition(selectedPositionId.value);
      setNotice("权限模板已应用");
    };

    const searchUsers = async () => {
      const result = await superAdminApi.searchUsers({
        keyword: userKeyword.value,
        role: "all",
        page: 1,
        pageSize: 20,
        includeCounts: false,
      });
      users.value = result.items;
    };

    const selectUser = async (userId: number) => {
      selectedUserId.value = userId;
      const user = users.value.find((item) => item.id === userId);
      selectedUserPositionId.value = user?.type_id ?? 0;
      const scopes = await superAdminApi.getUserScopes(userId);
      selectedScopeBranchIds.value = scopes.branch_ids;
      selectedScopeDepartmentIds.value = scopes.department_ids;
    };

    const saveUserPosition = async () => {
      if (!selectedUserId.value) return;
      await superAdminApi.updateUserPosition(
        selectedUserId.value,
        selectedUserPositionId.value > 0 ? selectedUserPositionId.value : null
      );
      setNotice("用户岗位已保存");
      await searchUsers();
    };

    const toggleDepartmentScope = (departmentId: number) => {
      selectedScopeDepartmentIds.value = selectedScopeDepartmentIds.value.includes(
        departmentId
      )
        ? selectedScopeDepartmentIds.value.filter((item) => item !== departmentId)
        : [...selectedScopeDepartmentIds.value, departmentId];
    };

    const toggleBranchScope = (branchId: number) => {
      selectedScopeBranchIds.value = selectedScopeBranchIds.value.includes(branchId)
        ? selectedScopeBranchIds.value.filter((item) => item !== branchId)
        : [...selectedScopeBranchIds.value, branchId];
    };

    const saveUserScopes = async () => {
      if (!selectedUserId.value) return;
      await superAdminApi.updateUserScopes(selectedUserId.value, {
        branch_ids: selectedScopeBranchIds.value,
        department_ids: selectedScopeDepartmentIds.value,
      });
      setNotice("用户组织范围已保存");
    };

    onMounted(() => {
      void loadAll();
      void searchUsers();
    });

    return {
      departments,
      positions,
      permissions,
      templates,
      branches,
      users,
      selectedPermissions,
      selectedScopeBranchIds,
      selectedScopeDepartmentIds,
      selectedPositionId,
      selectedPosition,
      selectedUserId,
      selectedUserPositionId,
      departmentBranchId,
      departmentName,
      userKeyword,
      positionForm,
      savingPermissions,
      error,
      message,
      loadAll,
      createDepartment,
      createPosition,
      selectPosition,
      togglePermission,
      savePositionPermissions,
      applyTemplate,
      searchUsers,
      selectUser,
      saveUserPosition,
      toggleBranchScope,
      toggleDepartmentScope,
      saveUserScopes,
    };
  },
});
</script>

<style scoped>
.rbac-page {
  gap: 14px;
  padding: 18px;
}

.rbac-head,
.panel-title,
.lp-head {
  align-items: center;
  display: flex;
  justify-content: space-between;
}

.rbac-head h2 {
  font-size: 22px;
  margin: 2px 0 0;
}

.section-label {
  color: #64748b;
  font-size: 12px;
  font-weight: 700;
  letter-spacing: 0;
  margin: 0;
}

.rbac-grid {
  display: grid;
  gap: 14px;
  grid-template-columns: 320px minmax(0, 1fr);
  min-height: 0;
}

.rbac-panel {
  min-height: 0;
  overflow: hidden;
}

.rbac-panel--wide {
  grid-column: 1 / -1;
}

.panel-title {
  border-bottom: 1px solid #e2e8f0;
  font-size: 15px;
  font-weight: 800;
  margin: -2px 0 12px;
  padding-bottom: 10px;
}

.panel-title span {
  color: #475569;
  font-size: 13px;
  font-weight: 700;
}

.inline-form,
.form-row,
.user-toolbar {
  display: grid;
  gap: 8px;
}

.inline-form {
  grid-template-columns: 120px minmax(0, 1fr) auto;
}

.form-row,
.user-toolbar {
  grid-template-columns: minmax(0, 1fr) auto;
}

.stack-form {
  display: grid;
  gap: 8px;
  margin-top: 10px;
}

input,
select {
  background: #fff;
  border: 1px solid #cbd5e1;
  border-radius: 6px;
  color: #0f172a;
  height: 36px;
  min-width: 0;
  padding: 0 10px;
}

.position-list,
.permission-list,
.user-list,
.scope-checks {
  display: grid;
  gap: 8px;
  margin-top: 12px;
  max-height: 360px;
  overflow: auto;
}

.position-item,
.user-item,
.permission-item {
  align-items: center;
  background: #fff;
  border: 1px solid #e2e8f0;
  border-radius: 6px;
  color: #0f172a;
  display: flex;
  gap: 8px;
  min-height: 38px;
  padding: 8px 10px;
  text-align: left;
}

.position-item,
.user-item {
  justify-content: space-between;
}

.position-item small,
.user-item span {
  color: #64748b;
  font-size: 12px;
}

.position-item--active,
.user-item--active {
  border-color: #2563eb;
  box-shadow: inset 3px 0 0 #2563eb;
}

.permission-tools {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}

.permission-list {
  grid-template-columns: repeat(auto-fill, minmax(210px, 1fr));
}

.permission-item {
  justify-content: flex-start;
}

.permission-item input {
  height: 16px;
  width: 16px;
}

.permission-save {
  margin-top: 12px;
}

.user-scope-layout {
  display: grid;
  gap: 14px;
  grid-template-columns: minmax(280px, 420px) minmax(0, 1fr);
  margin-top: 12px;
}

.scope-editor {
  display: grid;
  gap: 12px;
}

.scope-editor label {
  display: grid;
  gap: 6px;
}

.scope-editor label > span {
  color: #475569;
  font-size: 13px;
  font-weight: 700;
}

.scope-group-title {
  color: #475569;
  font-size: 13px;
  font-weight: 800;
}

.state-banner {
  background: #eff6ff;
  border: 1px solid #bfdbfe;
  border-radius: 6px;
  color: #1e3a8a;
  padding: 10px 12px;
}

.state-banner--error {
  background: #fef2f2;
  border-color: #fecaca;
  color: #991b1b;
}

@media (max-width: 980px) {
  .rbac-grid,
  .user-scope-layout {
    grid-template-columns: 1fr;
  }
}
</style>
