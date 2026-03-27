#pragma once

// 统一业务码：
// 所有接口都应优先复用这些常量，避免在业务代码中继续使用魔法数字。
namespace ResponseCode
{
    inline constexpr int Success = 0;

    inline constexpr int BadRequest = 1000;
    inline constexpr int ValidationError = 1001;

    inline constexpr int AuthError = 2001;
    inline constexpr int VerificationCodeInvalid = 2002;
    inline constexpr int PermissionDenied = 2003;

    inline constexpr int NotFound = 3001;

    inline constexpr int BusinessConflict = 4009;

    inline constexpr int InternalServerError = 5000;
    inline constexpr int DatabaseError = 5001;
    inline constexpr int SystemError = 5002;
    inline constexpr int ServiceUnavailable = 5003;
}

// 统一错误类型：
// 与业务码配套使用，便于前端分类处理和日志筛选。
namespace ResponseErrorType
{
    inline constexpr const char *BadRequest = "BAD_REQUEST";
    inline constexpr const char *ValidationError = "VALIDATION_ERROR";
    inline constexpr const char *AuthError = "AUTH_ERROR";
    inline constexpr const char *PermissionError = "PERMISSION_ERROR";
    inline constexpr const char *NotFound = "NOT_FOUND";
    inline constexpr const char *BusinessConflict = "BUSINESS_CONFLICT";
    inline constexpr const char *DatabaseError = "DATABASE_ERROR";
    inline constexpr const char *SystemError = "SYSTEM_ERROR";
    inline constexpr const char *ServiceUnavailable = "SERVICE_UNAVAILABLE";
}
