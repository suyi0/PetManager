<template>
  <component :is="isMobile ? mobile : desktop" />
</template>

<script setup lang="ts">
import type { Component } from "vue";
import { useViewport } from "@/shared/composables/useViewport";

defineProps<{
  mobile: Component;
  desktop: Component;
}>();

const { isMobile } = useViewport();
</script>
