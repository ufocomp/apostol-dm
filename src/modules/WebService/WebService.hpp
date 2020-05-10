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

    namespace Module {

        //--------------------------------------------------------------------------------------------------------------

        //-- CWebService -----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CWebService: public CApostolModule {
        private:

            int m_Version;
            unsigned int m_SyncPeriod;

            CStringPairs m_Roots;
#ifdef WITH_CURL
            CCurlApi m_Curl;
#endif
            CString m_LocalHost;

            int m_ServerIndex;

            CDateTime m_FixedDate;
            CDateTime m_RandomDate;

            CStringPairs m_ServerList;

            CString m_PGP;
            CStringList m_BTCKeys;

            CHTTPProxyManager *m_ProxyManager;

            void InitRoots(const CSites &Sites);
            const CString& GetRoot(const CString &Host) const;

            CHTTPProxy *GetProxy(CHTTPServerConnection *AConnection);

            static bool CheckAuthorization(CHTTPServerConnection *AConnection, CAuthorization &Authorization);

            void RouteUser(CHTTPServerConnection *AConnection, const CString &Method, const CString &URI);
            void RouteDeal(CHTTPServerConnection *AConnection, const CString &Method, const CString &URI, const CString &Action);

            void RouteSignature(CHTTPServerConnection *AConnection);

        protected:

            int NextServerIndex();

            const CString &CurrentServer() const;
            bool ServerPing(const CString &URL);
            void NextServer();

            void LoadFromOpenPGP();
            void LoadFromBPS();

            static int CheckFee(const CString& Fee);

            static void CheckKeyForNull(LPCTSTR Key, LPCTSTR Value);

            void DoAPI(CHTTPServerConnection *AConnection);

            void DoOptions(CHTTPServerConnection *AConnection) override;

            void DoGet(CHTTPServerConnection *AConnection);
            void DoPost(CHTTPServerConnection *AConnection);

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

            void Heartbeat() override;
            void Execute(CHTTPServerConnection *AConnection) override;

            bool CheckUserAgent(const CString &Value) override;

            void LoadPGPKey();

            static void JsonStringToPGP(const CString& jsonString, CString& Key);
            void ParsePGPKey(const CString& Key, CStringPairs& ServerList, CStringList& BTCKeys);

            static void CheckDeal(const CDeal& Deal);

            static bool CheckVerifyPGPSignature(int VerifyPGPSignature, CString &Message);
            static int VerifyPGPSignature(const CString &ClearText, const CString &Key, CString &Message);

            static bool FindURLInLine(const CString &Line, CStringList &List);
            static void ExceptionToJson(int ErrorCode, const std::exception &AException, CString& Json);

            static CDateTime GetRandomDate(int a, int b, CDateTime Date = Now());

            static void Redirect(CHTTPServerConnection *AConnection, const CString& Location, bool SendNow = false);

            void SendResource(CHTTPServerConnection *AConnection, const CString &Path, LPCTSTR AContentType = nullptr, bool SendNow = false);

        };
    }
}

using namespace Apostol::Module;
}
#endif //APOSTOL_WEBSERVICE_HPP
