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

#include "jwt.h"
//----------------------------------------------------------------------------------------------------------------------

#include <random>
//----------------------------------------------------------------------------------------------------------------------

#define BPS_PGP_HASH "SHA512"
#define BM_PREFIX "BM-"
//----------------------------------------------------------------------------------------------------------------------

#define SYSTEM_PROVIDER_NAME "system"
#define SERVICE_APPLICATION_NAME "service"
#define CONFIG_SECTION_NAME "module"
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Workers {

        CString to_string(unsigned long Value) {
            TCHAR szString[_INT_T_LEN + 1] = {0};
            sprintf(szString, "%lu", Value);
            return CString(szString);
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CWebService -----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CWebService::CWebService(CModuleProcess *AProcess): CApostolModule(AProcess, "web service") {
            m_SyncPeriod = BPS_DEFAULT_SYNC_PERIOD;
            m_ServerIndex = -1;
            m_KeyIndex = 0;

            m_FixedDate = 0;
            m_RandomDate = 0;

            m_Status = psStopped;

            m_DefaultServer.Name() = BPS_BM_SERVER_ADDRESS;
            m_DefaultServer.Value().URI = BPS_SERVER_URL;
            m_DefaultServer.Value().PGP.Name = BPS_BM_SERVER_ADDRESS;

            CWebService::InitMethods();
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
            return Date + (CDateTime) (delta / SecsPerDay);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::LoadOAuth2(const CString &FileName, const CString &ProviderName, const CString &ApplicationName,
                                     CProviders &Providers) {

            CString ConfigFile(FileName);

            if (!path_separator(ConfigFile.front())) {
                ConfigFile = Config()->Prefix() + ConfigFile;
            }

            if (FileExists(ConfigFile.c_str())) {
                CJSONObject Json;
                Json.LoadFromFile(ConfigFile.c_str());

                int index = Providers.IndexOfName(ProviderName);
                if (index == -1)
                    index = Providers.AddPair(ProviderName, CProvider(ProviderName));
                auto& Provider = Providers[index].Value();
                Provider.Applications().AddPair(ApplicationName, Json);
            } else {
                Log()->Error(APP_LOG_WARN, 0, APP_FILE_NOT_FOUND, ConfigFile.c_str());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CWebService::FindURLInLine(const CString &Line, CStringList &List) {
            CString URL;

            TCHAR ch;
            int length = 0;
            size_t startPos, pos;

            pos = 0;
            while ((startPos = Line.Find(HTTP_PREFIX, pos)) != CString::npos) {

                URL.Clear();

                pos = startPos + HTTP_PREFIX_SIZE;
                if (Line.Length() < 5)
                    return false;

                URL.Append(HTTP_PREFIX);
                ch = Line.at(pos);
                if (ch == 's') {
                    URL.Append(ch);
                    pos++;
                }

                if (Line.Length() < 7 || Line.at(pos++) != ':' || Line.at(pos++) != '/' || Line.at(pos++) != '/')
                    return false;

                URL.Append("://");

                length = 0;
                ch = Line.at(pos);
                while (ch != 0 && (IsChar(ch) || IsNumeral(ch) || ch == ':' || ch == '.' || ch == '-')) {
                    URL.Append(ch);
                    length++;
                    ch = Line.at(++pos);
                }

                if (length < 3) {
                    return false;
                }

                if (startPos == 0) {
                    List.Add(URL);
                } else {
                    ch = Line.at(startPos - 1);
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

                size_t numbers = 0;
                size_t delimiter = 0;
                size_t percent = 0;

                size_t pos = 0;
                TCHAR ch;

                ch = Fee.at(pos);
                while (ch != 0) {
                    if (IsNumeral(ch))
                        numbers++;
                    if (ch == '.')
                        delimiter++;
                    if (ch == '%')
                        percent++;
                    ch = Fee.at(++pos);
                }

                if (numbers == 0 || delimiter > 1 || percent > 1 || ((numbers + percent + delimiter) != Fee.Length()))
                    return -1;

                return 1;
            }

            return 0;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::ExceptionToJson(int ErrorCode, const std::exception &E, CString& Json) {
            Json.Format(R"({"error": {"code": %u, "message": "%s"}})", ErrorCode, Delphi::Json::EncodeJsonString(E.what()).c_str());
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CWebService::CreateToken(const CProvider &Provider, const CString &Application) {
            auto token = jwt::create()
                    .set_issuer(Provider.Issuer(Application))
                    .set_audience(Provider.ClientId(Application))
                    .set_issued_at(std::chrono::system_clock::now())
                    .set_expires_at(std::chrono::system_clock::now() + std::chrono::seconds{3600})
                    .sign(jwt::algorithm::hs256{std::string(Provider.Secret(Application))});

            return token;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::FetchAccessToken(const CString &URI, const CString &Assertion, COnSocketExecuteEvent &&OnDone,
                                           COnSocketExceptionEvent &&OnFailed) {

            auto OnRequest = [](CHTTPClient *Sender, CHTTPRequest *ARequest) {

                const auto &token_uri = Sender->Data()["token_uri"];
                const auto &grant_type = Sender->Data()["grant_type"];
                const auto &assertion = Sender->Data()["assertion"];

                ARequest->Content = _T("grant_type=");
                ARequest->Content << CHTTPServer::URLEncode(grant_type);

                ARequest->Content << _T("&assertion=");
                ARequest->Content << CHTTPServer::URLEncode(assertion);

                CHTTPRequest::Prepare(ARequest, _T("POST"), token_uri.c_str(), _T("application/x-www-form-urlencoded"));

                DebugRequest(ARequest);
            };

            auto OnException = [this](CTCPConnection *Sender, const Delphi::Exception::Exception &E) {

                auto pConnection = dynamic_cast<CHTTPClientConnection *> (Sender);
                auto pClient = dynamic_cast<CHTTPClient *> (pConnection->Client());

                DebugReply(pConnection->Reply());

                m_FixedDate = Now() + (CDateTime) 30 / SecsPerDay;

                Log()->Error(APP_LOG_ERR, 0, "[%s:%d] %s", pClient->Host().c_str(), pClient->Port(), E.what());
            };

            CLocation token_uri(URI);

            auto pClient = GetClient(token_uri.hostname, token_uri.port);

            pClient->Data().Values("token_uri", token_uri.pathname);
            pClient->Data().Values("grant_type", "urn:ietf:params:oauth:grant-type:jwt-bearer");
            pClient->Data().Values("assertion", Assertion);

            pClient->OnRequest(OnRequest);
            pClient->OnExecute(static_cast<COnSocketExecuteEvent &&>(OnDone));
            pClient->OnException(OnFailed == nullptr ? OnException : static_cast<COnSocketExceptionEvent &&>(OnFailed));

            pClient->Active(true);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::CreateAccessToken(const CProvider &Provider, const CString &Application, CStringList &Tokens) {

            auto OnDone = [this, &Tokens](CTCPConnection *Sender) {

                auto pConnection = dynamic_cast<CHTTPClientConnection *> (Sender);
                auto pReply = pConnection->Reply();

                DebugReply(pReply);

                const CJSON Json(pReply->Content);

                Tokens.Values("access_token", Json["access_token"].AsString());

                m_Status = psRunning;

                return true;
            };

            CString server_uri("http://localhost:4977");

            const auto &token_uri = Provider.TokenURI(Application);
            const auto &service_token = CreateToken(Provider, Application);

            Tokens.Values("service_token", service_token);

            if (!token_uri.IsEmpty()) {
                FetchAccessToken(token_uri.front() == '/' ? server_uri + token_uri : token_uri, service_token, OnDone);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::VerifyToken(const CString &Token) {

            const auto& GetSecret = [](const CProvider &Provider, const CString &Application) {
                const auto &Secret = Provider.Secret(Application);
                if (Secret.IsEmpty())
                    throw ExceptionFrm("Not found secret for \"%s:%s\"", Provider.Name().c_str(), Application.c_str());
                return Secret;
            };

            auto decoded = jwt::decode(Token);
            const auto& aud = CString(decoded.get_audience());

            CString Application;

            const auto& Providers = Server().Providers();

            const auto Index = OAuth2::Helper::ProviderByClientId(Providers, aud, Application);
            if (Index == -1)
                throw COAuth2Error(_T("Not found provider by Client ID."));

            const auto& Provider = Providers[Index].Value();

            const auto& iss = CString(decoded.get_issuer());

            CStringList Issuers;
            Provider.GetIssuers(Application, Issuers);
            if (Issuers[iss].IsEmpty())
                throw jwt::token_verification_exception("Token doesn't contain the required issuer.");

            const auto& alg = decoded.get_algorithm();

            const auto& caSecret = GetSecret(Provider, Application);

            if (alg == "HS256") {
                auto verifier = jwt::verify()
                        .allow_algorithm(jwt::algorithm::hs256{caSecret});
                verifier.verify(decoded);
            } else if (alg == "HS384") {
                auto verifier = jwt::verify()
                        .allow_algorithm(jwt::algorithm::hs384{caSecret});
                verifier.verify(decoded);
            } else if (alg == "HS512") {
                auto verifier = jwt::verify()
                        .allow_algorithm(jwt::algorithm::hs512{caSecret});
                verifier.verify(decoded);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CWebService::CheckAuthorizationData(CHTTPRequest *ARequest, CAuthorization &Authorization) {

            const auto &caHeaders = ARequest->Headers;
            const auto &caCookies = ARequest->Cookies;

            const auto &caAuthorization = caHeaders["Authorization"];

            if (caAuthorization.IsEmpty()) {

                Authorization.Username = caHeaders["Session"];
                Authorization.Password = caHeaders["Secret"];

                if (Authorization.Username.IsEmpty() || Authorization.Password.IsEmpty())
                    return false;

                Authorization.Schema = CAuthorization::asBasic;
                Authorization.Type = CAuthorization::atSession;

            } else {
                Authorization << caAuthorization;
            }

            return true;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::FetchCerts(CProvider &Provider, const CString &Application) {

            const auto& URI = Provider.CertURI(Application);

            if (URI.IsEmpty()) {
                Log()->Error(APP_LOG_INFO, 0, _T("Certificate URI in provider \"%s\" is empty."), Provider.Name().c_str());
                return;
            }

            Log()->Error(APP_LOG_INFO, 0, _T("Trying to fetch public keys from: %s"), URI.c_str());

            auto OnRequest = [&Provider, &Application](CHTTPClient *Sender, CHTTPRequest *ARequest) {
                const auto& client_x509_cert_url = std::string(Provider.Applications()[Application]["client_x509_cert_url"].AsString());

                Provider.KeyStatusTime(Now());
                Provider.KeyStatus(ksFetching);

                CLocation Location(client_x509_cert_url);
                CHTTPRequest::Prepare(ARequest, "GET", Location.pathname.c_str());
            };

            auto OnExecute = [this, &Provider, &Application](CTCPConnection *AConnection) {

                auto pConnection = dynamic_cast<CHTTPClientConnection *> (AConnection);
                auto pReply = pConnection->Reply();

                try {
                    DebugRequest(pConnection->Request());
                    DebugReply(pReply);

                    Provider.KeyStatusTime(Now());

                    Provider.Keys().Clear();
                    Provider.Keys() << pReply->Content;

                    Provider.KeyStatus(ksSuccess);

                    CreateAccessToken(Provider, Application, m_Tokens[Provider.Name()]);
                } catch (Delphi::Exception::Exception &E) {
                    Provider.KeyStatus(ksFailed);
                    Log()->Error(APP_LOG_ERR, 0, "[Certificate] Message: %s", E.what());
                }

                pConnection->CloseConnection(true);
                return true;
            };

            auto OnException = [&Provider](CTCPConnection *AConnection, const Delphi::Exception::Exception &E) {
                auto pConnection = dynamic_cast<CHTTPClientConnection *> (AConnection);
                auto pClient = dynamic_cast<CHTTPClient *> (pConnection->Client());

                Provider.KeyStatusTime(Now());
                Provider.KeyStatus(ksFailed);

                Log()->Error(APP_LOG_ERR, 0, "[%s:%d] %s", pClient->Host().c_str(), pClient->Port(), E.what());
            };

            CLocation Location(URI);
            auto pClient = GetClient(Location.hostname, Location.port);

            pClient->OnRequest(OnRequest);
            pClient->OnExecute(OnExecute);
            pClient->OnException(OnException);

            pClient->Active(true);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::FetchProviders() {
            for (int i = 0; i < m_Providers.Count(); i++) {
                auto& Provider = m_Providers[i].Value();
                for (int j = 0; j < Provider.Applications().Count(); ++j) {
                    const auto &app = Provider.Applications().Members(j);
                    if (app["type"].AsString() == "service_account") {
                        if (!app["auth_provider_x509_cert_url"].AsString().IsEmpty()) {
                            if (Provider.KeyStatus() == ksUnknown) {
                                FetchCerts(Provider, app.String());
                            }
                        } else {
                            if (Provider.KeyStatus() == ksUnknown) {
                                Provider.KeyStatusTime(Now());
                                CreateAccessToken(Provider, app.String(), m_Tokens[Provider.Name()]);
                                Provider.KeyStatus(ksSuccess);
                            }
                        }
                    }
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::CheckProviders() {
            for (int i = 0; i < m_Providers.Count(); i++) {
                auto& Provider = m_Providers[i].Value();
                if (Provider.KeyStatus() != ksUnknown) {
                    Provider.KeyStatusTime(Now());
                    Provider.KeyStatus(ksUnknown);
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::DoVerbose(CSocketEvent *Sender, CTCPConnection *AConnection, LPCTSTR AFormat, va_list args) {
            Log()->Debug(0, AFormat, args);
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CWebService::DoProxyExecute(CTCPConnection *AConnection) {
            auto pConnection = dynamic_cast<CHTTPClientConnection*> (AConnection);
            auto pProxy = dynamic_cast<CHTTPProxy*> (pConnection->Client());

            auto pServerRequest = pProxy->Connection()->Request();
            auto pServerReply = pProxy->Connection()->Reply();
            auto pProxyReply = pConnection->Reply();

            const auto& Format = pServerRequest->Params["payload"];
            if (!Format.IsEmpty()) {

                if (Format == "html") {
                    pServerReply->ContentType = CHTTPReply::html;
                } else if (Format == "json") {
                    pServerReply->ContentType = CHTTPReply::json;
                } else if (Format == "xml") {
                    pServerReply->ContentType = CHTTPReply::xml;
                } else {
                    pServerReply->ContentType = CHTTPReply::text;
                }

                if (pProxyReply->Status == CHTTPReply::ok) {
                    if (!pProxyReply->Content.IsEmpty()) {
                        const CJSON json(pProxyReply->Content);
                        pServerReply->Content = base64_decode(json["payload"].AsString());
                    }
                    pProxy->Connection()->SendReply(pProxyReply->Status, nullptr, true);
                } else {
                    pProxy->Connection()->SendStockReply(pProxyReply->Status, true);
                }
            } else {
                if (pProxyReply->Status == CHTTPReply::ok) {
                    pServerReply->Content = pProxyReply->Content;
                    pProxy->Connection()->SendReply(pProxyReply->Status, nullptr, true);
                } else {
                    pProxy->Connection()->SendStockReply(pProxyReply->Status, true);
                }
            }

            pConnection->CloseConnection(true);

            return true;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::DoProxyException(CTCPConnection *AConnection, const Delphi::Exception::Exception &E) {
            auto pConnection = dynamic_cast<CHTTPClientConnection*> (AConnection);
            auto pProxy = dynamic_cast<CHTTPProxy*> (pConnection->Client());

            auto pServerRequest = pProxy->Connection()->Request();
            auto pServerReply = pProxy->Connection()->Reply();

            const auto& Format = pServerRequest->Params["format"];
            if (Format == "html") {
                pServerReply->ContentType = CHTTPReply::html;
            }

            try {
                pProxy->Connection()->SendStockReply(CHTTPReply::bad_gateway, true);
            } catch (...) {

            }

            Log()->Error(APP_LOG_EMERG, 0, "[%s:%d] %s", pProxy->Host().c_str(), pProxy->Port(),
                    E.what());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::DoEventHandlerException(CPollEventHandler *AHandler, const Delphi::Exception::Exception &E) {
            auto pConnection = dynamic_cast<CHTTPClientConnection*> (AHandler->Binding());
            auto pProxy = dynamic_cast<CHTTPProxy*> (pConnection->Client());

            if (Assigned(pProxy)) {
                auto pReply = pProxy->Connection()->Reply();
                ExceptionToJson(0, E, pReply->Content);
                pProxy->Connection()->SendReply(CHTTPReply::internal_server_error, nullptr, true);
            }

            Log()->Error(APP_LOG_EMERG, 0, E.what());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::DoProxyConnected(CObject *Sender) {
            auto pConnection = dynamic_cast<CHTTPClientConnection*> (Sender);
            if (pConnection != nullptr) {
                Log()->Message(_T("[%s:%d] Client connected."), pConnection->Socket()->Binding()->PeerIP(),
                               pConnection->Socket()->Binding()->PeerPort());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::DoProxyDisconnected(CObject *Sender) {
            auto pConnection = dynamic_cast<CHTTPClientConnection*> (Sender);
            if (pConnection != nullptr) {
                Log()->Message(_T("[%s:%d] Client disconnected."), pConnection->Socket()->Binding()->PeerIP(),
                               pConnection->Socket()->Binding()->PeerPort());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        CHTTPProxy *CWebService::GetProxy(CHTTPServerConnection *AConnection) {
            auto pProxy = m_ProxyManager.Add(AConnection);
#if defined(_GLIBCXX_RELEASE) && (_GLIBCXX_RELEASE >= 9)
            pProxy->OnVerbose([this](auto && Sender, auto && AConnection, auto && AFormat, auto && args) { DoVerbose(Sender, AConnection, AFormat, args); });

            pProxy->OnExecute([this](auto && AConnection) { return DoProxyExecute(AConnection); });

            pProxy->OnException([this](auto && AConnection, auto && E) { DoProxyException(AConnection, E); });
            pProxy->OnEventHandlerException([this](auto && AHandler, auto && E) { DoEventHandlerException(AHandler, E); });

            pProxy->OnConnected([this](auto && Sender) { DoProxyConnected(Sender); });
            pProxy->OnDisconnected([this](auto && Sender) { DoProxyDisconnected(Sender); });
#else
            pProxy->OnVerbose(std::bind(&CWebService::DoVerbose, this, _1, _2, _3, _4));

            pProxy->OnExecute(std::bind(&CWebService::DoProxyExecute, this, _1));

            pProxy->OnException(std::bind(&CWebService::DoProxyException, this, _1, _2));
            pProxy->OnEventHandlerException(std::bind(&CWebService::DoEventHandlerException, this, _1, _2));

            pProxy->OnConnected(std::bind(&CWebService::DoProxyConnected, this, _1));
            pProxy->OnDisconnected(std::bind(&CWebService::DoProxyDisconnected, this, _1));
#endif
            return pProxy;
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CWebService::CheckAuthorization(CHTTPServerConnection *AConnection, CAuthorization &Authorization) {

            auto pRequest = AConnection->Request();

            try {
                if (CheckAuthorizationData(pRequest, Authorization)) {
                    if (Authorization.Schema == CAuthorization::asBearer) {
                        VerifyToken(Authorization.Token);
                        return true;
                    }
                }

                if (Authorization.Schema == CAuthorization::asBasic)
                    AConnection->Data().Values("Authorization", "Basic");

                ReplyError(AConnection, CHTTPReply::unauthorized, "Unauthorized.");
            } catch (jwt::token_expired_exception &e) {
                ReplyError(AConnection, CHTTPReply::forbidden, e.what());
            } catch (jwt::token_verification_exception &e) {
                ReplyError(AConnection, CHTTPReply::bad_request, e.what());
            } catch (CAuthorizationError &e) {
                ReplyError(AConnection, CHTTPReply::bad_request, e.what());
            } catch (std::exception &e) {
                ReplyError(AConnection, CHTTPReply::bad_request, e.what());
            }

            return false;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::RouteUser(CHTTPServerConnection *AConnection, const CString& Method, const CString& URI) {

            CAuthorization Authorization;
            if (!CheckAuthorization(AConnection, Authorization)) {
                return;
            }

            auto pProxy = GetProxy(AConnection);
            auto pServerRequest = AConnection->Request();
            auto pProxyRequest = pProxy->Request();

            const auto& caModuleAddress = Config()->IniFile().ReadString("module", "address", "");

            const auto& caOrigin = pServerRequest->Headers.Values("origin");
            const auto& caUserAddress = pServerRequest->Params["address"];

            const auto& pgpValue = pServerRequest->Params["pgp"];

            const auto& caServerParam = pServerRequest->Params["server"];
            const auto& caServer = caServerParam.IsEmpty() ? CurrentServer().Value().URI : caServerParam;

            CLocation Location(caServer);

            pProxy->Host() = Location.hostname;
            pProxy->Port(Location.port == 0 ? BPS_SERVER_PORT : Location.port);

            CStringList caClearText;
            CString sPayload;

            if (!pServerRequest->Content.IsEmpty()) {

                const auto& ContentType = pServerRequest->Headers.Values(_T("content-type"));

                if (ContentType.Find("application/x-www-form-urlencoded") == 0) {

                    const CStringList &FormData = pServerRequest->FormData;

                    const auto& formDate = FormData["date"];
                    const auto& formAddress = FormData["address"];
                    const auto& formBitmessage = FormData["bitmessage"];
                    const auto& formKey = FormData["key"];
                    const auto& formPGP = FormData["pgp"];
                    const auto& formURL = FormData["url"];
                    const auto& formFlags = FormData["flags"];
                    const auto& formSign = FormData["sign"];

                    if (!formDate.IsEmpty()) {
                        caClearText << formDate;
                    }

                    if (!formAddress.IsEmpty()) {
                        caClearText << formAddress;
                    }

                    if (!formFlags.IsEmpty()) {
                        caClearText << formFlags;
                    }

                    if (!formBitmessage.IsEmpty()) {
                        caClearText << formBitmessage;
                    }

                    if (!formKey.IsEmpty()) {
                        caClearText << formKey;
                    }

                    if (!formPGP.IsEmpty()) {
                        caClearText << formPGP;
                    }

                    if (!formURL.IsEmpty()) {
                        caClearText << formURL;
                    }

                    if (!formSign.IsEmpty()) {
                        caClearText << formSign;
                    }

                } else if (ContentType.Find("multipart/form-data") == 0) {

                    CFormData FormData;
                    CHTTPRequestParser::ParseFormData(pServerRequest, FormData);

                    const auto& formDate = FormData.Data("date");
                    const auto& formAddress = FormData.Data("address");
                    const auto& formBitmessage = FormData.Data("bitmessage");
                    const auto& formKey = FormData.Data("key");
                    const auto& formPGP = FormData.Data("pgp");
                    const auto& formURL = FormData.Data("url");
                    const auto& formFlags = FormData.Data("flags");
                    const auto& formSign = FormData.Data("sign");

                    if (!formDate.IsEmpty()) {
                        caClearText << formDate;
                    }

                    if (!formAddress.IsEmpty()) {
                        caClearText << formAddress;
                    }

                    if (!formFlags.IsEmpty()) {
                        caClearText << formFlags;
                    }

                    if (!formBitmessage.IsEmpty()) {
                        caClearText << formBitmessage;
                    }

                    if (!formKey.IsEmpty()) {
                        caClearText << formKey;
                    }

                    if (!formPGP.IsEmpty()) {
                        caClearText << formPGP;
                    }

                    if (!formURL.IsEmpty()) {
                        caClearText << formURL;
                    }

                    if (!formSign.IsEmpty()) {
                        caClearText << formSign;
                    }

                } else if (ContentType.Find("application/json") == 0) {

                    const CJSON contextJson(pServerRequest->Content);

                    const auto& jsonDate = contextJson["date"].AsString();
                    const auto& jsonAddress = contextJson["address"].AsString();
                    const auto& jsonBitmessage = contextJson["bitmessage"].AsString();
                    const auto& jsonKey = contextJson["key"].AsString();
                    const auto& jsonPGP = contextJson["pgp"].AsString();
                    const auto& jsonFlags = contextJson["flags"].AsString();
                    const auto& jsonSign = contextJson["sign"].AsString();

                    if (!jsonDate.IsEmpty()) {
                        caClearText << jsonDate;
                    }

                    if (!jsonAddress.IsEmpty()) {
                        caClearText << jsonAddress;
                    }

                    if (!jsonFlags.IsEmpty()) {
                        caClearText << jsonFlags;
                    }

                    if (!jsonBitmessage.IsEmpty()) {
                        caClearText << jsonBitmessage;
                    }

                    if (!jsonKey.IsEmpty()) {
                        caClearText << jsonKey;
                    }

                    if (!jsonPGP.IsEmpty()) {
                        caClearText << jsonPGP;
                    }

                    const CJSONValue &jsonURL = contextJson["url"];
                    if (jsonURL.IsArray()) {
                        const CJSONArray &arrayURL = jsonURL.Array();
                        for (int i = 0; i < arrayURL.Count(); i++) {
                            caClearText << arrayURL[i].AsString();
                        }
                    }

                    if (!jsonSign.IsEmpty()) {
                        caClearText << jsonSign;
                    }

                } else {
                    caClearText = pServerRequest->Content;
                }

                const auto& caPGPPrivateFile = Config()->IniFile().ReadString("pgp", "private", "");
                const auto& caPGPPassphrase = Config()->IniFile().ReadString("pgp", "passphrase", "");

                if (!FileExists(caPGPPrivateFile.c_str()))
                    throw Delphi::Exception::Exception("PGP: Private key file not opened.");

                CString sPGPPrivate;
                sPGPPrivate.LoadFromFile(caPGPPrivateFile.c_str());

                if (pgpValue == "off" || pgpValue == "false") {
                    sPayload = caClearText.Text();
                } else {
                    Apostol::PGP::CleartextSignature(
                            sPGPPrivate,
                            caPGPPassphrase,
                            BPS_PGP_HASH,
                            caClearText.Text(),
                            sPayload);
                }
            }

            //DebugMessage("[RouteUser] Server request:\n%s\n", pServerRequest->Content.c_str());
            //DebugMessage("[RouteUser] sPayload:\n%s\n", sPayload.c_str());

            CJSON Json(jvtObject);

            Json.Object().AddPair("id", ApostolUID());
            Json.Object().AddPair("address", caUserAddress.IsEmpty() ? caModuleAddress : caUserAddress);

            if (!sPayload.IsEmpty())
                Json.Object().AddPair("payload", base64_encode(sPayload));

            pProxyRequest->Clear();

            pProxyRequest->Location = pServerRequest->Location;
            pProxyRequest->CloseConnection = true;
            pProxyRequest->ContentType = CHTTPRequest::json;
            pProxyRequest->Content << Json;

            CHTTPRequest::Prepare(pProxyRequest, Method.c_str(), URI.c_str());

            if (!caModuleAddress.IsEmpty())
                pProxyRequest->AddHeader("Module-Address", caModuleAddress);

            if (!caOrigin.IsEmpty())
                pProxyRequest->AddHeader("Origin", caOrigin);

            pProxy->Active(true);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::CheckDeal(const CDeal &Deal) {

            const auto& Data = Deal.Data();

            const auto DateTime = UTC();
            const auto Date = StringToDate(Data.Date);

            if (Data.Order == doCreate) {
                if (DateTime < Date)
                    throw ExceptionFrm("Invalid deal date.");

                if ((DateTime - Date) > (CDateTime) 180 / SecsPerDay)
                    throw ExceptionFrm("Deal date expired.");
            }

            if (Data.Order == doComplete) {
                const CDateTime LeaveBefore = StringToDate(Data.FeedBack.LeaveBefore);
                if (DateTime > LeaveBefore)
                    throw ExceptionFrm("Date feedback expired.");
            }

            if (Odd(int(Data.Order)) || Data.Order == doExecute || Data.Order == doDelete)
                throw ExceptionFrm("Invalid \"order\" value for deal module.");

            if (Data.Order == doCancel) {
                const CDateTime Until = StringToDate(Data.Payment.Until);
                if (DateTime > Until)
                    throw ExceptionFrm("Deal cancellation expired.");

                CString message(Data.Payment.Address);
                if (!Data.FeedBack.Comments.IsEmpty()) {
                    message += LINEFEED;
                    message += Data.FeedBack.Comments;
                }

                if (Data.Seller.Signature.IsEmpty() || !VerifyMessage(message, Data.Seller.Address, Data.Seller.Signature))
                    throw ExceptionFrm("The deal is not signed by the seller.");
            }

            if (Data.Order == doFeedback) {
                CString message(Data.Payment.Address);
                if (!Data.FeedBack.Comments.IsEmpty()) {
                    message += LINEFEED;
                    message += Data.FeedBack.Comments;
                }

                if (Data.Customer.Signature.IsEmpty() || !VerifyMessage(message, Data.Customer.Address, Data.Customer.Signature))
                    throw ExceptionFrm("The deal is not signed by the customer.");
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

            CAuthorization Authorization;
            if (!CheckAuthorization(AConnection, Authorization)) {
                return;
            }

            auto pProxy = GetProxy(AConnection);
            auto pServerRequest = AConnection->Request();
            auto pProxyRequest = pProxy->Request();

            const auto& caModuleAddress = Config()->IniFile().ReadString("module", "address", "");
            const auto& caModuleFee = Config()->IniFile().ReadString("module", "fee", "0.1%");

            const auto checkFee = CheckFee(caModuleFee);
            if (checkFee == -1)
                throw ExceptionFrm("Invalid module fee value: %s", caModuleFee.c_str());

            const auto& caOrigin = pServerRequest->Headers.Values("origin");
            const auto& caUserAddress = pServerRequest->Params["address"];

            const auto& pgpValue = pServerRequest->Params["pgp"];

            const auto& caServerParam = pServerRequest->Params["server"];
            const auto& caServer = caServerParam.IsEmpty() ? CurrentServer().Value().URI : caServerParam;

            CLocation Location(caServer);

            pProxy->Host() = Location.hostname;
            pProxy->Port(Location.port == 0 ? BPS_SERVER_PORT : Location.port);

            YAML::Node Node;

            CString sPayload;

            if (!pServerRequest->Content.IsEmpty()) {

                const auto& ContentType = pServerRequest->Headers.Values(_T("content-type"));

                if (ContentType.Find("application/x-www-form-urlencoded") == 0) {

                    const CStringList &FormData = pServerRequest->FormData;

                    const auto& formType = FormData["type"];
                    const auto& formAt = FormData["at"];
                    const auto& formDate = FormData["date"];
                    const auto& formSellerAddress = FormData["seller_address"];
                    const auto& formSellerRating = FormData["seller_rating"];
                    const auto& formSellerSignature = FormData["seller_signature"];
                    const auto& formCustomerAddress = FormData["customer_address"];
                    const auto& formCustomerRating = FormData["customer_rating"];
                    const auto& formCustomerSignature = FormData["customer_signature"];
                    const auto& formPaymentAddress = FormData["payment_address"];
                    const auto& formPaymentUntil = FormData["payment_until"];
                    const auto& formPaymentSum = FormData["payment_sum"];
                    const auto& formFeedbackLeaveBefore = FormData["feedback_leave_before"];
                    const auto& formFeedbackStatus = FormData["feedback_status"];
                    const auto& formFeedbackComments = FormData["feedback_comments"];

                    CheckKeyForNull("order", Action.c_str());
                    CheckKeyForNull("type", formType.c_str());
                    CheckKeyForNull("at", formAt.c_str());
                    CheckKeyForNull("date", formDate.c_str());
                    CheckKeyForNull("seller_address", formSellerAddress.c_str());
                    CheckKeyForNull("customer_address", formCustomerAddress.c_str());
                    CheckKeyForNull("payment_sum", formPaymentSum.c_str());

                    if (Action == "cancel") {
                        CheckKeyForNull("seller_signature", formSellerSignature.c_str());
                    }

                    if (Action == "feedback") {
                        CheckKeyForNull("customer_signature", formCustomerSignature.c_str());
                    }

                    YAML::Node Deal = Node["deal"];

                    Deal["order"] = Action.c_str();
                    Deal["type"] = formType.c_str();

                    Deal["at"] = formAt.c_str();
                    Deal["date"] = formDate.c_str();

                    YAML::Node Seller = Deal["seller"];

                    Seller["address"] = formSellerAddress.c_str();

                    if (!formSellerRating.IsEmpty())
                        Seller["rating"] = formSellerRating.c_str();

                    if (!formSellerSignature.IsEmpty())
                        Seller["signature"] = formSellerSignature.c_str();

                    YAML::Node Customer = Deal["customer"];

                    Customer["address"] = formCustomerAddress.c_str();

                    if (!formCustomerRating.IsEmpty())
                        Customer["rating"] = formCustomerRating.c_str();

                    if (!formCustomerSignature.IsEmpty())
                        Customer["signature"] = formCustomerSignature.c_str();

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

                } else if (ContentType.Find("multipart/form-data") == 0) {

                    CFormData FormData;
                    CHTTPRequestParser::ParseFormData(pServerRequest, FormData);

                    const auto& formType = FormData.Data("type");
                    const auto& formAt = FormData.Data("at");
                    const auto& formDate = FormData.Data("date");
                    const auto& formSellerAddress = FormData.Data("seller_address");
                    const auto& formSellerRating = FormData.Data("seller_rating");
                    const auto& formSellerSignature = FormData.Data("seller_signature");
                    const auto& formCustomerAddress = FormData.Data("customer_address");
                    const auto& formCustomerRating = FormData.Data("customer_rating");
                    const auto& formCustomerSignature = FormData.Data("customer_signature");
                    const auto& formPaymentAddress = FormData.Data("payment_address");
                    const auto& formPaymentUntil = FormData.Data("payment_until");
                    const auto& formPaymentSum = FormData.Data("payment_sum");
                    const auto& formFeedbackLeaveBefore = FormData.Data("feedback_leave_before");
                    const auto& formFeedbackStatus = FormData.Data("feedback_status");
                    const auto& formFeedbackComments = FormData.Data("feedback_comments");

                    CheckKeyForNull("order", Action.c_str());
                    CheckKeyForNull("type", formType.c_str());
                    CheckKeyForNull("at", formAt.c_str());
                    CheckKeyForNull("date", formDate.c_str());
                    CheckKeyForNull("seller_address", formSellerAddress.c_str());
                    CheckKeyForNull("customer_address", formCustomerAddress.c_str());
                    CheckKeyForNull("payment_sum", formPaymentSum.c_str());

                    if (Action == "cancel") {
                        CheckKeyForNull("seller_signature", formSellerSignature.c_str());
                    }

                    if (Action == "feedback") {
                        CheckKeyForNull("customer_signature", formCustomerSignature.c_str());
                    }

                    YAML::Node Deal = Node["deal"];

                    Deal["order"] = Action.c_str();
                    Deal["type"] = formType.c_str();

                    Deal["at"] = formAt.c_str();
                    Deal["date"] = formDate.c_str();

                    YAML::Node Seller = Deal["seller"];

                    Seller["address"] = formSellerAddress.c_str();

                    if (!formSellerRating.IsEmpty())
                        Seller["rating"] = formSellerRating.c_str();

                    if (!formSellerSignature.IsEmpty())
                        Seller["signature"] = formSellerSignature.c_str();

                    YAML::Node Customer = Deal["customer"];

                    Customer["address"] = formCustomerAddress.c_str();

                    if (!formSellerRating.IsEmpty())
                        Customer["rating"] = formCustomerRating.c_str();

                    if (!formCustomerSignature.IsEmpty())
                        Customer["signature"] = formCustomerSignature.c_str();

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

                } else if (ContentType.Find("application/json") == 0) {

                    const CJSON jsonData(pServerRequest->Content);

                    const auto& formOrder = jsonData["order"].AsString().Lower();
                    const auto& formType = jsonData["type"].AsString();

                    const auto& formAt = jsonData["at"].AsString();
                    const auto& formDate = jsonData["date"].AsString();

                    const CJSONValue &jsonSeller = jsonData["seller"];

                    const auto& formSellerAddress = jsonSeller["address"].AsString();
                    const auto& formSellerRating = jsonSeller["rating"].AsString();
                    const auto& formSellerSignature = jsonSeller["signature"].AsString();

                    const CJSONValue &jsonCustomer = jsonData["customer"];

                    const auto& formCustomerAddress = jsonCustomer["address"].AsString();
                    const auto& formCustomerRating = jsonCustomer["rating"].AsString();
                    const auto& formCustomerSignature = jsonCustomer["signature"].AsString();

                    const CJSONValue &jsonPayment = jsonData["payment"];

                    const auto& formPaymentAddress = jsonPayment["address"].AsString();
                    const auto& formPaymentUntil = jsonPayment["until"].AsString();
                    const auto& formPaymentSum = jsonPayment["sum"].AsString();

                    const CJSONValue &jsonFeedback = jsonData["feedback"];

                    const auto& formFeedbackLeaveBefore = jsonFeedback["leave-before"].AsString();
                    const auto& formFeedbackStatus = jsonFeedback["status"].AsString();
                    const auto& formFeedbackComments = jsonFeedback["comments"].AsString();

                    const auto &action = Action.IsEmpty() ? formOrder : Action;

                    CheckKeyForNull("order", action.c_str());
                    CheckKeyForNull("type", formType.c_str());
                    CheckKeyForNull("at", formAt.c_str());
                    CheckKeyForNull("date", formDate.c_str());
                    CheckKeyForNull("seller.address", formSellerAddress.c_str());
                    CheckKeyForNull("customer.address", formCustomerAddress.c_str());
                    CheckKeyForNull("payment.sum", formPaymentSum.c_str());

                    if (action == "cancel") {
                        CheckKeyForNull("seller.signature", formSellerSignature.c_str());
                    }

                    if (action == "feedback") {
                        CheckKeyForNull("customer.signature", formCustomerSignature.c_str());
                    }

                    YAML::Node Deal = Node["deal"];

                    Deal["order"] = action.c_str();
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
                    Node = YAML::Load(pServerRequest->Content.c_str());
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
                            Deal.Data().Transaction.Key, BitcoinConfig.version_hd, BitcoinConfig.version_script);

                    Node["deal"]["date"] = Data.Date.c_str();

                    YAML::Node Payment = Node["deal"]["payment"];
                    Payment.remove("sum");

                    Payment["address"] = Data.Payment.Address.c_str();
                    Payment["until"] = Data.Payment.Until.c_str();
                    Payment["sum"] = Data.Payment.Sum.c_str();

                    Node["deal"]["feedback"]["leave-before"] = Data.FeedBack.LeaveBefore.c_str();
                }

                CheckDeal(Deal);

                const auto& caPGPPrivateFile = Config()->IniFile().ReadString("pgp", "private", "");
                const auto& caPGPPassphrase = Config()->IniFile().ReadString("pgp", "passphrase", "");

                if (!FileExists(caPGPPrivateFile.c_str()))
                    throw Delphi::Exception::Exception("PGP: Private key file not opened.");

                CString sPGPPrivate;
                sPGPPrivate.LoadFromFile(caPGPPrivateFile.c_str());

                const CString caClearText(YAML::Dump(Node));

                if (pgpValue == "off" || pgpValue == "false") {
                    sPayload = caClearText;
                } else {
                    Apostol::PGP::CleartextSignature(
                            sPGPPrivate,
                            caPGPPassphrase,
                            BPS_PGP_HASH,
                            caClearText,
                            sPayload);
                }
            }

            //DebugMessage("[RouteDeal] Server request:\n%s\n", pServerRequest->Content.c_str());
            //DebugMessage("[RouteDeal] sPayload:\n%s\n", sPayload.c_str());

            CJSON Json(jvtObject);

            Json.Object().AddPair("id", ApostolUID());
            if (!caUserAddress.IsEmpty())
                Json.Object().AddPair("address", caUserAddress.IsEmpty() ? caModuleAddress : caUserAddress);

            if (!sPayload.IsEmpty())
                Json.Object().AddPair("payload", base64_encode(sPayload));

            pProxyRequest->Clear();

            pProxyRequest->Location = pServerRequest->Location;
            pProxyRequest->CloseConnection = true;
            pProxyRequest->ContentType = CHTTPRequest::json;
            pProxyRequest->Content << Json;

            CHTTPRequest::Prepare(pProxyRequest, Method.c_str(), URI.c_str());

            if (!caModuleAddress.IsEmpty())
                pProxyRequest->AddHeader("Module-Address", caModuleAddress);

            if (!caModuleFee.IsEmpty())
                pProxyRequest->AddHeader("Module-Fee", caModuleFee);

            if (!caOrigin.IsEmpty())
                pProxyRequest->AddHeader("Origin", caOrigin);

            pProxy->Active(true);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::RouteSignature(CHTTPServerConnection *AConnection) {

            CAuthorization Authorization;
            if (!CheckAuthorization(AConnection, Authorization)) {
                return;
            }

            auto pRequest = AConnection->Request();
            auto pReply = AConnection->Reply();

            if (pRequest->Content.IsEmpty()) {
                AConnection->SendStockReply(CHTTPReply::no_content);
                return;
            }

            const auto& caServerKey = CurrentServer().Value().PGP.Key;

            if (caServerKey.IsEmpty())
                throw ExceptionFrm("Server PGP key not added.");

            CString message;
            CJSON Json(jvtObject);

            const auto& ContentType = pRequest->Headers.Values(_T("content-type"));

            if (ContentType.Find("application/x-www-form-urlencoded") == 0) {
                const CStringList &FormData = pRequest->FormData;

                const auto& caClearText = FormData["message"];
                CheckKeyForNull("message", caClearText.c_str());

                const auto bVerified = CheckVerifyPGPSignature(VerifyPGPSignature(caClearText, caServerKey, message), message);
                Json.Object().AddPair("verified", bVerified);
            } else if (ContentType.Find("multipart/form-data") == 0) {
                CFormData FormData;
                CHTTPRequestParser::ParseFormData(pRequest, FormData);

                const auto& caClearText = FormData.Data("message");
                CheckKeyForNull("message", caClearText.c_str());

                const auto bVerified = CheckVerifyPGPSignature(VerifyPGPSignature(caClearText, caServerKey, message), message);
                Json.Object().AddPair("verified", bVerified);
            } else if (ContentType.Find("application/json") == 0) {
                const CJSON jsonData(pRequest->Content);

                const auto& caClearText = jsonData["message"].AsString();
                CheckKeyForNull("message", caClearText.c_str());

                const auto bVerified = CheckVerifyPGPSignature(VerifyPGPSignature(caClearText, caServerKey, message), message);
                Json.Object().AddPair("verified", bVerified);
            } else {
                const auto& caClearText = pRequest->Content;
                const auto bVerified = CheckVerifyPGPSignature(VerifyPGPSignature(caClearText, caServerKey, message), message);
                Json.Object().AddPair("verified", bVerified);
            }

            Json.Object().AddPair("message", message);

            AConnection->CloseConnection(true);
            pReply->Content << Json;

            AConnection->SendReply(CHTTPReply::ok);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::DoAPI(CHTTPServerConnection *AConnection) {

            auto pRequest = AConnection->Request();
            auto pReply = AConnection->Reply();

            pReply->ContentType = CHTTPReply::json;

            CStringList slRouts;
            SplitColumns(pRequest->Location.pathname, slRouts, '/');

            if (slRouts.Count() < 3) {
                AConnection->SendStockReply(CHTTPReply::not_found);
                return;
            }

            const auto& caService = slRouts[0].Lower();
            const auto& caVersion = slRouts[1].Lower();
            const auto& caCommand = slRouts[2].Lower();
            const auto& caAction = slRouts.Count() == 4 ? slRouts[3].Lower() : "";

            if (caService != "api") {
                AConnection->SendStockReply(CHTTPReply::not_found);
                return;
            }

            if (caVersion != "v1") {
                AConnection->SendStockReply(CHTTPReply::not_found);
                return;
            }

            CString sRoute;
            for (int I = 0; I < slRouts.Count(); ++I) {
                sRoute.Append('/');
                sRoute.Append(slRouts[I]);
            }

            try {
                if (caCommand == "ping") {

                    AConnection->SendStockReply(CHTTPReply::ok);

                } else if (caCommand == "time") {

                    pReply->Content << "{\"serverTime\": " << to_string(MsEpoch()) << "}";

                    AConnection->SendReply(CHTTPReply::ok);

                } else if (caCommand == "user" && (caAction == "help" || caAction == "status")) {

                    pRequest->Content.Clear();

                    RouteUser(AConnection, "GET", sRoute);

                } else if (caCommand == "deal" && caAction == "status") {

                    pRequest->Content.Clear();

                    RouteDeal(AConnection, "GET", sRoute, caAction);

                } else {

                    AConnection->SendStockReply(CHTTPReply::not_found);

                }

            } catch (std::exception &e) {
                CHTTPReply::CStatusType LStatus = CHTTPReply::internal_server_error;

                ExceptionToJson(0, e, pReply->Content);

                AConnection->SendReply(LStatus);
                Log()->Error(APP_LOG_EMERG, 0, e.what());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::DoGet(CHTTPServerConnection *AConnection) {

            auto pRequest = AConnection->Request();

            CString sPath(pRequest->Location.pathname);

            // Request sPath must be absolute and not contain "..".
            if (sPath.empty() || sPath.front() != '/' || sPath.find(_T("..")) != CString::npos) {
                AConnection->SendStockReply(CHTTPReply::bad_request);
                return;
            }

            if (sPath.SubString(0, 5) == "/api/") {
                DoAPI(AConnection);
                return;
            }

            SendResource(AConnection, sPath);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::DoPost(CHTTPServerConnection *AConnection) {

            auto pRequest = AConnection->Request();
            auto pReply = AConnection->Reply();

            pReply->ContentType = CHTTPReply::json;

            CStringList slRouts;
            SplitColumns(pRequest->Location.pathname, slRouts, '/');

            if (slRouts.Count() < 3) {
                AConnection->SendStockReply(CHTTPReply::not_found);
                return;
            }

            const auto& caService = slRouts[0].Lower();
            const auto& caVersion = slRouts[1].Lower();
            const auto& caCommand = slRouts[2].Lower();
            const auto& caAction = slRouts.Count() == 4 ? slRouts[3].Lower() : "";

            if (caService != "api") {
                AConnection->SendStockReply(CHTTPReply::not_found);
                return;
            }

            if (caVersion != "v1") {
                AConnection->SendStockReply(CHTTPReply::not_found);
                return;
            }

            CString sRoute;
            for (int I = 0; I < slRouts.Count(); ++I) {
                sRoute.Append('/');
                sRoute.Append(slRouts[I]);
            }

            try {

                if (caCommand == "user") {

                    RouteUser(AConnection, "POST", sRoute);

                } else if (caCommand == "deal") {

                    RouteDeal(AConnection, "POST", sRoute, caAction);

                } else if (caCommand == "signature") {

                    RouteSignature(AConnection);

                } else {

                    AConnection->SendStockReply(CHTTPReply::not_found);

                }

            } catch (std::exception &e) {
                ExceptionToJson(0, e, pReply->Content);

                AConnection->SendReply(CHTTPReply::internal_server_error);
                Log()->Error(APP_LOG_EMERG, 0, e.what());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        int CWebService::NextServerIndex() {
            m_ServerIndex++;
            if (m_ServerIndex >= m_Servers.Count())
                m_ServerIndex = -1;
            return m_ServerIndex;
        }
        //--------------------------------------------------------------------------------------------------------------

        const CServer &CWebService::CurrentServer() const {
            if (m_Servers.Count() == 0 && m_ServerIndex == -1)
                return m_DefaultServer;
            if (m_ServerIndex == -1)
                return m_Servers[0];
            return m_Servers[m_ServerIndex];
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

        int CWebService::VerifyPGPSignature(const CString &caClearText, const CString &Key, CString &Message) {
            const OpenPGP::Key signer(Key.c_str());
            const OpenPGP::CleartextSignature cleartext(caClearText.c_str());

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

            if (Json.HasOwnProperty("error")) {
                const auto &error = Json["error"];
                if (error.ValueType() == jvtObject)
                    throw Delphi::Exception::Exception(error["message"].AsString().c_str());
            }

            if (Json.HasOwnProperty("data")) {
                Key = Json["data"].AsString();
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::FetchPGP(CKeyContext &PGP) {

            const auto& caServerContext = CurrentServer().Value();

            Log()->Debug(0, "Trying to fetch a PGP key \"%s\" from: %s", PGP.Name.c_str(), caServerContext.URI.c_str());

            auto OnRequest = [this, &PGP](CHTTPClient *Sender, CHTTPRequest *ARequest) {
                PGP.StatusTime = Now();
                PGP.Status = CKeyContext::ksFetching;

                CJSON Content;
                CJSONArray Fields;

                Fields.Add("data");

                Content.Object().AddPair("system", "pgp.system");
                Content.Object().AddPair("client", PGP.Name.c_str());
                Content.Object().AddPair("code", PGP.Name.c_str());
                Content.Object().AddPair("fields", Fields);

                ARequest->Content = Content.ToString();

                CHTTPRequest::Prepare(ARequest, "POST", "/api/v1/key/public", "application/json");

                ARequest->AddHeader("Authorization", "Bearer " + m_Tokens[SYSTEM_PROVIDER_NAME]["access_token"]);
            };

            auto OnExecute = [this, &PGP](CTCPConnection *AConnection) {

                auto pConnection = dynamic_cast<CHTTPClientConnection *> (AConnection);
                auto pReply = pConnection->Reply();

                try {
                    PGP.StatusTime = Now();
                    PGP.Status = CKeyContext::ksError;

                    JsonStringToKey(pReply->Content, PGP.Key);

                    if (PGP.Key.IsEmpty())
                        throw Delphi::Exception::Exception("Not found.");

                    DebugMessage("[PGP] Key:\n%s\n", PGP.Key.c_str());

                    CStringPairs ServerList;
                    CStringList BTCKeys;

                    ParsePGPKey(PGP.Key, ServerList, BTCKeys);

                    if (ServerList.Count() != 0) {
                        CStringPairs::ConstEnumerator em(ServerList);
                        while (em.MoveNext()) {
                            const auto &current = em.Current();
                            if (m_Servers.IndexOfName(current.Name()) == -1) {
                                m_Servers.AddPair(current.Name(), CServerContext(current.Value()));
                            }
                        }
                    }

                    m_BTCKeys = BTCKeys;
                    PGP.Status = CKeyContext::ksSuccess;
                    m_RandomDate = GetRandomDate(10 * 60, m_SyncPeriod * 60, Now()); // 10..m_SyncPeriod min
                } catch (Delphi::Exception::Exception &e) {
                    Log()->Error(APP_LOG_INFO, 0, "[PGP] Message: %s", e.what());
                    PGP.Status = CKeyContext::ksError;
                    m_RandomDate = Now() + (CDateTime) 30 / SecsPerDay;
                }

                pConnection->CloseConnection(true);
                return true;
            };

            auto OnException = [this, &PGP](CTCPConnection *AConnection, const Delphi::Exception::Exception &E) {
                auto pConnection = dynamic_cast<CHTTPClientConnection *> (AConnection);
                auto pClient = dynamic_cast<CHTTPClient *> (pConnection->Client());

                Log()->Error(APP_LOG_EMERG, 0, "[%s:%d] %s", pClient->Host().c_str(), pClient->Port(), E.what());

                PGP.Status = CKeyContext::ksError;
                m_RandomDate = Now() + (CDateTime) 5 / SecsPerDay;
            };

            CLocation Location(caServerContext.URI);
            auto pClient = GetClient(Location.hostname, Location.port == 0 ? BPS_SERVER_PORT : Location.port);

            pClient->OnRequest(OnRequest);
            pClient->OnExecute(OnExecute);
            pClient->OnException(OnException);

            pClient->Active(true);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::FetchBTC(CKeyContext &BTC) {

            const auto& caServerContext = CurrentServer().Value();

            Log()->Debug(0, "Trying to fetch a BTC KEY%d from: %s", m_KeyIndex + 1, caServerContext.URI.c_str());

            auto OnRequest = [this, &BTC](CHTTPClient *Sender, CHTTPRequest *ARequest) {
                const auto& caServerContext = CurrentServer().Value();
                BTC.StatusTime = Now();
                BTC.Status = CKeyContext::ksFetching;
                CString URI("/api/v1/key?type=BTC-PUBLIC&name=KEY");
                URI << m_KeyIndex + 1;
                CHTTPRequest::Prepare(ARequest, "GET", URI.c_str());
            };

            auto OnExecute = [this, &BTC](CTCPConnection *AConnection) {
                auto pConnection = dynamic_cast<CHTTPClientConnection *> (AConnection);
                auto pReply = pConnection->Reply();

                try {
                    CString Key;
                    JsonStringToKey(pReply->Content, Key);
                    if (!Key.IsEmpty()) {
                        m_BTCKeys.Add(Key);
                        m_KeyIndex++;
                        //FetchBTC();
                    }
                } catch (Delphi::Exception::Exception &e) {
                    Log()->Error(APP_LOG_INFO, 0, "[BTC] Message: %s", e.what());
                    if (m_BTCKeys.Count() == 0) {
                        BTC.Status = CKeyContext::ksError;
                    } else {
                        BTC.Status = CKeyContext::ksSuccess;
                    }
                }

                pConnection->CloseConnection(true);

                return true;
            };

            auto OnException = [&BTC](CTCPConnection *AConnection, const Delphi::Exception::Exception &E) {
                auto pConnection = dynamic_cast<CHTTPClientConnection *> (AConnection);
                auto pClient = dynamic_cast<CHTTPClient *> (pConnection->Client());

                Log()->Error(APP_LOG_EMERG, 0, "[%s:%d] %s", pClient->Host().c_str(), pClient->Port(), E.what());
                BTC.Status = CKeyContext::ksError;
            };

            CLocation Location(caServerContext.URI);
            auto pClient = GetClient(Location.hostname, Location.port == 0 ? BPS_SERVER_PORT : Location.port);

            pClient->OnRequest(OnRequest);
            pClient->OnExecute(OnExecute);
            pClient->OnException(OnException);

            pClient->Active(true);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::FetchKeys() {
            m_KeyIndex = 0;
            if (NextServerIndex() != -1) {
                m_PGP.Status = CKeyContext::ksUnknown;
                FetchPGP(m_PGP);
                FetchPGP(m_Servers[m_ServerIndex].Value().PGP);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::Reload() {

            m_Providers.Clear();

            const auto &oauth2 = Config()->IniFile().ReadString(CONFIG_SECTION_NAME, "oauth2", "oauth2/service.json");
            const auto &provider = CString(SYSTEM_PROVIDER_NAME);
            const auto &application = CString(SERVICE_APPLICATION_NAME);

            if (!oauth2.empty()) {
                m_Tokens.AddPair(provider, CStringList());
                LoadOAuth2(oauth2, provider, application, m_Providers);
            }

            m_FixedDate = 0;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::Initialization(CModuleProcess *AProcess) {
            CApostolModule::Initialization(AProcess);
            Reload();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::Heartbeat() {
            const auto now = Now();

            if ((now >= m_FixedDate)) {
                m_FixedDate = now + (CDateTime) 55 / MinsPerDay; // 55 min
                CheckProviders();
                FetchProviders();
            }

            if (m_Servers.Count() == 0) {
#ifdef _DEBUG
                int Index = m_Servers.AddPair("BM-2cXtL92m3CavBKx8qsV2LbZtAU3eQxW2rB", CServerContext("http://localhost:4977"));
                m_Servers[Index].Value().PGP.Name = m_Servers[Index].Name();
#else
                m_Servers.Add(m_DefaultServer);
#endif
            }

//            const auto& caServer = CurrentServer().Value();

            if (m_Status == psRunning) {
                if (now >= m_RandomDate) {
                    FetchKeys();
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CWebService::Enabled() {
            if (m_ModuleStatus == msUnknown)
                m_ModuleStatus = msEnabled;
            return m_ModuleStatus == msEnabled;
        }
        //--------------------------------------------------------------------------------------------------------------

    }
}
}