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
#define BPS_DEFAULT_SYNC_PERIOD 30
#define BPS_SERVER_PORT 4977
#define BPS_SERVER_URL "http://185.141.62.25:4977"
#define BPS_BM_SERVER_ADDRESS "BM-2cX8y9u9yEi3fdqQfndx9F6NdR5Hv79add"
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Workers {

        //--------------------------------------------------------------------------------------------------------------

        //-- CWebService -----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CWebService: public CApostolModule {
        private:

            enum CKeyStatus {
                ksUnknown = -1,
                ksPGPFetching,
                ksPGPSuccess,
                ksPGPError,
                ksBTCFetching,
                ksBTCSuccess,
                ksBTCError
            } m_KeyStatus;

            int m_SyncPeriod;

            CString m_LocalHost;

            int m_ServerIndex;
            int m_KeyIndex;

            CDateTime m_RandomDate;

            CStringPairs m_ServerList;

            CString m_PGP;
            CStringList m_BTCKeys;

            CHTTPProxyManager *m_ProxyManager;

            CHTTPProxy *GetProxy(CHTTPServerConnection *AConnection);

            void InitMethods() override;

            static bool CheckAuthorization(CHTTPServerConnection *AConnection, CAuthorization &Authorization);

            void RouteUser(CHTTPServerConnection *AConnection, const CString &Method, const CString &URI);
            void RouteDeal(CHTTPServerConnection *AConnection, const CString &Method, const CString &URI, const CString &Action);

            void RouteSignature(CHTTPServerConnection *AConnection);

        protected:

            int NextServerIndex();

            const CString &CurrentServer() const;

            void FetchPGP();
            void FetchBTC();

            static int CheckFee(const CString& Fee);

            static void CheckKeyForNull(LPCTSTR Key, LPCTSTR Value);

            void DoAPI(CHTTPServerConnection *AConnection);

            void DoGet(CHTTPServerConnection *AConnection) override;
            void DoPost(CHTTPServerConnection *AConnection);

            void DoVerbose(CSocketEvent *Sender, CTCPConnection *AConnection, LPCTSTR AFormat, va_list args);
            bool DoProxyExecute(CTCPConnection *AConnection);
            void DoProxyException(CTCPConnection *AConnection, Delphi::Exception::Exception *AException);
            void DoEventHandlerException(CPollEventHandler *AHandler, Delphi::Exception::Exception *AException);

            void DoProxyConnected(CObject *Sender);
            void DoProxyDisconnected(CObject *Sender);

        public:

            explicit CWebService(CModuleProcess *AProcess);

            ~CWebService() override;

            static class CWebService *CreateModule(CModuleProcess *AProcess) {
                return new CWebService(AProcess);
            }

            void Heartbeat() override;

            bool IsEnabled() override;
            bool CheckUserAgent(const CString& Value) override;

            void FetchKeys();

            static void JsonStringToKey(const CString& jsonString, CString& Key);

            void ParsePGPKey(const CString& Key, CStringPairs& ServerList, CStringList& BTCKeys);

            static void CheckDeal(const CDeal& Deal);

            static bool CheckVerifyPGPSignature(int VerifyPGPSignature, CString &Message);
            static int VerifyPGPSignature(const CString &ClearText, const CString &Key, CString &Message);

            static bool FindURLInLine(const CString &Line, CStringList &List);
            static void ExceptionToJson(int ErrorCode, const std::exception &AException, CString& Json);

            static CDateTime GetRandomDate(int a, int b, CDateTime Date = Now());

        };
    }
}

using namespace Apostol::Workers;
}
#endif //APOSTOL_WEBSERVICE_HPP
