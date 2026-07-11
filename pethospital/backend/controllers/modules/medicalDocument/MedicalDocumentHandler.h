#pragma once

#include "../../../utils/Utils.h"

class MedicalDocumentHandler : public BaseHandler
{
public:
    explicit MedicalDocumentHandler(std::shared_ptr<DatabaseManagerInterface> db)
        : BaseHandler(db), dbManager(std::move(db)) {}

    crow::response getByOrder(const crow::request &req, int orderId);
    crow::response updateDraft(const crow::request &req, int orderId, int actorId);
    crow::response preview(const crow::request &req, int orderId);
    crow::response finalize(const crow::request &req, int orderId, int actorId);
    crow::response downloadLatestPdf(const crow::request &req, int orderId);

private:
    std::shared_ptr<DatabaseManagerInterface> dbManager;

    nlohmann::json loadDocument(int orderId);
    nlohmann::json buildReportPayload(const nlohmann::json &document) const;
};
