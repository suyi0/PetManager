#include "../middleware/rateLimitMiddleware/rateLimitMiddleware.h"

#include <cassert>

int main()
{
    assert(!RateLimitMiddleware::shouldRateLimitPath("/", "GET"));
    assert(!RateLimitMiddleware::shouldRateLimitPath("/js/app.js", "GET"));
    assert(!RateLimitMiddleware::shouldRateLimitPath("/css/app.css", "GET"));
    assert(!RateLimitMiddleware::shouldRateLimitPath("/service-worker.js", "GET"));
    assert(!RateLimitMiddleware::shouldRateLimitPath("/manifest.json", "GET"));
    assert(!RateLimitMiddleware::shouldRateLimitPath("/api/users/sessions", "OPTIONS"));

    assert(RateLimitMiddleware::shouldRateLimitPath("/api/users/sessions", "POST"));
    assert(RateLimitMiddleware::shouldRateLimitPath("/api/users/me/profile", "GET"));
    assert(RateLimitMiddleware::shouldRateLimitPath("/realtime/admins/home-data", "GET"));

    return 0;
}
