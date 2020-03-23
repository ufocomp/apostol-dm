/*++

Library name:

  apostol-core

Module Name:

  CURL.cpp

Notices:

  Apostol Core (cURL)

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#include "Core.hpp"
#include "CURL.hpp"
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace URL {

        CCurlApi::CCurlApi() {
            m_curl = nullptr;
            curl_global_init(CURL_GLOBAL_DEFAULT);
            Init();
        }
        //--------------------------------------------------------------------------------------------------------------

        CCurlApi::~CCurlApi() {
            Cleanup();
            curl_global_cleanup();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCurlApi::Init() {
            m_curl = curl_easy_init();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCurlApi::Cleanup() {
            if (m_curl != nullptr)
                curl_easy_cleanup(m_curl);
        }
        //--------------------------------------------------------------------------------------------------------------

        size_t CCurlApi::CallBack(void *content, size_t size, size_t nmemb, CString *Buffer) {
            Buffer->Append((char *) content, size * nmemb);
            return Buffer->Size();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCurlApi::Send(const CString &url, const CString &Result) {
            Send(url, Result, CStringList(), CString(), "GET");
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCurlApi::Send(const CString &url, const CString &Result, const CStringList &Headers,
                const CString &PostData, const CString &Action) {

            CURLcode Code;

            if ( m_curl ) {
                curl_easy_reset(m_curl);

                curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
                curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, CCurlApi::CallBack);
                curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &Result);
                curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, false);
                curl_easy_setopt(m_curl, CURLOPT_ENCODING, "gzip");

                if ( Headers.Count() > 0 ) {

                    struct curl_slist *chunk = nullptr;
                    for ( int i = 0; i < Headers.Count(); i++ ) {
                        chunk = curl_slist_append(chunk, Headers[i].c_str());
                    }

                    curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, chunk);
                }

                if ( Action == "GET" ) {

                    curl_easy_setopt(m_curl, CURLOPT_HTTPGET, TRUE);

                } else if ( Action == "POST" || Action == "PUT" || Action == "DELETE" ) {

                    if ( Action == "PUT" || Action == "DELETE" ) {
                        curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, Action.c_str() );
                    } else {
                        curl_easy_setopt(m_curl, CURLOPT_HTTPPOST, TRUE);

                        if (!PostData.IsEmpty()) {
                            curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, PostData.c_str());
                        }
                    }
                }

                clock_t start = clock();

                Code = curl_easy_perform(m_curl);

                Log()->Debug(0, "[cURL] Send runtime: %.2f ms.", (double) ((clock() - start) / (double) CLOCKS_PER_SEC * 1000));

                /* Check for errors */
                if ( Code != CURLE_OK ) {
                    Log()->Error(APP_LOG_EMERG, 0, "[cURL] curl_easy_perform() failed: %s" , curl_easy_strerror(Code) );
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

    }
}
}
