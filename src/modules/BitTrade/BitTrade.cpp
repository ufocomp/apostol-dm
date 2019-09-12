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

        //-- CBitTrade -------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CBitTrade::CBitTrade(CModuleManager *AManager): CApostolModule(AManager) {
            InitHeaders();
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

        void CBitTrade::ExceptionToJson(int ErrorCode, Delphi::Exception::Exception *AException, CString &Json) {

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

            Json.Format(R"({"error": {"code": %u, "message": "%s"}})", ErrorCode, Message.c_str());
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

                ExceptionToJson(0, &E, LReply->Content);

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

                ExceptionToJson(0, &E, LReply->Content);

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

        void CBitTrade::BeforeExecute(Pointer Data) {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CBitTrade::AfterExecute(Pointer Data) {

        }
        //--------------------------------------------------------------------------------------------------------------

        bool CBitTrade::CheckUserAgent(const CString &Value) {
            return true;
        }

    }
}
}