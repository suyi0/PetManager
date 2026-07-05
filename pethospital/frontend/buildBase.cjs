// 构建 base（publicPath）的单一决定点，vite.config.ts 与 build.base.test.cjs 共用。
// Electron 桌面端用 file:// 打开 dist/index.html，资源路径必须是相对路径 "./"；
// Web 部署用绝对路径 "/"。
function resolvePublicBase(env = process.env) {
  return env.VUE_APP_DESKTOP_CLIENT === "true" ? "./" : "/";
}

module.exports = { resolvePublicBase };
