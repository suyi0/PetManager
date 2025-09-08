const { defineConfig } = require("@vue/cli-service");
module.exports = defineConfig({
  transpileDependencies: true,
  devServer: {
    host: "localhost",
    port: 8080,
    proxy: {
      "/api": {
        target: "http://localhost:8081",
        changeOrigin: true,
        secure: false,
      },
      "/websocket": {
        target: "http://localhost:8081",
        ws: true,
        changeOrigin: true,
        secure: false,
      },
    },
  },
});
