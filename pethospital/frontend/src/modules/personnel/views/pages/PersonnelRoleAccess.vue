<template>
  <section class="page">
    <div class="panel form-panel">
      <div class="form-panel__head">
        <div>
          <h3>人员权限管理</h3>
          <p>左侧管理医生权限，右侧管理仓库管理员权限。</p>
        </div>
        <div class="row">
          <label for="uid">用户ID</label>
          <input
            id="uid"
            v-model.number="userID"
            type="number"
            min="1"
            placeholder="输入用户ID"
          />
        </div>
      </div>

      <div class="grant-grid">
        <section class="grant-card">
          <div class="grant-card__badge">角色一</div>
          <h4>医生权限管理</h4>
          <p>适用于接诊、排班、创建订单等医生端功能授权。</p>
          <div class="actions">
            <button class="primary" @click="grantDoctor">授予医生权限</button>
            <button class="danger" @click="revokeDoctor">移除医生权限</button>
          </div>
        </section>

        <section class="grant-card grant-card--pending">
          <div class="grant-card__badge grant-card__badge--warehouse">
            角色二
          </div>
          <h4>仓库管理员授权</h4>
          <p>预留库存管理、药品维护、预警查看等仓库端功能授权。</p>
          <div class="actions">
            <button class="primary" @click="grantWarehouseAdmin">
              授予仓库管理员
            </button>
            <button class="danger" @click="revokeWarehouseAdmin">
              移除仓库管理员
            </button>
          </div>
          <span class="grant-card__tip"
            >当前页面已接入仓库管理员授权接口。</span
          >
        </section>
      </div>

      <p class="message">{{ message }}</p>
    </div>

    <div class="panel user-panel">
      <div class="user-panel__head">
        <div>
          <h3>用户列表</h3>
          <p>点击卡片可快速填入用户 ID。</p>
        </div>
        <div class="toolbar">
          <input
            v-model.trim="keywordInput"
            class="search-input"
            type="text"
            placeholder="搜索用户名 / 邮箱 / 手机号"
            @keyup.enter="applySearch"
          />
          <button class="ghost" @click="applySearch">搜索</button>
          <button class="ghost" @click="loadUsers">刷新列表</button>
          <AppPager
            :page="page"
            :total-pages="totalPages"
            @update:page="page = $event"
          />
        </div>
      </div>

      <div class="user-board">
        <section class="user-column">
          <header class="column-head">
            <span class="column-head__id">ID</span>
            <span class="column-head__role">职位</span>
            <span class="column-head__name">用户名</span>
            <span class="column-head__contact">邮箱/手机号</span>
          </header>
          <button
            v-for="item in leftColumn"
            :key="item.id"
            type="button"
            class="user-card"
            :class="{ active: userID === item.id }"
            @click="selectUser(item.id)"
          >
            <span class="user-card__id">#{{ item.id }}</span>
            <span
              class="user-card__role"
              :class="roleClassName(item.type_name, item.type_id)"
            >
              {{ formatRole(item.type_name, item.type_id) }}
            </span>
            <strong class="user-card__name">{{
              item.name || "未命名用户"
            }}</strong>
            <span class="user-card__contact">{{
              item.email || item.phone || "暂无联系方式"
            }}</span>
          </button>
          <div
            v-for="placeholder in leftPlaceholders"
            :key="`left-${placeholder}`"
            class="user-card user-card--placeholder"
          ></div>
        </section>

        <section class="user-column">
          <header class="column-head">
            <span class="column-head__id">ID</span>
            <span class="column-head__role">职位</span>
            <span class="column-head__name">用户名</span>
            <span class="column-head__contact">邮箱/手机号</span>
          </header>
          <button
            v-for="item in rightColumn"
            :key="item.id"
            type="button"
            class="user-card"
            :class="{ active: userID === item.id }"
            @click="selectUser(item.id)"
          >
            <span class="user-card__id">#{{ item.id }}</span>
            <span
              class="user-card__role"
              :class="roleClassName(item.type_name, item.type_id)"
            >
              {{ formatRole(item.type_name, item.type_id) }}
            </span>
            <strong class="user-card__name">{{
              item.name || "未命名用户"
            }}</strong>
            <span class="user-card__contact">{{
              item.email || item.phone || "暂无联系方式"
            }}</span>
          </button>
          <div
            v-for="placeholder in rightPlaceholders"
            :key="`right-${placeholder}`"
            class="user-card user-card--placeholder"
          ></div>
        </section>
      </div>
    </div>
  </section>
</template>

<script lang="ts">
import { computed, defineComponent, onMounted, ref, watch } from "vue";
import { useStore } from "vuex";
import {
  isSuperAdminPortalRole,
  resolveRoleName,
} from "@/core/auth/utils/roleUtils";
import { storeKey } from "@/app/store";
import AppPager from "@/shared/components/AppPager.vue";

export default defineComponent({
  name: "PersonnelRoleAccess",
  components: { AppPager },
  setup() {
    const store = useStore(storeKey);
    const userID = ref<number | null>(null);
    const message = ref("等待操作");
    const users = computed(() => store.state.personnel.users);
    const page = ref(1);
    const pageSize = 8;
    const keywordInput = ref("");
    const keyword = ref("");

    const ensureUserID = () => {
      if (!userID.value || userID.value <= 0) {
        throw new Error("请先输入合法用户ID");
      }
      return userID.value;
    };

    const formatRole = (typeName?: string, typeId?: number | null) => {
      return resolveRoleName(typeName, typeId) || "未知角色";
    };

    const roleClassName = (typeName?: string, typeId?: number | null) => {
      const role = formatRole(typeName, typeId);
      if (isSuperAdminPortalRole(role)) return "user-card__role--super";
      if (role === "医生" || role === "护士") return "user-card__role--doctor";
      if (role === "仓库管理员") return "user-card__role--warehouse";
      return "user-card__role--user";
    };

    const manageableUsers = computed(() => {
      const source = users.value;
      const search = keyword.value.toLowerCase();

      if (!search) {
        return source;
      }

      return source.filter((item) =>
        [item.name, item.email, item.phone]
          .filter((field): field is string => typeof field === "string")
          .some((field) => field.toLowerCase().includes(search))
      );
    });

    const totalPages = computed(() =>
      Math.max(1, Math.ceil(manageableUsers.value.length / pageSize))
    );

    const pagedUsers = computed(() => {
      const start = (page.value - 1) * pageSize;
      return manageableUsers.value.slice(start, start + pageSize);
    });

    const leftColumn = computed(() => pagedUsers.value.slice(0, 4));
    const rightColumn = computed(() => pagedUsers.value.slice(4, 8));
    const leftPlaceholders = computed(() =>
      Array.from(
        { length: Math.max(0, 4 - leftColumn.value.length) },
        (_, i) => i
      )
    );
    const rightPlaceholders = computed(() =>
      Array.from(
        { length: Math.max(0, 4 - rightColumn.value.length) },
        (_, i) => i
      )
    );

    const loadUsers = async () => {
      await store.dispatch("personnel/ensureUsers", { force: true });
      if (page.value > totalPages.value) {
        page.value = totalPages.value;
      }
    };

    watch(keyword, () => {
      page.value = 1;
    });

    const applySearch = () => {
      keyword.value = keywordInput.value.trim();
    };

    const selectUser = (id: number) => {
      userID.value = id;
      message.value = `已选择用户 #${id}`;
    };

    const grantDoctor = async () => {
      try {
        await store.dispatch("personnel/createDoctor", ensureUserID());
        message.value = "医生权限授予成功";
      } catch (error) {
        message.value =
          error instanceof Error ? error.message : "医生权限授予失败";
      }
    };

    const revokeDoctor = async () => {
      try {
        await store.dispatch("personnel/deleteDoctor", ensureUserID());
        message.value = "医生权限移除成功";
      } catch (error) {
        message.value =
          error instanceof Error ? error.message : "医生权限移除失败";
      }
    };

    const grantWarehouseAdmin = async () => {
      try {
        await store.dispatch(
          "personnel/createWarehouseManager",
          ensureUserID()
        );
        message.value = "仓库管理员授权成功";
      } catch (error) {
        message.value =
          error instanceof Error ? error.message : "仓库管理员授权失败";
      }
    };

    const revokeWarehouseAdmin = async () => {
      try {
        await store.dispatch(
          "personnel/deleteWarehouseManager",
          ensureUserID()
        );
        message.value = "仓库管理员权限移除成功";
      } catch (error) {
        message.value =
          error instanceof Error ? error.message : "仓库管理员权限移除失败";
      }
    };

    onMounted(() => {
      void loadUsers();
    });

    return {
      applySearch,
      formatRole,
      grantDoctor,
      grantWarehouseAdmin,
      keywordInput,
      leftColumn,
      leftPlaceholders,
      loadUsers,
      message,
      page,
      revokeDoctor,
      revokeWarehouseAdmin,
      rightColumn,
      rightPlaceholders,
      roleClassName,
      selectUser,
      totalPages,
      userID,
    };
  },
});
</script>

<style scoped>
.page {
  display: grid;
  gap: 20px;
}

.panel {
  background: linear-gradient(180deg, rgba(255, 255, 255, 0.96), #f8fbff);
  border: 1px solid rgba(168, 202, 255, 0.45);
  border-radius: 28px;
  padding: 22px 24px;
  box-shadow: 0 18px 40px rgba(100, 132, 181, 0.12);
}

.form-panel {
  display: grid;
  gap: 20px;
}

.form-panel__head,
.user-panel__head {
  display: flex;
  justify-content: space-between;
  gap: 18px;
  align-items: flex-start;
}

.form-panel__head h3,
.user-panel__head h3 {
  margin: 0;
  font-size: 24px;
  color: #1f3560;
}

.form-panel__head p,
.user-panel__head p {
  margin: 8px 0 0;
  color: #6680a8;
}

.row {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 12px 16px;
  border-radius: 18px;
  background: rgba(233, 242, 255, 0.85);
}

.row label {
  font-size: 14px;
  color: #53709b;
}

.row input,
.search-input {
  border: none;
  background: transparent;
  outline: none;
  color: #1f3560;
  font-size: 15px;
}

.grant-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 18px;
}

.grant-card {
  position: relative;
  padding: 22px;
  border-radius: 24px;
  background: linear-gradient(180deg, rgba(236, 245, 255, 0.95), #ffffff);
  border: 1px solid rgba(146, 193, 255, 0.42);
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.7);
}

.grant-card--pending {
  background: linear-gradient(180deg, rgba(242, 249, 255, 0.95), #ffffff);
}

.grant-card__badge {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 72px;
  padding: 6px 12px;
  border-radius: 999px;
  background: rgba(88, 152, 255, 0.14);
  color: #3e70c0;
  font-size: 13px;
  font-weight: 700;
}

.grant-card__badge--warehouse {
  background: rgba(90, 178, 152, 0.14);
  color: #2b8c70;
}

.grant-card h4 {
  margin: 16px 0 8px;
  font-size: 20px;
  color: #1d3158;
}

.grant-card p {
  margin: 0;
  color: #6b83ab;
  line-height: 1.6;
}

.actions {
  display: flex;
  gap: 12px;
  margin-top: 22px;
}

.primary,
.danger,
.ghost {
  border: none;
  border-radius: 16px;
  padding: 11px 18px;
  font-size: 14px;
  font-weight: 700;
  cursor: pointer;
  transition: transform 0.2s ease, box-shadow 0.2s ease;
}

.primary {
  color: #fff;
  background: linear-gradient(135deg, #5f8dff, #6fb9ff);
  box-shadow: 0 12px 24px rgba(90, 136, 255, 0.22);
}

.danger {
  color: #b34a59;
  background: rgba(255, 230, 235, 0.9);
}

.ghost {
  color: #41658f;
  background: rgba(232, 241, 255, 0.86);
}

.primary:hover,
.danger:hover,
.ghost:hover,
.user-card:hover {
  transform: translateY(-1px);
}

.grant-card__tip,
.message {
  color: #6782aa;
  font-size: 14px;
}

.toolbar {
  display: flex;
  align-items: center;
  gap: 12px;
  flex-wrap: wrap;
}

.search-input {
  min-width: 260px;
  padding: 12px 16px;
  border-radius: 16px;
  background: rgba(233, 242, 255, 0.85);
}

.user-panel {
  display: grid;
  gap: 18px;
}

.user-board {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 18px;
}

.user-column {
  display: grid;
  gap: 12px;
}

.column-head,
.user-card {
  display: grid;
  grid-template-columns: 90px 110px 1fr 1.25fr;
  gap: 12px;
  align-items: center;
}

.column-head {
  padding: 0 12px;
  color: #6d85ac;
  font-size: 13px;
}

.user-card {
  border: 1px solid rgba(166, 201, 255, 0.3);
  border-radius: 20px;
  background: linear-gradient(180deg, rgba(244, 248, 255, 0.96), #ffffff);
  padding: 16px 14px;
  cursor: pointer;
  text-align: left;
  box-shadow: 0 12px 26px rgba(108, 138, 184, 0.1);
}

.user-card.active {
  border-color: rgba(93, 146, 255, 0.72);
  box-shadow: 0 16px 28px rgba(93, 146, 255, 0.16);
}

.user-card--placeholder {
  min-height: 74px;
  background: rgba(244, 248, 255, 0.45);
  box-shadow: none;
  cursor: default;
}

.user-card__id {
  font-weight: 700;
  color: #4e678f;
}

.user-card__role {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  padding: 6px 10px;
  border-radius: 999px;
  font-size: 12px;
  font-weight: 700;
}

.user-card__role--super {
  color: #3259b5;
  background: rgba(100, 139, 255, 0.14);
}

.user-card__role--doctor {
  color: #2e8e7d;
  background: rgba(102, 206, 178, 0.14);
}

.user-card__role--warehouse {
  color: #9a6a16;
  background: rgba(255, 201, 96, 0.16);
}

.user-card__role--user {
  color: #5d769d;
  background: rgba(206, 221, 243, 0.52);
}

.user-card__name {
  color: #20365e;
}

.user-card__contact {
  color: #6d84a9;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

@media (max-width: 1280px) {
  .grant-grid,
  .user-board {
    grid-template-columns: 1fr;
  }
}

@media (max-width: 860px) {
  .form-panel__head,
  .user-panel__head {
    flex-direction: column;
  }

  .toolbar {
    width: 100%;
  }

  .search-input {
    min-width: 0;
    width: 100%;
  }

  .column-head,
  .user-card {
    grid-template-columns: 76px 100px 1fr;
  }

  .column-head__contact,
  .user-card__contact {
    grid-column: 1 / -1;
  }
}
</style>
