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

    namespace WebService {

        //--------------------------------------------------------------------------------------------------------------

        //-- CWebService -----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CWebService: public CApostolModule {
        private:

            typedef struct CAuthData {
                CString Username;
                CString Password;
            } CAuthData;
            //----------------------------------------------------------------------------------------------------------

            CHTTPProxyManager *m_ProxyManager;

            CHTTPProxy *GetProxy(CHTTPServerConnection *AConnection);

            void DoOptions(CHTTPServerConnection *AConnection) override;

            void DoGet(CHTTPServerConnection *AConnection);
            void DoPost(CHTTPServerConnection *AConnection);

            static void DoWWW(CHTTPServerConnection *AConnection);

            void RouteUser(CHTTPServerConnection *AConnection, const CString& Method, const CString& Uri);

        protected:

            void DoVerbose(CSocketEvent *Sender, CTCPConnection *AConnection, LPCTSTR AFormat, va_list args);
            bool DoProxyExecute(CTCPConnection *AConnection);
            void DoProxyException(CTCPConnection *AConnection, Delphi::Exception::Exception *AException);
            void DoEventHandlerException(CPollEventHandler *AHandler, Delphi::Exception::Exception *AException);

            void DoProxyConnected(CObject *Sender);
            void DoProxyDisconnected(CObject *Sender);

            static void ExceptionToJson(int ErrorCode, Delphi::Exception::Exception *AException, CString& Json);

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

            bool CheckUserAgent(const CString& Value) override;

        };

    }
}

using namespace Apostol::WebService;
}
#endif //APOSTOL_WEBSERVICE_HPP
