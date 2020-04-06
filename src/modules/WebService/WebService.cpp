/*++

Library name:

  apostol-module

Module Name:

  WebService.cpp

Notices:

  WebService - Deal Module Web Service

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

//----------------------------------------------------------------------------------------------------------------------

#include "Core.hpp"
#include "WebService.hpp"
//----------------------------------------------------------------------------------------------------------------------

#include <random>
//----------------------------------------------------------------------------------------------------------------------

#include "yaml-cpp/yaml.h"
//----------------------------------------------------------------------------------------------------------------------

#define BM_PREFIX "BM-"
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Module {

        CString to_string(unsigned long Value) {
            TCHAR szString[_INT_T_LEN + 1] = {0};
            sprintf(szString, "%lu", Value);
            return CString(szString);
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CWebService -----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CWebService::CWebService(CModuleManager *AManager): CApostolModule(AManager) {
            m_Version = -1;
            m_ServerIndex = -1;
            m_FixedDate = Now();
            m_RandomDate = Now();
            m_LocalHost = "http://localhost:";
            m_LocalHost << BPS_SERVER_PORT;
            m_ProxyManager = new CHTTPProxyManager();
        }
        //--------------------------------------------------------------------------------------------------------------

        CWebService::~CWebService() {
            delete m_ProxyManager;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::InitMethods() {
            m_Methods.AddObject(_T("GET"), (CObject *) new CMethodHandler(true, std::bind(&CWebService::DoGet, this, _1)));
            m_Methods.AddObject(_T("POST"), (CObject *) new CMethodHandler(true, std::bind(&CWebService::DoPost, this, _1)));
            m_Methods.AddObject(_T("OPTIONS"), (CObject *) new CMethodHandler(true, std::bind(&CWebService::DoOptions, this, _1)));
            m_Methods.AddObject(_T("PUT"), (CObject *) new CMethodHandler(false, std::bind(&CWebService::MethodNotAllowed, this, _1)));
            m_Methods.AddObject(_T("DELETE"), (CObject *) new CMethodHandler(false, std::bind(&CWebService::MethodNotAllowed, this, _1)));
            m_Methods.AddObject(_T("TRACE"), (CObject *) new CMethodHandler(false, std::bind(&CWebService::MethodNotAllowed, this, _1)));
            m_Methods.AddObject(_T("HEAD"), (CObject *) new CMethodHandler(false, std::bind(&CWebService::MethodNotAllowed, this, _1)));
            m_Methods.AddObject(_T("PATCH"), (CObject *) new CMethodHandler(false, std::bind(&CWebService::MethodNotAllowed, this, _1)));
            m_Methods.AddObject(_T("CONNECT"), (CObject *) new CMethodHandler(false, std::bind(&CWebService::MethodNotAllowed, this, _1)));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::DebugRequest(CRequest *ARequest) {
            DebugMessage("[%p] Request:\n%s %s HTTP/%d.%d\n", ARequest, ARequest->Method.c_str(), ARequest->Uri.c_str(), ARequest->VMajor, ARequest->VMinor);

            for (int i = 0; i < ARequest->Headers.Count(); i++)
                DebugMessage("%s: %s\n", ARequest->Headers[i].Name.c_str(), ARequest->Headers[i].Value.c_str());

            if (!ARequest->Content.IsEmpty())
                DebugMessage("\n%s\n", ARequest->Content.c_str());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::DebugReply(CReply *AReply) {
            DebugMessage("[%p] Reply:\nHTTP/%d.%d %d %s\n", AReply, AReply->VMajor, AReply->VMinor, AReply->Status, AReply->StatusText.c_str());

            for (int i = 0; i < AReply->Headers.Count(); i++)
                DebugMessage("%s: %s\n", AReply->Headers[i].Name.c_str(), AReply->Headers[i].Value.c_str());

            if (!AReply->Content.IsEmpty())
                DebugMessage("\n%s\n", AReply->Content.c_str());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::DebugConnection(CHTTPServerConnection *AConnection) {
            DebugMessage("\n[%p] [%s:%d] [%d] ", AConnection, AConnection->Socket()->Binding()->PeerIP(),
                         AConnection->Socket()->Binding()->PeerPort(), AConnection->Socket()->Binding()->Handle());

            DebugRequest(AConnection->Request());

            static auto OnReply = [](CObject *Sender) {
                auto LConnection = dynamic_cast<CHTTPServerConnection *> (Sender);
                auto LBinding = LConnection->Socket()->Binding();

                if (Assigned(LBinding)) {
                    DebugMessage("\n[%p] [%s:%d] [%d] ", LConnection, LBinding->PeerIP(),
                                 LBinding->PeerPort(), LBinding->Handle());
                }

                DebugReply(LConnection->Reply());
            };

            AConnection->OnReply(OnReply);
        }
        //--------------------------------------------------------------------------------------------------------------

        CDateTime CWebService::GetRandomDate(int a, int b, CDateTime Date) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> time(a, b);
            CDateTime delta = time(gen);
            return Date + (CDateTime) (delta / 86400);
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CWebService::FindURLInLine(const CString &Line, CStringList &List) {
            CString URL;

            TCHAR ch;
            int Length = 0;
            size_t StartPos, Pos;

            Pos = 0;
            while ((StartPos = Line.Find(HTTP_PREFIX, Pos)) != CString::npos) {

                URL.Clear();

                Pos = StartPos + HTTP_PREFIX_SIZE;
                if (Line.Length() < 5)
                    return false;

                URL.Append(HTTP_PREFIX);
                ch = Line.at(Pos);
                if (ch == 's') {
                    URL.Append(ch);
                    Pos++;
                }

                if (Line.Length() < 7 || Line.at(Pos++) != ':' || Line.at(Pos++) != '/' || Line.at(Pos++) != '/')
                    return false;

                URL.Append("://");

                Length = 0;
                ch = Line.at(Pos);
                while (ch != 0 && (IsChar(ch) || IsNumeral(ch) || ch == ':' || ch == '.' || ch == '-')) {
                    URL.Append(ch);
                    Length++;
                    ch = Line.at(++Pos);
                }

                if (Length < 3) {
                    return false;
                }

                if (StartPos == 0) {
                    List.Add(URL);
                } else {
                    ch = Line.at(StartPos - 1);
                    switch (ch) {
                        case ' ':
                        case ',':
                        case ';':
                            List.Add(URL);
                            break;
                        default:
                            return false;
                    }
                }
            }

            return true;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::CheckKeyForNull(LPCTSTR Key, LPCTSTR Value) {
            if (Value == nullptr)
                throw ExceptionFrm("Invalid format: key \"%s\" cannot be empty.", Key);
        }
        //--------------------------------------------------------------------------------------------------------------

        int CWebService::CheckFee(const CString &Fee) {
            if (!Fee.IsEmpty()) {

                if (Fee.Length() >= 10)
                    return -1;

                size_t Numbers = 0;
                size_t Delimiter = 0;
                size_t Percent = 0;

                size_t Pos = 0;
                TCHAR ch;

                ch = Fee.at(Pos);
                while (ch != 0) {
                    if (IsNumeral(ch))
                        Numbers++;
                    if (ch == '.')
                        Delimiter++;
                    if (ch == '%')
                        Percent++;
                    ch = Fee.at(++Pos);
                }

                if (Numbers == 0 || Delimiter > 1 || Percent > 1 || ((Numbers + Percent + Delimiter) != Fee.Length()))
                    return -1;

                return 1;
            }

            return 0;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::ExceptionToJson(int ErrorCode, const std::exception &AException, CString& Json) {
            Json.Format(R"({"error": {"code": %u, "message": "%s"}})", ErrorCode, Delphi::Json::EncodeJsonString(AException.what()).c_str());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::DoVerbose(CSocketEvent *Sender, CTCPConnection *AConnection, LPCTSTR AFormat, va_list args) {
            Log()->Debug(0, AFormat, args);
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CWebService::DoProxyExecute(CTCPConnection *AConnection) {
            auto LConnection = dynamic_cast<CHTTPClientConnection*> (AConnection);
            auto LProxy = dynamic_cast<CHTTPProxy*> (LConnection->Client());

            auto LServerRequest = LProxy->Connection()->Request();
            auto LServerReply = LProxy->Connection()->Reply();
            auto LProxyReply = LConnection->Reply();

            const auto& Format = LServerRequest->Params["payload"];
            if (!Format.IsEmpty()) {

                if (Format == "html") {
                    LServerReply->ContentType = CReply::html;
                } else if (Format == "json") {
                    LServerReply->ContentType = CReply::json;
                } else if (Format == "xml") {
                    LServerReply->ContentType = CReply::xml;
                } else {
                    LServerReply->ContentType = CReply::text;
                }

                if (LProxyReply->Status == CReply::ok) {
                    if (!LProxyReply->Content.IsEmpty()) {
                        const CJSON json(LProxyReply->Content);
                        LServerReply->Content = base64_decode(json["payload"].AsString());
                    }
                    LProxy->Connection()->SendReply(LProxyReply->Status, nullptr, true);
                } else {
                    LProxy->Connection()->SendStockReply(LProxyReply->Status, true);
                }
            } else {
                if (LProxyReply->Status == CReply::ok) {
                    LServerReply->Content = LProxyReply->Content;
                    LProxy->Connection()->SendReply(LProxyReply->Status, nullptr, true);
                } else {
                    LProxy->Connection()->SendStockReply(LProxyReply->Status, true);
                }
            }

            LConnection->CloseConnection(true);

            return true;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::DoProxyException(CTCPConnection *AConnection, Delphi::Exception::Exception *AException) {
            auto LConnection = dynamic_cast<CHTTPClientConnection*> (AConnection);
            auto LProxy = dynamic_cast<CHTTPProxy*> (LConnection->Client());

            auto LServerRequest = LProxy->Connection()->Request();
            auto LServerReply = LProxy->Connection()->Reply();

            const auto& Format = LServerRequest->Params["format"];
            if (Format == "html") {
                LServerReply->ContentType = CReply::html;
            }

            LProxy->Connection()->SendStockReply(CReply::bad_gateway, true);
            Log()->Error(APP_LOG_EMERG, 0, "[%s:%d] %s", LProxy->Host().c_str(), LProxy->Port(),
                    AException->what());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::DoEventHandlerException(CPollEventHandler *AHandler, Delphi::Exception::Exception *AException) {
            auto LConnection = dynamic_cast<CHTTPClientConnection*> (AHandler->Binding());
            auto LProxy = dynamic_cast<CHTTPProxy*> (LConnection->Client());

            if (Assigned(LProxy)) {
                auto LReply = LProxy->Connection()->Reply();
                ExceptionToJson(0, *AException, LReply->Content);
                LProxy->Connection()->SendReply(CReply::internal_server_error, nullptr, true);
            }

            Log()->Error(APP_LOG_EMERG, 0, AException->what());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::DoProxyConnected(CObject *Sender) {
            auto LConnection = dynamic_cast<CHTTPClientConnection*> (Sender);
            if (LConnection != nullptr) {
                Log()->Message(_T("[%s:%d] Client connected."), LConnection->Socket()->Binding()->PeerIP(),
                               LConnection->Socket()->Binding()->PeerPort());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::DoProxyDisconnected(CObject *Sender) {
            auto LConnection = dynamic_cast<CHTTPClientConnection*> (Sender);
            if (LConnection != nullptr) {
                Log()->Message(_T("[%s:%d] Client disconnected."), LConnection->Socket()->Binding()->PeerIP(),
                               LConnection->Socket()->Binding()->PeerPort());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        CHTTPProxy *CWebService::GetProxy(CHTTPServerConnection *AConnection) {
            auto LProxy = m_ProxyManager->Add(AConnection);

            LProxy->OnExecute(std::bind(&CWebService::DoProxyExecute, this, _1));

            LProxy->OnVerbose(std::bind(&CWebService::DoVerbose, this, _1, _2, _3, _4));

            LProxy->OnException(std::bind(&CWebService::DoProxyException, this, _1, _2));
            LProxy->OnEventHandlerException(std::bind(&CWebService::DoEventHandlerException, this, _1, _2));

            LProxy->OnConnected(std::bind(&CWebService::DoProxyConnected, this, _1));
            LProxy->OnDisconnected(std::bind(&CWebService::DoProxyDisconnected, this, _1));

            //LProxy->OnNoCommandHandler(std::bind(&CWebService::DoNoCommandHandler, this, _1, _2, _3));
            return LProxy;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::RouteUser(CHTTPServerConnection *AConnection, const CString& Method, const CString& Uri) {
            auto LProxy = GetProxy(AConnection);
            auto LServerRequest = AConnection->Request();
            auto LProxyRequest = LProxy->Request();

            const auto &LModuleAddress = Config()->ModuleAddress();

            const auto &LOrigin = LServerRequest->Headers.Values("origin");
            const auto &LUserAddress = LServerRequest->Params["address"];

            const auto &pgpValue = LServerRequest->Params["pgp"];

            const auto &LServerParam = LServerRequest->Params["server"];
            const auto &LServer = LServerParam.IsEmpty() ? CurrentServer() : LServerParam;

            CLocation Location(LServer);

            LProxy->Host() = Location.hostname;
            LProxy->Port(Location.port == 0 ? BPS_SERVER_PORT : Location.port);

            CString ClearText;
            CString Payload;

            if (!LServerRequest->Content.IsEmpty()) {

                const auto &ContentType = LServerRequest->Headers.Values(_T("content-type"));

                if (ContentType == "application/x-www-form-urlencoded") {

                    const CStringList &FormData = LServerRequest->FormData;

                    const auto &formDate = FormData["date"];
                    const auto &formAddress = FormData["address"];
                    const auto &formBitmessage = FormData["bitmessage"];
                    const auto &formKey = FormData["key"];
                    const auto &formPGP = FormData["pgp"];
                    const auto &formURL = FormData["url"];
                    const auto &formFlags = FormData["flags"];
                    const auto &formSign = FormData["sign"];

                    if (!formDate.IsEmpty()) {
                        ClearText << formDate << LINEFEED;
                    }

                    if (!formAddress.IsEmpty()) {
                        ClearText << formAddress << LINEFEED;
                    }

                    if (!formFlags.IsEmpty()) {
                        ClearText << formFlags << LINEFEED;
                    }

                    if (!formBitmessage.IsEmpty()) {
                        ClearText << formBitmessage << LINEFEED;
                    }

                    if (!formKey.IsEmpty()) {
                        ClearText << formKey << LINEFEED;
                    }

                    if (!formPGP.IsEmpty()) {
                        ClearText << formPGP << LINEFEED;
                    }

                    if (!formURL.IsEmpty()) {
                        ClearText << formURL << LINEFEED;
                    }

                    if (!formSign.IsEmpty()) {
                        ClearText << formSign;
                    }

                } else if (ContentType == "multipart/form-data") {

                    CFormData FormData;
                    CRequestParser::ParseFormData(LServerRequest, FormData);

                    const auto &formDate = FormData.Data("date");
                    const auto &formAddress = FormData.Data("address");
                    const auto &formBitmessage = FormData.Data("bitmessage");
                    const auto &formKey = FormData.Data("key");
                    const auto &formPGP = FormData.Data("pgp");
                    const auto &formURL = FormData.Data("url");
                    const auto &formFlags = FormData.Data("flags");
                    const auto &formSign = FormData.Data("sign");

                    if (!formDate.IsEmpty()) {
                        ClearText << formDate << LINEFEED;
                    }

                    if (!formAddress.IsEmpty()) {
                        ClearText << formAddress << LINEFEED;
                    }

                    if (!formFlags.IsEmpty()) {
                        ClearText << formFlags << LINEFEED;
                    }

                    if (!formBitmessage.IsEmpty()) {
                        ClearText << formBitmessage << LINEFEED;
                    }

                    if (!formKey.IsEmpty()) {
                        ClearText << formKey << LINEFEED;
                    }

                    if (!formPGP.IsEmpty()) {
                        ClearText << formPGP << LINEFEED;
                    }

                    if (!formURL.IsEmpty()) {
                        ClearText << formURL << LINEFEED;
                    }

                    if (!formSign.IsEmpty()) {
                        ClearText << formSign;
                    }

                } else if (ContentType == "application/json") {

                    const CJSON contextJson(LServerRequest->Content);

                    const auto &jsonDate = contextJson["date"].AsString();
                    const auto &jsonAddress = contextJson["address"].AsString();
                    const auto &jsonBitmessage = contextJson["bitmessage"].AsString();
                    const auto &jsonKey = contextJson["key"].AsString();
                    const auto &jsonPGP = contextJson["pgp"].AsString();
                    const auto &jsonFlags = contextJson["flags"].AsString();
                    const auto &jsonSign = contextJson["sign"].AsString();

                    if (!jsonDate.IsEmpty()) {
                        ClearText << jsonDate << LINEFEED;
                    }

                    if (!jsonAddress.IsEmpty()) {
                        ClearText << jsonAddress << LINEFEED;
                    }

                    if (!jsonFlags.IsEmpty()) {
                        ClearText << jsonFlags << LINEFEED;
                    }

                    if (!jsonBitmessage.IsEmpty()) {
                        ClearText << jsonBitmessage << LINEFEED;
                    }

                    if (!jsonKey.IsEmpty()) {
                        ClearText << jsonKey << LINEFEED;
                    }

                    if (!jsonPGP.IsEmpty()) {
                        ClearText << jsonPGP << LINEFEED;
                    }

                    const CJSONValue &jsonURL = contextJson["url"];
                    if (jsonURL.IsArray()) {
                        const CJSONArray &arrayURL = jsonURL.Array();
                        for (int i = 0; i < arrayURL.Count(); i++) {
                            ClearText << arrayURL[i].AsString() << LINEFEED;
                        }
                    }

                    if (!jsonSign.IsEmpty()) {
                        ClearText << jsonSign;
                    }

                } else {
                    ClearText = LServerRequest->Content;
                }

                if (pgpValue == "off" || pgpValue == "false") {
                    Payload = ClearText;
                } else {
                    Apostol::PGP::CleartextSignature(
                            Config()->PGPPrivate(),
                            Config()->PGPPassphrase(),
                            "SHA256",
                            ClearText,
                            Payload);
                }
            }

            //DebugMessage("[RouteUser] Server request:\n%s\n", LServerRequest->Content.c_str());
            //DebugMessage("[RouteUser] Payload:\n%s\n", Payload.c_str());

            CJSON Json(jvtObject);

            Json.Object().AddPair("id", ApostolUID());
            Json.Object().AddPair("address", LUserAddress.IsEmpty() ? LModuleAddress : LUserAddress);

            if (!Payload.IsEmpty())
                Json.Object().AddPair("payload", base64_encode(Payload));

            LProxyRequest->Clear();

            const auto &LHost = LServerRequest->Headers.Values("host");
            if (!LHost.IsEmpty()) {
                const size_t Pos = LHost.Find(':');
                if (Pos != CString::npos) {
                    LProxyRequest->Host = LHost.SubString(0, Pos);
                    LProxyRequest->Port = StrToIntDef(LHost.SubString(Pos + 1).c_str(), 0);
                } else {
                    LProxyRequest->Host = LHost;
                    LProxyRequest->Port = 0;
                }
            }

            LProxyRequest->CloseConnection = true;
            LProxyRequest->ContentType = CRequest::json;
            LProxyRequest->Content << Json;

            CRequest::Prepare(LProxyRequest, Method.c_str(), Uri.c_str());

            if (!LModuleAddress.IsEmpty())
                LProxyRequest->AddHeader("Module-Address", LModuleAddress);

            if (!LOrigin.IsEmpty())
                LProxyRequest->AddHeader("Origin", LOrigin);

            LProxy->Active(true);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::RouteDeal(CHTTPServerConnection *AConnection, const CString &Method, const CString &Uri, const CString &Action) {
            auto LProxy = GetProxy(AConnection);
            auto LServerRequest = AConnection->Request();
            auto LProxyRequest = LProxy->Request();

            const auto &LModuleAddress = Config()->ModuleAddress();
            const auto &LModuleFee = Config()->ModuleFee();

            const auto checkFee = CheckFee(LModuleFee);
            if (checkFee == -1)
                throw ExceptionFrm("Invalid module fee value: %s", LModuleFee.c_str());

            const auto &LOrigin = LServerRequest->Headers.Values("origin");
            const auto &LUserAddress = LServerRequest->Params["address"];

            const auto &pgpValue = LServerRequest->Params["pgp"];

            const auto &LServerParam = LServerRequest->Params["server"];
            const auto &LServer = LServerParam.IsEmpty() ? CurrentServer() : LServerParam;

            CLocation Location(LServer);

            LProxy->Host() = Location.hostname;
            LProxy->Port(Location.port == 0 ? BPS_SERVER_PORT : Location.port);

            YAML::Node Node;

            CString ClearText;
            CString Payload;
            CStringList Data;

            if (!LServerRequest->Content.IsEmpty()) {

                const auto &ContentType = LServerRequest->Headers.Values(_T("content-type"));

                if (ContentType == "application/x-www-form-urlencoded") {

                    const CStringList &FormData = LServerRequest->FormData;

                    const auto &formAt = FormData["at"];
                    const auto &formDate = FormData["date"];
                    const auto &formSellerAddress = FormData["seller_address"];
                    const auto &formSellerRating = FormData["seller_rating"];
                    const auto &formCustomerAddress = FormData["customer_address"];
                    const auto &formCustomerRating = FormData["customer_rating"];
                    const auto &formPaymentAddress = FormData["payment_address"];
                    const auto &formPaymentUntil = FormData["payment_until"];
                    const auto &formPaymentSum = FormData["payment_sum"];
                    const auto &formFeedbackLeaveBefore = FormData["feedback_leave_before"];
                    const auto &formFeedbackStatus = FormData["feedback_status"];
                    const auto &formFeedbackComments = FormData["feedback_comments"];

                    CheckKeyForNull("at", formAt.c_str());
                    CheckKeyForNull("date", formDate.c_str());
                    CheckKeyForNull("seller_address", formSellerAddress.c_str());
                    CheckKeyForNull("customer_address", formCustomerAddress.c_str());
                    CheckKeyForNull("payment_sum", formPaymentSum.c_str());

                    Node["deal"]["order"] = Action.IsEmpty() ? "" : Action.c_str();
                    YAML::Node Deal = Node["deal"];

                    Deal["at"] = formAt.c_str();
                    Deal["date"] = formDate.c_str();

                    YAML::Node Seller = Deal["seller"];

                    Seller["address"] = formSellerAddress.c_str();

                    if (!formSellerRating.IsEmpty())
                        Seller["rating"] = formSellerRating.c_str();

                    YAML::Node Customer = Deal["customer"];

                    Customer["address"] = formCustomerAddress.c_str();

                    if (!formSellerRating.IsEmpty())
                        Customer["rating"] = formCustomerRating.c_str();

                    YAML::Node Payment = Deal["payment"];

                    if (!formPaymentAddress.IsEmpty())
                        Payment["address"] = formPaymentAddress.c_str();

                    if (!formPaymentUntil.IsEmpty())
                        Payment["until"] = formPaymentUntil.c_str();

                    Payment["sum"] = formPaymentSum.c_str();

                    if (!formFeedbackLeaveBefore.IsEmpty()) {
                        YAML::Node Feedback = Deal["feedback"];

                        Feedback["leave-before"] = formFeedbackLeaveBefore.c_str();

                        if (!formFeedbackStatus.IsEmpty())
                            Feedback["status"] = formFeedbackStatus.c_str();

                        if (!formFeedbackComments.IsEmpty())
                            Feedback["comments"] = formFeedbackComments.c_str();
                    }

                } else if (ContentType == "multipart/form-data") {

                    CFormData FormData;
                    CRequestParser::ParseFormData(LServerRequest, FormData);

                    const auto &formAt = FormData.Data("at");
                    const auto &formDate = FormData.Data("date");
                    const auto &formSellerAddress = FormData.Data("seller_address");
                    const auto &formSellerRating = FormData.Data("seller_rating");
                    const auto &formCustomerAddress = FormData.Data("customer_address");
                    const auto &formCustomerRating = FormData.Data("customer_rating");
                    const auto &formPaymentAddress = FormData.Data("payment_address");
                    const auto &formPaymentUntil = FormData.Data("payment_until");
                    const auto &formPaymentSum = FormData.Data("payment_sum");
                    const auto &formFeedbackLeaveBefore = FormData.Data("feedback_leave_before");
                    const auto &formFeedbackStatus = FormData.Data("feedback_status");
                    const auto &formFeedbackComments = FormData.Data("feedback_comments");

                    CheckKeyForNull("at", formAt.c_str());
                    CheckKeyForNull("date", formDate.c_str());
                    CheckKeyForNull("seller_address", formSellerAddress.c_str());
                    CheckKeyForNull("customer_address", formCustomerAddress.c_str());
                    CheckKeyForNull("payment_sum", formPaymentSum.c_str());

                    Node["deal"]["order"] = Action.IsEmpty() ? "" : Action.c_str();
                    YAML::Node Deal = Node["deal"];

                    Deal["at"] = formAt.c_str();
                    Deal["date"] = formDate.c_str();

                    YAML::Node Seller = Deal["seller"];

                    Seller["address"] = formSellerAddress.c_str();

                    if (!formSellerRating.IsEmpty())
                        Seller["rating"] = formSellerRating.c_str();

                    YAML::Node Customer = Deal["customer"];

                    Customer["address"] = formCustomerAddress.c_str();

                    if (!formSellerRating.IsEmpty())
                        Customer["rating"] = formCustomerRating.c_str();

                    YAML::Node Payment = Deal["payment"];

                    if (!formPaymentAddress.IsEmpty())
                        Payment["address"] = formPaymentAddress.c_str();

                    if (!formPaymentUntil.IsEmpty())
                        Payment["until"] = formPaymentUntil.c_str();

                    Payment["sum"] = formPaymentSum.c_str();

                    if (!formFeedbackLeaveBefore.IsEmpty()) {
                        YAML::Node Feedback = Deal["feedback"];

                        Feedback["leave-before"] = formFeedbackLeaveBefore.c_str();

                        if (!formFeedbackStatus.IsEmpty())
                            Feedback["status"] = formFeedbackStatus.c_str();

                        if (!formFeedbackComments.IsEmpty())
                            Feedback["comments"] = formFeedbackComments.c_str();
                    }

                } else if (ContentType == "application/json") {

                    const CJSON jsonData(LServerRequest->Content);

                    const auto &formOrder = jsonData["order"].AsString();

                    const auto &formAt = jsonData["at"].AsString();
                    const auto &formDate = jsonData["date"].AsString();

                    const CJSONValue &jsonSeller = jsonData["seller"];

                    const auto &formSellerAddress = jsonSeller["address"].AsString();
                    const auto &formSellerRating = jsonSeller["rating"].AsString();

                    const CJSONValue &jsonCustomer = jsonData["customer"];

                    const auto &formCustomerAddress = jsonCustomer["address"].AsString();
                    const auto &formCustomerRating = jsonCustomer["rating"].AsString();

                    const CJSONValue &jsonPayment = jsonData["payment"];

                    const auto &formPaymentAddress = jsonPayment["address"].AsString();
                    const auto &formPaymentUntil = jsonPayment["until"].AsString();
                    const auto &formPaymentSum = jsonPayment["sum"].AsString();

                    const CJSONValue &jsonFeedback = jsonData["feedback"];

                    const auto &formFeedbackLeaveBefore = jsonFeedback["leave-before"].AsString();
                    const auto &formFeedbackStatus = jsonFeedback["status"].AsString();
                    const auto &formFeedbackComments = jsonFeedback["comments"].AsString();

                    CheckKeyForNull("at", formAt.c_str());
                    CheckKeyForNull("date", formDate.c_str());
                    CheckKeyForNull("seller->address", formSellerAddress.c_str());
                    CheckKeyForNull("customer->address", formCustomerAddress.c_str());
                    CheckKeyForNull("payment->sum", formPaymentSum.c_str());

                    Node["deal"]["order"] = Action.IsEmpty() ? formOrder.c_str() : Action.c_str();
                    YAML::Node Deal = Node["deal"];

                    Deal["at"] = formAt.c_str();
                    Deal["date"] = formDate.c_str();

                    YAML::Node Seller = Deal["seller"];

                    Seller["address"] = formSellerAddress.c_str();

                    if (!formSellerRating.IsEmpty())
                        Seller["rating"] = formSellerRating.c_str();

                    YAML::Node Customer = Deal["customer"];

                    Customer["address"] = formCustomerAddress.c_str();

                    if (!formSellerRating.IsEmpty())
                        Customer["rating"] = formCustomerRating.c_str();

                    YAML::Node Payment = Deal["payment"];

                    if (!formPaymentAddress.IsEmpty())
                        Payment["address"] = formPaymentAddress.c_str();

                    if (!formPaymentUntil.IsEmpty())
                        Payment["until"] = formPaymentUntil.c_str();

                    Payment["sum"] = formPaymentSum.c_str();

                    if (!formFeedbackLeaveBefore.IsEmpty()) {
                        YAML::Node Feedback = Deal["feedback"];

                        Feedback["leave-before"] = formFeedbackLeaveBefore.c_str();

                        if (!formFeedbackStatus.IsEmpty())
                            Feedback["status"] = formFeedbackStatus.c_str();

                        if (!formFeedbackComments.IsEmpty())
                            Feedback["comments"] = formFeedbackComments.c_str();
                    }

                } else {
                    Node = YAML::Load(LServerRequest->Content.c_str());
                }

                ClearText = YAML::Dump(Node);

                if (pgpValue == "off" || pgpValue == "false") {
                    Payload = ClearText;
                } else {
                    Apostol::PGP::CleartextSignature(
                            Config()->PGPPrivate(),
                            Config()->PGPPassphrase(),
                            "SHA256",
                            ClearText,
                            Payload);
                }
            }

            //DebugMessage("[RouteDeal] Server request:\n%s\n", LServerRequest->Content.c_str());
            //DebugMessage("[RouteDeal] Payload:\n%s\n", Payload.c_str());

            CJSON Json(jvtObject);

            Json.Object().AddPair("id", ApostolUID());

            if (!Payload.IsEmpty())
                Json.Object().AddPair("payload", base64_encode(Payload));

            LProxyRequest->Clear();

            const auto &LHost = LServerRequest->Headers.Values("host");
            if (!LHost.IsEmpty()) {
                const size_t Pos = LHost.Find(':');
                if (Pos != CString::npos) {
                    LProxyRequest->Host = LHost.SubString(0, Pos);
                    LProxyRequest->Port = StrToIntDef(LHost.SubString(Pos + 1).c_str(), 0);
                } else {
                    LProxyRequest->Host = LHost;
                    LProxyRequest->Port = 0;
                }
            }

            LProxyRequest->CloseConnection = true;
            LProxyRequest->ContentType = CRequest::json;
            LProxyRequest->Content << Json;

            CRequest::Prepare(LProxyRequest, Method.c_str(), Uri.c_str());

            if (!LModuleAddress.IsEmpty())
                LProxyRequest->AddHeader("Module-Address", LModuleAddress);

            if (!LModuleFee.IsEmpty())
                LProxyRequest->AddHeader("Module-Fee", LModuleFee);

            if (!LOrigin.IsEmpty())
                LProxyRequest->AddHeader("Origin", LOrigin);

            LProxy->Active(true);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::RouteSignature(CHTTPServerConnection *AConnection) {
            auto LRequest = AConnection->Request();
            auto LReply = AConnection->Reply();

            if (LRequest->Content.IsEmpty()) {
                AConnection->SendStockReply(CReply::no_content);
                return;
            }

            if (m_PGP.IsEmpty())
                throw ExceptionFrm("Server PGP signature not added.");

            CString Message;
            CJSON Json(jvtObject);

            const auto &ContentType = LRequest->Headers.Values(_T("content-type"));

            if (ContentType == "application/x-www-form-urlencoded") {
                const CStringList &FormData = LRequest->FormData;

                const auto &ClearText = FormData["message"];
                CheckKeyForNull("message", ClearText.c_str());

                const auto Verified = CheckVerifyPGPSignature(VerifyPGPSignature(ClearText, m_PGP, Message), Message);
                Json.Object().AddPair("verified", Verified);
            } else if (ContentType == "multipart/form-data") {
                CFormData FormData;
                CRequestParser::ParseFormData(LRequest, FormData);

                const auto &ClearText = FormData.Data("message");
                CheckKeyForNull("message", ClearText.c_str());

                const auto Verified = CheckVerifyPGPSignature(VerifyPGPSignature(ClearText, m_PGP, Message), Message);
                Json.Object().AddPair("verified", Verified);
            } else if (ContentType == "application/json") {
                const CJSON jsonData(LRequest->Content);

                const auto &ClearText = jsonData["message"].AsString();
                CheckKeyForNull("message", ClearText.c_str());

                const auto Verified = CheckVerifyPGPSignature(VerifyPGPSignature(ClearText, m_PGP, Message), Message);
                Json.Object().AddPair("verified", Verified);
            } else {
                const auto &ClearText = LRequest->Content;
                const auto Verified = CheckVerifyPGPSignature(VerifyPGPSignature(ClearText, m_PGP, Message), Message);
                Json.Object().AddPair("verified", Verified);
            }

            Json.Object().AddPair("message", Message);

            AConnection->CloseConnection(true);
            LReply->Content << Json;

            AConnection->SendReply(CReply::ok);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::DoWWW(CHTTPServerConnection *AConnection) {
            auto LServer = dynamic_cast<CHTTPServer *> (AConnection->Server());
            auto LRequest = AConnection->Request();
            auto LReply = AConnection->Reply();

            TCHAR szExt[PATH_MAX] = {0};

            LReply->ContentType = CReply::html;

            // Decode url to path.
            CString LRequestPath;
            if (!LServer->URLDecode(LRequest->Uri, LRequestPath)) {
                AConnection->SendStockReply(CReply::bad_request);
                return;
            }

            // Request path must be absolute and not contain "..".
            if (LRequestPath.empty() || LRequestPath.front() != '/' || LRequestPath.find("..") != CString::npos) {
                AConnection->SendStockReply(CReply::bad_request);
                return;
            }

            const auto& LAuthorization = LRequest->Headers.Values(_T("authorization"));
            if (LAuthorization.IsEmpty()) {
                AConnection->SendStockReply(CReply::unauthorized);
                return;
            }

            try {
                CAuthorization Authorization(LAuthorization);

                if (Authorization.Username != "module" || Authorization.Password != Config()->ModuleAddress()) {
                    AConnection->SendStockReply(CReply::unauthorized);
                    return;
                }

                // If path ends in slash (i.e. is a directory) then add "index.html".
                if (LRequestPath.back() == '/') {
                    LRequestPath += "index.html";
                }

                // Open the file to send back.
                const CString LFullPath = LServer->DocRoot() + LRequestPath;
                if (!FileExists(LFullPath.c_str())) {
                    AConnection->SendStockReply(CReply::not_found);
                    return;
                }

                LReply->Content.LoadFromFile(LFullPath.c_str());

                // Fill out the CReply to be sent to the client.
                AConnection->SendReply(CReply::ok, Mapping::ExtToType(ExtractFileExt(szExt, LRequestPath.c_str())));
            } catch (Delphi::Exception::Exception &E) {
                AConnection->SendStockReply(CReply::bad_request);
                Log()->Error(APP_LOG_EMERG, 0, E.what());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::DoOptions(CHTTPServerConnection *AConnection) {
            CApostolModule::DoOptions(AConnection);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::DoGet(CHTTPServerConnection *AConnection) {

            auto LRequest = AConnection->Request();
            auto LReply = AConnection->Reply();

            CStringList LUri;
            SplitColumns(LRequest->Uri.c_str(), LRequest->Uri.Size(), &LUri, '/');

            if (LUri.Count() < 3) {
                DoWWW(AConnection);
                return;
            }

            const auto& LService = LUri[0].Lower();
            const auto& LVersion = LUri[1].Lower();
            const auto& LCommand = LUri[2].Lower();
            const auto& LAction = LUri.Count() == 4 ? LUri[3].Lower() : "";

            if (LVersion == "v1") {
                m_Version = 1;
            } else if (LVersion == "v2") {
                m_Version = 2;
            }

            if (LService != "api" || (m_Version == -1)) {
                DoWWW(AConnection);
                return;
            }

            const CString &LContentType = LRequest->Headers.Values("content-type");
            if (!LContentType.IsEmpty() && LRequest->ContentLength == 0) {
                AConnection->SendStockReply(CReply::no_content);
                return;
            }

            CString LRoute;
            for (int I = 0; I < LUri.Count(); ++I) {
                LRoute.Append('/');
                LRoute.Append(LUri[I]);
            }

            try {
                if (LCommand == "ping") {

                    AConnection->SendStockReply(CReply::ok);

                } else if (LCommand == "time") {

                    LReply->Content << "{\"serverTime\": " << to_string(MsEpoch()) << "}";

                    AConnection->SendReply(CReply::ok);

                } else if (m_Version == 1 && LCommand == "user" && (LAction == "help" || LAction == "status")) {

                    LRequest->Content.Clear();

                    RouteUser(AConnection, "GET", LRoute);

                } else {

                    AConnection->SendStockReply(CReply::not_found);

                }

            } catch (std::exception &e) {
                CReply::status_type LStatus = CReply::internal_server_error;

                ExceptionToJson(0, e, LReply->Content);

                AConnection->SendReply(LStatus);
                Log()->Error(APP_LOG_EMERG, 0, e.what());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::DoPost(CHTTPServerConnection *AConnection) {

            auto LRequest = AConnection->Request();
            auto LReply = AConnection->Reply();

            CStringList LUri;
            SplitColumns(LRequest->Uri.c_str(), LRequest->Uri.Size(), &LUri, '/');

            if (LUri.Count() < 3) {
                AConnection->SendStockReply(CReply::not_found);
                return;
            }

            const auto& LService = LUri[0].Lower();
            const auto& LVersion = LUri[1].Lower();
            const auto& LCommand = LUri[2].Lower();
            const auto& LAction = LUri.Count() == 4 ? LUri[3].Lower() : "";

            if (LVersion == "v1") {
                m_Version = 1;
            } else if (LVersion == "v2") {
                m_Version = 2;
            }

            if (LService != "api" || (m_Version == -1)) {
                AConnection->SendStockReply(CReply::not_found);
                return;
            }

            const CString &LContentType = LRequest->Headers.Values("content-type");
            if (!LContentType.IsEmpty() && LRequest->ContentLength == 0) {
                AConnection->SendStockReply(CReply::no_content);
                return;
            }

            CString LRoute;
            for (int I = 0; I < LUri.Count(); ++I) {
                LRoute.Append('/');
                LRoute.Append(LUri[I]);
            }

            try {

                if (LCommand == "user") {

                    RouteUser(AConnection, "POST", LRoute);

                } else if (LCommand == "deal") {

                    RouteDeal(AConnection, "POST", LRoute, LAction);

                } else if (LCommand == "signature") {

                    RouteSignature(AConnection);

                } else {

                    AConnection->SendStockReply(CReply::not_found);

                }

            } catch (std::exception &e) {
                ExceptionToJson(0, e, LReply->Content);

                AConnection->SendReply(CReply::internal_server_error);
                Log()->Error(APP_LOG_EMERG, 0, e.what());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::BeforeExecute(Pointer Data) {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::Execute(CHTTPServerConnection *AConnection) {
            int i = 0;

            auto LRequest = AConnection->Request();
            auto LReply = AConnection->Reply();
#ifdef _DEBUG
            DebugConnection(AConnection);
#endif
            LReply->Clear();
            LReply->ContentType = CReply::json;

            CMethodHandler *Handler;
            for (i = 0; i < m_Methods.Count(); ++i) {
                Handler = (CMethodHandler *) m_Methods.Objects(i);
                if (Handler->Allow()) {
                    const auto& Method = m_Methods.Strings(i);
                    if (Method == LRequest->Method) {
                        CORS(AConnection);
                        Handler->Handler(AConnection);
                        break;
                    }
                }
            }

            if (i == m_Methods.Count()) {
                AConnection->SendStockReply(CReply::not_implemented);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::AfterExecute(Pointer Data) {

        }
        //--------------------------------------------------------------------------------------------------------------

        int CWebService::NextServerIndex() {
            m_ServerIndex++;
            if (m_ServerIndex == m_ServerList.Count())
                m_ServerIndex = -1;
            return m_ServerIndex;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::NextServer() {
            while (NextServerIndex() != -1 && !ServerPing(CurrentServer() + "/api/v1/ping")) {

            }
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CWebService::ServerPing(const CString &URL) {
            CString Result;

            m_Curl.Reset();
            m_Curl.Send(URL, Result);

            return m_Curl.Code() == CURLE_OK;
        }

        const CString &CWebService::CurrentServer() const {
            if (m_ServerList.Count() == 0 || m_ServerIndex == -1)
                return m_LocalHost;
            return m_ServerList[m_ServerIndex].Value;
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CWebService::CheckVerifyPGPSignature(int VerifyPGPSignature, CString &Message) {
            if (VerifyPGPSignature == -2) {
                Message = "PGP public key is not meaningful.";
            } else if (VerifyPGPSignature == -1) {
                Message = "PGP signature not valid.";
            } else if (VerifyPGPSignature == 0) {
                Message = "PGP signature not found.";
            } else if (VerifyPGPSignature > 1) {
                Message = "PGP signature status: Unknown.";
            }
            return VerifyPGPSignature == 1;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CWebService::VerifyPGPSignature(const CString &ClearText, const CString &Key, CString &Message) {
            const OpenPGP::Key signer(Key.c_str());
            const OpenPGP::CleartextSignature cleartext(ClearText.c_str());

            if (!cleartext.meaningful())
                return -2;

            const int verified = OpenPGP::Verify::cleartext_signature(signer, cleartext);

            if (verified) {
                Message = cleartext.get_message().c_str();
            }

            return verified;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::ParsePGPKey(const CString &Key) {
            if (Key.IsEmpty())
                return;

            const Apostol::PGP::Key key(Key.c_str());
            if (!key.meaningful())
                return;

            CPGPUserIdList List;
            key.ExportUID(List);

            m_ServerList.Clear();
            for (int i = 0; i < List.Count(); i++) {
                const auto& uid = List[i];
                if (uid.Name.Length() >= 35 && uid.Name.SubString(0, 3) == BM_PREFIX) {
                    CStringList urlList;
                    if (FindURLInLine(uid.Desc, urlList)) {
                        for (int l = 0; l < urlList.Count(); l++) {
                            m_ServerList.AddPair(uid.Name, urlList[l]);
                        }
                    }
                }
            }

            m_PGP = Key;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::LoadFromOpenPGP(CString &Key) {
            const auto& FingerPrint = Config()->PGPFingerPrint();
            const auto& KeyId = Config()->PGPKeyId();

            if (FingerPrint.IsEmpty() && KeyId.IsEmpty()) {
                Log()->Message(_T("PGP Fingerprint or KEY-ID not set in configuration file. Application will be stopped."));
                //sig_quit = 1;
            } else {
                if (!FingerPrint.IsEmpty()) {
                    KeyFromOpenPGPByFingerPrint(FingerPrint, Key);
                } else {
                    KeyFromOpenPGPByKeyId(KeyId, Key);
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::LoadFromBPS(CString &Key) {

            CJSON Json;
            CString jsonString;
            CString URL(CurrentServer() + "/api/v1/pgp");

            Log()->Debug(0, "[PGP] Trying to download a key from: %s", URL.c_str());

            m_Curl.Reset();
            m_Curl.Send(URL, jsonString);

            /* Check for errors */
            if ( m_Curl.Code() == CURLE_OK ) {
                try {
                    Json << jsonString;

                    const auto& Error = Json["error"];
                    if (Error.ValueType() == jvtObject)
                        throw Delphi::Exception::Exception(Error["message"].AsString().c_str());

                    const auto Result = Json["result"].AsBoolean();
                    const auto& Message = Json["message"].AsString();

                    if (!Result)
                        throw Delphi::Exception::Exception(Message.c_str());

                    const auto& PGP = Json["key"];
                    if (PGP.ValueType() == jvtString)
                        Key = PGP.AsString();
                } catch (Delphi::Exception::Exception &e) {
                    Log()->Error(APP_LOG_ERR, 0, "[PGP] Error: %s", e.what());
                }
            } else {
                Log()->Error(APP_LOG_EMERG, 0, "[PGP] Failed: %s (%s)." , m_Curl.GetErrorMessage().c_str(), URL.c_str());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::LoadPGPKey() {
            CString Key;
            while (Key.IsEmpty() && NextServerIndex() != -1) {
                LoadFromBPS(Key);
            };

            if (!Key.IsEmpty()) {
                DebugMessage("%s\n", Key.c_str());
                ParsePGPKey(Key);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::Heartbeat() {
            const CDateTime now = Now();

            if (m_ServerList.Count() == 0) {
                m_ServerList.AddPair(BPS_BM_SERVER_ADDRESS, BPS_SERVER_URL);
                m_ServerList.AddPair("local", m_LocalHost);
            }

            if (now >= m_RandomDate) {
                LoadPGPKey();
                if (m_PGP.IsEmpty()) {
                    m_RandomDate = now + (CDateTime) 30 / 86400; // 30 sec
                } else {
                    m_RandomDate = GetRandomDate(5 * 60, 30 * 60, now); // 5..30 min
                }
            }
/*
            if ((now >= m_FixedDate)) {
                NextServer();
                m_FixedDate = now + (CDateTime) 1800 / 86400; // 30 min
            }
*/
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CWebService::CheckUserAgent(const CString& Value) {
            return true;
        }
        //--------------------------------------------------------------------------------------------------------------

    }
}
}