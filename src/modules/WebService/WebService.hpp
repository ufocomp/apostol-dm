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

#define BPS_SERVER_PORT 4977
#define BPS_SERVER_URL "http://185.141.62.25:4977"
#define BPS_BM_SERVER_ADDRESS "BM-2cX8y9u9yEi3fdqQfndx9F6NdR5Hv79add"
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Module {

        enum CAuthorizationSchemes { asUnknown, asBasic };

        typedef struct CAuthorization {

            CAuthorizationSchemes Schema;

            CString Username;
            CString Password;

            CAuthorization(): Schema(asUnknown) {

            }

            explicit CAuthorization(const CString& String): CAuthorization() {
                Parse(String);
            }

            void Parse(const CString& String) {
                if (String.SubString(0, 5).Lower() == "basic") {
                    const CString LPassphrase(base64_decode(String.SubString(6)));

                    const size_t LPos = LPassphrase.Find(':');
                    if (LPos == CString::npos)
                        throw Delphi::Exception::Exception("Authorization error: Incorrect passphrase.");

                    Schema = asBasic;
                    Username = LPassphrase.SubString(0, LPos);
                    Password = LPassphrase.SubString(LPos + 1);

                    if (Username.IsEmpty() || Password.IsEmpty())
                        throw Delphi::Exception::Exception("Authorization error: Username and password has not be empty.");
                } else {
                    throw Delphi::Exception::Exception("Authorization error: Unknown schema.");
                }
            }

            CAuthorization &operator << (const CString& String) {
                Parse(String);
                return *this;
            }

        } CAuthorization;

        //--------------------------------------------------------------------------------------------------------------

        //-- CWebService -----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CWebService: public CApostolModule {
        private:

            int m_Version;

#ifdef WITH_CURL
            CCurlApi m_Curl;
#endif
            CString m_LocalHost;

            int m_ServerIndex;

            CDateTime m_FixedDate;
            CDateTime m_RandomDate;

            CStringPairs m_ServerList;

            CString m_PGP;

            CHTTPProxyManager *m_ProxyManager;

            CHTTPProxy *GetProxy(CHTTPServerConnection *AConnection);

            static void DebugRequest(CRequest *ARequest);
            static void DebugReply(CReply *AReply);
            static void DebugConnection(CHTTPServerConnection *AConnection);

            void RouteUser(CHTTPServerConnection *AConnection, const CString &Method, const CString &Uri);
            void RouteDeal(CHTTPServerConnection *AConnection, const CString &Method, const CString &Uri, const CString &Action);

            void RouteSignature(CHTTPServerConnection *AConnection);

        protected:

            int NextServerIndex();

            const CString &CurrentServer() const;
            bool ServerPing(const CString &URL);
            void NextServer();

            static void LoadFromOpenPGP(CString &Key);
            void LoadFromBPS(CString &Key);

            void ParsePGPKey(const CString& Key);

            static int CheckFee(const CString& Fee);

            static void CheckKeyForNull(LPCTSTR Key, LPCTSTR Value);

            void DoOptions(CHTTPServerConnection *AConnection) override;

            void DoGet(CHTTPServerConnection *AConnection);
            void DoPost(CHTTPServerConnection *AConnection);

            static void DoWWW(CHTTPServerConnection *AConnection);

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

            static bool CheckVerifyPGPSignature(int VerifyPGPSignature, CString &Message);
            static int VerifyPGPSignature(const CString &ClearText, const CString &Key, CString &Message);

            static bool FindURLInLine(const CString &Line, CStringList &List);
            static void ExceptionToJson(int ErrorCode, const std::exception &AException, CString& Json);

            static CDateTime GetRandomDate(int a, int b, CDateTime Date = Now());

        };

    }
}

using namespace Apostol::Module;
}
#endif //APOSTOL_WEBSERVICE_HPP
