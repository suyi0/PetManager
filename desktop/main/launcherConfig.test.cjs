const assert = require("node:assert");
const test = require("node:test");
const path = require("node:path");

const {
  createLauncherConfig,
  createBackendEnvironment,
  parseEnvFile,
} = require("./launcherConfig.cjs");

const projectRoot = path.resolve(__dirname, "../..");

test("createLauncherConfig resolves project paths", () => {
  // 默认配置代表开发模式：前端 dist 和后端可执行文件都按仓库结构推导。
  const config = createLauncherConfig({ projectRoot });

  assert.equal(config.projectRoot, projectRoot);
  assert.equal(
    config.backendExecutable,
    path.join(projectRoot, "pethospital/backend/build/main")
  );
  assert.equal(
    config.frontendDist,
    path.join(projectRoot, "pethospital/frontend/dist")
  );
  assert.equal(config.appUrl, "http://localhost:8081/");
  assert.equal(config.desktopMode, "dev");
  assert.equal(
    config.frontendIndex,
    path.join(projectRoot, "pethospital/frontend/dist/index.html")
  );
});

test("createLauncherConfig supports client mode", () => {
  // client 模式只关心前端静态入口，不应该要求本地后端先启动。
  const config = createLauncherConfig({
    projectRoot,
    desktopMode: "client",
  });

  assert.equal(config.desktopMode, "client");
  assert.equal(
    config.frontendIndex,
    path.join(projectRoot, "pethospital/frontend/dist/index.html")
  );
});

test("createBackendEnvironment preserves PATH and points backend to dist", () => {
  // 启动本地后端时，需要保留原环境变量，同时覆盖前端 dist 路径。
  const env = createBackendEnvironment({
    baseEnv: {
      PATH: "/usr/bin:/bin",
      DB_HOST: "localhost",
      PETMANAGER_FRONTEND_DIST: "old-value",
    },
    frontendDist: path.join(projectRoot, "pethospital/frontend/dist"),
  });

  assert.equal(env.PATH, "/usr/bin:/bin");
  assert.equal(env.DB_HOST, "localhost");
  assert.equal(
    env.PETMANAGER_FRONTEND_DIST,
    path.join(projectRoot, "pethospital/frontend/dist")
  );
});

test("parseEnvFile ignores comments and PATH", () => {
  // .env 只允许普通配置进入后端环境；PATH 被跳过，避免污染进程执行环境。
  const env = parseEnvFile(`
# comment
DB_HOST=localhost
PATH=/bad/path
EMPTY=
DB_NAME=pethospital
`);

  assert.deepEqual(env, {
    DB_HOST: "localhost",
    EMPTY: "",
    DB_NAME: "pethospital",
  });
});
