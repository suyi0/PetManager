const { defineConfig } = require("@vue/cli-service");
module.exports = defineConfig({
  transpileDependencies: true,
  devServer: {
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
      "/websocket": {
        target: "http://localhost:8081",
        ws: true,
        changeOrigin: true,
        secure: false,
      },
      "/ws": {
        target: "http://localhost:8081",
        ws: true,
        changeOrigin: true,
        secure: false,
      },
    },
  },
});
