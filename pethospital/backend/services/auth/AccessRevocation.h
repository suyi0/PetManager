#pragma once

namespace AccessRevocation
{
    // 访问权变更（派职位/改权限/转客户）后的下游一致性三件套，缺一即留"降权不生效"窗口：
    // 1. 角色名缓存失效（展示层不再读到旧职位名）
    // 2. 会话版本 bump（该用户所有已签发 token 下一请求即失效）
    // 3. 断开全部实时连接（WS 在 onaccept 只校验一次，必须踢掉重连重鉴权）
    // admin 与 personnel 的派职路径都必须走这里，不要各自手写子集。
    void revokeUserSessions(int userId);      // 1 + 2
    void closeRealtimeConnections();          // 3（全量断开，重连时按新权限重新准入）
    void onUserAccessChanged(int userId);     // 1 + 2 + 3
}
