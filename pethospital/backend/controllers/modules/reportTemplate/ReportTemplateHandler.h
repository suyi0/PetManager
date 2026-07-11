#pragma once

#include "../../../utils/Utils.h"

class ReportTemplateHandler : public BaseHandler
{
public:
    explicit ReportTemplateHandler(std::shared_ptr<DatabaseManagerInterface> db)
        : BaseHandler(db), dbManager(std::move(db)) {}

    crow::response dataContract(const crow::request &req);
    crow::response list(const crow::request &req);
    crow::response versions(const crow::request &req, int templateId);
    crow::response createVersion(const crow::request &req, int templateId, int actorId);
    crow::response publish(const crow::request &req, int templateId, int actorId);

private:
    std::shared_ptr<DatabaseManagerInterface> dbManager;
};
