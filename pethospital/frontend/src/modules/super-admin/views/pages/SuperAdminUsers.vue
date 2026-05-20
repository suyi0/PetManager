<template>
  <section class="page">
    <div class="hero panel">
      <div>
        <p class="eyebrow">User Directory</p>
        <h2>角色分组用户中心</h2>
      </div>
      <div class="hero__actions">
        <div class="hero__actions-first">
          <button class="ghost" @click="refreshUsers">刷新列表</button>
          <button @click="openCreateDialog">创建用户</button>
        </div>
        <div class="hero__actions-second">
          <input
            v-model.trim="keywordInput"
            class="search-input"
            type="text"
            placeholder="搜索用户名 / 邮箱 / 手机号"
            @keyup.enter="applySearch"
          />
          <button class="ghost" @click="applySearch">搜索</button>
        </div>
      </div>
      <div class="hero_card">
        <article
          v-for="card in heroCards"
          :key="card.key"
          class="hero-card-item"
          :class="card.tone"
        >
          <div class="hero-card-item__copy">
            <p>{{ card.eyebrow }}</p>
            <strong>{{ card.label }}</strong>
            <span>{{ card.count }} 人</span>
          </div>
          <div class="hero-card-item__art">
            <img :src="card.image" :alt="card.label" />
          </div>
        </article>
      </div>
    </div>

    <div class="lists-grid">
      <section class="panel role-panel">
        <header class="role-panel__head">
          <div>
            <p class="role-panel__eyebrow">Role 01</p>
            <h3>普通用户</h3>
          </div>
          <span class="role-panel__count"
            >{{ filteredNormalUsers.length }} 人</span
          >
        </header>
        <div class="role-table-list">
          <button
            v-for="item in pagedNormalUsers"
            :key="item.id"
            type="button"
            class="role-row"
            @click="goToDetail(item.id)"
          >
            <strong class="role-row__id">#{{ item.id }}</strong>
            <span class="role-pill role-pill--user">普通用户</span>
            <strong class="role-row__name">{{
              item.name || "未命名用户"
            }}</strong>
            <span class="role-row__contact">
              {{ item.email || item.phone || "暂无联系方式" }}
            </span>
          </button>
          <div v-if="pagedNormalUsers.length === 0" class="empty-card">
            当前没有普通用户记录
          </div>
        </div>
        <AppPager
          :page="normalPage"
          :total-pages="normalTotalPages"
          @update:page="normalPage = $event"
        />
      </section>

      <section class="panel role-panel">
        <header class="role-panel__head">
          <div>
            <p class="role-panel__eyebrow">Role 02</p>
            <h3>医护人员</h3>
          </div>
          <span class="role-panel__count"
            >{{ filteredDoctorUsers.length }} 人</span
          >
        </header>
        <div class="role-table-list">
          <button
            v-for="item in pagedDoctorUsers"
            :key="item.id"
            type="button"
            class="role-row role-row--doctor"
            @click="goToDetail(item.id)"
          >
            <strong class="role-row__id">#{{ item.id }}</strong>
            <span class="role-pill role-pill--doctor">
              {{ formatRole(item) }}
            </span>
            <strong class="role-row__name">{{
              item.name || "未命名医护人员"
            }}</strong>
            <span class="role-row__contact">
              {{ item.email || item.phone || "暂无联系方式" }}
            </span>
            <span
              class="status-pill"
              :class="
                item.status === 'online'
                  ? 'status-pill--online'
                  : 'status-pill--offline'
              "
            >
              {{ item.status === "online" ? "在线" : "离线" }}
            </span>
          </button>
          <div v-if="pagedDoctorUsers.length === 0" class="empty-card">
            当前没有医护人员记录
          </div>
        </div>
        <AppPager
          :page="doctorPage"
          :total-pages="doctorTotalPages"
          @update:page="doctorPage = $event"
        />
      </section>

      <section class="panel role-panel">
        <header class="role-panel__head">
          <div>
            <p class="role-panel__eyebrow">Role 03</p>
            <h3>管理员</h3>
          </div>
          <span class="role-panel__count"
            >{{ filteredAdminUsers.length }} 人</span
          >
        </header>
        <div class="role-table-list">
          <button
            v-for="item in pagedAdminUsers"
            :key="item.id"
            type="button"
            class="role-row"
            @click="goToDetail(item.id)"
          >
            <strong class="role-row__id">#{{ item.id }}</strong>
            <span
              class="role-pill"
              :class="
                isSuperAdminPortalRole(formatRole(item))
                  ? 'role-pill--super'
                  : 'role-pill--admin'
              "
            >
              {{ formatRole(item) }}
            </span>
            <strong class="role-row__name">{{
              item.name || "未命名管理员"
            }}</strong>
            <span class="role-row__contact">
              {{ item.email || item.phone || "暂无联系方式" }}
            </span>
          </button>
          <div v-if="pagedAdminUsers.length === 0" class="empty-card">
            当前没有管理员记录
          </div>
        </div>
        <AppPager
          :page="adminPage"
          :total-pages="adminTotalPages"
          @update:page="adminPage = $event"
        />
      </section>
    </div>

    <div
      v-if="showCreateDialog"
      class="dialog-backdrop"
      @click.self="closeCreateDialog"
    >
      <div class="dialog">
        <div class="dialog__head">
          <div>
            <p class="eyebrow">Create User</p>
            <h3>创建普通用户</h3>
          </div>
          <button class="ghost" @click="closeCreateDialog">关闭</button>
        </div>

        <form class="form" @submit.prevent="handleCreate">
          <label>
            <span>姓名</span>
            <input
              v-model.trim="form.name"
              type="text"
              placeholder="请输入姓名"
            />
          </label>
          <label>
            <span>手机号</span>
            <input
              v-model.trim="form.phone"
              type="text"
              placeholder="选填，建议填写"
            />
          </label>
          <label>
            <span>邮箱</span>
            <input
              v-model.trim="form.email"
              type="email"
              placeholder="选填，建议填写"
            />
          </label>
          <label>
            <span>初始密码</span>
            <input
              v-model.trim="form.password"
              type="text"
              placeholder="留空则默认 123456"
            />
          </label>
          <label>
            <span>生日</span>
            <input v-model="form.birthday" type="date" />
          </label>
          <label>
            <span>地址ID</span>
            <input v-model.number="form.address_id" type="number" min="0" />
          </label>

          <p v-if="formError" class="form-error">{{ formError }}</p>

          <div class="dialog__actions">
            <button class="ghost" type="button" @click="closeCreateDialog">
              取消
            </button>
            <button :disabled="creating">
              {{ creating ? "创建中..." : "确认创建" }}
            </button>
          </div>
        </form>
      </div>
    </div>
  </section>
</template>

<script lang="ts">
import {
  computed,
  defineComponent,
  onMounted,
  reactive,
  ref,
  watch,
} from "vue";
import { useStore } from "vuex";
import { useRouter } from "vue-router";
import {
  isSuperAdminPortalRole,
  resolveRoleName,
} from "@/core/auth/utils/roleUtils";
import { storeKey } from "@/app/store";
import AppPager from "@/shared/components/AppPager.vue";
import { superAdminApi } from "../../api/superAdminApi";
import { UserRow } from "../../api/types";
import allUsersIllustration from "@/assets/photo/super-admin-users-all.svg";
import normalUsersIllustration from "@/assets/photo/super-admin-users-normal.svg";
import doctorUsersIllustration from "@/assets/photo/super-admin-users-doctor.svg";
import adminUsersIllustration from "@/assets/photo/super-admin-users-admin.svg";

export default defineComponent({
  name: "SuperAdminUsers",
  components: { AppPager },
  setup() {
    const getErrorDetails = (error: unknown) => {
      const responseError = error as {
        response?: {
          data?: {
            error?: {
              details?: string;
            };
          };
        };
      };

      return responseError.response?.data?.error?.details;
    };

    const store = useStore(storeKey);
    const showCreateDialog = ref(false);
    const creating = ref(false);
    const formError = ref("");
    const keywordInput = ref("");
    const normalPage = ref(1);
    const doctorPage = ref(1);
    const adminPage = ref(1);
    const pageSize = 10;
    const router = useRouter();
    const users = computed<UserRow[]>(() => store.state.superAdmin.users);
    const form = reactive({
      name: "",
      phone: "",
      email: "",
      password: "",
      birthday: "",
      address_id: 0,
    });

    /**
     * 获取用户角色类型
     * @param user 用户数据类型
     * @returns 角色类型名称
     */
    const formatRole = (user: UserRow) =>
      resolveRoleName(user.type_name, user.type_id) || "未知角色";

    const sortedById = (list: UserRow[]) =>
      [...list].sort((a, b) => a.id - b.id);

    const normalUsers = computed(() =>
      sortedById(users.value.filter((item) => formatRole(item) === "普通用户"))
    );

    const doctorUsers = computed(() =>
      [...users.value]
        .filter((item) => {
          const role = formatRole(item);
          return role === "医生" || role === "护士";
        })
        .sort((a, b) => {
          const aOnline = a.status === "online" ? 1 : 0;
          const bOnline = b.status === "online" ? 1 : 0;
          if (aOnline !== bOnline) {
            return bOnline - aOnline;
          }
          return a.id - b.id;
        })
    );

    const adminUsers = computed(() =>
      sortedById(
        users.value.filter((item) => {
          const role = formatRole(item);
          return isSuperAdminPortalRole(role) || role === "仓库管理员";
        })
      )
    );

    const heroCards = computed(() => [
      {
        key: "all",
        eyebrow: "All Users",
        label: "全部用户",
        count: users.value.length,
        image: allUsersIllustration,
        tone: "hero-card-item--all",
      },
      {
        key: "normal",
        eyebrow: "General",
        label: "普通用户",
        count: normalUsers.value.length,
        image: normalUsersIllustration,
        tone: "hero-card-item--normal",
      },
      {
        key: "doctor",
        eyebrow: "Medical Staff",
        label: "医护人员",
        count: doctorUsers.value.length,
        image: doctorUsersIllustration,
        tone: "hero-card-item--doctor",
      },
      {
        key: "admin",
        eyebrow: "Admins",
        label: "管理员",
        count: adminUsers.value.length,
        image: adminUsersIllustration,
        tone: "hero-card-item--admin",
      },
    ]);

    const matchesKeyword = (item: UserRow) => {
      const search = keywordInput.value.trim().toLowerCase();
      if (!search) return true;

      return [item.name, item.email, item.phone]
        .filter(Boolean)
        .some((field) => field.toLowerCase().includes(search));
    };

    const filteredNormalUsers = computed(() =>
      normalUsers.value.filter(matchesKeyword)
    );

    const filteredDoctorUsers = computed(() =>
      doctorUsers.value.filter(matchesKeyword)
    );

    const filteredAdminUsers = computed(() =>
      adminUsers.value.filter(matchesKeyword)
    );

    const normalTotalPages = computed(() =>
      Math.max(1, Math.ceil(filteredNormalUsers.value.length / pageSize))
    );

    const doctorTotalPages = computed(() =>
      Math.max(1, Math.ceil(filteredDoctorUsers.value.length / pageSize))
    );

    const adminTotalPages = computed(() =>
      Math.max(1, Math.ceil(filteredAdminUsers.value.length / pageSize))
    );

    const pagedNormalUsers = computed(() => {
      const start = (normalPage.value - 1) * pageSize;
      return filteredNormalUsers.value.slice(start, start + pageSize);
    });

    const pagedDoctorUsers = computed(() => {
      const start = (doctorPage.value - 1) * pageSize;
      return filteredDoctorUsers.value.slice(start, start + pageSize);
    });

    const pagedAdminUsers = computed(() => {
      const start = (adminPage.value - 1) * pageSize;
      return filteredAdminUsers.value.slice(start, start + pageSize);
    });

    /**
     * 加载用户列表
     */
    const loadUsers = async () => {
      // 优先读全局缓存，只有脏数据、超时或首次进入时才会访问接口。
      await store.dispatch("superAdmin/ensureUsers");
      normalPage.value = Math.min(normalPage.value, normalTotalPages.value);
      doctorPage.value = Math.min(doctorPage.value, doctorTotalPages.value);
      adminPage.value = Math.min(adminPage.value, adminTotalPages.value);
    };

    /**
     * 监听搜索词
     */
    watch(keywordInput, () => {
      normalPage.value = 1;
      doctorPage.value = 1;
      adminPage.value = 1;
    });

    /**
     * 搜索
     */
    const applySearch = () => {
      normalPage.value = 1;
      doctorPage.value = 1;
      adminPage.value = 1;
    };

    /**
     * 刷新
     */
    const refreshUsers = async () => {
      keywordInput.value = "";
      normalPage.value = 1;
      doctorPage.value = 1;
      adminPage.value = 1;
      // 显式刷新时跳过缓存，直接请求最新数据。
      await store.dispatch("superAdmin/refreshUsers");
    };

    /**
     * 重置表单
     */
    const resetForm = () => {
      form.name = "";
      form.phone = "";
      form.email = "";
      form.password = "";
      form.birthday = "";
      form.address_id = 0;
      formError.value = "";
    };

    /**
     * 打开创建用户页面
     */
    const openCreateDialog = () => {
      resetForm();
      showCreateDialog.value = true;
    };

    /**
     * 关闭创建用户页面
     */
    const closeCreateDialog = () => {
      showCreateDialog.value = false;
    };

    /**
     * 跳转到用户详情页面
     */
    const goToDetail = (userId: number) => {
      void router.push({ name: "superAdminUserDetail", params: { userId } });
    };

    /**
     * 创建用户
     */
    const handleCreate = async () => {
      if (!form.name) {
        formError.value = "请先填写姓名";
        return;
      }

      if (!form.phone && !form.email) {
        formError.value = "手机号和邮箱至少填写一项";
        return;
      }

      creating.value = true;
      formError.value = "";

      try {
        await superAdminApi.createUser({
          name: form.name,
          phone: form.phone || undefined,
          email: form.email || undefined,
          password: form.password || undefined,
          birthday: form.birthday || undefined,
          address_id: form.address_id || 0,
        });
        // 创建成功后，用户列表和日志都已经过期，后续页面会读到新数据。
        store.commit("superAdmin/markUsersDirty");
        store.commit("superAdmin/markLogsDirty");
        store.commit("superAdmin/markHomePageDataDirty");
        closeCreateDialog();
        await store.dispatch("superAdmin/refreshUsers");
      } catch (error: unknown) {
        formError.value = getErrorDetails(error) || "创建失败，请稍后重试";
      } finally {
        creating.value = false;
      }
    };

    onMounted(() => {
      void loadUsers();
    });

    return {
      users,
      normalUsers,
      doctorUsers,
      adminUsers,
      heroCards,
      filteredNormalUsers,
      filteredDoctorUsers,
      filteredAdminUsers,
      pagedNormalUsers,
      pagedDoctorUsers,
      pagedAdminUsers,
      normalPage,
      doctorPage,
      adminPage,
      normalTotalPages,
      doctorTotalPages,
      adminTotalPages,
      keywordInput,
      showCreateDialog,
      creating,
      form,
      formError,
      formatRole,
      isSuperAdminPortalRole,
      loadUsers,
      refreshUsers,
      applySearch,
      openCreateDialog,
      closeCreateDialog,
      handleCreate,
      goToDetail,
    };
  },
});
</script>

<style scoped>
.page {
  display: grid;
  gap: 18px;
}

.panel {
  border: 1px solid #dce7ff;
  border-radius: 24px;
  background: radial-gradient(
      circle at top left,
      rgba(47, 111, 243, 0.12),
      transparent 34%
    ),
    linear-gradient(180deg, #ffffff 0%, #f6f9ff 100%);
  padding: 20px;
  box-shadow: 0 22px 44px rgba(34, 64, 128, 0.08);
}

.hero {
  display: grid;
  grid-template-columns: minmax(0, 1fr) minmax(360px, 460px);
  gap: 24px;
  align-items: start;
}

.eyebrow {
  margin: 0 0 8px;
  font-size: 12px;
  letter-spacing: 0.24em;
  text-transform: uppercase;
  color: #6c7a9f;
}

.hero h2 {
  margin: 0;
  font-size: 30px;
  color: #13203a;
}

.hero__copy {
  max-width: 720px;
  margin: 10px 0 0;
  color: #617196;
  line-height: 1.7;
}

.hero__actions {
  display: grid;
  align-content: start;
  gap: 12px;

  .hero__actions-first {
    display: flex;
    justify-content: space-between;
  }

  .hero__actions-second {
    display: grid;
    grid-template-columns: minmax(0, 1fr) minmax(0, 0.4fr);
    gap: 12px;
  }
}

.hero_card {
  grid-column: 1 / -1;
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 14px;
}

.hero-card-item {
  position: relative;
  overflow: hidden;
  display: grid;
  grid-template-columns: minmax(0, 1fr) 96px;
  gap: 12px;
  align-items: center;
  min-height: 134px;
  padding: 18px 18px 16px;
  border-radius: 22px;
  border: 1px solid rgba(98, 141, 226, 0.16);
  background: linear-gradient(180deg, rgba(255, 255, 255, 0.96), #f8fbff);
  box-shadow: 0 18px 36px rgba(54, 85, 150, 0.08);
}

.hero-card-item::after {
  content: "";
  position: absolute;
  inset: auto -18px -24px auto;
  width: 110px;
  height: 110px;
  border-radius: 999px;
  background: rgba(255, 255, 255, 0.34);
}

.hero-card-item__copy {
  position: relative;
  z-index: 1;
  display: grid;
  gap: 6px;
}

.hero-card-item__copy p {
  margin: 0;
  color: #5d7fb7;
  letter-spacing: 0.18em;
  text-transform: uppercase;
  font-size: 11px;
  font-weight: 700;
}

.hero-card-item__copy strong {
  color: #18325d;
  font-size: 22px;
  line-height: 1.1;
}

.hero-card-item__copy span {
  color: #355e9c;
  font-size: 28px;
  font-weight: 800;
}

.hero-card-item__art {
  position: relative;
  z-index: 1;
  display: grid;
  place-items: center;
}

.hero-card-item__art img {
  width: 96px;
  height: 96px;
  object-fit: contain;
  filter: drop-shadow(0 14px 24px rgba(78, 119, 194, 0.18));
}

.hero-card-item--all {
  background: linear-gradient(
    135deg,
    rgba(239, 249, 255, 0.98),
    rgba(245, 248, 255, 0.96)
  );
}

.hero-card-item--normal {
  background: linear-gradient(
    135deg,
    rgba(241, 248, 255, 0.98),
    rgba(248, 250, 255, 0.96)
  );
}

.hero-card-item--doctor {
  background: linear-gradient(
    135deg,
    rgba(238, 252, 247, 0.98),
    rgba(243, 248, 255, 0.96)
  );
}

.hero-card-item--admin {
  background: linear-gradient(
    135deg,
    rgba(241, 246, 255, 0.98),
    rgba(248, 248, 255, 0.96)
  );
}

.search-input {
  width: min(340px, 100%);
  border: 1px solid #cfdbff;
  border-radius: 14px;
  padding: 11px 14px;
  background: #fbfcff;
  color: #1e2f56;
}

.lists-grid {
  display: grid;
  gap: 18px;
}

.role-panel {
  display: grid;
  grid-template-rows: auto 1fr auto;
  gap: 10px;
  height: 500px;
  min-height: 500px;
}

.role-panel__head {
  display: flex;
  align-items: end;
  justify-content: space-between;
  gap: 16px;
}

.role-panel__eyebrow {
  margin: 0 0 4px;
  font-size: 11px;
  letter-spacing: 0.22em;
  text-transform: uppercase;
  color: #7f8db0;
}

.role-panel__head h3 {
  margin: 0;
  font-size: 20px;
  color: #13203a;
}

.role-panel__count {
  color: #6b7ca3;
  font-size: 13px;
  font-weight: 700;
}

.role-table-list {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  grid-template-rows: repeat(5, minmax(0, 1fr));
  gap: 8px;
  min-height: 0;
  overflow: hidden;
}

.role-row,
.empty-card {
  width: 100%;
  text-align: left;
  padding: 10px 12px;
  border-radius: 14px;
  border: 1px solid rgba(47, 111, 243, 0.12);
  background: rgba(255, 255, 255, 0.94);
  box-shadow: 0 12px 24px rgba(36, 62, 124, 0.06);
  min-height: 0;
}

.role-row {
  cursor: pointer;
  display: grid;
  grid-template-columns: 72px 92px minmax(72px, 1fr) minmax(120px, 1.4fr);
  align-items: center;
  gap: 10px;
  transition: transform 0.18s ease, box-shadow 0.18s ease,
    border-color 0.18s ease;
}

.role-row:hover {
  transform: translateY(-2px);
  border-color: rgba(47, 111, 243, 0.28);
  box-shadow: 0 18px 36px rgba(36, 62, 124, 0.12);
}

.role-row--doctor {
  grid-template-columns: 72px 76px minmax(72px, 0.9fr) minmax(120px, 1.2fr) 64px;
  background: linear-gradient(
    180deg,
    rgba(236, 246, 255, 0.95) 0%,
    rgba(255, 255, 255, 0.98) 100%
  );
}

.role-row__id {
  color: #5b6c92;
  font-size: 12px;
  white-space: nowrap;
}

.role-row__name,
.role-row__contact {
  display: block;
  min-width: 0;
}

.role-row__name {
  font-size: 15px;
  color: #142445;
  line-height: 1.2;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  text-align: center;
}

.role-row__contact,
.empty-card {
  margin: 0;
  color: #67779d;
  line-height: 1.35;
  text-align: center;
}

.role-row__contact {
  font-size: 12px;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.role-pill,
.status-pill {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 64px;
  padding: 4px 8px;
  border-radius: 999px;
  font-size: 11px;
  font-weight: 700;
  justify-self: start;
  white-space: nowrap;
}

:deep(.pager) {
  justify-content: flex-end;
  gap: 8px;
  font-size: 12px;
}

:deep(.pager-button),
:deep(.pager-button--ghost) {
  padding: 7px 10px;
  border-radius: 12px;
  font-size: 11px;
}

:deep(.pager-jump input) {
  width: 54px;
  min-height: 30px;
}

.role-pill--user {
  background: #edf4ff;
  color: #000000;
}

.role-pill--doctor {
  background: #92dffb;
  color: #2863da;
}

.role-pill--admin {
  background: #eff8f3;
  color: #25734c;
}

.role-pill--super {
  background: #fff2e8;
  color: #ad5a21;
}

.status-pill--online {
  background: #e9f8ef;
  color: #217149;
}

.status-pill--offline {
  background: #fff1f1;
  color: #b14f57;
}

.dialog-backdrop {
  position: fixed;
  inset: 0;
  background: rgba(12, 22, 45, 0.28);
  display: grid;
  place-items: center;
  padding: 20px;
  z-index: 30;
}

.dialog {
  width: min(560px, 100%);
  border-radius: 24px;
  background: #fff;
  border: 1px solid #dce7ff;
  box-shadow: 0 24px 60px rgba(25, 42, 92, 0.18);
  padding: 22px;
}

.dialog__head,
.dialog__actions {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}

.dialog__head h3 {
  margin: 0;
  color: #16284d;
}

.form {
  display: grid;
  gap: 14px;
  margin-top: 18px;
}

.form label {
  display: grid;
  gap: 8px;
  color: #425171;
}

.form input {
  border: 1px solid #cfdbff;
  border-radius: 14px;
  padding: 11px 12px;
  background: #fbfcff;
}

.form-error {
  margin: 0;
  color: #bf4a54;
}

button {
  border: 0;
  border-radius: 14px;
  padding: 11px 16px;
  background: #2f6ff3;
  color: #fff;
  cursor: pointer;
  font-weight: 700;
}

button.ghost {
  background: #edf3ff;
  color: #2a4c92;
}

button:disabled {
  cursor: not-allowed;
  opacity: 0.6;
}

@media (max-width: 1080px) {
  .hero {
    grid-template-columns: 1fr;
  }

  .hero_card {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }

  .role-panel {
    height: auto;
    min-height: 0;
  }

  .role-table-list {
    grid-template-columns: 1fr;
    grid-template-rows: none;
  }

  .role-row {
    grid-template-columns: 64px 84px minmax(72px, 1fr) minmax(96px, 1.1fr);
    gap: 8px;
  }

  .role-row--doctor {
    grid-template-columns: 56px 68px minmax(60px, 0.8fr) minmax(88px, 1fr) 56px;
  }
}

@media (max-width: 640px) {
  .hero_card {
    grid-template-columns: 1fr;
  }

  .hero-card-item {
    grid-template-columns: minmax(0, 1fr) 84px;
    min-height: 118px;
    padding: 16px;
  }

  .hero-card-item__copy strong {
    font-size: 20px;
  }

  .hero-card-item__copy span {
    font-size: 24px;
  }

  .hero-card-item__art img {
    width: 84px;
    height: 84px;
  }
}
</style>
