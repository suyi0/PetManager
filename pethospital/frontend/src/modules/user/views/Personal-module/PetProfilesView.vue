<template>
  <div class="pet-board">
    <section class="pet-board__hero">
      <div>
        <p class="pet-board__eyebrow">Pet Archive</p>
        <h3>宠物档案总览</h3>
        <span>
          先把每只宠物的基础资料整理出来，后续预约、订单和护理提醒都会更顺畅。
        </span>
      </div>

      <div class="pet-board__summary">
        <article>
          <strong>{{ pets.length }}</strong>
          <span>当前档案</span>
        </article>
        <article>
          <strong>{{ catCount }}</strong>
          <span>猫咪数量</span>
        </article>
        <article>
          <strong>{{ dogCount }}</strong>
          <span>狗狗数量</span>
        </article>
      </div>
    </section>

    <section class="pet-board__content">
      <div class="pet-board__list">
        <button
          class="pet-create-card"
          :class="{ 'pet-create-card--active': createPet }"
          @click="openCreateForm"
        >
          <small>New Profile</small>
          <strong>新增宠物档案</strong>
          <span>添加名称、品种、年龄、性格与护理备注。</span>
        </button>

        <button
          v-for="pet in pets"
          :key="pet.id"
          class="pet-card"
          :class="{
            'pet-card--active': !createPet && pet.id === selectedPetId,
          }"
          @click="openPet(pet)"
        >
          <div class="pet-card__header">
            <div class="pet-card__badge">
              {{ pet.name.charAt(0).toUpperCase() }}
            </div>
            <span>{{ pet.species }}</span>
          </div>
          <strong>{{ pet.name }}</strong>
          <p>{{ pet.breed || "品种待补充" }}</p>
          <div class="pet-card__meta">
            <span>{{ pet.age || "年龄待补充" }}</span>
            <span>{{ pet.gender || "性别待补充" }}</span>
          </div>
        </button>
      </div>

      <div class="pet-board__detail">
        <section v-if="!createPet && selectedPet" class="pet-spotlight">
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

        <section v-if="!createPet && !selectedPet" class="pet-empty-state">
          <strong>还没有宠物档案</strong>
          <span>先新增第一只宠物，后续预约和订单都会更容易关联。</span>
          <button class="pet-primary" type="button" @click="openCreateForm">
            创建第一只宠物
          </button>
        </section>

        <section v-if="createPet" class="pet-form-shell">
          <div class="pet-form-shell__head">
            <div>
              <small>{{
                editingPetId ? "Edit Profile" : "Create Profile"
              }}</small>
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
    </section>
  </div>
</template>

<script setup lang="ts">
import { computed, onMounted, reactive, ref, watch } from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/store/appStore";

interface PetProfile {
  id: string;
  name: string;
  species: string;
  breed: string;
  age: string;
  gender: string;
  neutered: string;
  vaccineStatus: string;
  preference: string;
  notes: string;
}

const emit = defineEmits(["updateCount"]);

const store = useStore(storeKey);

const pets = ref<PetProfile[]>([]);
const createPet = ref<boolean>(false);
const selectedPetId = ref<string>("");
const editingPetId = ref<string>("");

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

const getStorageKey = () => {
  const userKey =
    store.state.currentUser.userPhone ||
    store.state.currentUser.userEmail ||
    store.state.currentUser.userName ||
    "default-user";

  return `petmanager-user-pets:${userKey}`;
};

const persistPets = () => {
  window.localStorage.setItem(getStorageKey(), JSON.stringify(pets.value));
  emit("updateCount", pets.value.length);
};

const loadPets = () => {
  try {
    const raw = window.localStorage.getItem(getStorageKey());
    if (!raw) {
      pets.value = [];
      selectedPetId.value = "";
      createPet.value = true;
      emit("updateCount", 0);
      return;
    }

    const parsed = JSON.parse(raw);
    pets.value = Array.isArray(parsed) ? parsed : [];
    selectedPetId.value = pets.value[0]?.id || "";
    createPet.value = pets.value.length === 0;
    emit("updateCount", pets.value.length);
  } catch (error) {
    pets.value = [];
    selectedPetId.value = "";
    createPet.value = true;
    emit("updateCount", 0);
  }
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

const openPet = (pet: any) => {
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

const savePet = () => {
  if (!form.name.trim()) {
    return;
  }

  const payload: PetProfile = {
    id: editingPetId.value || `pet-${Date.now()}`,
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

  if (editingPetId.value) {
    pets.value = pets.value.map((pet) =>
      pet.id === editingPetId.value ? payload : pet
    );
  } else {
    pets.value = [payload, ...pets.value];
  }

  selectedPetId.value = payload.id;
  persistPets();
  createPet.value = false;
  resetForm();
};

const removePet = (petId: string) => {
  pets.value = pets.value.filter((pet) => pet.id !== petId);
  if (selectedPetId.value === petId) {
    selectedPetId.value = pets.value[0]?.id || "";
  }
  persistPets();
  createPet.value = pets.value.length === 0;
  resetForm();
};

watch(
  () => store.state.currentUser.userPhone,
  () => {
    loadPets();
  }
);

onMounted(() => {
  loadPets();
});
</script>

<style scoped lang="scss">
.pet-board {
  display: grid;
  gap: 20px;
}

.pet-board__hero,
.pet-board__list,
.pet-board__detail {
  border-radius: 30px;
  border: 1px solid rgba(21, 91, 92, 0.08);
  background: rgba(255, 255, 255, 0.7);
  box-shadow: 0 18px 44px rgba(24, 90, 91, 0.06);
}

.pet-board__hero {
  display: flex;
  justify-content: space-between;
  gap: 18px;
  padding: 24px 26px;
  background: linear-gradient(
    135deg,
    rgba(140, 220, 210, 0.2),
    rgba(245, 203, 164, 0.2)
  );
}

.pet-board__eyebrow,
.pet-spotlight__headline small,
.pet-form-shell__head small {
  margin: 0;
  color: #1f8e89;
  letter-spacing: 0.14em;
  text-transform: uppercase;
  font-size: 11px;
  font-weight: 700;
}

.pet-board__hero h3,
.pet-spotlight__headline h4,
.pet-form-shell__head h4 {
  margin: 0;
  color: #133f42;
  font-size: 32px;
}

.pet-board__hero span,
.pet-spotlight__headline p {
  color: #5d7974;
  line-height: 1.8;
  font-size: 14px;
}

.pet-board__summary {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 12px;
  min-width: 320px;
}

.pet-board__summary article {
  display: grid;
  align-content: center;
  gap: 4px;
  padding: 16px;
  border-radius: 20px;
  background: rgba(255, 255, 255, 0.76);
}

.pet-board__summary strong {
  color: #134043;
  font-family: "Rajdhani", "Noto Sans SC", sans-serif;
  font-size: 32px;
}

.pet-board__summary span {
  color: #6c8682;
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
  border: 1px solid rgba(21, 91, 92, 0.08);
  border-radius: 24px;
  background: rgba(255, 252, 247, 0.95);
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
  color: #1f8e89;
  letter-spacing: 0.1em;
  text-transform: uppercase;
  font-size: 10px;
  font-weight: 700;
}

.pet-create-card strong,
.pet-card strong {
  color: #133f42;
  font-size: 20px;
}

.pet-create-card span,
.pet-card p {
  margin: 0;
  color: #617975;
  line-height: 1.7;
  font-size: 13px;
}

.pet-card--active,
.pet-create-card--active {
  border-color: rgba(29, 134, 135, 0.18);
  background: linear-gradient(
    135deg,
    rgba(136, 214, 206, 0.22),
    rgba(243, 197, 155, 0.18)
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
  background: linear-gradient(135deg, #8fd9cf, #f1c49c);
  color: #173f42;
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
  background: rgba(20, 82, 84, 0.08);
  color: #55716d;
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
  border-radius: 26px;
  background: rgba(255, 255, 255, 0.76);
  border: 1px solid rgba(21, 91, 92, 0.08);
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
  background: rgba(20, 82, 84, 0.08);
  color: #154144;
}

.pet-danger {
  background: rgba(214, 116, 116, 0.12);
  color: #9c4141;
}

.pet-primary {
  background: linear-gradient(135deg, #167f80, #2ca7a4);
  color: #fff;
  box-shadow: 0 16px 30px rgba(23, 104, 105, 0.22);
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
  background: rgba(255, 249, 242, 0.95);
}

.pet-spotlight__grid span,
.pet-notes small {
  color: #6a847f;
  font-size: 12px;
}

.pet-spotlight__grid strong {
  color: #143f42;
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
  color: #5d7974;
  line-height: 1.8;
  font-size: 14px;
}

.pet-empty-state {
  display: grid;
  gap: 6px;
  justify-items: start;
}

.pet-empty-state strong {
  color: #143f42;
  font-size: 22px;
}

.pet-form-shell {
  display: grid;
  gap: 16px;
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
  background: rgba(255, 249, 242, 0.7);
  border: 1px solid rgba(21, 91, 92, 0.06);
}

.pet-form__section small {
  color: #1f8e89;
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
  color: #24484b;
  font-size: 13px;
  font-weight: 700;
}

.pet-form input,
.pet-form select,
.pet-form textarea {
  width: 100%;
  padding: 13px 14px;
  border: 1px solid rgba(20, 82, 84, 0.12);
  border-radius: 16px;
  background: rgba(255, 255, 255, 0.94);
  color: #173f42;
  font-size: 14px;
}

.pet-form input:focus,
.pet-form select:focus,
.pet-form textarea:focus {
  outline: none;
  border-color: rgba(24, 128, 127, 0.5);
  box-shadow: 0 0 0 4px rgba(141, 218, 210, 0.18);
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
