#!/usr/bin/env node

// 启动方式：在项目根目录执行：
// bin/repair-chrome-channel.mjs
// 如果不想重启 Chrome，只生成配置和检查：
// bin/repair-chrome-channel.mjs --no-restart
// 如果只是看当前状态，不做修复、不重启：
// bin/repair-chrome-channel.mjs --verify-only --no-restart

import { existsSync, readFileSync } from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { spawnSync } from 'node:child_process';

const codexHome = process.env.CODEX_HOME || path.join(os.homedir(), '.codex');
const latestRoot = path.join(
  codexHome,
  'plugins/cache/openai-bundled/chrome/latest'
);
// The bundled Chrome plugin is versioned and its version changes independently
// from this project. Always resolve through the maintained `latest` link so a
// plugin update does not make this repair script unusable.
const pluginRoot = latestRoot;
const configPath = path.join(
  latestRoot,
  'extension-host/macos/arm64/extension-host-config.json'
);
const registryPath = path.join(codexHome, 'chrome-native-hosts-v2.json');
const openChromeScript = path.join(pluginRoot, 'scripts/open-chrome-window.js');
const installManifestPath = path.join(pluginRoot, 'scripts/installManifest.mjs');
const browserSocketDir =
  process.platform === 'win32'
    ? null
    : '/tmp/codex-browser-use';

const args = new Set(process.argv.slice(2));
const shouldRestartChrome = !args.has('--no-restart');
const shouldVerifyOnly = args.has('--verify-only');

function run(command, args, options = {}) {
  const result = spawnSync(command, args, {
    cwd: options.cwd || process.cwd(),
    encoding: 'utf8',
    stdio: options.stdio || 'pipe',
  });

  if (result.stdout) process.stdout.write(result.stdout);
  if (result.stderr) process.stderr.write(result.stderr);

  if (result.status !== 0) {
    throw new Error(
      `${command} ${args.join(' ')} failed with exit code ${result.status}`
    );
  }

  return result;
}

function readRegistry() {
  if (!existsSync(registryPath)) return {};

  const parsed = JSON.parse(readFileSync(registryPath, 'utf8'));
  return parsed.entries?.[0] || {};
}

async function regenerateConfig() {
  if (!existsSync(installManifestPath)) {
    throw new Error(`Missing install manifest script: ${installManifestPath}`);
  }

  const registry = readRegistry();
  const paths = registry.paths || {};
  const { install } = await import(`file://${installManifestPath}`);

  await install({
    appServerRuntimePaths: {
      codexCliPath:
        paths.codexCliPath ||
        path.join(codexHome, 'plugins/.plugin-appserver/codex'),
      nodePath:
        paths.nodePath ||
        '/Applications/ChatGPT.app/Contents/Resources/cua_node/bin/node',
      nodeReplPath:
        paths.nodeReplPath ||
        '/Applications/ChatGPT.app/Contents/Resources/cua_node/bin/node_repl',
      proxyHost: registry.proxyHost || '127.0.0.1',
      proxyPort: Number.isInteger(registry.proxyPort) ? registry.proxyPort : 0,
    },
  });
}

function verifyStaticChecks() {
  console.log('\n[check] Chrome process');
  run('node', ['scripts/chrome-is-running.js', '--json'], { cwd: pluginRoot });

  console.log('\n[check] ChatGPT Chrome Extension');
  run('node', ['scripts/check-extension-installed.js', '--json'], {
    cwd: pluginRoot,
  });

  console.log('\n[check] Native host manifest');
  run('node', ['scripts/check-native-host-manifest.js', '--json'], {
    cwd: pluginRoot,
  });

  console.log('\n[check] extension-host-config.json');
  if (!existsSync(configPath)) {
    throw new Error(`Missing ${configPath}`);
  }
  console.log(configPath);
}

function verifyRuntimeHints() {
  console.log('\n[check] Runtime bridge hints');

  const registry = readRegistry();
  const proxyPort = registry.proxyPort;
  console.log(`registry: ${registryPath}`);
  console.log(`registry.proxyPort: ${Number.isInteger(proxyPort) ? proxyPort : 'missing'}`);

  if (browserSocketDir && existsSync(browserSocketDir)) {
    const result = spawnSync(
      'find',
      [browserSocketDir, '-maxdepth', '1', '-type', 's', '-print'],
      { encoding: 'utf8', stdio: 'pipe' }
    );
    if (result.status !== 0) {
      throw new Error(
        `find ${browserSocketDir} -maxdepth 1 -type s -print failed with exit code ${result.status}`
      );
    }
    const sockets = result.stdout
      .trim()
      .split('\n')
      .filter(Boolean);
    console.log(`browser sockets: ${sockets.length}`);
    sockets.slice(-5).forEach((socketPath) => console.log(socketPath));
  } else if (browserSocketDir) {
    console.log(`browser sockets: missing ${browserSocketDir}`);
  }

  console.log(
    [
      'note: Static Chrome checks can pass while the current Codex session still',
      'cannot control Chrome. If browser discovery remains empty, verify that',
      '`nodeRepl.nativePipe` exists in the active Codex session. When it is missing,',
      'reload bundled plugins from the ChatGPT/Codex desktop app or restart the app',
      'and return to this project.',
    ].join(' ')
  );
}

function restartChrome() {
  console.log('\n[restart] Closing Google Chrome');
  run('osascript', ['-e', 'tell application "Google Chrome" to quit']);

  console.log('\n[restart] Opening Google Chrome Default profile');
  run('node', [openChromeScript], { cwd: pluginRoot });
}

async function main() {
  if (process.platform !== 'darwin') {
    throw new Error('This repair script currently supports macOS only.');
  }

  if (!existsSync(pluginRoot)) {
    throw new Error(`Missing Chrome plugin root: ${pluginRoot}`);
  }

  if (!shouldVerifyOnly) {
    console.log('[repair] Regenerating Chrome native host config');
    await regenerateConfig();
    console.log(`[repair] Generated ${configPath}`);
  }

  if (shouldRestartChrome) {
    restartChrome();
  }

  verifyStaticChecks();
  verifyRuntimeHints();

  console.log('\nDone. Return to Codex and run: 继续验证 Chrome 通道');
}

main().catch((error) => {
  console.error(`\nRepair failed: ${error.message}`);
  process.exit(1);
});
