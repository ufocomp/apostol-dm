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

#define BPS_PGP_HASH "SHA512"
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
            m_SyncPeriod = BPS_DEFAULT_SYNC_PERIOD;
            m_ServerIndex = -1;
            m_KeyIndex = 0;
            m_KeyStatus = ksUnknown;
            m_RandomDate = Now();
            m_LocalHost = "http://localhost:";
            m_LocalHost << BPS_SERVER_PORT;
            m_ProxyManager = new CHTTPProxyManager();

            CWebService::InitMethods();
        }
        //--------------------------------------------------------------------------------------------------------------

        CWebService::~CWebService() {
            delete m_ProxyManager;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::InitMethods() {
#if defined(_GLIBCXX_RELEASE) && (_GLIBCXX_RELEASE >= 9)
            m_pMethods->AddObject(_T("GET")    , (CObject *) new CMethodHandler(true , [this](auto && Connection) { DoGet(Connection); }));
            m_pMethods->AddObject(_T("POST")   , (CObject *) new CMethodHandler(true , [this](auto && Connection) { DoPost(Connection); }));
            m_pMethods->AddObject(_T("HEAD")   , (CObject *) new CMethodHandler(true , [this](auto && Connection) { DoHead(Connection); }));
            m_pMethods->AddObject(_T("OPTIONS"), (CObject *) new CMethodHandler(true , [this](auto && Connection) { DoOptions(Connection); }));
            m_pMethods->AddObject(_T("PUT")    , (CObject *) new CMethodHandler(false, [this](auto && Connection) { MethodNotAllowed(Connection); }));
            m_pMethods->AddObject(_T("DELETE") , (CObject *) new CMethodHandler(false, [this](auto && Connection) { MethodNotAllowed(Connection); }));
            m_pMethods->AddObject(_T("TRACE")  , (CObject *) new CMethodHandler(false, [this](auto && Connection) { MethodNotAllowed(Connection); }));
            m_pMethods->AddObject(_T("PATCH")  , (CObject *) new CMethodHandler(false, [this](auto && Connection) { MethodNotAllowed(Connection); }));
            m_pMethods->AddObject(_T("CONNECT"), (CObject *) new CMethodHandler(false, [this](auto && Connection) { MethodNotAllowed(Connection); }));
#else
            m_pMethods->AddObject(_T("GET"), (CObject *) new CMethodHandler(true, std::bind(&CWebService::DoGet, this, _1)));
            m_pMethods->AddObject(_T("POST"), (CObject *) new CMethodHandler(true, std::bind(&CWebService::DoPost, this, _1)));
            m_pMethods->AddObject(_T("HEAD"), (CObject *) new CMethodHandler(true, std::bind(&CWebService::DoHead, this, _1)));
            m_pMethods->AddObject(_T("OPTIONS"), (CObject *) new CMethodHandler(true, std::bind(&CWebService::DoOptions, this, _1)));
            m_pMethods->AddObject(_T("PUT"), (CObject *) new CMethodHandler(false, std::bind(&CWebService::MethodNotAllowed, this, _1)));
            m_pMethods->AddObject(_T("DELETE"), (CObject *) new CMethodHandler(false, std::bind(&CWebService::MethodNotAllowed, this, _1)));
            m_pMethods->AddObject(_T("TRACE"), (CObject *) new CMethodHandler(false, std::bind(&CWebService::MethodNotAllowed, this, _1)));
            m_pMethods->AddObject(_T("PATCH"), (CObject *) new CMethodHandler(false, std::bind(&CWebService::MethodNotAllowed, this, _1)));
            m_pMethods->AddObject(_T("CONNECT"), (CObject *) new CMethodHandler(false, std::bind(&CWebService::MethodNotAllowed, this, _1)));
#endif
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
#if defined(_GLIBCXX_RELEASE) && (_GLIBCXX_RELEASE >= 9)
            LProxy->OnVerbose([this](auto && Sender, auto && AConnection, auto && AFormat, auto && args) { DoVerbose(Sender, AConnection, AFormat, args); });

            LProxy->OnExecute([this](auto && AConnection) { return DoProxyExecute(AConnection); });

            LProxy->OnException([this](auto && AConnection, auto && AException) { DoProxyException(AConnection, AException); });
            LProxy->OnEventHandlerException([this](auto && AHandler, auto && AException) { DoEventHandlerException(AHandler, AException); });

            LProxy->OnConnected([this](auto && Sender) { DoProxyConnected(Sender); });
            LProxy->OnDisconnected([this](auto && Sender) { DoProxyDisconnected(Sender); });
#else
            LProxy->OnVerbose(std::bind(&CWebService::DoVerbose, this, _1, _2, _3, _4));

            LProxy->OnExecute(std::bind(&CWebService::DoProxyExecute, this, _1));

            LProxy->OnException(std::bind(&CWebService::DoProxyException, this, _1, _2));
            LProxy->OnEventHandlerException(std::bind(&CWebService::DoEventHandlerException, this, _1, _2));

            LProxy->OnConnected(std::bind(&CWebService::DoProxyConnected, this, _1));
            LProxy->OnDisconnected(std::bind(&CWebService::DoProxyDisconnected, this, _1));
#endif
            return LProxy;
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CWebService::CheckAuthorization(CHTTPServerConnection *AConnection, CAuthorization &Authorization) {
            auto LRequest = AConnection->Request();
            auto LReply = AConnection->Reply();

            const auto& LAuthorization = LRequest->Headers.Values(_T("Authorization"));
            if (LAuthorization.IsEmpty())
                return false;

            try {
                Authorization << LAuthorization;
                if (Authorization.Username == "module" && Authorization.Password == Config()->ModuleAddress()) {
                    return true;
                }
            } catch (std::exception &e) {
                Log()->Error(APP_LOG_ERR, 0, e.what());
            }

            return false;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::RouteUser(CHTTPServerConnection *AConnection, const CString& Method, const CString& URI) {
            auto LProxy = GetProxy(AConnection);
            auto LServerRequest = AConnection->Request();
            auto LProxyRequest = LProxy->Request();

            const auto& LModuleAddress = Config()->ModuleAddress();

            const auto& LOrigin = LServerRequest->Headers.Values("origin");
            const auto& LUserAddress = LServerRequest->Params["address"];

            const auto& pgpValue = LServerRequest->Params["pgp"];

            const auto& LServerParam = LServerRequest->Params["server"];
            const auto& LServer = LServerParam.IsEmpty() ? CurrentServer() : LServerParam;

            CLocation Location(LServer);

            LProxy->Host() = Location.hostname;
            LProxy->Port(Location.port == 0 ? BPS_SERVER_PORT : Location.port);

            CStringList ClearText;
            CString Payload;

            if (!LServerRequest->Content.IsEmpty()) {

                const auto& ContentType = LServerRequest->Headers.Values(_T("content-type"));

                if (ContentType == "application/x-www-form-urlencoded") {

                    const CStringList &FormData = LServerRequest->FormData;

                    const auto& formDate = FormData["date"];
                    const auto& formAddress = FormData["address"];
                    const auto& formBitmessage = FormData["bitmessage"];
                    const auto& formKey = FormData["key"];
                    const auto& formPGP = FormData["pgp"];
                    const auto& formURL = FormData["url"];
                    const auto& formFlags = FormData["flags"];
                    const auto& formSign = FormData["sign"];

                    if (!formDate.IsEmpty()) {
                        ClearText << formDate;
                    }

                    if (!formAddress.IsEmpty()) {
                        ClearText << formAddress;
                    }

                    if (!formFlags.IsEmpty()) {
                        ClearText << formFlags;
                    }

                    if (!formBitmessage.IsEmpty()) {
                        ClearText << formBitmessage;
                    }

                    if (!formKey.IsEmpty()) {
                        ClearText << formKey;
                    }

                    if (!formPGP.IsEmpty()) {
                        ClearText << formPGP;
                    }

                    if (!formURL.IsEmpty()) {
                        ClearText << formURL;
                    }

                    if (!formSign.IsEmpty()) {
                        ClearText << formSign;
                    }

                } else if (ContentType == "multipart/form-data") {

                    CFormData FormData;
                    CRequestParser::ParseFormData(LServerRequest, FormData);

                    const auto& formDate = FormData.Data("date");
                    const auto& formAddress = FormData.Data("address");
                    const auto& formBitmessage = FormData.Data("bitmessage");
                    const auto& formKey = FormData.Data("key");
                    const auto& formPGP = FormData.Data("pgp");
                    const auto& formURL = FormData.Data("url");
                    const auto& formFlags = FormData.Data("flags");
                    const auto& formSign = FormData.Data("sign");

                    if (!formDate.IsEmpty()) {
                        ClearText << formDate;
                    }

                    if (!formAddress.IsEmpty()) {
                        ClearText << formAddress;
                    }

                    if (!formFlags.IsEmpty()) {
                        ClearText << formFlags;
                    }

                    if (!formBitmessage.IsEmpty()) {
                        ClearText << formBitmessage;
                    }

                    if (!formKey.IsEmpty()) {
                        ClearText << formKey;
                    }

                    if (!formPGP.IsEmpty()) {
                        ClearText << formPGP;
                    }

                    if (!formURL.IsEmpty()) {
                        ClearText << formURL;
                    }

                    if (!formSign.IsEmpty()) {
                        ClearText << formSign;
                    }

                } else if (ContentType == "application/json") {

                    const CJSON contextJson(LServerRequest->Content);

                    const auto& jsonDate = contextJson["date"].AsString();
                    const auto& jsonAddress = contextJson["address"].AsString();
                    const auto& jsonBitmessage = contextJson["bitmessage"].AsString();
                    const auto& jsonKey = contextJson["key"].AsString();
                    const auto& jsonPGP = contextJson["pgp"].AsString();
                    const auto& jsonFlags = contextJson["flags"].AsString();
                    const auto& jsonSign = contextJson["sign"].AsString();

                    if (!jsonDate.IsEmpty()) {
                        ClearText << jsonDate;
                    }

                    if (!jsonAddress.IsEmpty()) {
                        ClearText << jsonAddress;
                    }

                    if (!jsonFlags.IsEmpty()) {
                        ClearText << jsonFlags;
                    }

                    if (!jsonBitmessage.IsEmpty()) {
                        ClearText << jsonBitmessage;
                    }

                    if (!jsonKey.IsEmpty()) {
                        ClearText << jsonKey;
                    }

                    if (!jsonPGP.IsEmpty()) {
                        ClearText << jsonPGP;
                    }

                    const CJSONValue &jsonURL = contextJson["url"];
                    if (jsonURL.IsArray()) {
                        const CJSONArray &arrayURL = jsonURL.Array();
                        for (int i = 0; i < arrayURL.Count(); i++) {
                            ClearText << arrayURL[i].AsString();
                        }
                    }

                    if (!jsonSign.IsEmpty()) {
                        ClearText << jsonSign;
                    }

                } else {
                    ClearText = LServerRequest->Content;
                }

                if (pgpValue == "off" || pgpValue == "false") {
                    Payload = ClearText.Text();
                } else {
                    Apostol::PGP::CleartextSignature(
                            Config()->PGPPrivate(),
                            Config()->PGPPassphrase(),
                            BPS_PGP_HASH,
                            ClearText.Text(),
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

            LProxyRequest->Location = LServerRequest->Location;
            LProxyRequest->CloseConnection = true;
            LProxyRequest->ContentType = CRequest::json;
            LProxyRequest->Content << Json;

            CRequest::Prepare(LProxyRequest, Method.c_str(), URI.c_str());

            if (!LModuleAddress.IsEmpty())
                LProxyRequest->AddHeader("Module-Address", LModuleAddress);

            if (!LOrigin.IsEmpty())
                LProxyRequest->AddHeader("Origin", LOrigin);

            LProxy->Active(true);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::CheckDeal(const CDeal &Deal) {
            const auto& Data = Deal.Data();

            const auto DateTime = UTC();
            const auto Date = StringToDate(Data.Date);

            if (Data.Order == doCreate) {
                if (DateTime < Date)
                    throw ExceptionFrm("Invalid deal date.");

                if ((DateTime - Date) > (CDateTime) 180 / 86400)
                    throw ExceptionFrm("Deal date expired.");
            }

            if (Data.Order == doComplete) {
                const CDateTime LeaveBefore = StringToDate(Data.FeedBack.LeaveBefore);
                if (DateTime > LeaveBefore)
                    throw ExceptionFrm("Deal feedback expired.");
            }

            if (Odd(int(Data.Order)) || Data.Order == doExecute)
                throw ExceptionFrm("Invalid \"order\" value for deal module.");

            if (Data.Order == doCancel) {
                const CDateTime Until = StringToDate(Data.Payment.Until);
                if (DateTime > Until)
                    throw ExceptionFrm("Deal cancellation expired.");
            }

            if (!valid_address(Data.Seller.Address))
                throw ExceptionFrm("Invalid Seller address: %s.", Data.Seller.Address.c_str());

            if (!valid_address(Data.Customer.Address))
                throw ExceptionFrm("Invalid Customer address: %s.", Data.Customer.Address.c_str());

            if (!valid_address(Data.Payment.Address))
                throw ExceptionFrm("Invalid Payment address: %s.", Data.Payment.Address.c_str());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::RouteDeal(CHTTPServerConnection *AConnection, const CString &Method, const CString &URI, const CString &Action) {
            auto LProxy = GetProxy(AConnection);
            auto LServerRequest = AConnection->Request();
            auto LProxyRequest = LProxy->Request();

            const auto& LModuleAddress = Config()->ModuleAddress();
            const auto& LModuleFee = Config()->ModuleFee();

            const auto checkFee = CheckFee(LModuleFee);
            if (checkFee == -1)
                throw ExceptionFrm("Invalid module fee value: %s", LModuleFee.c_str());

            const auto& LOrigin = LServerRequest->Headers.Values("origin");
            const auto& LUserAddress = LServerRequest->Params["address"];

            const auto& pgpValue = LServerRequest->Params["pgp"];

            const auto& LServerParam = LServerRequest->Params["server"];
            const auto& LServer = LServerParam.IsEmpty() ? CurrentServer() : LServerParam;

            CLocation Location(LServer);

            LProxy->Host() = Location.hostname;
            LProxy->Port(Location.port == 0 ? BPS_SERVER_PORT : Location.port);

            YAML::Node Node;

            CString Payload;

            if (!LServerRequest->Content.IsEmpty()) {

                const auto& ContentType = LServerRequest->Headers.Values(_T("content-type"));

                if (ContentType == "application/x-www-form-urlencoded") {

                    const CStringList &FormData = LServerRequest->FormData;

                    const auto& formType = FormData["type"];
                    const auto& formAt = FormData["at"];
                    const auto& formDate = FormData["date"];
                    const auto& formSellerAddress = FormData["seller_address"];
                    const auto& formSellerRating = FormData["seller_rating"];
                    const auto& formCustomerAddress = FormData["customer_address"];
                    const auto& formCustomerRating = FormData["customer_rating"];
                    const auto& formPaymentAddress = FormData["payment_address"];
                    const auto& formPaymentUntil = FormData["payment_until"];
                    const auto& formPaymentSum = FormData["payment_sum"];
                    const auto& formFeedbackLeaveBefore = FormData["feedback_leave_before"];
                    const auto& formFeedbackStatus = FormData["feedback_status"];
                    const auto& formFeedbackComments = FormData["feedback_comments"];

                    CheckKeyForNull("type", formType.c_str());
                    CheckKeyForNull("at", formAt.c_str());
                    CheckKeyForNull("date", formDate.c_str());
                    CheckKeyForNull("seller_address", formSellerAddress.c_str());
                    CheckKeyForNull("customer_address", formCustomerAddress.c_str());
                    CheckKeyForNull("payment_sum", formPaymentSum.c_str());

                    YAML::Node Deal = Node["deal"];

                    Deal["order"] = Action.IsEmpty() ? "" : Action.c_str();
                    Deal["type"] = formType.c_str();

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

                } else if (ContentType.Find("multipart/form-data") != CString::npos) {

                    CFormData FormData;
                    CRequestParser::ParseFormData(LServerRequest, FormData);

                    const auto& formType = FormData.Data("type");
                    const auto& formAt = FormData.Data("at");
                    const auto& formDate = FormData.Data("date");
                    const auto& formSellerAddress = FormData.Data("seller_address");
                    const auto& formSellerRating = FormData.Data("seller_rating");
                    const auto& formCustomerAddress = FormData.Data("customer_address");
                    const auto& formCustomerRating = FormData.Data("customer_rating");
                    const auto& formPaymentAddress = FormData.Data("payment_address");
                    const auto& formPaymentUntil = FormData.Data("payment_until");
                    const auto& formPaymentSum = FormData.Data("payment_sum");
                    const auto& formFeedbackLeaveBefore = FormData.Data("feedback_leave_before");
                    const auto& formFeedbackStatus = FormData.Data("feedback_status");
                    const auto& formFeedbackComments = FormData.Data("feedback_comments");

                    CheckKeyForNull("type", formType.c_str());
                    CheckKeyForNull("at", formAt.c_str());
                    CheckKeyForNull("date", formDate.c_str());
                    CheckKeyForNull("seller_address", formSellerAddress.c_str());
                    CheckKeyForNull("customer_address", formCustomerAddress.c_str());
                    CheckKeyForNull("payment_sum", formPaymentSum.c_str());

                    YAML::Node Deal = Node["deal"];

                    Deal["order"] = Action.IsEmpty() ? "" : Action.c_str();
                    Deal["type"] = formType.c_str();

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

                    const auto& formOrder = jsonData["order"].AsString();
                    const auto& formType = jsonData["type"].AsString();

                    const auto& formAt = jsonData["at"].AsString();
                    const auto& formDate = jsonData["date"].AsString();

                    const CJSONValue &jsonSeller = jsonData["seller"];

                    const auto& formSellerAddress = jsonSeller["address"].AsString();
                    const auto& formSellerRating = jsonSeller["rating"].AsString();

                    const CJSONValue &jsonCustomer = jsonData["customer"];

                    const auto& formCustomerAddress = jsonCustomer["address"].AsString();
                    const auto& formCustomerRating = jsonCustomer["rating"].AsString();

                    const CJSONValue &jsonPayment = jsonData["payment"];

                    const auto& formPaymentAddress = jsonPayment["address"].AsString();
                    const auto& formPaymentUntil = jsonPayment["until"].AsString();
                    const auto& formPaymentSum = jsonPayment["sum"].AsString();

                    const CJSONValue &jsonFeedback = jsonData["feedback"];

                    const auto& formFeedbackLeaveBefore = jsonFeedback["leave-before"].AsString();
                    const auto& formFeedbackStatus = jsonFeedback["status"].AsString();
                    const auto& formFeedbackComments = jsonFeedback["comments"].AsString();

                    CheckKeyForNull("type", formType.c_str());
                    CheckKeyForNull("at", formAt.c_str());
                    CheckKeyForNull("date", formDate.c_str());
                    CheckKeyForNull("seller->address", formSellerAddress.c_str());
                    CheckKeyForNull("customer->address", formCustomerAddress.c_str());
                    CheckKeyForNull("payment->sum", formPaymentSum.c_str());

                    YAML::Node Deal = Node["deal"];

                    Deal["order"] = Action.IsEmpty() ? formOrder.c_str() : Action.c_str();
                    Deal["type"] = formType.c_str();

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

                if (m_BTCKeys.Count() < 2)
                    throw ExceptionFrm("Bitcoin keys cannot be empty.");

                for (int i = 0; i < m_BTCKeys.Count(); ++i) {
                    if (m_BTCKeys[i].IsEmpty())
                        throw ExceptionFrm("Bitcoin KEY%d cannot be empty.", i);
                }

                CDeal Deal(Node);

                auto& Data = Deal.Data();

                if (Data.Order == doCreate) {
                    Data.Payment.Address = Deal.GetPaymentHD(m_BTCKeys.Names(0), m_BTCKeys.Names(1),
                            Deal.Data().Transaction.Key, BitcoinConfig.VersionHD, BitcoinConfig.VersionScript);

                    Node["deal"]["date"] = Data.Date.c_str();

                    Node["deal"]["payment"]["address"] = Data.Payment.Address.c_str();
                    Node["deal"]["payment"]["until"] = Data.Payment.Until.c_str();
                    Node["deal"]["payment"]["sum"] = Data.Payment.Sum.c_str();

                    Node["deal"]["feedback"]["leave-before"] = Data.FeedBack.LeaveBefore.c_str();
                }

                CheckDeal(Deal);

                const CString ClearText(YAML::Dump(Node));

                if (pgpValue == "off" || pgpValue == "false") {
                    Payload = ClearText;
                } else {
                    Apostol::PGP::CleartextSignature(
                            Config()->PGPPrivate(),
                            Config()->PGPPassphrase(),
                            BPS_PGP_HASH,
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

            LProxyRequest->Location = LServerRequest->Location;
            LProxyRequest->CloseConnection = true;
            LProxyRequest->ContentType = CRequest::json;
            LProxyRequest->Content << Json;

            CRequest::Prepare(LProxyRequest, Method.c_str(), URI.c_str());

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

            const auto& ContentType = LRequest->Headers.Values(_T("content-type"));

            if (ContentType == "application/x-www-form-urlencoded") {
                const CStringList &FormData = LRequest->FormData;

                const auto& ClearText = FormData["message"];
                CheckKeyForNull("message", ClearText.c_str());

                const auto Verified = CheckVerifyPGPSignature(VerifyPGPSignature(ClearText, m_PGP, Message), Message);
                Json.Object().AddPair("verified", Verified);
            } else if (ContentType == "multipart/form-data") {
                CFormData FormData;
                CRequestParser::ParseFormData(LRequest, FormData);

                const auto& ClearText = FormData.Data("message");
                CheckKeyForNull("message", ClearText.c_str());

                const auto Verified = CheckVerifyPGPSignature(VerifyPGPSignature(ClearText, m_PGP, Message), Message);
                Json.Object().AddPair("verified", Verified);
            } else if (ContentType == "application/json") {
                const CJSON jsonData(LRequest->Content);

                const auto& ClearText = jsonData["message"].AsString();
                CheckKeyForNull("message", ClearText.c_str());

                const auto Verified = CheckVerifyPGPSignature(VerifyPGPSignature(ClearText, m_PGP, Message), Message);
                Json.Object().AddPair("verified", Verified);
            } else {
                const auto& ClearText = LRequest->Content;
                const auto Verified = CheckVerifyPGPSignature(VerifyPGPSignature(ClearText, m_PGP, Message), Message);
                Json.Object().AddPair("verified", Verified);
            }

            Json.Object().AddPair("message", Message);

            AConnection->CloseConnection(true);
            LReply->Content << Json;

            AConnection->SendReply(CReply::ok);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::DoAPI(CHTTPServerConnection *AConnection) {
            auto LRequest = AConnection->Request();
            auto LReply = AConnection->Reply();

            CStringList LRouts;
            SplitColumns(LRequest->Location.pathname, LRouts, '/');

            if (LRouts.Count() < 3) {
                AConnection->SendStockReply(CReply::not_found);
                return;
            }

            const auto& LService = LRouts[0].Lower();
            const auto& LVersion = LRouts[1].Lower();
            const auto& LCommand = LRouts[2].Lower();
            const auto& LAction = LRouts.Count() == 4 ? LRouts[3].Lower() : "";

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
            for (int I = 0; I < LRouts.Count(); ++I) {
                LRoute.Append('/');
                LRoute.Append(LRouts[I]);
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

        void CWebService::DoGet(CHTTPServerConnection *AConnection) {
            auto LRequest = AConnection->Request();

            CString LPath(LRequest->Location.pathname);

            // Request path must be absolute and not contain "..".
            if (LPath.empty() || LPath.front() != '/' || LPath.find("..") != CString::npos) {
                AConnection->SendStockReply(CReply::bad_request);
                return;
            }

            CAuthorization Authorization;
            if (!CheckAuthorization(AConnection, Authorization)) {
                AConnection->SendStockReply(CReply::unauthorized);
                return;
            }

            if (LPath.SubString(0, 5) == "/api/") {
                DoAPI(AConnection);
                return;
            }

            SendResource(AConnection, LPath);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::DoPost(CHTTPServerConnection *AConnection) {

            auto LRequest = AConnection->Request();
            auto LReply = AConnection->Reply();

            CStringList LRouts;
            SplitColumns(LRequest->Location.pathname, LRouts, '/');

            if (LRouts.Count() < 3) {
                AConnection->SendStockReply(CReply::not_found);
                return;
            }

            const auto& LService = LRouts[0].Lower();
            const auto& LVersion = LRouts[1].Lower();
            const auto& LCommand = LRouts[2].Lower();
            const auto& LAction = LRouts.Count() == 4 ? LRouts[3].Lower() : "";

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
            for (int I = 0; I < LRouts.Count(); ++I) {
                LRoute.Append('/');
                LRoute.Append(LRouts[I]);
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

        int CWebService::NextServerIndex() {
            m_ServerIndex++;
            if (m_ServerIndex >= m_ServerList.Count())
                m_ServerIndex = -1;
            return m_ServerIndex;
        }
        //--------------------------------------------------------------------------------------------------------------

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

        void CWebService::ParsePGPKey(const CString &Key, CStringPairs& ServerList, CStringList& BTCKeys) {
            if (Key.IsEmpty())
                return;

            const Apostol::PGP::Key pgp(Key.c_str());
            if (!pgp.meaningful())
                return;

            CStringList Data;
            CPGPUserIdList List;
            CStringList KeyList;

            pgp.ExportUID(List);

            for (int i = 0; i < List.Count(); i++) {

                const auto& uid = List[i];
                const auto& name = uid.Name.Lower();
                const auto& data = uid.Desc.Lower();

                if (name == "technical_data") {
                    SplitColumns(data, Data, ';');

                    DebugMessage("Technical data: ");
                    if (Data.Count() == 3) {
                        m_SyncPeriod = StrToIntDef(Data[1].Trim().c_str(), BPS_DEFAULT_SYNC_PERIOD);
                        DebugMessage("\n  - Fee   : %s", Data[0].Trim().c_str());
                        DebugMessage("\n  - Period: %s", Data[1].Trim().c_str());
                        DebugMessage("\n  - Keys  : %s", Data[2].Trim().c_str());
                    } else if (Data.Count() == 2) {
                        m_SyncPeriod = StrToIntDef(Data[0].Trim().c_str(), BPS_DEFAULT_SYNC_PERIOD);
                        DebugMessage("\n  - Period: %s", Data[0].Trim().c_str());
                        DebugMessage("\n  - Keys  : %s", Data[1].Trim().c_str());
                    } else if (Data.Count() == 1) {
                        m_SyncPeriod = StrToIntDef(Data[0].Trim().c_str(), BPS_DEFAULT_SYNC_PERIOD);
                        DebugMessage("\n  - Period: %s", Data[0].Trim().c_str());
                    } else {
                        DebugMessage("Unknown error.");
                    }

                } if (uid.Name.Length() >= 35 && uid.Name.SubString(0, 3) == BM_PREFIX) {
                    CStringList urlList;
                    if (FindURLInLine(uid.Desc, urlList)) {
                        for (int l = 0; l < urlList.Count(); l++) {
                            ServerList.AddPair(uid.Name, urlList[l]);
                        }
                    }
                } else if (name.Find("bitcoin_key") != CString::npos) {
                    const auto& key = wallet::ec_public(data.c_str());
                    if (verify(key))
                        KeyList.AddPair(name, key.encoded());
                }
            }

            CString Name;
            for (int i = 1; i <= KeyList.Count(); i++) {
                Name = "bitcoin_key";
                Name << i;
                const auto& key = KeyList[Name];
                if (!key.IsEmpty())
                    BTCKeys.Add(key);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::JsonStringToKey(const CString &jsonString, CString& Key) {
            CJSON Json;
            Json << jsonString;

            const auto& Error = Json["error"];
            if (Error.ValueType() == jvtObject)
                throw Delphi::Exception::Exception(Error["message"].AsString().c_str());

            const auto& key = Json["key"].AsString();
            const auto result = Json["result"].AsBoolean();
            const auto& message = Json["message"].AsString();

            Log()->Debug(0, "Key message: %s", message.c_str());

            if (result) {
                Key = key;
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::FetchPGP() {
            const auto& LServer = CurrentServer();

            Log()->Debug(0, "Trying to fetch a PGP key from: %s", LServer.c_str());

            auto OnRequest = [this](CRequest *ARequest) {
                m_KeyStatus = ksPGPFetching;
                CRequest::Prepare(ARequest, "GET", "/api/v1/key?type=PGP-PUBLIC&name=DEFAULT");
            };

            auto OnExecute = [this](CTCPConnection *AConnection) {
                auto LConnection = dynamic_cast<CHTTPClientConnection *> (AConnection);
                auto LReply = LConnection->Reply();

                try {
                    JsonStringToKey(LReply->Content, m_PGP);

                    if (!m_PGP.IsEmpty()) {
                        DebugMessage("%s\n", m_PGP.c_str());

                        CStringPairs ServerList;
                        CStringList BTCKeys;

                        ParsePGPKey(m_PGP, ServerList, BTCKeys);

                        if (ServerList.Count() != 0)
                            m_ServerList = ServerList;

                        m_BTCKeys = BTCKeys;
                        m_KeyStatus = ksPGPSuccess;
                    }
                } catch (Delphi::Exception::Exception &e) {
                    Log()->Error(APP_LOG_INFO, 0, "[PGP] Message: %s", e.what());
                    m_KeyStatus = ksPGPError;
                    FetchBTC();
                }

                LConnection->CloseConnection(true);
                return true;
            };

            auto OnException = [this](CTCPConnection *AConnection, Delphi::Exception::Exception *AException) {
                auto LConnection = dynamic_cast<CHTTPClientConnection *> (AConnection);
                auto LClient = dynamic_cast<CHTTPClient *> (LConnection->Client());

                Log()->Error(APP_LOG_EMERG, 0, "[%s:%d] %s", LClient->Host().c_str(), LClient->Port(), AException->what());

                m_KeyStatus = ksPGPError;
                FetchBTC();
            };

            CLocation Location(LServer);
            auto LClient = GetClient(Location.hostname, Location.port == 0 ? BPS_SERVER_PORT : Location.port);

            LClient->OnRequest(OnRequest);
            LClient->OnExecute(OnExecute);
            LClient->OnException(OnException);

            LClient->Active(true);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::FetchBTC() {
            const auto& LServer = CurrentServer();

            Log()->Debug(0, "Trying to fetch a BTC KEY%d from: %s", m_KeyIndex + 1, LServer.c_str());

            auto OnRequest = [this](CRequest *ARequest) {
                m_KeyStatus = ksBTCFetching;
                CString URI("/api/v1/key?type=BTC-PUBLIC&name=KEY");
                URI << m_KeyIndex + 1;
                CRequest::Prepare(ARequest, "GET", URI.c_str());
            };

            auto OnExecute = [this](CTCPConnection *AConnection) {
                auto LConnection = dynamic_cast<CHTTPClientConnection *> (AConnection);
                auto LReply = LConnection->Reply();

                try {
                    CString Key;
                    JsonStringToKey(LReply->Content, Key);
                    if (!Key.IsEmpty()) {
                        m_BTCKeys.Add(Key);
                        m_KeyIndex++;
                        FetchBTC();
                    }
                } catch (Delphi::Exception::Exception &e) {
                    Log()->Error(APP_LOG_INFO, 0, "[BTC] Message: %s", e.what());
                    if (m_BTCKeys.Count() == 0) {
                        m_KeyStatus = ksBTCError;
                    } else {
                        m_KeyStatus = ksBTCSuccess;
                    }
                }

                LConnection->CloseConnection(true);

                return true;
            };

            auto OnException = [this](CTCPConnection *AConnection, Delphi::Exception::Exception *AException) {
                auto LConnection = dynamic_cast<CHTTPClientConnection *> (AConnection);
                auto LClient = dynamic_cast<CHTTPClient *> (LConnection->Client());

                Log()->Error(APP_LOG_EMERG, 0, "[%s:%d] %s", LClient->Host().c_str(), LClient->Port(), AException->what());
                m_KeyStatus = ksBTCError;
            };

            CLocation Location(LServer);
            auto LClient = GetClient(Location.hostname, Location.port == 0 ? BPS_SERVER_PORT : Location.port);

            LClient->OnRequest(OnRequest);
            LClient->OnExecute(OnExecute);
            LClient->OnException(OnException);

            LClient->Active(true);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::FetchKeys() {
            m_KeyIndex = 0;
            m_KeyStatus = ksUnknown;
            if (NextServerIndex() != -1)
                FetchPGP();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::Heartbeat() {
            const CDateTime now = Now();

            if (m_ServerList.Count() == 0) {
                m_ServerList.AddPair(BPS_BM_SERVER_ADDRESS, BPS_SERVER_URL);
                m_ServerList.AddPair("local", m_LocalHost);
            }

            if (m_KeyStatus == ksBTCError) {
                FetchKeys();
            } else if (now >= m_RandomDate) {
                FetchKeys();
                m_RandomDate = GetRandomDate(10 * 60, m_SyncPeriod * 60, now); // 10..m_SyncPeriod min
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CWebService::CheckUserAgent(const CString& Value) {
            return true;
        }
        //--------------------------------------------------------------------------------------------------------------

    }
}
}