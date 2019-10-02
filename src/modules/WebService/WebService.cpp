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
            m_ProxyManager = new CHTTPProxyManager();
            InitMethods();
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

            const CString& Format = LServerRequest->Params["format"];
            if (Format == "html") {
                LServerReply->ContentType = CReply::html;
                if (LProxyReply->Status == CReply::ok) {
                    if (!LProxyReply->Content.IsEmpty()) {
                        const CJSON json(LProxyReply->Content);
                        LServerReply->Content = base64_decode(json["payload"].AsSiring());
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

            const CString& Format = LServerRequest->Params["format"];
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
                ExceptionToJson(0, AException, LReply->Content);
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

        void CWebService::ExceptionToJson(int ErrorCode, Delphi::Exception::Exception *AException, CString& Json) {
            TCHAR ch;
            LPCTSTR lpMessage = AException->what();
            CString Message;

            while ((ch = *lpMessage++) != 0) {
                if ((ch == '"') || (ch == '\\'))
                    Message.Append('\\');
                Message.Append(ch);
            }

            Json.Format(R"({"error": {"code": %u, "message": "%s"}})", ErrorCode, Message.c_str());
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

            const CString& LModuleAddress = Config()->ModuleAddress();
            const CString& LOrigin = LServerRequest->Headers.Values("origin");
            const CString& LUserAddress = LServerRequest->Params["address"];

            const CString& LServer = LServerRequest->Params["server"];
            const CString& pgpValue = LServerRequest->Params["pgp"];

            LProxy->Host() = LServer.IsEmpty() ? "localhost" : LServer;
            LProxy->Port(4977);

            CString ClearText;
            CString Payload;

            if (!LServerRequest->Content.IsEmpty()) {

                const CString& ContentType = LServerRequest->Headers.Values(_T("content-type"));

                if (ContentType == "application/x-www-form-urlencoded") {

                    const CStringList& FormData = LServerRequest->FormData;

                    const CString& formDate = FormData["date"];
                    const CString& formAddress = FormData["address"];
                    const CString& formBitmessage = FormData["bitmessage"];
                    const CString& formKey = FormData["key"];
                    const CString& formPGP = FormData["pgp"];
                    const CString& formURL = FormData["url"];
                    const CString& formFlags = FormData["flags"];
                    const CString& formSign = FormData["sign"];

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

                    const CString& formDate = FormData.Data("date");
                    const CString& formAddress = FormData.Data("address");
                    const CString& formBitmessage = FormData.Data("bitmessage");
                    const CString& formKey = FormData.Data("key");
                    const CString& formPGP = FormData.Data("pgp");
                    const CString& formURL = FormData.Data("url");
                    const CString& formFlags = FormData.Data("flags");
                    const CString& formSign = FormData.Data("sign");

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

                    const CString& jsonDate = contextJson["date"].AsSiring();
                    const CString& jsonAddress = contextJson["address"].AsSiring();
                    const CString& jsonBitmessage = contextJson["bitmessage"].AsSiring();
                    const CString& jsonKey = contextJson["key"].AsSiring();
                    const CString& jsonPGP = contextJson["pgp"].AsSiring();
                    const CString& jsonFlags = contextJson["flags"].AsSiring();
                    const CString& jsonSign = contextJson["sign"].AsSiring();

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

                    const CJSONValue& jsonURL = contextJson["url"];
                    if (jsonURL.IsArray()) {
                        const CJSONArray& arrayURL = jsonURL.Array();
                        for (int i = 0; i < arrayURL.Count(); i++) {
                            ClearText << arrayURL[i].AsSiring() << LINEFEED;
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

            Json.Object().AddPair("id", GetUID(APOSTOL_MODULE_UID_LENGTH));
            Json.Object().AddPair("address", LUserAddress.IsEmpty() ? LModuleAddress : LUserAddress);

            if (!Payload.IsEmpty())
                Json.Object().AddPair("payload", base64_encode(Payload));

            LProxyRequest->Clear();

            const CString& LHost = LServerRequest->Headers.Values("host");
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

            const CString& LAuthorization = LRequest->Headers.Values(_T("authorization"));
            if (LAuthorization.IsEmpty()) {
                AConnection->SendStockReply(CReply::unauthorized);
                return;
            }

            if (LAuthorization.SubString(0, 5).Lower() == "basic") {
                const CString LPassphrase(base64_decode(LAuthorization.SubString(6)));

                const size_t LPos = LPassphrase.Find(':');
                if (LPos == CString::npos) {
                    AConnection->SendStockReply(CReply::bad_request);
                    return;
                }

                const CAuthData LAuthData = { LPassphrase.SubString(0, LPos), LPassphrase.SubString(LPos + 1) };

                if (LAuthData.Username.IsEmpty() || LAuthData.Password.IsEmpty()) {
                    AConnection->SendStockReply(CReply::unauthorized);
                    return;
                }

                if (LAuthData.Username != "module" || LAuthData.Password != Config()->ModuleAddress()) {
                    AConnection->SendStockReply(CReply::unauthorized);
                    return;
                }
            } else {
                AConnection->SendStockReply(CReply::bad_request);
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
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::DoOptions(CHTTPServerConnection *AConnection) {
            CApostolModule::DoOptions(AConnection);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::DoGet(CHTTPServerConnection *AConnection) {
            auto LRequest = AConnection->Request();
            auto LReply = AConnection->Reply();

            int LVersion = -1;

            CStringList LUri;
            SplitColumns(LRequest->Uri.c_str(), LRequest->Uri.Size(), &LUri, '/');

            if (LUri.Count() < 2) {
                DoWWW(AConnection);
                return;
            }

            if (LUri[1] == _T("v1")) {
                LVersion = 1;
            }

            if (LUri[0] != _T("api") || (LVersion == -1)) {
                DoWWW(AConnection);
                return;
            }

            const CString& LContentType = LRequest->Headers.Values(_T("content-type"));
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
                const CString& R2 = LUri[2].Lower();
                const CString& R3 = LUri.Count() == 4 ? LUri[3].Lower() : "help";

                if (R2 == "ping") {

                    AConnection->SendStockReply(CReply::ok);

                } else if (R2 == "time") {

                    LReply->Content << "{\"serverTime\": " << to_string(MsEpoch()) << "}";

                    AConnection->SendReply(CReply::ok);

                } else if (R2 == "user" && (R3 == "help" || R3 == "status")) {

                    LRequest->Content.Clear();

                    RouteUser(AConnection, "GET", LRoute);

                } else {

                    AConnection->SendStockReply(CReply::not_found);

                }

            } catch (Delphi::Exception::Exception &E) {
                CReply::status_type LStatus = CReply::internal_server_error;

                ExceptionToJson(0, &E, LReply->Content);

                AConnection->SendReply(LStatus);
                Log()->Error(APP_LOG_EMERG, 0, E.what());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::DoPost(CHTTPServerConnection *AConnection) {

            auto LRequest = AConnection->Request();
            int LVersion = -1;

            CStringList LUri;
            SplitColumns(LRequest->Uri.c_str(), LRequest->Uri.Size(), &LUri, '/');
            if (LUri.Count() < 3) {
                AConnection->SendStockReply(CReply::not_found);
                return;
            }

            if (LUri[1] == _T("v1")) {
                LVersion = 1;
            }

            if (LUri[0] != _T("api") || (LVersion == -1)) {
                AConnection->SendStockReply(CReply::not_found);
                return;
            }

            CString LRoute;
            for (int I = 0; I < LUri.Count(); ++I) {
                LRoute.Append('/');
                LRoute.Append(LUri[I]);
            }

            auto LReply = AConnection->Reply();

            try {

                if (LUri[2] == _T("user")) {

                    RouteUser(AConnection, "POST", LRoute);

                } else if (LUri[2] == _T("deal")) {

                    if (LUri[3] == _T("new")) {

                    } else if (LUri[3] == _T("check")) {

                    } else if (LUri[3] == _T("complete")) {

                    }

                } else if (LUri[2] == _T("key")) {

                    if (LUri[3] == _T("update")) {

                    }

                } else {

                    AConnection->SendStockReply(CReply::not_found);

                }

            } catch (Delphi::Exception::Exception &E) {
                CReply::status_type LStatus = CReply::internal_server_error;

                ExceptionToJson(0, &E, LReply->Content);

                AConnection->SendReply(LStatus);
                Log()->Error(APP_LOG_EMERG, 0, E.what());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::Execute(CHTTPServerConnection *AConnection) {

            int i = 0;
            auto LRequest = AConnection->Request();
            auto LReply = AConnection->Reply();

            LReply->Clear();
            LReply->ContentType = CReply::json;

            CMethodHandler *Handler;
            for (i = 0; i < m_Methods.Count(); ++i) {
                Handler = (CMethodHandler *) m_Methods.Objects(i);
                if (Handler->Allow()) {
                    const CString& Method = m_Methods.Strings(i);
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

        void CWebService::BeforeExecute(Pointer Data) {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::AfterExecute(Pointer Data) {

        }
        //--------------------------------------------------------------------------------------------------------------

        bool CWebService::CheckUserAgent(const CString& Value) {
            return true;
        }

    }
}
}