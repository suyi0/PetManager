import { fileURLToPath, URL } from "node:url";

import vue from "@vitejs/plugin-vue";
import { defineConfig } from "vite";
import { VitePWA } from "vite-plugin-pwa";

import { resolvePublicBase } from "./buildBase.cjs";

// base 由 VUE_APP_DESKTOP_CLIENT 决定（"./" 或 "/"），逻辑在 buildBase.cjs，
// 由根目录 npm run desktop:test 覆盖测试。
const base = resolvePublicBase();

export default defineConfig({
  base,
  plugins: [
    vue(),
    // 迁移自 @vue/cli-plugin-pwa（GenerateSW 模式）：
    // manifest 与产物文件名（service-worker.js / manifest.json）保持与旧构建一致，
    // 注册逻辑沿用 src/registerServiceWorker.ts，所以关闭插件自带的注册注入。
    VitePWA({
      filename: "service-worker.js",
      manifestFilename: "manifest.json",
      injectRegister: null,
      manifest: {
        name: "pethospital",
        short_name: "pethospital",
        theme_color: "#4DBA87",
        start_url: ".",
        display: "standalone",
        background_color: "#000000",
        icons: [
          {
            src: "./img/icons/android-chrome-192x192.png",
            sizes: "192x192",
            type: "image/png",
          },
          {
            src: "./img/icons/android-chrome-512x512.png",
            sizes: "512x512",
            type: "image/png",
          },
          {
            src: "./img/icons/android-chrome-maskable-192x192.png",
            sizes: "192x192",
            type: "image/png",
            purpose: "maskable",
          },
          {
            src: "./img/icons/android-chrome-maskable-512x512.png",
            sizes: "512x512",
            type: "image/png",
            purpose: "maskable",
          },
        ],
      },
    }),
  ],
  resolve: {
    alias: {
      "@": fileURLToPath(new URL("./src", import.meta.url)),
    },
  },
  build: {
    // 不能用 Vite 默认的 "assets"：后端 staticFileHandler 把 /assets 视为后端自有路径
    // （isBackendOwnedPath），生产环境会拦截产物请求导致 404。
    assetsDir: "static",
  },
  // 源码沿用 Vue CLI 时代的 process.env.* 读法，这里在构建期做等价替换，
  // 避免为迁移改动业务文件；新代码请改用 import.meta.env。
  define: {
    "process.env.BASE_URL": JSON.stringify(base),
    "process.env.VUE_APP_DESKTOP_CLIENT": JSON.stringify(
      process.env.VUE_APP_DESKTOP_CLIENT ?? ""
    ),
    "process.env.VUE_APP_API_BASE_URL": JSON.stringify(
      process.env.VUE_APP_API_BASE_URL ?? ""
    ),
    "process.env.VUE_APP_LOGIN_STATUS_WS_URL": JSON.stringify(
      process.env.VUE_APP_LOGIN_STATUS_WS_URL ?? ""
    ),
  },
  server: {
    host: "0.0.0.0",
    port: 8080,
    proxy: {
      "/uploads": {
        target: "http://localhost:8081",
        changeOrigin: true,
        secure: false,
      },
      "/api": {
        target: "http://localhost:8081",
        changeOrigin: true,
        secure: false,
      },
      "/assets": {
        target: "http://localhost:8081",
        changeOrigin: true,
        secure: false,
      },
      "/realtime": {
        target: "http://localhost:8081",
        ws: true,
        changeOrigin: true,
        secure: false,
      },
    },
  },
});
