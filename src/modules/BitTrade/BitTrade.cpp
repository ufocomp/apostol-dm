/*++

Programm name:

  Apostol Bitcoin

Module Name:

  BitTrade.cpp

Notices:

  BitTrade - Bitcoin trading module

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

//----------------------------------------------------------------------------------------------------------------------

#include "Core.hpp"
#include "BitTrade.hpp"
//----------------------------------------------------------------------------------------------------------------------

#include <sstream>
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace BitTrade {

        CString to_string(unsigned long Value) {
            TCHAR szString[_INT_T_LEN + 1] = {0};
            sprintf(szString, "%lu", Value);
            return CString(szString);
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CBitTrade ------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CBitTrade::CBitTrade(CModuleManager *AManager): CApostolModule(AManager) {
            m_Headers = new CStringList(true);
            m_curl = curl_easy_init();
            InitHeaders();
        }
        //--------------------------------------------------------------------------------------------------------------

        CBitTrade::~CBitTrade() {
            delete m_Headers;
            curl_easy_cleanup(m_curl);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CBitTrade::InitHeaders() {
            m_Headers->AddObject(_T("GET"), (CObject *) new CHeaderHandler(true, std::bind(&CBitTrade::DoGet, this, _1)));
            m_Headers->AddObject(_T("POST"), (CObject *) new CHeaderHandler(true, std::bind(&CBitTrade::DoPost, this, _1)));
            m_Headers->AddObject(_T("OPTIONS"), (CObject *) new CHeaderHandler(true, std::bind(&CBitTrade::DoOptions, this, _1)));
            m_Headers->AddObject(_T("PUT"), (CObject *) new CHeaderHandler(false, std::bind(&CBitTrade::MethodNotAllowed, this, _1)));
            m_Headers->AddObject(_T("DELETE"), (CObject *) new CHeaderHandler(false, std::bind(&CBitTrade::MethodNotAllowed, this, _1)));
            m_Headers->AddObject(_T("TRACE"), (CObject *) new CHeaderHandler(false, std::bind(&CBitTrade::MethodNotAllowed, this, _1)));
            m_Headers->AddObject(_T("HEAD"), (CObject *) new CHeaderHandler(false, std::bind(&CBitTrade::MethodNotAllowed, this, _1)));
            m_Headers->AddObject(_T("PATCH"), (CObject *) new CHeaderHandler(false, std::bind(&CBitTrade::MethodNotAllowed, this, _1)));
            m_Headers->AddObject(_T("CONNECT"), (CObject *) new CHeaderHandler(false, std::bind(&CBitTrade::MethodNotAllowed, this, _1)));
        }
        //--------------------------------------------------------------------------------------------------------------

        const CString &CBitTrade::GetAllowedMethods() {
            if (m_AllowedMethods.IsEmpty()) {
                if (m_Headers->Count() > 0) {
                    CHeaderHandler *Handler;
                    for (int i = 0; i < m_Headers->Count(); ++i) {
                        Handler = (CHeaderHandler *) m_Headers->Objects(i);
                        if (Handler->Allow()) {
                            if (m_AllowedMethods.IsEmpty())
                                m_AllowedMethods = m_Headers->Strings(i);
                            else
                                m_AllowedMethods += _T(", ") + m_Headers->Strings(i);
                        }
                    }
                }
            }
            return m_AllowedMethods;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CBitTrade::MethodNotAllowed(CHTTPServerConnection *AConnection) {
            auto LReply = AConnection->Reply();

            CReply::GetStockReply(LReply, CReply::not_allowed);

            if (!AllowedMethods().IsEmpty())
                LReply->AddHeader(_T("Allow"), AllowedMethods().c_str());

            AConnection->SendReply();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CBitTrade::LoadConfig() {

            CIniFile ConfFile(Config()->ConfFile().c_str());



        }
        //--------------------------------------------------------------------------------------------------------------

        // Curl's callback
        size_t CBitTrade::curl_cb(void *content, size_t size, size_t nmemb, CString *buffer)
        {
            Log()->Debug(0, "[curl_api] curl_cb");

            buffer->Append((char *) content, size * nmemb);

            //Log()->Debug(0, "[curl_api] Buffer:\n%s", buffer->c_str());

            Log()->Debug(0, "[curl_api] curl_cb: Done!");

            return size * nmemb;
        }
        //--------------------------------------------------------------------------------------------------------------

        // Do the curl
        void CBitTrade::curl_api_with_header(const CString &url, const CString &str_result,
                                                const CStringList &extra_http_header, const CString &post_data, const CString &action) {

            Log()->Debug(0, "[curl_api] curl_api_with_header");

            CURLcode res;

            if ( m_curl ) {

                curl_easy_reset(m_curl);

                curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
                curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, CBitTrade::curl_cb);
                curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &str_result);
                curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, false);
                curl_easy_setopt(m_curl, CURLOPT_ENCODING, "gzip");

                if ( extra_http_header.Count() > 0 ) {

                    struct curl_slist *chunk = nullptr;
                    for ( int i = 0; i < extra_http_header.Count(); i++ ) {
                        chunk = curl_slist_append(chunk, extra_http_header[i].c_str());
                    }

                    curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, chunk);
                }

                if ( action == "GET" ) {

                    curl_easy_setopt(m_curl, CURLOPT_HTTPGET, TRUE );

                } else if ( action == "POST" || action == "PUT" || action == "DELETE" ) {

                    if ( action == "PUT" || action == "DELETE" ) {
                        curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, action.c_str() );
                    } else {
                        curl_easy_setopt(m_curl, CURLOPT_HTTPPOST, TRUE );

                        if (!post_data.IsEmpty()) {
                            curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, post_data.c_str());
                        }
                    }
                }

                clock_t start = clock();

                res = curl_easy_perform(m_curl);

                Log()->Debug(0, "[curl_api] curl_easy_perform runtime: %.2f ms.", (double) ((clock() - start) / (double) CLOCKS_PER_SEC * 1000));

                /* Check for errors */
                if ( res != CURLE_OK ) {
                    Log()->Error(APP_LOG_EMERG, 0, "[curl_api] curl_easy_perform() failed: %s" , curl_easy_strerror(res) );
                }

            }

            Log()->Debug(0, "[curl_api] curl_api_with_header: Done!" ) ;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CBitTrade::curl_api(CString &url, CString &result_json) {
            curl_api_with_header(url, result_json, CStringList(), CString(), "GET");
        }
        //--------------------------------------------------------------------------------------------------------------

        void CBitTrade::ExceptionToJson(Delphi::Exception::Exception *AException, CString &Json) {

            LPCTSTR lpMessage = AException->what();
            CString Message;
            TCHAR ch = 0;

            while (*lpMessage) {
                ch = *lpMessage++;
                if ((ch == '"') || (ch == '\\')) {
                    Message.Append('\\');
                }
                Message.Append(ch);
            }

            Json.Format(R"({"error": {"errors": [{"domain": "%s", "reason": "%s", "message": "%s", "locationType": "%s",
                            "location": "%s"}], "code": %u, "message": "%s"}})",
                            "module", "exception", Message.c_str(),
                            "SQL", "BitTrade", 500, "Internal Server Error");
        }
        //--------------------------------------------------------------------------------------------------------------

        void CBitTrade::DoOptions(CHTTPServerConnection *AConnection) {

            auto LReply = AConnection->Reply();
#ifdef _DEBUG
            auto LRequest = AConnection->Request();
            if (LRequest->Uri == _T("/quit"))
                Application::Application->SignalProcess()->Quit();
#endif
            CReply::GetStockReply(LReply, CReply::ok);

            if (!AllowedMethods().IsEmpty())
                LReply->AddHeader(_T("Allow"), AllowedMethods().c_str());

            AConnection->SendReply();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CBitTrade::UserGet(CHTTPServerConnection *AConnection) {
            auto LRequest = AConnection->Request();
            auto LReply = AConnection->Reply();

        }
        //--------------------------------------------------------------------------------------------------------------

        void CBitTrade::UserPost(CHTTPServerConnection *AConnection) {
            auto LRequest = AConnection->Request();
            auto LReply = AConnection->Reply();

            if (LRequest->Content.IsEmpty()) {
                AConnection->SendStockReply(CReply::no_content);
                return;
            }

            const CJSON Json(LRequest->Content);
            CString post_data;

            const CJSONValue& Address = Json["address"];
            if (!Address.IsEmpty()) {
                post_data << Address.AsSiring();
                post_data << LINEFEED LINEFEED;
            }

            const CJSONValue& Bitmassage = Json["bitmessage"];
            if (!Bitmassage.IsEmpty()) {
                post_data << Bitmassage.AsSiring();
                post_data << LINEFEED LINEFEED;
            }

            const CJSONValue& BTCKey = Json["key"];
            if (!BTCKey.IsEmpty()) {
                post_data << BTCKey.AsSiring();
                post_data << LINEFEED LINEFEED;
            }

            const CJSONValue& PGPKey = Json["PGP"];
            if (!PGPKey.IsEmpty()) {
                post_data << PGPKey.AsSiring();
                post_data << LINEFEED LINEFEED;
            }

            const CJSONValue& URL = Json["URL"];
            if (!URL.IsArray()) {
                for (int i = 0; i < URL.Count(); i++) {
                    post_data << URL[i].AsSiring();
                    post_data << LINEFEED;
                }
                post_data << LINEFEED;
            }

            CString action = "POST";

            CString url("");
            url += "/api/v1/user";

            CStringList extra_http_header;
            extra_http_header.AddPair("host", "");

            curl_api_with_header( url, LReply->Content, extra_http_header, post_data, action );

            if (!LReply->Content.IsEmpty()) {
                Log()->Debug(0, "[UserPost] Done!");
            } else {
                Log()->Debug(0, "[UserPost] Failed to get anything");
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CBitTrade::DoGet(CHTTPServerConnection *AConnection) {

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

            auto LReply = AConnection->Reply();

            try {
                if (LUri[2] == _T("ping")) {

                    AConnection->SendStockReply(CReply::ok);
                    return;

                } else if (LUri[2] == _T("time")) {

                    LReply->Content << "{\"serverTime\": " << to_string( MsEpoch() ) << "}";

                    AConnection->SendReply(CReply::ok);
                    return;

                } else if (LUri[2] == _T("user")) {

                    if (LUri[3] == _T("ratingActual")) {

                    } else if (LUri[3] == _T("ratingPast")) {

                    } else if (LUri[3] == _T("volume")) {

                    }
                }

                AConnection->SendStockReply(CReply::not_found);

            } catch (Delphi::Exception::Exception &E) {
                CReply::status_type LStatus = CReply::internal_server_error;

                ExceptionToJson(&E, LReply->Content);

                AConnection->SendReply(LStatus);
                Log()->Error(APP_LOG_EMERG, 0, E.what());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CBitTrade::DoPost(CHTTPServerConnection *AConnection) {

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

            auto LReply = AConnection->Reply();

            try {

                if (LUri[2] == _T("user")) {

                    UserPost(AConnection);

                } else if (LUri[2] == _T("deal")) {

                    if (LUri[3] == _T("new")) {

                    } else if (LUri[3] == _T("check")) {

                    } else if (LUri[3] == _T("complete")) {

                    }

                } else if (LUri[2] == _T("key")) {

                    if (LUri[3] == _T("update")) {

                    }
                }

                AConnection->SendStockReply(CReply::not_found);

            } catch (Delphi::Exception::Exception &E) {
                CReply::status_type LStatus = CReply::internal_server_error;

                ExceptionToJson(&E, LReply->Content);

                AConnection->SendReply(LStatus);
                Log()->Error(APP_LOG_EMERG, 0, E.what());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CBitTrade::Execute(CHTTPServerConnection *AConnection) {

            int i = 0;
            auto LRequest = AConnection->Request();
            auto LReply = AConnection->Reply();

            LReply->Clear();
            LReply->ContentType = CReply::json;
            LReply->AddHeader("Access-Control-Allow-Origin", "*");

            CHeaderHandler *Handler;
            for (i = 0; i < m_Headers->Count(); ++i) {
                Handler = (CHeaderHandler *) m_Headers->Objects(i);
                if (Handler->Allow()) {
                    const CString& Method = m_Headers->Strings(i);
                    if (Method == LRequest->Method) {
                        Handler->Handler(AConnection);
                        break;
                    }
                }
            }

            if (i == m_Headers->Count()) {
                AConnection->SendStockReply(CReply::not_implemented);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CBitTrade::CheckUrerArent(const CString &Value) {
            return true;
        }

    }
}
}