/*++

Programm name:

  Apostol Bitcoin

Module Name:

  BitTrade.hpp

Notices:

  BitTrade - Bitcoin trading module

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

    namespace BitTrade {

        class CBitTrade: public CApostolModule {
        private:

            void DoGet(CHTTPServerConnection *AConnection);
            void DoPost(CHTTPServerConnection *AConnection);

        protected:

            static void ExceptionToJson(int ErrorCode, Delphi::Exception::Exception *AException, CString& Json);

            void UserPost(CHTTPServerConnection *AConnection);
            void UserGet(CHTTPServerConnection *AConnection);

        public:

            explicit CBitTrade(CModuleManager *AManager);

            ~CBitTrade() override = default;

            static class CBitTrade *CreateModule(CModuleManager *AManager) {
                return new CBitTrade(AManager);
            }

            void InitHeaders() override;

            void BeforeExecute(Pointer Data) override;
            void AfterExecute(Pointer Data) override;

            void Execute(CHTTPServerConnection *AConnection) override;

            bool CheckUserAgent(const CString& Value) override;

        };

    }
}

using namespace Apostol::BitTrade;
}
#endif //APOSTOL_BITTRADE_HPP
