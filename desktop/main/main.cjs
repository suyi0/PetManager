const fs = require("node:fs");  // 引入 node:fs 模块
const { spawn } = require("node:child_process");  // 引入 node:child_process 模块
const http = require("node:http");  // 引入 node:http 模块
const path = require("node:path");  // 引入 node:path 模块
const { app, BrowserWindow, dialog } = require("electron"); // 引入 Electron 模块
const {
  createBackendEnvironment,
  createLauncherConfig,
  parseEnvFile,
} = require("./launcherConfig.cjs");

// Electron 主进程的运行状态：
// mainWindow 是桌面窗口；backendProcess 只在 dev 模式下可能由 Electron 启动。
// ownsBackendProcess 表示当前进程是否启动了后端进程。； appIsQuitting 表示当前进程正在退出。
let mainWindow = null;
let backendProcess = null;
let ownsBackendProcess = false;
let appIsQuitting = false;

// launcherConfig 负责集中决定当前模式、前端 dist 路径、本地后端地址等启动配置。
const config = createLauncherConfig();

// 读取 .env 文件，并把变量设置到环境变量中。
function loadDotEnv(projectRoot) {
  // dev 模式启动本地后端时，把项目根目录的 .env 读进后端进程环境变量。
  const envPath = path.join(projectRoot, ".env");
  if (!fs.existsSync(envPath)) {
    return {};
  }

  return parseEnvFile(fs.readFileSync(envPath, "utf8"));
}

// 等待一个 HTTP 地址可访问 (默认 30s )
function waitForUrl(url, timeoutMs = 30000) {
  // 轮询一个 HTTP 地址，直到它能响应或超时。用于确认本地后端是否已经可访问。
  const deadline = Date.now() + timeoutMs;

  return new Promise((resolve, reject) => {
    const check = () => {
      const request = http.get(url, (response) => {
        response.resume();
        resolve(true);
      });

      request.on("error", () => {
        if (Date.now() >= deadline) {
          reject(new Error(`Timed out waiting for ${url}`));
          return;
        }

        setTimeout(check, 500);
      });

      request.setTimeout(2000, () => {
        request.destroy();
      });
    };

    check();
  });
}

// 检测本地后端是否已启动
async function isBackendAlreadyRunning() {
  // dev 模式下优先复用已经启动的本地后端，避免 Electron 重复启动一个服务。
  try {
    await waitForUrl(config.appUrl, 1000);
    return true;
  } catch {
    return false;
  }
}

// 创建桌面窗口
function createWindow() {
  // BrowserWindow 是真正展示给用户看的桌面窗口。
  // 安全相关配置保持保守：渲染进程不能直接使用 Node，降低页面代码接触系统能力的风险。
  mainWindow = new BrowserWindow({
    width: 1280,
    height: 860,
    minWidth: 1024,
    minHeight: 720,
    title: "PetManager",
    backgroundColor: "#f4f7f4",
    webPreferences: {
      contextIsolation: true,
      nodeIntegration: false,
      sandbox: true,
    },
  });

  if (config.desktopMode === "client") {
    // client 模式：只加载本地构建好的前端静态文件，不启动本地 C++ 后端。
    // 前端 API 会通过 VUE_APP_API_BASE_URL 连接本地或远程服务器。
    mainWindow.loadFile(config.frontendIndex);
    return;
  }

  // dev 模式：访问本地后端托管的页面，例如 http://localhost:8081/。
  mainWindow.loadURL(config.appUrl);
}

// 启动本地后端
function startBackend() {
  // 只有 dev 模式会走到这里。client 模式不会启动本地后端。
  if (!fs.existsSync(config.backendExecutable)) {
    throw new Error(
      `Backend executable not found: ${config.backendExecutable}\nRun: bash bin/build.sh`
    );
  }

  // 本地后端会托管前端 dist，所以启动前必须确认前端已经构建过。
  if (!fs.existsSync(path.join(config.frontendDist, "index.html"))) {
    throw new Error(
      `Frontend dist not found: ${config.frontendDist}\nRun: cd pethospital/frontend && npm run build`
    );
  }

  const env = createBackendEnvironment({
    baseEnv: {
      ...process.env,
      ...loadDotEnv(config.projectRoot),
    },
    frontendDist: config.frontendDist,
  });

  // 启动 C++ 后端可执行文件，并把 stdout/stderr 转发到当前终端，方便调试。
  backendProcess = spawn(config.backendExecutable, {
    cwd: config.projectRoot,
    env,
    stdio: ["ignore", "pipe", "pipe"],
  });
  ownsBackendProcess = true;

  backendProcess.stdout.on("data", (chunk) => {
    process.stdout.write(`[backend] ${chunk}`);
  });
  backendProcess.stderr.on("data", (chunk) => {
    process.stderr.write(`[backend] ${chunk}`);
  });
  backendProcess.on("exit", (code, signal) => {
    if (mainWindow && ownsBackendProcess && !appIsQuitting) {
      // 如果是 Electron 启动的后端意外退出，给用户一个明确错误，而不是让窗口静默失效。
      void dialog.showMessageBox(mainWindow, {
        type: "error",
        title: "后端服务已停止",
        message: `PetManager 后端服务已退出。code=${code ?? ""} signal=${signal ?? ""}`,
      });
    }
  });
}

// 启动应用
async function startApp() {
  // client 模式启动路径：
  // 1. 检查前端 dist/index.html 是否存在；
  // 2. 直接打开这个静态文件；
  // 3. 不检查、不启动本地后端。
  if (config.desktopMode === "client") {
    if (!fs.existsSync(config.frontendIndex)) {
      throw new Error(
        `Frontend dist not found: ${config.frontendDist}\nRun: npm run desktop:prepare:client`
      );
    }

    createWindow();
    return;
  }

  // dev 模式启动路径：
  // 1. 如果 localhost 后端已在运行，就直接复用；
  // 2. 否则由 Electron 启动本地后端；
  // 3. 等后端可访问后再创建窗口。
  if (!(await isBackendAlreadyRunning())) {
    startBackend();
    await waitForUrl(config.appUrl);
  }

  createWindow();
}

// 停止本地后端
function stopBackend() {
  // 只关闭由当前 Electron 进程启动的后端，避免误杀用户自己手动启动的服务。
  if (!ownsBackendProcess || !backendProcess || backendProcess.killed) {
    return;
  }

  backendProcess.kill("SIGTERM");
}

app.whenReady().then(() => {
  // Electron 初始化完成后开始启动应用；启动失败时弹窗展示具体原因。
  startApp().catch((error) => {
    void dialog.showMessageBox({
      type: "error",
      title: "PetManager 启动失败",
      message: error instanceof Error ? error.message : String(error),
    });
    app.quit();
  });
});

app.on("before-quit", () => {
  // 标记为主动退出，避免后端被关闭时触发“后端服务已停止”的误报弹窗。
  appIsQuitting = true;
  stopBackend();
});

app.on("window-all-closed", () => {
  // 所有窗口关闭后退出应用。macOS 上有些应用会保留菜单栏进程，这里选择直接退出。
  app.quit();
});
