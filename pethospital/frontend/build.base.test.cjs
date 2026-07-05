const assert = require("node:assert");
const test = require("node:test");

const { resolvePublicBase } = require("./buildBase.cjs");

test("desktop client builds use relative asset paths", () => {
  // Electron 用 loadFile 打开 dist/index.html，如果 base 不是 "./"，
  // 产物会引用 /assets/... 绝对路径，file:// 下找不到，表现为空白窗口。
  assert.equal(resolvePublicBase({ VUE_APP_DESKTOP_CLIENT: "true" }), "./");
});

test("web builds use absolute asset paths", () => {
  assert.equal(resolvePublicBase({}), "/");
  assert.equal(resolvePublicBase({ VUE_APP_DESKTOP_CLIENT: "false" }), "/");
});
