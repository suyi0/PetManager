<template>
  <section class="page">
    <div class="panel form-panel">
      <h3>医生权限管理</h3>
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
      <div class="actions">
        <button class="primary" @click="grantDoctor">授予医生权限</button>
        <button class="danger" @click="revokeDoctor">移除医生权限</button>
      </div>
      <p class="message">{{ message }}</p>
    </div>

    <div class="panel user-panel">
      <div class="user-panel__head">
        <div>
          <h3>用户列表</h3>
          <p>点击卡片可快速填入上方用户 ID。</p>
        </div>
        <div class="toolbar">
          <input
            v-model.trim="keyword"
            class="search-input"
            type="text"
            placeholder="搜索用户名 / 邮箱 / 手机号"
          />
          <button class="ghost" @click="loadUsers">刷新列表</button>
          <div class="pager">
            <button class="ghost" :disabled="page <= 1" @click="page -= 1">
              上一页
            </button>
            <span>{{ page }} / {{ totalPages }}</span>
            <button
              class="ghost"
              :disabled="page >= totalPages"
              @click="page += 1"
            >
              下一页
            </button>
          </div>
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
            <span class="user-card__role" :class="roleClassName(item.type_id)">
              {{ formatRole(item.type_id) }}
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
            <span class="user-card__role" :class="roleClassName(item.type_id)">
              {{ formatRole(item.type_id) }}
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
import { superAdminApi } from "../../api/superAdminApi";
import { UserRow } from "../../api/types";

export default defineComponent({
  name: "SuperAdminDoctors",
  setup() {
    const userID = ref<number | null>(null);
    const message = ref("等待操作");
    const users = ref<UserRow[]>([]);
    const page = ref(1);
    const pageSize = 8;
    const keyword = ref("");

    const ensureUserID = () => {
      if (!userID.value || userID.value <= 0) {
        throw new Error("请先输入合法用户ID");
      }
      return userID.value;
    };

    const formatRole = (typeId: number) => {
      if (typeId === 1) return "超级管理员";
      if (typeId === 2) return "医生";
      return "普通用户";
    };

    const roleClassName = (typeId: number) => {
      if (typeId === 1) return "user-card__role--super";
      if (typeId === 2) return "user-card__role--doctor";
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
          .filter(Boolean)
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
      users.value = await superAdminApi.getUsers();
      if (page.value > totalPages.value) {
        page.value = totalPages.value;
      }
    };

    watch(keyword, () => {
      page.value = 1;
    });

    const selectUser = (id: number) => {
      userID.value = id;
      message.value = `已选择用户 #${id}`;
    };

    const grantDoctor = async () => {
      try {
        await superAdminApi.createDoctor(ensureUserID());
        message.value = "授予成功";
        await loadUsers();
      } catch (err: unknown) {
        message.value = `授予失败: ${String((err as Error).message || err)}`;
      }
    };

    const revokeDoctor = async () => {
      try {
        await superAdminApi.deleteDoctor(ensureUserID());
        message.value = "移除成功";
        await loadUsers();
      } catch (err: unknown) {
        message.value = `移除失败: ${String((err as Error).message || err)}`;
      }
    };

    onMounted(loadUsers);

    return {
      userID,
      page,
      keyword,
      totalPages,
      leftColumn,
      rightColumn,
      leftPlaceholders,
      rightPlaceholders,
      message,
      formatRole,
      roleClassName,
      loadUsers,
      selectUser,
      grantDoctor,
      revokeDoctor,
    };
  },
});
</script>

<style scoped>
.page {
  display: grid;
  gap: 16px;
}

.panel {
  border: 1px solid #dce7ff;
  border-radius: 16px;
  background: #fff;
  padding: 16px;
  box-sizing: border-box;
}

.form-panel h3 {
  margin: 0;
}

.form-panel p {
  color: #5c6781;
}

.user-panel {
  display: grid;
  gap: 14px;
  min-height: 520px;
}

.user-panel__head {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 16px;
}

.user-panel__head h3,
.user-panel__head p {
  margin: 0;
}

.user-panel__head p {
  margin-top: 6px;
  color: #6a7693;
  font-size: 13px;
}

.row {
  display: grid;
  gap: 6px;
  max-width: 380px;
}

input {
  border: 1px solid #cfdcff;
  border-radius: 10px;
  padding: 10px 12px;
  font-size: 14px;
}

.actions {
  display: flex;
  gap: 10px;
  margin-top: 12px;
}

.pager {
  display: flex;
  align-items: center;
  gap: 10px;
  color: #576684;
  font-size: 13px;
  margin-left: auto;
}

.toolbar {
  display: flex;
  align-items: center;
  gap: 12px;
  flex-wrap: wrap;
  width: 100%;
  justify-content: flex-end;
}

.search-input {
  min-width: 280px;
  flex: 1 1 320px;
  max-width: 360px;
  border: 1px solid #cfdcff;
  border-radius: 10px;
  padding: 10px 12px;
  font-size: 13px;
  background: #fff;
}

button {
  border: 0;
  padding: 10px 14px;
  border-radius: 10px;
  color: #fff;
  cursor: pointer;
}

button.primary {
  background: #2f6ff3;
}

button.danger {
  background: #e24f4f;
}

button.ghost {
  background: #eaf1ff;
  color: #2f6ff3;
}

button:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.message {
  margin-top: 14px;
  font-size: 13px;
  color: #1d2e57;
}

.user-board {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 16px;
  min-height: 0;
  overflow: hidden;
}

.user-column {
  --user-grid: 54px 110px minmax(120px, 1fr) minmax(170px, 1.35fr);
  display: grid;
  grid-template-rows: auto repeat(4, minmax(0, 1fr));
  gap: 10px;
  min-height: 0;
  padding: 14px;
  border-radius: 14px;
  border: 1px solid #e4ecff;
  background: linear-gradient(180deg, #fbfcff, #f5f8ff);
}

.column-head {
  display: grid;
  grid-template-columns: var(--user-grid);
  gap: 10px;
  align-items: center;
  padding: 6px 10px 12px;
  border-bottom: 1px solid #e3ebff;
  color: #6d7da1;
  font-size: 12px;
  font-weight: 700;
}

.column-head__id,
.column-head__name,
.column-head__contact,
.user-card__name,
.user-card__contact,
.column-head__role {
  justify-self: center;
  text-align: center;
  width: 100%;
}

.user-card {
  display: grid;
  grid-template-columns: var(--user-grid);
  align-items: center;
  gap: 10px;
  min-height: 72px;
  padding: 10px 12px;
  border: 1px solid #d8e4ff;
  border-radius: 12px;
  background: #ffffff;
  color: #243454;
  text-align: left;
  box-shadow: 0 10px 24px rgba(53, 86, 150, 0.06);
  transition: transform 0.18s ease, box-shadow 0.18s ease,
    border-color 0.18s ease;
}

.user-card strong,
.user-card span {
  min-width: 0;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.user-card:hover {
  transform: translateY(-1px);
  box-shadow: 0 14px 28px rgba(53, 86, 150, 0.1);
}

.user-card.active {
  border-color: #78a4ff;
  background: linear-gradient(180deg, #eef4ff, #e3ecff);
  box-shadow: 0 16px 30px rgba(74, 122, 217, 0.16);
}

.user-card__id {
  display: inline-flex;
  align-items: center;
  color: #2f6ff3;
  font-weight: 700;
  letter-spacing: 0.02em;
  font-variant-numeric: tabular-nums;
  justify-self: center;
  text-align: center;
}

.user-card__role {
  font-size: 12px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  justify-self: center;
  min-width: 88px;
  padding: 5px 10px;
  border-radius: 999px;
  font-weight: 700;
  white-space: nowrap;
}

.user-card__role--super {
  background: #fff0da;
  color: #b56a00;
}

.user-card__role--doctor {
  background: #dff5eb;
  color: #1f8a61;
}

.user-card__role--user {
  background: #edf2ff;
  color: #4f67b5;
}

.user-card__name {
  font-size: 14px;
  color: #1f3052;
  font-weight: 700;
}

.user-card__contact {
  color: #607193;
  font-size: 13px;
}

.user-card--placeholder {
  visibility: hidden;
  pointer-events: none;
}

@media (max-width: 980px) {
  .user-panel {
    min-height: auto;
  }

  .user-panel__head {
    flex-direction: column;
  }

  .toolbar {
    width: 100%;
    justify-content: flex-start;
  }

  .search-input {
    min-width: 0;
    width: 100%;
    max-width: none;
    flex-basis: 100%;
  }

  .user-board {
    grid-template-columns: 1fr;
  }
}
</style>
