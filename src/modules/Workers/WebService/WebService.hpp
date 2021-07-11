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

        struct CKeyContext {

            CString Name;
            CString Key;

            enum CKeyStatus {
                ksUnknown = -1,
                ksFetching,
                ksSuccess,
                ksError,
            } Status;

            CDateTime StatusTime;

            CKeyContext(): Status(ksUnknown), StatusTime(Now()) {
                Name = "DEFAULT";
            }

            CKeyContext(const CString& Name, const CString& Key): CKeyContext() {
                this->Name = Name;
                this->Key = Key;
            }

        };

        struct CServerContext {

            CString URI;
            CKeyContext PGP;

            CServerContext() = default;

            explicit CServerContext(const CString& URI) {
                this->URI = URI;
            }

        };

        typedef TPair<CServerContext> CServer;
        typedef TPairs<CServerContext> CServerList;

        //--------------------------------------------------------------------------------------------------------------

        //-- CWebService -----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CWebService: public CApostolModule {
        private:

            CProcessStatus m_Status;

            CServer m_DefaultServer;

            int m_SyncPeriod;
            int m_ServerIndex;
            int m_KeyIndex;

            CDateTime m_FixedDate;
            CDateTime m_RandomDate;

            CKeyContext m_PGP;

            CServerList m_Servers;

            CStringList m_BTCKeys;

            CProviders m_Providers;
            CStringListPairs m_Tokens;

            CHTTPProxyManager m_ProxyManager;

            CHTTPProxy *GetProxy(CHTTPServerConnection *AConnection);

            void InitMethods() override;

            static bool CheckAuthorizationData(CHTTPRequest *ARequest, CAuthorization &Authorization);
            void VerifyToken(const CString &Token);

            void FetchCerts(CProvider &Provider, const CString &Application);

            void FetchProviders();
            void CheckProviders();

            void RouteUser(CHTTPServerConnection *AConnection, const CString &Method, const CString &URI);
            void RouteDeal(CHTTPServerConnection *AConnection, const CString &Method, const CString &URI, const CString &Action);

            void RouteSignature(CHTTPServerConnection *AConnection);

            void FetchAccessToken(const CString &URI, const CString &Assertion,
                                  COnSocketExecuteEvent && OnDone, COnSocketExceptionEvent && OnFailed = nullptr);
            void CreateAccessToken(const CProvider &Provider, const CString &Application, CStringList &Tokens);

            static CString CreateToken(const CProvider& Provider, const CString &Application);

        protected:

            int NextServerIndex();

            const CServer &CurrentServer() const;

            void FetchPGP(CKeyContext& PGP);
            void FetchBTC(CKeyContext& BTC);

            static int CheckFee(const CString& Fee);

            static void CheckKeyForNull(LPCTSTR Key, LPCTSTR Value);

            void DoAPI(CHTTPServerConnection *AConnection);

            void DoGet(CHTTPServerConnection *AConnection) override;
            void DoPost(CHTTPServerConnection *AConnection);

            void DoVerbose(CSocketEvent *Sender, CTCPConnection *AConnection, LPCTSTR AFormat, va_list args);
            bool DoProxyExecute(CTCPConnection *AConnection);
            void DoProxyException(CTCPConnection *AConnection, const Delphi::Exception::Exception &E);
            void DoEventHandlerException(CPollEventHandler *AHandler, const Delphi::Exception::Exception &E);

            void DoProxyConnected(CObject *Sender);
            void DoProxyDisconnected(CObject *Sender);

        public:

            explicit CWebService(CModuleProcess *AProcess);

            ~CWebService() override = default;

            static class CWebService *CreateModule(CModuleProcess *AProcess) {
                return new CWebService(AProcess);
            }

            void Initialization(CModuleProcess *AProcess) override;

            bool CheckAuthorization(CHTTPServerConnection *AConnection, CAuthorization &Authorization);

            void Heartbeat() override;

            bool Enabled() override;

            void FetchKeys();

            void Reload();

            static void JsonStringToKey(const CString& jsonString, CString& Key);

            void ParsePGPKey(const CString& Key, CStringPairs& ServerList, CStringList& BTCKeys);

            static void CheckDeal(const CDeal& Deal);

            static bool CheckVerifyPGPSignature(int VerifyPGPSignature, CString &Message);
            static int VerifyPGPSignature(const CString &ClearText, const CString &Key, CString &Message);

            static bool FindURLInLine(const CString &Line, CStringList &List);
            static void ExceptionToJson(int ErrorCode, const std::exception &AException, CString& Json);

            static CDateTime GetRandomDate(int a, int b, CDateTime Date = Now());

            static void LoadOAuth2(const CString &FileName, const CString &ProviderName, const CString &ApplicationName,
                                   CProviders &Providers);
        };
    }
}

using namespace Apostol::Workers;
}
#endif //APOSTOL_WEBSERVICE_HPP
