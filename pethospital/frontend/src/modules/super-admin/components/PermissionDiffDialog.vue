<template>
  <div v-if="open" class="dialog-backdrop" @click.self="$emit('cancel')">
    <section class="dialog" role="dialog" aria-modal="true" aria-labelledby="permission-dialog-title">
      <div class="dialog__head">
        <div><p>保存前核对</p><h3 id="permission-dialog-title">{{ title }}</h3></div>
        <button type="button" class="dialog__close" aria-label="关闭" @click="$emit('cancel')">×</button>
      </div>
      <div class="dialog__body">
        <div class="diff-group diff-group--added">
          <strong>＋ 新增（{{ added.length }}）</strong>
          <p v-if="!added.length" class="empty">无新增权限</p>
          <div v-for="key in added" :key="key" class="diff-row" :class="{ 'diff-row--danger': isDanger(key) }">
            <span>{{ label(key) }}</span><code>{{ key }}</code><b v-if="isDanger(key)">⚠ 高危</b>
          </div>
        </div>
        <div class="diff-group diff-group--removed">
          <strong>－ 移除（{{ removed.length }}）</strong>
          <p v-if="!removed.length" class="empty">无移除权限</p>
          <div v-for="key in removed" :key="key" class="diff-row" :class="{ 'diff-row--danger': isDanger(key) }">
            <span>{{ label(key) }}</span><code>{{ key }}</code><b v-if="isDanger(key)">⚠ 高危</b>
          </div>
        </div>
      </div>
      <div class="dialog__actions">
        <button type="button" class="button button--ghost" @click="$emit('cancel')">取消</button>
        <button type="button" class="button" :disabled="saving" @click="$emit('confirm')">{{ saving ? '保存中…' : '确认并保存' }}</button>
      </div>
    </section>
  </div>
</template>

<script lang="ts">
import { defineComponent } from "vue";
import { PERMISSION_META, permissionLabel } from "@/modules/super-admin/utils/permissionMeta";
export default defineComponent({
  name: "PermissionDiffDialog",
  props: { open: Boolean, title: { type: String, required: true }, added: { type: Array as () => string[], required: true }, removed: { type: Array as () => string[], required: true }, saving: Boolean },
  emits: ["cancel", "confirm"],
  setup() { return { label: permissionLabel, isDanger: (key: string) => Boolean(PERMISSION_META[key]?.danger) }; },
});
</script>

<style scoped>
.dialog-backdrop{position:fixed;inset:0;z-index:1200;display:grid;place-items:center;padding:24px;background:rgba(20,38,63,.42);backdrop-filter:blur(2px)}
.dialog{width:min(680px,100%);max-height:min(720px,calc(100vh - 48px));overflow:auto;background:#fff;border:1px solid #d7e3f2;border-radius:16px;box-shadow:0 24px 64px rgba(20,38,63,.22)}
.dialog__head,.dialog__actions{display:flex;align-items:center;justify-content:space-between;padding:18px 20px;border-bottom:1px solid #d7e3f2}.dialog__head p{margin:0;color:#2f7dd0;font-size:12px;font-weight:800}.dialog__head h3{margin:3px 0 0;color:#14263f}.dialog__close{border:0;background:none;color:#5c7085;font-size:28px;cursor:pointer}.dialog__body{display:grid;gap:14px;padding:18px 20px}.diff-group{display:grid;gap:7px}.diff-group>strong{color:#14263f}.diff-row{display:grid;grid-template-columns:minmax(120px,1fr) minmax(160px,auto) auto;gap:10px;align-items:center;padding:9px 10px;border:1px solid #d7e3f2;border-radius:9px;color:#14263f}.diff-row code{color:#51708f;font-size:11px}.diff-row--danger{border-color:#f2c477;background:#fff8e8}.diff-row b{color:#a85c00;font-size:12px}.empty{margin:0;color:#8090a3;font-size:13px}.dialog__actions{justify-content:flex-end;gap:10px;border-top:1px solid #d7e3f2;border-bottom:0}
</style>
