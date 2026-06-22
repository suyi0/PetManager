<template>
  <div class="pet-board">
    <div class="pet-head">
      <div class="pet-stats">
        <span class="pet-stat"
          ><b>{{ pets.length }}</b> 当前档案</span
        >
        <span class="pet-stat"
          ><b>{{ catCount }}</b> 猫咪</span
        >
        <span class="pet-stat"
          ><b>{{ dogCount }}</b> 狗狗</span
        >
      </div>
      <button
        v-if="view === 'list'"
        class="pet-add-btn"
        @click="openCreateForm"
      >
        ＋ 新增宠物
      </button>
      <button v-else class="pet-back-btn" @click="backToList">
        ← 返回列表
      </button>
    </div>

    <div v-if="view === 'list'" class="pet-rows">
      <article v-for="pet in pets" :key="pet.id" class="pet-row">
        <div
          class="pet-row__av"
          :class="`pet-row__av--${speciesKind(pet.species)}`"
        >
          {{ speciesEmoji(pet.species) }}
        </div>
        <div class="pet-row__name">
          <b>{{ pet.name }}</b>
          <span
            >{{ pet.breed || pet.species }} ·
            {{ pet.gender || "性别待补充" }}</span
          >
        </div>
        <div class="pet-row__meta">
          <div>
            <small>年龄</small><b>{{ pet.age || "待补充" }}</b>
          </div>
          <div>
            <small>绝育</small><b>{{ pet.neutered || "未记录" }}</b>
          </div>
          <div>
            <small>疫苗</small><b>{{ pet.vaccineStatus || "待完善" }}</b>
          </div>
        </div>
        <div class="pet-row__act">
          <button @click="openEditForm(pet.id)">编辑</button>
          <button @click="openPet(pet)">详情</button>
          <button class="danger" @click="removePet(pet.id)">删除</button>
        </div>
      </article>
    </div>

    <section v-else-if="view === 'detail' && selectedPet" class="pet-spotlight">
      <div class="pet-spotlight__headline">
        <div>
          <small>当前宠物</small>
          <h4>{{ selectedPet.name }}</h4>
          <p>
            {{ selectedPet.species }} ·
            {{ selectedPet.breed || "未补充品种" }}
          </p>
        </div>
        <div class="pet-spotlight__actions">
          <button class="pet-ghost" @click="openEditForm(selectedPet.id)">
            编辑资料
          </button>
          <button class="pet-danger" @click="removePet(selectedPet.id)">
            删除档案
          </button>
        </div>
      </div>

      <div class="pet-spotlight__grid">
        <article>
          <span>年龄</span>
          <strong>{{ selectedPet.age || "未设置" }}</strong>
        </article>
        <article>
          <span>性别</span>
          <strong>{{ selectedPet.gender || "未设置" }}</strong>
        </article>
        <article>
          <span>绝育状态</span>
          <strong>{{ selectedPet.neutered || "未记录" }}</strong>
        </article>
        <article>
          <span>疫苗状态</span>
          <strong>{{ selectedPet.vaccineStatus || "待完善" }}</strong>
        </article>
      </div>

      <div class="pet-notes">
        <article>
          <small>护理偏好</small>
          <p>{{ selectedPet.preference || "还没有护理偏好记录。" }}</p>
        </article>
        <article>
          <small>特殊备注</small>
          <p>{{ selectedPet.notes || "还没有额外说明。" }}</p>
        </article>
      </div>
    </section>

    <section v-else-if="view === 'form'" class="pet-form-shell">
      <div class="pet-form-shell__head">
        <div>
          <small>{{ editingPetId ? "Edit Profile" : "Create Profile" }}</small>
          <h4>{{ editingPetId ? "编辑宠物资料" : "新增宠物资料" }}</h4>
        </div>
        <button class="pet-ghost" @click="resetForm">清空表单</button>
      </div>

      <div class="pet-form-shell__preview">
        <div class="pet-card pet-card--preview">
          <div class="pet-card__header">
            <div class="pet-card__badge">
              {{ (form.name || "P").charAt(0).toUpperCase() }}
            </div>
            <span>{{ form.species }}</span>
          </div>
          <strong>{{ form.name || "等待命名" }}</strong>
          <p>{{ form.breed || "品种待补充" }}</p>
          <div class="pet-card__meta">
            <span>{{ form.age || "年龄待补充" }}</span>
            <span>{{ form.gender || "性别待补充" }}</span>
          </div>
        </div>
      </div>

      <form class="pet-form" @submit.prevent="savePet">
        <div class="pet-form__section">
          <small>基础资料</small>
          <div class="pet-form__grid">
            <label>
              <span>宠物名</span>
              <input
                v-model.trim="form.name"
                type="text"
                placeholder="例如：可乐"
              />
            </label>
            <label>
              <span>品类</span>
              <select v-model="form.species">
                <option value="猫咪">猫咪</option>
                <option value="狗狗">狗狗</option>
                <option value="兔兔">兔兔</option>
                <option value="其他">其他</option>
              </select>
            </label>
            <label>
              <span>品种</span>
              <input
                v-model.trim="form.breed"
                type="text"
                placeholder="例如：英短、柯基"
              />
            </label>
            <label>
              <span>年龄</span>
              <input
                v-model.trim="form.age"
                type="text"
                placeholder="例如：2 岁 3 个月"
              />
            </label>
            <label>
              <span>性别</span>
              <select v-model="form.gender">
                <option value="公">公</option>
                <option value="母">母</option>
                <option value="未知">未知</option>
              </select>
            </label>
          </div>
        </div>

        <div class="pet-form__section">
          <small>健康状态</small>
          <div class="pet-form__grid">
            <label>
              <span>绝育状态</span>
              <select v-model="form.neutered">
                <option value="已绝育">已绝育</option>
                <option value="未绝育">未绝育</option>
                <option value="待确认">待确认</option>
              </select>
            </label>
            <label class="pet-form__wide">
              <span>疫苗状态</span>
              <input
                v-model.trim="form.vaccineStatus"
                type="text"
                placeholder="例如：已完成基础三联"
              />
            </label>
          </div>
        </div>

        <div class="pet-form__section">
          <small>护理信息</small>
          <div class="pet-form__grid">
            <label class="pet-form__wide">
              <span>护理偏好</span>
              <input
                v-model.trim="form.preference"
                type="text"
                placeholder="例如：剪指甲前需要安抚、怕吹风机"
              />
            </label>
            <label class="pet-form__wide">
              <span>特殊备注</span>
              <textarea
                v-model.trim="form.notes"
                rows="4"
                placeholder="记录近期症状、饮食偏好或医生建议。"
              ></textarea>
            </label>
          </div>
        </div>

        <div class="pet-form__actions">
          <button class="pet-ghost" type="button" @click="cancelForm">
            取消
          </button>
          <button class="pet-primary" type="submit">
            {{ editingPetId ? "保存更新" : "创建档案" }}
          </button>
        </div>
      </form>
    </section>
  </div>
</template>

<script setup lang="ts">
import { computed, onMounted, reactive, ref, watch } from "vue";
import { PetProfile } from "@/modules/user/api/types";
import { petApi } from "@/modules/user/api/userApi";

const emit = defineEmits(["updateCount"]);

const createPet = ref<boolean>(false);
const selectedPetId = ref<string>("");
const editingPetId = ref<string>("");
const pets = ref<PetProfile[]>([]);

const form = reactive<Omit<PetProfile, "id">>({
  name: "",
  species: "猫咪",
  breed: "",
  age: "",
  gender: "未知",
  neutered: "待确认",
  vaccineStatus: "",
  preference: "",
  notes: "",
});

const selectedPet = computed(() =>
  pets.value.find((pet) => pet.id === selectedPetId.value)
);
const catCount = computed(
  () => pets.value.filter((pet) => pet.species === "猫咪").length
);
const dogCount = computed(
  () => pets.value.filter((pet) => pet.species === "狗狗").length
);

// 视图：表单 / 详情 / 列表（默认列表）
const view = computed<"form" | "detail" | "list">(() => {
  if (createPet.value) return "form";
  if (selectedPetId.value) return "detail";
  return "list";
});

const speciesKind = (species: string) =>
  species === "狗狗" ? "dog" : species === "猫咪" ? "cat" : "other";
const speciesEmoji = (species: string) =>
  species === "狗狗" ? "🐕" : species === "猫咪" ? "🐈" : "🐾";

const backToList = () => {
  createPet.value = false;
  selectedPetId.value = "";
  resetForm();
};

const syncPetPanelState = (rows: PetProfile[]) => {
  if (rows.length === 0) {
    selectedPetId.value = "";
    createPet.value = true;
    emit("updateCount", 0);
    return;
  }

  // 列表优先：选中的宠物若已不存在则回到列表，不再自动选第一只。
  if (
    selectedPetId.value &&
    !rows.some((pet) => pet.id === selectedPetId.value)
  ) {
    selectedPetId.value = "";
  }

  createPet.value = false;
  emit("updateCount", rows.length);
};

const resetForm = () => {
  editingPetId.value = "";
  form.name = "";
  form.species = "猫咪";
  form.breed = "";
  form.age = "";
  form.gender = "未知";
  form.neutered = "待确认";
  form.vaccineStatus = "";
  form.preference = "";
  form.notes = "";
};

const openCreateForm = () => {
  createPet.value = true;
  resetForm();
};

const cancelForm = () => {
  resetForm();
  createPet.value = pets.value.length === 0;
};

const openPet = (pet: PetProfile) => {
  createPet.value = false;
  selectedPetId.value = pet.id;
};

const openEditForm = (petId: string) => {
  const pet = pets.value.find((item) => item.id === petId);
  if (!pet) {
    return;
  }

  createPet.value = true;
  editingPetId.value = pet.id;
  form.name = pet.name;
  form.species = pet.species;
  form.breed = pet.breed;
  form.age = pet.age;
  form.gender = pet.gender;
  form.neutered = pet.neutered;
  form.vaccineStatus = pet.vaccineStatus;
  form.preference = pet.preference;
  form.notes = pet.notes;
};

const savePet = async () => {
  if (!form.name.trim()) {
    return;
  }

  const payload = {
    name: form.name.trim(),
    species: form.species,
    breed: form.breed.trim(),
    age: form.age.trim(),
    gender: form.gender,
    neutered: form.neutered,
    vaccineStatus: form.vaccineStatus.trim(),
    preference: form.preference.trim(),
    notes: form.notes.trim(),
  };

  let savedPet: PetProfile;
  if (editingPetId.value) {
    savedPet = await petApi.updatePetProfile({
      id: editingPetId.value,
      ...payload,
    });
    pets.value = pets.value.map((pet) =>
      pet.id === savedPet.id ? savedPet : pet
    );
  } else {
    savedPet = await petApi.createPetProfile(payload);
    pets.value = [savedPet, ...pets.value];
  }

  selectedPetId.value = savedPet.id;
  createPet.value = false;
  resetForm();
};

const removePet = async (petId: string) => {
  await petApi.deletePetProfile(petId);
  pets.value = pets.value.filter((pet) => pet.id !== petId);
  const nextPets = pets.value;
  if (selectedPetId.value === petId) {
    selectedPetId.value = nextPets[0]?.id || "";
  }
  createPet.value = nextPets.length === 0;
  resetForm();
};

watch(
  pets,
  (rows) => {
    syncPetPanelState(rows);
  },
  { immediate: true }
);

onMounted(async () => {
  pets.value = await petApi.getPetProfiles();
});
</script>

<style scoped lang="scss">
.pet-board {
  display: grid;
  gap: 14px;
}

.pet-board__list,
.pet-board__detail {
  border-radius: 16px;
  border: 1px solid #efe7dc;
  background: #ffffff;
  box-shadow: 0 8px 20px rgba(47, 158, 143, 0.06);
}

/* 头部：统计 + 新增/返回 */
.pet-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
  flex-wrap: wrap;
}

.pet-add-btn {
  height: 38px;
  padding: 0 18px;
  border: 0;
  border-radius: 10px;
  background: #2f9e8f;
  color: #fff;
  font-size: 13px;
  font-weight: 700;
  cursor: pointer;
}

.pet-add-btn:hover {
  background: #1f7a6c;
}

.pet-back-btn {
  height: 38px;
  padding: 0 16px;
  border: 1px solid #efe7dc;
  border-radius: 10px;
  background: #fff;
  color: #2f9e8f;
  font-size: 13px;
  font-weight: 700;
  cursor: pointer;
}

.pet-back-btn:hover {
  border-color: #cfe7e1;
}

/* 横向列表行 */
.pet-rows {
  display: grid;
  gap: 10px;
}

.pet-row {
  display: flex;
  align-items: center;
  gap: 14px;
  padding: 12px 16px;
  border: 1px solid #efe7dc;
  border-radius: 14px;
  background: #fff;
  box-shadow: 0 8px 20px rgba(47, 158, 143, 0.05);
}

.pet-row__av {
  width: 46px;
  height: 46px;
  flex: 0 0 auto;
  border-radius: 14px;
  display: grid;
  place-items: center;
  font-size: 22px;
}

.pet-row__av--dog {
  background: #e7f5f1;
}
.pet-row__av--cat {
  background: #fff4e6;
}
.pet-row__av--other {
  background: #f1f5f9;
}

.pet-row__name {
  min-width: 120px;
}

.pet-row__name b {
  font-size: 15px;
  color: #1f3a36;
}

.pet-row__name span {
  display: block;
  color: #6b7d77;
  font-size: 12px;
  margin-top: 2px;
}

.pet-row__meta {
  display: flex;
  gap: 22px;
  margin-left: 14px;
  color: #6b7d77;
  font-size: 12px;
}

.pet-row__meta b {
  display: block;
  color: #1f3a36;
  font-size: 14px;
  margin-top: 2px;
}

.pet-row__act {
  margin-left: auto;
  display: flex;
  gap: 8px;
}

.pet-row__act button {
  height: 32px;
  padding: 0 12px;
  border: 1px solid #efe7dc;
  border-radius: 8px;
  background: #fffdfa;
  color: #1f3a36;
  font-size: 13px;
  font-weight: 600;
  cursor: pointer;
}

.pet-row__act button:hover {
  border-color: #cfe7e1;
}

.pet-row__act button.danger {
  color: #be4b5b;
  border-color: #f3c9cd;
}

.pet-row__act button.danger:hover {
  background: #fdeef0;
}

.pet-stats {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
}

.pet-stat {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  padding: 6px 12px;
  border-radius: 999px;
  border: 1px solid #efe7dc;
  background: #fff;
  color: #6b7d77;
  font-size: 13px;
}

.pet-stat b {
  color: #1f3a36;
  font-size: 15px;
  font-variant-numeric: tabular-nums;
}

.pet-spotlight__headline small,
.pet-form-shell__head small {
  margin: 0;
  color: #2f9e8f;
  letter-spacing: 0.14em;
  text-transform: uppercase;
  font-size: 11px;
  font-weight: 700;
}

.pet-spotlight__headline h4,
.pet-form-shell__head h4 {
  margin: 0;
  color: #1f3a36;
  font-size: 18px;
}

.pet-spotlight__headline p {
  color: #6b7d77;
  line-height: 1.6;
  font-size: 13px;
}

.pet-board__content {
  display: grid;
  grid-template-columns: 280px minmax(0, 1fr);
  gap: 18px;
}

.pet-board__list {
  display: grid;
  align-content: start;
  gap: 12px;
  padding: 18px;
}

.pet-create-card,
.pet-card {
  display: grid;
  gap: 6px;
  justify-items: start;
  padding: 18px;
  border: 1px solid rgba(47, 158, 143, 0.08);
  border-radius: 16px;
  background: rgba(255, 253, 250, 0.95);
  text-align: left;
  cursor: pointer;
  transition: transform 0.2s ease, box-shadow 0.2s ease, border-color 0.2s ease;
}

.pet-create-card:hover,
.pet-card:hover {
  transform: translateY(-1px);
  box-shadow: 0 16px 30px rgba(22, 80, 82, 0.08);
}

.pet-create-card small,
.pet-card__header span {
  color: #2f9e8f;
  letter-spacing: 0.1em;
  text-transform: uppercase;
  font-size: 10px;
  font-weight: 700;
}

.pet-create-card strong,
.pet-card strong {
  color: #1f3a36;
  font-size: 20px;
}

.pet-create-card span,
.pet-card p {
  margin: 0;
  color: #6b7d77;
  line-height: 1.7;
  font-size: 13px;
}

.pet-card--active,
.pet-create-card--active {
  border-color: rgba(47, 158, 143, 0.18);
  background: linear-gradient(
    135deg,
    rgba(56, 178, 163, 0.22),
    rgba(255, 217, 176, 0.18)
  );
}

.pet-card__header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  width: 100%;
}

.pet-card__badge {
  width: 42px;
  height: 42px;
  display: grid;
  place-items: center;
  border-radius: 14px;
  background: linear-gradient(135deg, #cfe7e1, #ffd9b0);
  color: #1f3a36;
  font-family: "Rajdhani", "Noto Sans SC", sans-serif;
  font-size: 22px;
  font-weight: 700;
}

.pet-card__meta {
  display: flex;
  gap: 10px;
  flex-wrap: wrap;
}

.pet-card__meta span {
  padding: 5px 10px;
  border-radius: 999px;
  background: rgba(47, 158, 143, 0.08);
  color: #6b7d77;
  font-size: 12px;
}

.pet-board__detail {
  display: grid;
  gap: 18px;
  padding: 20px;
}

.pet-spotlight,
.pet-form-shell,
.pet-empty-state {
  padding: 22px;
  border-radius: 16px;
  background: rgba(255, 255, 255, 0.76);
  border: 1px solid rgba(47, 158, 143, 0.08);
}

.pet-spotlight__headline {
  display: flex;
  justify-content: space-between;
  gap: 16px;
  align-items: start;
}

.pet-spotlight__actions,
.pet-form__actions {
  display: flex;
  gap: 10px;
  flex-wrap: wrap;
}

.pet-ghost,
.pet-danger,
.pet-primary {
  border: none;
  border-radius: 999px;
  padding: 11px 16px;
  font-size: 13px;
  font-weight: 700;
  cursor: pointer;
}

.pet-ghost {
  background: rgba(47, 158, 143, 0.08);
  color: #1f3a36;
}

.pet-danger {
  background: rgba(214, 116, 116, 0.12);
  color: #be4b5b;
}

.pet-primary {
  background: linear-gradient(135deg, #1f7a6c, #2f9e8f);
  color: #fff;
  box-shadow: 0 16px 30px rgba(47, 158, 143, 0.22);
}

.pet-spotlight__grid {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 12px;
  margin-top: 18px;
}

.pet-spotlight__grid article {
  display: grid;
  gap: 8px;
  padding: 16px;
  border-radius: 18px;
  background: rgba(255, 253, 250, 0.95);
}

.pet-spotlight__grid span,
.pet-notes small {
  color: #6b7d77;
  font-size: 12px;
}

.pet-spotlight__grid strong {
  color: #1f3a36;
  font-size: 18px;
}

.pet-notes {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
  margin-top: 16px;
}

.pet-notes article {
  padding: 16px 18px;
  border-radius: 18px;
  background: rgba(250, 253, 252, 0.94);
}

.pet-notes p,
.pet-empty-state span {
  margin: 8px 0 0;
  color: #6b7d77;
  line-height: 1.8;
  font-size: 14px;
}

.pet-empty-state {
  display: grid;
  gap: 6px;
  justify-items: start;
}

.pet-empty-state strong {
  color: #1f3a36;
  font-size: 22px;
}

.pet-spotlight,
.pet-form-shell {
  display: grid;
  gap: 16px;
  padding: 20px;
  border-radius: 16px;
  border: 1px solid #efe7dc;
  background: #ffffff;
  box-shadow: 0 8px 20px rgba(47, 158, 143, 0.06);
}

.pet-form-shell__preview {
  display: grid;
}

.pet-card--preview {
  cursor: default;
  width: min(320px, 100%);
}

.pet-form-shell__head {
  display: flex;
  align-items: end;
  justify-content: space-between;
  gap: 12px;
}

.pet-form {
  display: grid;
  gap: 16px;
}

.pet-form__section {
  display: grid;
  gap: 12px;
  padding: 18px;
  border-radius: 22px;
  background: rgba(255, 253, 250, 0.7);
  border: 1px solid rgba(47, 158, 143, 0.06);
}

.pet-form__section small {
  color: #2f9e8f;
  letter-spacing: 0.12em;
  text-transform: uppercase;
  font-size: 11px;
  font-weight: 700;
}

.pet-form__grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 14px;
}

.pet-form label {
  display: grid;
  gap: 8px;
}

.pet-form label span {
  color: #1f3a36;
  font-size: 13px;
  font-weight: 700;
}

.pet-form input,
.pet-form select,
.pet-form textarea {
  width: 100%;
  padding: 13px 14px;
  border: 1px solid rgba(47, 158, 143, 0.12);
  border-radius: 16px;
  background: rgba(255, 255, 255, 0.94);
  color: #1f3a36;
  font-size: 14px;
}

.pet-form input:focus,
.pet-form select:focus,
.pet-form textarea:focus {
  outline: none;
  border-color: rgba(47, 158, 143, 0.5);
  box-shadow: 0 0 0 4px rgba(56, 178, 163, 0.18);
}

.pet-form textarea {
  resize: vertical;
}

.pet-form__wide,
.pet-form__actions {
  grid-column: 1 / -1;
}

@media (max-width: 1100px) {
  .pet-board__content {
    grid-template-columns: 1fr;
  }
}

@media (max-width: 900px) {
  .pet-board__hero,
  .pet-spotlight__headline,
  .pet-form-shell__head {
    grid-template-columns: 1fr;
    display: grid;
  }

  .pet-board__summary,
  .pet-spotlight__grid,
  .pet-notes,
  .pet-form__grid {
    grid-template-columns: 1fr;
  }
}
</style>
