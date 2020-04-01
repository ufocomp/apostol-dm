/*++

Library name:

  apostol-core

Module Name:

  CURL.hpp

Notices:

  Apostol Core (cURL)

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef APOSTOL_CURL_HPP
#define APOSTOL_CURL_HPP
//----------------------------------------------------------------------------------------------------------------------

#include <curl/curl.h>
//----------------------------------------------------------------------------------------------------------------------

#ifdef  __cplusplus
extern "C++" {
#endif // __cplusplus

namespace Apostol {

    namespace URL {

        //--------------------------------------------------------------------------------------------------------------

        //-- CCurlComponent --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CCurlComponent {
        public:
            CCurlComponent();
            ~CCurlComponent();
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CCurlApi --------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CCurlApi: public CCurlComponent, public CGlobalComponent {
        private:

            CURL *m_curl;

            void Init();
            void Cleanup();

        protected:

            static size_t CallBack(void *content, size_t size, size_t nmemb, CString *Buffer);

        public:

            CCurlApi();

            ~CCurlApi();

            void Send(const CString &url, const CString &Result);
            void Send(const CString &url, const CString &Result,
                      const CStringList &Headers, const CString &PostData, const CString &Action);

        };
    }
}

using namespace Apostol::URL;
#ifdef  __cplusplus
}
#endif // __cplusplus

#endif // APOSTOL_CURL_HPP
