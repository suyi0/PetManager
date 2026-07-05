// 源码沿用 Vue CLI 时代的 process.env.* 读法，Vite 在构建期由 vite.config.ts 的
// define 做文本替换。这里声明被替换的确切键集合，供 TypeScript 使用；
// 新增 VUE_APP_* 变量时须同步 vite.config.ts 的 define 与本声明。
declare const process: {
  env: {
    NODE_ENV: string;
    BASE_URL: string;
    VUE_APP_DESKTOP_CLIENT?: string;
    VUE_APP_API_BASE_URL?: string;
    VUE_APP_LOGIN_STATUS_WS_URL?: string;
  };
};
