<template>
  <section class="page">
    <div class="panel">
      <div class="head">
        <h3>用户列表</h3>
        <div class="actions">
          <button class="ghost" @click="loadUsers">刷新</button>
          <button @click="openCreateDialog">创建用户</button>
        </div>
      </div>
      <table>
        <thead>
          <tr>
            <th>ID</th>
            <th>角色</th>
            <th>姓名</th>
            <th>手机</th>
            <th>邮箱</th>
            <th>生日</th>
            <th>地址ID</th>
            <th>操作</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="item in users" :key="item.id">
            <td>{{ item.id }}</td>
            <td>{{ formatRole(item.type_id) }}</td>
            <td>{{ item.name }}</td>
            <td>{{ item.phone }}</td>
            <td>{{ item.email }}</td>
            <td>{{ item.birthday }}</td>
            <td>{{ item.address_id }}</td>
            <td>
              <button
                class="danger"
                :disabled="item.type_id !== 3 || deletingId === item.id"
                @click="handleDelete(item)"
              >
                {{ deletingId === item.id ? "删除中..." : "删除" }}
              </button>
            </td>
          </tr>
        </tbody>
      </table>
    </div>

    <div
      v-if="showCreateDialog"
      class="dialog-backdrop"
      @click.self="closeCreateDialog"
    >
      <div class="dialog">
        <div class="dialog__head">
          <h3>创建普通用户</h3>
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
import { defineComponent, onMounted, reactive, ref } from "vue";
import { superAdminApi } from "../../api/superAdminApi";
import { UserRow } from "../../api/types";

export default defineComponent({
  name: "SuperAdminUsers",
  setup() {
    const users = ref<UserRow[]>([]);
    const deletingId = ref<number | null>(null);
    const showCreateDialog = ref(false);
    const creating = ref(false);
    const formError = ref("");
    const form = reactive({
      name: "",
      phone: "",
      email: "",
      password: "",
      birthday: "",
      address_id: 0,
    });

    const loadUsers = async () => {
      users.value = await superAdminApi.getUsers();
    };

    const formatRole = (typeId: number) => {
      if (typeId === 1) return "超级管理员";
      if (typeId === 2) return "医生";
      return "普通用户";
    };

    const resetForm = () => {
      form.name = "";
      form.phone = "";
      form.email = "";
      form.password = "";
      form.birthday = "";
      form.address_id = 0;
      formError.value = "";
    };

    const openCreateDialog = () => {
      resetForm();
      showCreateDialog.value = true;
    };

    const closeCreateDialog = () => {
      showCreateDialog.value = false;
    };

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
        closeCreateDialog();
        await loadUsers();
      } catch (error: any) {
        formError.value =
          error?.response?.data?.error || "创建失败，请稍后重试";
      } finally {
        creating.value = false;
      }
    };

    const handleDelete = async (item: UserRow) => {
      if (item.type_id !== 3) {
        return;
      }

      const confirmed = window.confirm(`确认删除用户「${item.name}」吗？`);
      if (!confirmed) {
        return;
      }

      deletingId.value = item.id;
      try {
        await superAdminApi.deleteUser(item.id);
        await loadUsers();
      } catch (error: any) {
        window.alert(error?.response?.data?.error || "删除失败，请稍后重试");
      } finally {
        deletingId.value = null;
      }
    };

    onMounted(loadUsers);

    return {
      users,
      deletingId,
      showCreateDialog,
      creating,
      form,
      formError,
      loadUsers,
      formatRole,
      openCreateDialog,
      closeCreateDialog,
      handleCreate,
      handleDelete,
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
}

.head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  margin-bottom: 12px;
}

.actions {
  display: flex;
  gap: 10px;
}

button {
  border: 0;
  border-radius: 10px;
  padding: 8px 12px;
  background: #2f6ff3;
  color: #fff;
  cursor: pointer;
}

button:disabled {
  cursor: not-allowed;
  opacity: 0.6;
}

.ghost {
  background: #ebf2ff;
  color: #2f6ff3;
}

.danger {
  background: #ffe7e7;
  color: #c73b3b;
}

table {
  width: 100%;
  border-collapse: collapse;
  font-size: 13px;
}

th,
td {
  text-align: left;
  padding: 10px;
  border-bottom: 1px solid #edf2ff;
}

.dialog-backdrop {
  position: fixed;
  inset: 0;
  display: grid;
  place-items: center;
  padding: 24px;
  background: rgba(14, 28, 56, 0.24);
}

.dialog {
  width: min(100%, 520px);
  border-radius: 18px;
  background: #fff;
  padding: 18px;
  box-shadow: 0 28px 60px rgba(35, 62, 119, 0.18);
}

.dialog__head,
.dialog__actions {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}

.dialog__head {
  margin-bottom: 14px;
}

.dialog__head h3 {
  margin: 0;
}

.form {
  display: grid;
  gap: 12px;
}

.form label {
  display: grid;
  gap: 6px;
  font-size: 13px;
  color: #45608f;
}

.form input {
  border: 1px solid #d7e4ff;
  border-radius: 10px;
  padding: 10px 12px;
  font-size: 14px;
}

.form-error {
  margin: 0;
  color: #c73b3b;
  font-size: 13px;
}
</style>
