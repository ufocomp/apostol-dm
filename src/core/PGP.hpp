/*++

Library name:

  apostol-core

Module Name:

  PGP.hpp

Notices:

  Apostol Core

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#include "OpenPGP.h"
using namespace OpenPGP;
//----------------------------------------------------------------------------------------------------------------------
#ifdef USE_LIB_GCRYPT
#include <gcrypt.h>
//----------------------------------------------------------------------------------------------------------------------
#endif

#ifndef APOSTOL_BITCOIN_PGP_HPP
#define APOSTOL_BITCOIN_PGP_HPP

extern "C++" {

namespace Apostol {

    namespace PGP {

#ifdef USE_LIB_GCRYPT
        class CPGP;

        extern CPGP *GPGP;
        //--------------------------------------------------------------------------------------------------------------

        typedef std::function<void (CPGP *Sender, LPCTSTR AFormat, va_list args)> COnPGPVerboseEvent;
        //--------------------------------------------------------------------------------------------------------------

        class EPGPError: public Delphi::Exception::Exception {
            typedef Delphi::Exception::Exception inherited;

        private:

            int m_LastError;

        public:

            EPGPError() : inherited(), m_LastError(GPG_ERR_NO_ERROR) {};

            EPGPError(int Error, LPCTSTR lpFormat) : inherited(lpFormat) {
                m_LastError = Error;
            };

            explicit EPGPError(LPCTSTR lpFormat, ...): m_LastError(GPG_ERR_NO_ERROR), inherited() {
                va_list argList;
                va_start(argList, lpFormat);
                FormatMessage(lpFormat, argList);
                va_end(argList);
            };

            ~EPGPError() override = default;

            int GetLastError() { return m_LastError; }
        };
        //--------------------------------------------------------------------------------------------------------------

        class CPGPComponent {
        public:

            CPGPComponent();

            ~CPGPComponent();

        }; // CPGPComponent

        //--------------------------------------------------------------------------------------------------------------

        //-- CPGP ------------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CPGP: public CObject {
        private:

            COnPGPVerboseEvent m_OnVerbose;

        protected:

            gcry_error_t m_LastError;

            virtual void DoVerbose(LPCTSTR AFormat, va_list args);

        public:

            CPGP();

            ~CPGP() override = default;

            inline static class CPGP *CreatePGP() { return GPGP = new CPGP(); };

            inline static void DeletePGP() { delete GPGP; };

            static void Initialize();

            bool CheckForCipherError(gcry_error_t err);

            bool CheckForCipherError(gcry_error_t err, int const ignore[], int count);

            void RaiseCipherError(gcry_error_t err);

            void Verbose(LPCSTR AFormat, ...);
            void Verbose(LPCSTR AFormat, va_list args);

            gcry_error_t LastError() const { return m_LastError; };

            const COnPGPVerboseEvent &OnVerbose() const { return m_OnVerbose; }
            void OnVerbose(COnPGPVerboseEvent && Value) { m_OnVerbose = Value; }

        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CPGPCipher ------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CPGPCipher: public CPGPComponent {
        private:

            gcry_cipher_hd_t m_Handle;

        public:

            CPGPCipher();

            gcry_cipher_hd_t Handle() { return m_Handle; };
            gcry_cipher_hd_t *Ptr() { return &m_Handle; };

            bool Open(int algo, int mode, unsigned int flags);

            void Close();

            bool SetKey(const void *key, size_t keylen);
            bool SetIV(const void *iv, size_t ivlen);
            bool SetCTR(const void *ctr, size_t ctrlen);

            bool Reset();

            bool Authenticate(const void *abuf, size_t abuflen);

            bool GetTag(void *tag, size_t taglen);
            bool CheckTag(gcry_cipher_hd_t h, const void *tag, size_t taglen);

            bool Encrypt(unsigned char *out, size_t outsize, const unsigned char *in, size_t inlen);
            bool Decrypt(unsigned char *out, size_t outsize, const unsigned char *in, size_t inlen);

            bool Final();
            bool Sync();

            bool Ctl(int cmd, void *buffer, size_t buflen);
            bool Info(gcry_cipher_hd_t h, int what, void *buffer, size_t *nbytes);

        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CPGPAlgo --------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CPGPAlgo: public CPGPComponent {
        public:

            CPGPAlgo();

            bool Info(int algo, int what, void *buffer, size_t *nbytes);

            size_t KeyLen(int algo);

            size_t BlkLen(int algo);

            const char *Name(int algo);

            int MapName(const char *name);

            int ModeFromOid(const char *string);

        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CPGPMPI ---------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CPGPMPI: public CPGPComponent {
        protected:

            gcry_mpi_t m_Handle;

        public:

            CPGPMPI();

            CPGPMPI(const CPGPMPI &Value) {
                m_Handle = Value.m_Handle;
            };

            explicit CPGPMPI(gcry_mpi_t AHandle) {
                m_Handle = AHandle;
            };

            ~CPGPMPI() = default;

            gcry_mpi_t Handle() { return m_Handle; };

            void New(unsigned int nbits);

            void SNew(unsigned int nbits);

            void Copy(const gcry_mpi_t a);

            void Release();

            gcry_mpi_t Set(const gcry_mpi_t u);

            gcry_mpi_t SetUI(unsigned long u);

            void Swap(gcry_mpi_t b);

            void Snatch(const gcry_mpi_t u);

            void Neg(gcry_mpi_t u);

            void Abs();

            int Cmp(const gcry_mpi_t v) const;
            int CmpUI(unsigned long v) const;
            int IsNeg() const;

            bool Scan (enum gcry_mpi_format format, const unsigned char *buffer, size_t buflen, size_t *nscanned);

            bool Print (enum gcry_mpi_format format, unsigned char *buffer, size_t buflen, size_t *nwritten) const;

            bool APrint(enum gcry_mpi_format format, unsigned char **buffer, size_t *nbytes) const;

            void Dump();


        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CPGPSexp --------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CPGPSexp: public CPGPComponent {
        protected:

            gcry_sexp_t m_Handle;

        public:

            CPGPSexp();

            CPGPSexp(const CPGPSexp& Value) {
                m_Handle = Value.m_Handle;
            };

            explicit CPGPSexp(gcry_sexp_t AHandle) {
                m_Handle = AHandle;
            };

            ~CPGPSexp();

            gcry_sexp_t Handle() { return m_Handle; };

            bool New(const void *buffer, size_t length, int autodetect);

            bool Create(void *buffer, size_t length, int autodetect, void (*freefnc)(void*));

            bool Sscan(size_t *erroff, const char *buffer, size_t length);

            bool Build(size_t *erroff, const char *format, ...);

            size_t Sprint(int mode, char *buffer, size_t maxlength);

            void Release();

            void Dump();

            static size_t CanonLen(const unsigned char *buffer, size_t length, size_t *erroff, gcry_error_t *errcode);

            gcry_sexp_t FindToken(const char *token, size_t toklen);

            int Length();

            gcry_sexp_t Nth(int number);

            gcry_sexp_t Car();

            gcry_sexp_t Cdr();

            const char *NthData(int number, size_t *datalen);

            void *NthBuffer(int number, size_t *rlength);

            char *NthString(int number);

            gcry_mpi_t NthMPI(int number, int mpifmt);

        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CPGPKey ---------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CPGPKey: public CPGPSexp {
        public:

            CPGPKey(): CPGPSexp() {

            };

            CPGPKey(const CPGPKey& Value) = default;

            explicit CPGPKey(gcry_sexp_t AHandle): CPGPSexp(AHandle) {

            };

            bool TestKey ();
            static bool AlgoInfo (int algo, int what, void *buffer, size_t *nbytes);
            static bool Ctl (int cmd, void *buffer, size_t buflen);
            bool GenKey (gcry_sexp_t parms);
            bool PubKeyGetSexp(int mode, gcry_ctx_t ctx);

            unsigned int GetNBits ();
            unsigned char * KeyGrip (unsigned char *array);

            bool Encrypt(gcry_sexp_t data, gcry_sexp_t pkey);
            bool Decrypt(gcry_sexp_t data, gcry_sexp_t skey);
            bool Sign(gcry_sexp_t data, gcry_sexp_t skey);
            bool Verify(gcry_sexp_t data, gcry_sexp_t pkey);

            static const char * AlgoName(int algo);
            static int MapName (const char *name);
            static int TestAlgo (int algo);

        };

        //--------------------------------------------------------------------------------------------------------------

        void check_rsa_keys(COnPGPVerboseEvent && Verbose = nullptr);
        void load_key(const char *KeyFile, const char* Passwd, COnPGPVerboseEvent &&Verbose = nullptr);
#endif

    }
}

using namespace Apostol::PGP;
}

#endif //APOSTOL_BITCOIN_PGP_HPP
