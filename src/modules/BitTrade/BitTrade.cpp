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
            InitHeaders();
        }
        //--------------------------------------------------------------------------------------------------------------

        CBitTrade::~CBitTrade() {
            delete m_Headers;
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

                    if (LUri[3] == _T("create")) {

                    } else if (LUri[3] == _T("change")) {

                    }

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