#pragma once
#include <crow.h>

// 数据库会话请求边界守卫：thread_local 会话被同线程的所有请求复用，
// 任何 handler 带着未完成事务返回（异常路径 / 漏 commit）都会把事务状态
// 泄漏给下一个请求（脏快照读 / 写入并入僵尸事务 / 行锁滞留）。
// 本中间件在请求边界调用 DatabaseManager::endOfRequestCleanup() 做防御性
// ROLLBACK：after_handle 在 handler 完成后立即清理；before_handle 兜底覆盖
// "上一个请求异常导致 after_handle 未执行"的情况。
// 开销：只有真正取用过会话的请求才产生一次 ROLLBACK 往返（脏标记门控）。
class DbSessionGuardMiddleware
{
public:
    struct context
    {
    };

    void before_handle(crow::request &req, crow::response &res, context &ctx);
    void after_handle(crow::request &req, crow::response &res, context &ctx);
};
