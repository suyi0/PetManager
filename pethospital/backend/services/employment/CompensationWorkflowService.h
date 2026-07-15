#pragma once

#include "../../database/DatabaseManagerInterface.h"

#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

// v6 薪酬闭环：人事拟案/改派 → Boss 管理批准 → Finance 激活 salaryProfile。
// 金额写路径仅经本服务；禁止 salary:write 旁路创建/改 salaryProfile。
namespace CompensationWorkflowService
{
struct OpResult
{
    bool ok = false;
    int httpStatus = 400;
    std::string message;
    std::string errorCode;
    nlohmann::json data = nlohmann::json::object();
};

struct ListResult
{
    bool ok = false;
    int httpStatus = 400;
    std::string message;
    std::string errorCode;
    nlohmann::json items = nlohmann::json::array();
    int total = 0;
    int page = 1;
    int pageSize = 20;
};

struct ListQuery
{
    int operatorUserId = 0;
    std::string status;
    std::string phase;
    int page = 1;
    int pageSize = 20;
    // personnel | boss | finance
    std::string audience;
};

struct CreateRequest
{
    int operatorUserId = 0;
    long long employmentId = 0;
    std::string phase;
    std::string payType;
    std::optional<double> baseSalary;
    std::optional<double> hourlyRate;
    double socialInsuranceHousingFund = 0.0;
    std::string effectiveFrom;
    std::string note;
    // 0 = 默认本人
    int assigneeUserId = 0;
};

struct UpdateRequest
{
    int operatorUserId = 0;
    long long proposalId = 0;
    std::string payType;
    std::optional<double> baseSalary;
    std::optional<double> hourlyRate;
    double socialInsuranceHousingFund = 0.0;
    bool hasSocial = false;
    std::string effectiveFrom;
    std::string note;
    bool hasNote = false;
    int expectedRowVersion = -1;
    bool hasExpectedRowVersion = false;
};

struct IdVersionRequest
{
    int operatorUserId = 0;
    long long proposalId = 0;
    int expectedRowVersion = -1;
    bool hasExpectedRowVersion = false;
    std::string reason;
};

struct ReassignRequest
{
    int operatorUserId = 0;
    long long proposalId = 0;
    int targetAssigneeUserId = 0;
    int expectedRowVersion = -1;
    bool hasExpectedRowVersion = false;
    std::string reason;
};

enum class DecisionAction
{
    Approve,
    Return,
};

struct DecisionRequest
{
    int operatorUserId = 0;
    long long proposalId = 0;
    DecisionAction action = DecisionAction::Approve;
    std::string reason;
    int expectedRowVersion = -1;
    bool hasExpectedRowVersion = false;
};

struct FinanceConfirmRequest
{
    int operatorUserId = 0;
    long long proposalId = 0;
    int expectedRowVersion = -1;
    bool hasExpectedRowVersion = false;
    std::string reason;
};

ListResult listProposals(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    const ListQuery &query);

OpResult createProposal(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    const CreateRequest &request);

OpResult updateProposal(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    const UpdateRequest &request);

OpResult submitProposal(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    const IdVersionRequest &request);

OpResult reassignProposal(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    const ReassignRequest &request);

OpResult decideProposal(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    const DecisionRequest &request);

OpResult confirmActivation(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    const FinanceConfirmRequest &request);

OpResult returnActivation(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    const FinanceConfirmRequest &request);
}
