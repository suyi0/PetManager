const assert = require("node:assert");
const test = require("node:test");

function loadConfig() {
  // vue.config.js 会在 require 时读取 process.env。
  // 测试里会临时切换环境变量，所以每次加载前先清缓存，确保读到最新环境。
  const configPath = require.resolve("./vue.config.js");
  delete require.cache[configPath];
  return require(configPath);
}

test("desktop client builds use relative asset paths", () => {
  // 保存原始环境变量，测试结束后恢复，避免影响同一进程里的其它测试。
  const originalValue = process.env.VUE_APP_DESKTOP_CLIENT;

  // 模拟 Electron client 构建模式。
  // 这个模式会用 loadFile 打开 dist/index.html，资源路径必须是相对路径。
  process.env.VUE_APP_DESKTOP_CLIENT = "true";

  try {
    const config = loadConfig();

    // 如果这里不是 "./"，构建产物会生成 /js/...、/css/... 这样的绝对路径。
    // Electron 用 file:// 打开页面时会找不到这些资源，最终表现为空白窗口。
    assert.equal(config.publicPath, "./");
  } finally {
    // 不管断言是否失败，都恢复环境变量，保持测试之间互不污染。
    if (originalValue === undefined) {
      delete process.env.VUE_APP_DESKTOP_CLIENT;
    } else {
      process.env.VUE_APP_DESKTOP_CLIENT = originalValue;
    }

    // 恢复后重新加载一次配置，避免缓存里留下 client 模式配置。
    loadConfig();
  }
});
