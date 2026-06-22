const path = require("node:path");

const DEFAULT_APP_URL = "http://localhost:8081/";
const DEFAULT_DESKTOP_MODE = "dev";

// 创建启动参数
function createLauncherConfig(options = {}) {
  // projectRoot 是仓库根目录。默认从 desktop/main 往上两级回到 PetManager 根目录。
  // 测试可以传 options.projectRoot，避免依赖当前机器的绝对路径。
  const projectRoot = path.resolve(options.projectRoot || path.join(__dirname, "../.."));

  // 前端构建产物目录。client 模式会直接打开这里的 index.html；
  // dev 模式会把这个目录交给本地后端，让后端托管静态文件。
  const frontendDist =
    options.frontendDist || path.join(projectRoot, "pethospital/frontend/dist");

  return {
    projectRoot,
    // desktopMode 控制启动策略：
    // - dev：Electron 可以启动/复用本地 C++ 后端，然后访问 http://localhost:8081/
    // - client：Electron 只打开前端 dist/index.html，不启动本地后端
    desktopMode:
      options.desktopMode ||
      process.env.PETMANAGER_DESKTOP_MODE ||
      DEFAULT_DESKTOP_MODE,
    // dev 模式下访问的本地后端地址。后端负责返回前端页面和处理 API。
    appUrl: options.appUrl || DEFAULT_APP_URL,
    // dev 模式下需要启动的 C++ 后端可执行文件。
    backendExecutable:
      options.backendExecutable ||
      path.join(projectRoot, "pethospital/backend/build/main"),
    frontendDist,
    // client 模式下 Electron loadFile 的入口文件。
    frontendIndex: path.join(frontendDist, "index.html"),
  };
}

// 创建本地 C++ 后端的环境变量
function createBackendEnvironment({ baseEnv = process.env, frontendDist }) {
  // 给本地 C++ 后端补充前端 dist 路径，让后端知道从哪里托管静态页面。
  // 保留 baseEnv 里的其它变量，例如 PATH、数据库配置等。
  return {
    ...baseEnv,
    PETMANAGER_FRONTEND_DIST: frontendDist,
  };
}

// 解析 .env 文件
function parseEnvFile(content) {
  // 只解析简单的 KEY=value 行，用于把项目根目录 .env 传给本地后端。
  // 这里不解析复杂 shell 语法，避免把 .env 当成可执行脚本。
  const env = {};

  for (const rawLine of content.split(/\r?\n/)) {
    const line = rawLine.trim();
    if (!line || line.startsWith("#") || !line.includes("=")) {
      continue;
    }

    const separatorIndex = line.indexOf("=");
    const key = line.slice(0, separatorIndex).trim();
    const value = line.slice(separatorIndex + 1);

    // 不接受 PATH 覆盖，避免 .env 意外改变后端进程查找系统命令/动态库的路径。
    if (!key || key === "PATH") {
      continue;
    }

    env[key] = value;
  }

  return env;
}

module.exports = {
  createLauncherConfig,
  createBackendEnvironment,
  parseEnvFile,
};
