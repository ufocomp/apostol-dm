/*++

Library name:

  apostol-module

Module Name:

  WebService.hpp

Notices:

  WebService - Deal Module Web Service

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef APOSTOL_WEBSERVICE_HPP
#define APOSTOL_WEBSERVICE_HPP
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Module {

        enum CAuthorizationSchemes { asUnknown, asBasic };

        typedef struct CAuthorization {

            CAuthorizationSchemes Schema;

            CString Username;
            CString Password;

            CAuthorization(): Schema(asUnknown) {

            }

            explicit CAuthorization(const CString& String): CAuthorization() {
                Parse(String);
            }

            void Parse(const CString& String) {
                if (String.SubString(0, 5).Lower() == "basic") {
                    const CString LPassphrase(base64_decode(String.SubString(6)));

                    const size_t LPos = LPassphrase.Find(':');
                    if (LPos == CString::npos)
                        throw Delphi::Exception::Exception("Authorization error: Incorrect passphrase.");

                    Schema = asBasic;
                    Username = LPassphrase.SubString(0, LPos);
                    Password = LPassphrase.SubString(LPos + 1);

                    if (Username.IsEmpty() || Password.IsEmpty())
                        throw Delphi::Exception::Exception("Authorization error: Username and password has not be empty.");
                } else {
                    throw Delphi::Exception::Exception("Authorization error: Unknown schema.");
                }
            }

            CAuthorization &operator << (const CString& String) {
                Parse(String);
                return *this;
            }

        } CAuthorization;

        //--------------------------------------------------------------------------------------------------------------

        //-- CWebService -----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CWebService: public CApostolModule {
        private:

            int m_Version;

            CHTTPProxyManager *m_ProxyManager;

            CHTTPProxy *GetProxy(CHTTPServerConnection *AConnection);

            static void DebugRequest(CRequest *ARequest);
            static void DebugReply(CReply *AReply);
            static void DebugConnection(CHTTPServerConnection *AConnection);

            static void ExceptionToJson(int ErrorCode, const std::exception &AException, CString& Json);

            void RouteUser(CHTTPServerConnection *AConnection, const CString &Method, const CString &Uri);
            void RouteDeal(CHTTPServerConnection *AConnection, const CString &Method, const CString &Uri, const CString &Action);

        protected:

            void DoOptions(CHTTPServerConnection *AConnection) override;

            void DoGet(CHTTPServerConnection *AConnection);
            void DoPost(CHTTPServerConnection *AConnection);

            static void DoWWW(CHTTPServerConnection *AConnection);

            void DoVerbose(CSocketEvent *Sender, CTCPConnection *AConnection, LPCTSTR AFormat, va_list args);
            bool DoProxyExecute(CTCPConnection *AConnection);
            void DoProxyException(CTCPConnection *AConnection, Delphi::Exception::Exception *AException);
            void DoEventHandlerException(CPollEventHandler *AHandler, Delphi::Exception::Exception *AException);

            void DoProxyConnected(CObject *Sender);
            void DoProxyDisconnected(CObject *Sender);

        public:

            explicit CWebService(CModuleManager *AManager);

            ~CWebService() override;

            static class CWebService *CreateModule(CModuleManager *AManager) {
                return new CWebService(AManager);
            }

            void InitMethods() override;

            void BeforeExecute(Pointer Data) override;
            void AfterExecute(Pointer Data) override;

            void Execute(CHTTPServerConnection *AConnection) override;

            bool CheckUserAgent(const CString &Value) override;

        };

    }
}

using namespace Apostol::Module;
}
#endif //APOSTOL_WEBSERVICE_HPP
