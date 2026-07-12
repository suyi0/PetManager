<template>
  <section class="template-page">
    <header>
      <div>
        <p>文书中心</p>
        <h1>诊疗单模板</h1>
      </div>
      <button v-if="canManage" type="button" @click="preview">
        预览当前内容
      </button>
    </header>
    <div class="workspace">
      <aside class="template-list">
        <button
          v-for="item in templates"
          :key="Number(item.id)"
          type="button"
          :class="{ active: Number(item.id) === templateId }"
          @click="selectTemplate(Number(item.id))"
        >
          <strong>{{ item.name }}</strong
          ><small>当前版本 v{{ item.currentVersionNo }}</small>
        </button>
      </aside>
      <main>
        <div class="version-bar">
          <label
            >历史版本<select v-model.number="versionId" @change="loadVersion">
              <option
                v-for="version in versions"
                :key="Number(version.id)"
                :value="Number(version.id)"
              >
                v{{ version.versionNo }} · {{ version.status }}
              </option>
            </select></label
          >
          <div>
            <button v-if="canManage" type="button" @click="saveVersion">
              另存为新版本</button
            ><button
              v-if="canPublish"
              class="primary"
              type="button"
              @click="publishVersion"
            >
              发布所选版本
            </button>
          </div>
        </div>
        <textarea
          v-model="content"
          :readonly="!canManage"
          spellcheck="false"
          aria-label="模板 HTML"
        />
        <p v-if="message">{{ message }}</p>
      </main>
      <aside class="field-catalog">
        <h2>字段目录</h2>
        <button
          v-for="field in fields"
          :key="String(field.path)"
          type="button"
          :disabled="!canManage"
          @click="insertField(field)"
        >
          <strong>{{ field.label }}</strong
          ><code>{{ field.path }}</code
          ><small>{{ field.example || field.type }}</small>
        </button>
      </aside>
    </div>
    <div v-if="previewHtml" class="overlay" @click.self="previewHtml = ''">
      <section>
        <header>
          <h2>模板预览</h2>
          <button type="button" @click="previewHtml = ''">关闭</button>
        </header>
        <iframe title="模板预览" sandbox="" :srcdoc="previewHtml" />
      </section>
    </div>
  </section>
</template>

<script setup lang="ts">
import { computed, onMounted, ref } from "vue";
import { superAdminApi } from "@/modules/super-admin/api/superAdminApi";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";

const store = useStore(storeKey);
const templates = ref<Array<Record<string, unknown>>>([]);
const versions = ref<Array<Record<string, unknown>>>([]);
const fields = ref<Array<Record<string, unknown>>>([]);
const templateId = ref(0);
const versionId = ref(0);
const content = ref("");
const previewHtml = ref("");
const message = ref("");
const canManage = computed(() =>
  store.state.auth.permissions.includes("report-template:manage")
);
const canPublish = computed(() =>
  store.state.auth.permissions.includes("report-template:publish")
);

const loadVersion = async () => {
  if (!templateId.value || !versionId.value) return;
  const row = await superAdminApi.getReportTemplateVersion(
    templateId.value,
    versionId.value
  );
  content.value = String(row.templateContent ?? "");
};
const selectTemplate = async (id: number) => {
  templateId.value = id;
  versions.value = await superAdminApi.getReportTemplateVersions(id);
  versionId.value = Number(versions.value[0]?.id ?? 0);
  await loadVersion();
};
const preview = async () => {
  previewHtml.value = await superAdminApi.previewReportTemplate(
    templateId.value,
    content.value
  );
};
const saveVersion = async () => {
  await superAdminApi.createReportTemplateVersion(
    templateId.value,
    content.value
  );
  await selectTemplate(templateId.value);
  message.value = "新版本已保存为草稿";
};
const publishVersion = async () => {
  await superAdminApi.publishReportTemplateVersion(
    templateId.value,
    versionId.value
  );
  message.value = "版本已发布";
  templates.value = await superAdminApi.getReportTemplates();
};
const insertField = (field: Record<string, unknown>) => {
  const path = String(field.path ?? "");
  if (!path) return;
  content.value +=
    field.type === "collection"
      ? `\n{{#${path}}}\n  <div>{{medicineName}} {{dosage}} {{frequency}} {{quantity}} {{unit}}</div>\n{{/${path}}}`
      : `{{${path}}}`;
};

onMounted(async () => {
  const contract = await superAdminApi.getMedicalDocumentDataContract();
  fields.value = contract.fields;
  templates.value = await superAdminApi.getReportTemplates();
  if (templates.value.length)
    await selectTemplate(Number(templates.value[0].id));
});
</script>

<style scoped>
.template-page {
  display: grid;
  gap: 18px;
  color: #183d42;
}
.template-page > header,
.version-bar,
.overlay section > header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
}
.template-page h1,
.template-page p,
.overlay h2 {
  margin: 0;
}
.template-page > header p {
  color: #738a84;
  font-size: 13px;
  margin-bottom: 6px;
}
.workspace {
  display: grid;
  grid-template-columns: 210px minmax(0, 1fr) 230px;
  min-height: 680px;
  border: 1px solid #dce8e4;
  background: #fff;
}
.workspace aside {
  padding: 12px;
  overflow: auto;
}
.template-list {
  border-right: 1px solid #dce8e4;
}
.workspace aside button {
  display: grid;
  width: 100%;
  gap: 5px;
  padding: 12px;
  border: 0;
  border-radius: 5px;
  background: transparent;
  text-align: left;
  color: #315854;
}
.workspace aside button.active {
  background: #e7f5f1;
}
.workspace small {
  color: #78908a;
}
.workspace main {
  display: grid;
  grid-template-rows: auto minmax(0, 1fr) auto;
  gap: 12px;
  padding: 16px;
}
.field-catalog {
  border-left: 1px solid #dce8e4;
}
.field-catalog h2 {
  margin: 4px 0 10px;
  font-size: 16px;
}
.field-catalog code {
  overflow: hidden;
  text-overflow: ellipsis;
  color: #2f7dd0;
  font-size: 11px;
}
.version-bar label,
.version-bar div {
  display: flex;
  align-items: center;
  gap: 10px;
}
button,
select {
  border: 1px solid #bcd2cb;
  border-radius: 5px;
  background: #fff;
  padding: 9px 12px;
  color: #28555b;
  cursor: pointer;
}
.primary {
  background: #2f7dd0;
  color: #fff;
  border-color: #2f7dd0;
}
textarea {
  width: 100%;
  height: 100%;
  box-sizing: border-box;
  padding: 16px;
  border: 1px solid #cfded9;
  border-radius: 5px;
  font: 13px/1.6 ui-monospace, monospace;
  resize: none;
}
.overlay {
  position: fixed;
  inset: 0;
  z-index: 1000;
  display: grid;
  place-items: center;
  background: rgba(20, 44, 44, 0.48);
  padding: 24px;
}
.overlay section {
  display: grid;
  grid-template-rows: auto minmax(0, 1fr);
  gap: 12px;
  width: min(920px, 96vw);
  height: min(880px, 92vh);
  padding: 18px;
  background: #f4f8f6;
  border-radius: 8px;
}
.overlay iframe {
  width: 100%;
  height: 100%;
  border: 1px solid #dce8e4;
  background: #fff;
}
@media (max-width: 1100px) {
  .workspace {
    grid-template-columns: 190px minmax(0, 1fr);
  }
  .field-catalog {
    grid-column: 1/-1;
    border-left: 0;
    border-top: 1px solid #dce8e4;
    max-height: 260px;
  }
  .version-bar {
    align-items: flex-start;
    flex-direction: column;
  }
}
</style>
