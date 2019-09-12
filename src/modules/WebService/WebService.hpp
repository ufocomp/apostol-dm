/*++

Program name:

  Apostol Bitcoin

Module Name:

  WebService.hpp

Notices:

  WebService - Bitcoin trading module

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef APOSTOL_BITTRADE_HPP
#define APOSTOL_BITTRADE_HPP
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace WebService {

        class CWebService: public CApostolModule {
        private:

            void DoGet(CHTTPServerConnection *AConnection);
            void DoPost(CHTTPServerConnection *AConnection);

        protected:

            static void ExceptionToJson(int ErrorCode, Delphi::Exception::Exception *AException, CString& Json);

            void UserPost(CHTTPServerConnection *AConnection);
            void UserGet(CHTTPServerConnection *AConnection);

        public:

            explicit CWebService(CModuleManager *AManager);

            ~CWebService() override = default;

            static class CWebService *CreateModule(CModuleManager *AManager) {
                return new CWebService(AManager);
            }

            void InitHeaders() override;

            void BeforeExecute(Pointer Data) override;
            void AfterExecute(Pointer Data) override;

            void Execute(CHTTPServerConnection *AConnection) override;

            bool CheckUserAgent(const CString& Value) override;

        };

    }
}

using namespace Apostol::WebService;
}
#endif //APOSTOL_BITTRADE_HPP
