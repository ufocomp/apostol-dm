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

            CServerContext() {

            };

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

            CServer m_DefaultServer;

            int m_SyncPeriod;
            int m_ServerIndex;
            int m_KeyIndex;

            CDateTime m_RandomDate;

            CKeyContext m_PGP;

            CServerList m_Servers;

            CStringList m_BTCKeys;

            CHTTPProxyManager *m_pProxyManager;

            CHTTPProxy *GetProxy(CHTTPServerConnection *AConnection);

            void InitMethods() override;

            static bool CheckAuthorization(CHTTPServerConnection *AConnection, CAuthorization &Authorization);

            void RouteUser(CHTTPServerConnection *AConnection, const CString &Method, const CString &URI);
            void RouteDeal(CHTTPServerConnection *AConnection, const CString &Method, const CString &URI, const CString &Action);

            void RouteSignature(CHTTPServerConnection *AConnection);

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

            bool Enabled() override;

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
