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

#include <curl/curl.h>

#ifndef APOSTOL_BITTRADE_HPP
#define APOSTOL_BITTRADE_HPP
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace BitTrade {

        typedef std::function<void (CHTTPServerConnection *AConnection)> COnHeaderHandlerEvent;
        //--------------------------------------------------------------------------------------------------------------

        class CHeaderHandler: CObject {
        private:

            bool m_Allow;
            COnHeaderHandlerEvent m_Handler;

        public:

            CHeaderHandler(bool Allow, COnHeaderHandlerEvent && Handler): CObject(),
                m_Allow(Allow), m_Handler(Handler) {

            };

            bool Allow() { return m_Allow; };

            void Handler(CHTTPServerConnection *AConnection) {
                if (m_Allow && m_Handler)
                    m_Handler(AConnection);
            }
        };
        //--------------------------------------------------------------------------------------------------------------

        class CBitTrade: public CApostolModule {
        private:

            CStringList *m_Headers;

            CString m_AllowedMethods;

            CURL* m_curl;

            typedef struct CConfig {
                CString Host;
                CString Secret;
                CString Public;
            } CConfig;

            CConfig m_Config;

            void DoOptions(CHTTPServerConnection *AConnection);
            void DoGet(CHTTPServerConnection *AConnection);
            void DoPost(CHTTPServerConnection *AConnection);

            void MethodNotAllowed(CHTTPServerConnection *AConnection);

        protected:

            static size_t curl_cb(void *content, size_t size, size_t nmemb, CString *buffer);
            void curl_api(CString &url, CString &result_json);
            void curl_api_with_header(const CString &url, const CString &str_result,
                                      const CStringList &extra_http_header, const CString &post_data, const CString &action);

            void InitHeaders();

            void LoadConfig();

            const CString& GetAllowedMethods();

            void ExceptionToJson(Delphi::Exception::Exception *AException, CString& Json);

            void UserPost(CHTTPServerConnection *AConnection);
            void UserGet(CHTTPServerConnection *AConnection);

        public:

            explicit CBitTrade(CModuleManager *AManager);

            ~CBitTrade() override;

            static class CBitTrade *CreateModule(CModuleManager *AManager) {
                return new CBitTrade(AManager);
            }

            bool CheckUrerArent(const CString& Value) override;

            void Execute(CHTTPServerConnection *AConnection) override;

            const CString& AllowedMethods() { return GetAllowedMethods(); };

        };

    }
}

using namespace Apostol::BitTrade;
}
#endif //APOSTOL_BITTRADE_HPP
