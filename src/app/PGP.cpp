/*++

Library name:

  apostol-core

Module Name:

  PGP.cpp

Notices:

  Apostol Core

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#include "Core.hpp"
#include "PGP.hpp"
//----------------------------------------------------------------------------------------------------------------------

#include <iomanip>
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace PGP {

        const std::map <uint8_t, std::string> Public_Key_Type = {
                std::make_pair(Packet::SECRET_KEY,    "sec"),
                std::make_pair(Packet::PUBLIC_KEY,    "pub"),
                std::make_pair(Packet::SECRET_SUBKEY, "ssb"),
                std::make_pair(Packet::PUBLIC_SUBKEY, "sub"),
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- Key::Key --------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        Key::Key(): OpenPGP::Key() {

        }
        //--------------------------------------------------------------------------------------------------------------

        Key::Key(const OpenPGP::PGP &copy): OpenPGP::Key(copy) {

        }
        //--------------------------------------------------------------------------------------------------------------

        Key::Key(const Key &copy): OpenPGP::Key(copy) {

        }
        //--------------------------------------------------------------------------------------------------------------

        Key::Key(const std::string &data): OpenPGP::Key(data) {

        }
        //--------------------------------------------------------------------------------------------------------------

        Key::Key(std::istream &stream): OpenPGP::Key(stream) {

        }
        //--------------------------------------------------------------------------------------------------------------

        std::string Key::ListKeys(std::size_t indents, std::size_t indent_size) const {
            if (!meaningful()) {
                return "Key data not meaningful.";
            }

            const std::string indent(indents * indent_size, ' ');

            // print Key and User packets
            std::stringstream out;
            for (Packet::Tag::Ptr const & p : packets) {
                // primary key/subkey
                if (Packet::is_key_packet(p -> get_tag())) {
                    const Packet::Key::Ptr key = std::static_pointer_cast <Packet::Key> (p);

                    if (Packet::is_subkey(p -> get_tag())) {
                        out << "\n";
                    }

                    out << indent << Apostol::PGP::Public_Key_Type.at(p -> get_tag()) << "  " << std::setfill(' ') << std::setw(4) << std::to_string(bitsize(key -> get_mpi()[0]))
                        << indent << PKA::SHORT.at(key -> get_pka()) << "/"
                        << indent << hexlify(key -> get_keyid()) << " "
                        << indent << show_date(key -> get_time());
                }
                    // User ID
                else if (p -> get_tag() == Packet::USER_ID) {
                    out << "\n"
                        << indent << "uid " << std::static_pointer_cast <Packet::Tag13> (p) -> get_contents();
                }
                    // User Attribute
                else if (p -> get_tag() == Packet::USER_ATTRIBUTE) {
                    for(Subpacket::Tag17::Sub::Ptr const & s : std::static_pointer_cast <Packet::Tag17> (p) -> get_attributes()) {
                        // since only subpacket type 1 is defined
                        out << "\n"
                            << indent << "att  att  [jpeg image of size " << std::static_pointer_cast <Subpacket::Tag17::Sub1> (s) -> get_image().size() << "]";
                    }
                }
                    // Signature
                else if (p -> get_tag() == Packet::SIGNATURE) {
                    out << indent << "sig ";

                    const Packet::Tag2::Ptr sig = std::static_pointer_cast <Packet::Tag2> (p);
                    if (Signature_Type::is_revocation(sig -> get_type())) {
                        out << "revok";
                    }
                    else if (sig -> get_type() == Signature_Type::SUBKEY_BINDING_SIGNATURE) {
                        out << "sbind";
                    }
                    else{
                        out << " sig ";
                    }

                    const std::array <uint32_t, 3> times = sig -> get_times();  // {signature creation time, signature expiration time, key expiration time}
                    out << "  " << hexlify(sig -> get_keyid());

                    // signature creation time (should always exist)
                    if (times[0]) {
                        out << " " << show_date(times[0]);
                    }
                    // else{
                    // out << " " << std::setfill(' ') << std::setw(10);
                    // }

                    // if the signature expires
                    if (times[1]) {
                        out << " " << show_date(times[1]);
                    }
                    else{
                        out << " " << std::setfill(' ') << std::setw(10);
                    }

                    // if the key expires
                    if (times[2]) {
                        out << " " << show_date(times[2]);
                    }
                    else{
                        out << " " << std::setfill(' ') << std::setw(10);
                    }
                }
                else{}

                out << "\n";
            }

            return out.str();
        }
        //--------------------------------------------------------------------------------------------------------------

        void Key::ExportUID(CPGPUserIdList &List) const {
            if (!meaningful()) {
                return;
            }

            for (Packet::Tag::Ptr const & p : packets) {
                if (p->get_tag() == Packet::USER_ID) {
                    const auto& uid = std::static_pointer_cast <Packet::Tag13> (p)->get_contents();

                    List.Add(CPGPUserId());
                    auto& UserId = List.Last();

                    int Index = 0;
                    bool Append;
                    for (char ch : uid) {
                        Append = false;
                        switch (ch) {
                            case '<':
                                Index = 1;
                                break;
                            case '>':
                                Index = 0;
                                break;
                            case '(':
                                Index = 2;
                                break;
                            case ')':
                                Index = 0;
                                break;
                            default:
                                Append = true;
                                break;
                        }

                        if (Append) {
                            if (Index == 0) {
                                UserId.Name.Append(ch);
                            } else if (Index == 1) {
                                UserId.Mail.Append(ch);
                            } else {
                                UserId.Desc.Append(ch);
                            }
                        }
                    }

                    UserId.Name = UserId.Name.TrimRight();
                }
            }
        }

        //--------------------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        bool CleartextSignature(const CString &Key, const CString &Pass, const CString &Hash, const CString &ClearText,
                                CString &SignText) {

            const std::string key(Key);
            const std::string pass(Pass);
            const std::string text(ClearText);
            const std::string hash(Hash.IsEmpty() ? "SHA256" : Hash);

            if (OpenPGP::Hash::NUMBER.find(hash) == OpenPGP::Hash::NUMBER.end()) {
                throw ExceptionFrm("PGP: Bad Hash Algorithm: %s.", hash.c_str());
            }

            const OpenPGP::Sign::Args SignArgs(OpenPGP::SecretKey(key), pass,4, OpenPGP::Hash::NUMBER.at(hash));
            const OpenPGP::CleartextSignature signature = OpenPGP::Sign::cleartext_signature(SignArgs, text);

            if (!signature.meaningful()) {
                throw Delphi::Exception::Exception("PGP: Generated bad cleartext signature.");
            }

            SignText << signature.write();

            return true;
        }
        //--------------------------------------------------------------------------------------------------------------

#ifdef USE_LIB_GCRYPT
        static int GInstanceCount = 0;

        pthread_mutex_t GPGPCriticalSection;
        CPGP *GPGP = nullptr;

        //--------------------------------------------------------------------------------------------------------------

        //-- CPGPComponent ---------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        inline void AddPGP() {

            if (GInstanceCount == 0)
                pthread_mutex_init(&GPGPCriticalSection, nullptr);

            pthread_mutex_lock(&GPGPCriticalSection);

            try {
                if (GInstanceCount == 0) {
                    GPGP = CPGP::CreatePGP();
                }

                GInstanceCount++;
            } catch (...) {
            }

            pthread_mutex_unlock(&GPGPCriticalSection);
        };
        //--------------------------------------------------------------------------------------------------------------

        inline void RemovePGP() {

            pthread_mutex_lock(&GPGPCriticalSection);

            try {
                GInstanceCount--;

                if (GInstanceCount == 0)
                {
                    CPGP::DeletePGP();
                    GPGP = nullptr;
                }
            } catch (...) {
            }

            pthread_mutex_unlock(&GPGPCriticalSection);

            if (GInstanceCount == 0)
                pthread_mutex_destroy(&GPGPCriticalSection);
        };
        //--------------------------------------------------------------------------------------------------------------

        CPGPComponent::CPGPComponent() {
            AddPGP();
        };
        //--------------------------------------------------------------------------------------------------------------

        CPGPComponent::~CPGPComponent() {
            RemovePGP();
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CPGP ------------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CPGP::CPGP(): CObject() {
            m_LastError = GPG_ERR_NO_ERROR;
            m_OnVerbose = nullptr;

            if (!gcry_control(GCRYCTL_INITIALIZATION_FINISHED_P))
                Initialize();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPGP::Initialize() {
            /* Version check should be the very first call because it
               makes sure that important subsystems are initialized. */
            if (!gcry_check_version(GCRYPT_VERSION))
            {
                throw EPGPError("Error: libgcrypt version mismatch");
            }

            /* We don't want to see any warnings, e.g. because we have not yet
               parsed program options which might be used to suppress such
               warnings. */
            //gcry_control(GCRYCTL_SUSPEND_SECMEM_WARN);

            /* Allocate a pool of 16k secure memory.  This makes the secure memory
               available and also drops privileges where needed.  Note that by
               using functions like gcry_xmalloc_secure and gcry_mpi_snew Libgcrypt
               may expand the secure memory pool with memory which lacks the
               property of not being swapped out to disk.   */
            //gcry_control (GCRYCTL_INIT_SECMEM, 16384, 0);

            /* It is now okay to let Libgcrypt complain when there was/is
               a problem with the secure memory. */
            //gcry_control(GCRYCTL_RESUME_SECMEM_WARN);

            /* Disable secure memory.  */
            gcry_control(GCRYCTL_DISABLE_SECMEM, 0);

            /* No valuable keys are create, so we can speed up our RNG. */
            gcry_control(GCRYCTL_ENABLE_QUICK_RANDOM, 0);

            /* Tell Libgcrypt that initialization has completed. */
            gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPGP::CheckForCipherError(gcry_error_t error) {
            int ignore[] = {0};
            return CheckForCipherError(error, ignore, 0);
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPGP::CheckForCipherError(gcry_error_t err, const int *ignore, int count) {
            if (err != GPG_ERR_NO_ERROR) {
                m_LastError = err;
                for (int i = 0; i < count; ++i)
                    if (m_LastError == ignore[i])
                        return true;
                RaiseCipherError(m_LastError);
            }
            return false;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPGP::RaiseCipherError(gcry_error_t err) {
            throw EPGPError("Error: %s/%s", gcry_strsource(err), gcry_strerror(err));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPGP::Verbose(LPCSTR AFormat, ...) {
            va_list args;
            va_start(args, AFormat);
            DoVerbose(AFormat, args);
            va_end(args);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPGP::Verbose(LPCSTR AFormat, va_list args) {
            DoVerbose(AFormat, args);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPGP::DoVerbose(LPCTSTR AFormat, va_list args) {
            if (m_OnVerbose != nullptr) {
                m_OnVerbose(this, AFormat, args);
            }
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CPGPCipher ------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CPGPCipher::CPGPCipher(): CPGPComponent() {
            m_Handle = nullptr;
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPGPCipher::Open(int algo, int mode, unsigned int flags) {
            return !GPGP->CheckForCipherError(gcry_cipher_open(&m_Handle, algo, mode, flags));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPGPCipher::Close() {
            if (m_Handle != nullptr) {
                gcry_cipher_close(m_Handle);
            }
            m_Handle = nullptr;
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPGPCipher::SetKey(const void *key, size_t keylen) {
            return !GPGP->CheckForCipherError(gcry_cipher_setkey(m_Handle, key, keylen));
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPGPCipher::SetIV(const void *iv, size_t ivlen) {
            return !GPGP->CheckForCipherError(gcry_cipher_setiv(m_Handle, iv, ivlen));
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPGPCipher::SetCTR(const void *ctr, size_t ctrlen) {
            return !GPGP->CheckForCipherError(gcry_cipher_setctr(m_Handle, ctr, ctrlen));
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPGPCipher::Reset() {
            return !GPGP->CheckForCipherError(gcry_cipher_reset(m_Handle));
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPGPCipher::Authenticate(const void *abuf, size_t abuflen) {
            return !GPGP->CheckForCipherError(gcry_cipher_authenticate(m_Handle, abuf, abuflen));
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPGPCipher::GetTag(void *tag, size_t taglen) {
            return !GPGP->CheckForCipherError(gcry_cipher_gettag(m_Handle, tag, taglen));
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPGPCipher::CheckTag(gcry_cipher_hd_t h, const void *tag, size_t taglen) {
            return !GPGP->CheckForCipherError(gcry_cipher_checktag(m_Handle, tag, taglen));
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPGPCipher::Encrypt(unsigned char *out, size_t outsize, const unsigned char *in, size_t inlen) {
            return !GPGP->CheckForCipherError(gcry_cipher_encrypt(m_Handle, out, outsize, in, inlen));
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPGPCipher::Decrypt(unsigned char *out, size_t outsize, const unsigned char *in, size_t inlen) {
            return !GPGP->CheckForCipherError(gcry_cipher_decrypt(m_Handle, out, outsize, in, inlen));
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPGPCipher::Final() {
            return !GPGP->CheckForCipherError(gcry_cipher_final(m_Handle));
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPGPCipher::Sync() {
            return !GPGP->CheckForCipherError(gcry_cipher_sync(m_Handle));
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPGPCipher::Ctl(int cmd, void *buffer, size_t buflen) {
            return !GPGP->CheckForCipherError(gcry_cipher_ctl(m_Handle, cmd, buffer, buflen));
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPGPCipher::Info(gcry_cipher_hd_t h, int what, void *buffer, size_t *nbytes) {
            return !GPGP->CheckForCipherError(gcry_cipher_info(m_Handle, what, buffer, nbytes));
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CPGPAlgo --------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CPGPAlgo::CPGPAlgo(): CPGPComponent() {

        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPGPAlgo::Info(int algo, int what, void *buffer, size_t *nbytes) {
            return !GPGP->CheckForCipherError(gcry_cipher_algo_info(algo, what, buffer, nbytes));
        }
        //--------------------------------------------------------------------------------------------------------------

        size_t CPGPAlgo::KeyLen(int algo) {
            return !GPGP->CheckForCipherError(gcry_cipher_get_algo_keylen(algo));
        }
        //--------------------------------------------------------------------------------------------------------------

        size_t CPGPAlgo::BlkLen(int algo) {
            return !GPGP->CheckForCipherError(gcry_cipher_get_algo_blklen(algo));
        }
        //--------------------------------------------------------------------------------------------------------------

        const char *CPGPAlgo::Name(int algo) {
            return gcry_cipher_algo_name(algo);
        }
        //--------------------------------------------------------------------------------------------------------------

        int CPGPAlgo::MapName(const char *name) {
            return gcry_cipher_map_name(name);
        }
        //--------------------------------------------------------------------------------------------------------------

        int CPGPAlgo::ModeFromOid(const char *string) {
            return gcry_cipher_mode_from_oid(string);
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CPGPMPI ----------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CPGPMPI::CPGPMPI() {
            m_Handle = nullptr;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPGPMPI::New(unsigned int nbits) {
            m_Handle = gcry_mpi_new(nbits);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPGPMPI::SNew(unsigned int nbits) {
            m_Handle = gcry_mpi_snew(nbits);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPGPMPI::Copy(const gcry_mpi_t a) {
            m_Handle = gcry_mpi_copy(a);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPGPMPI::Release() {
            gcry_mpi_release(m_Handle);
        }
        //--------------------------------------------------------------------------------------------------------------

        gcry_mpi_t CPGPMPI::Set(const gcry_mpi_t u) {
            return gcry_mpi_set(m_Handle, u);
        }
        //--------------------------------------------------------------------------------------------------------------

        gcry_mpi_t CPGPMPI::SetUI(unsigned long u) {
            return gcry_mpi_set_ui(m_Handle, u);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPGPMPI::Swap(gcry_mpi_t b) {
            gcry_mpi_swap(m_Handle, b);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPGPMPI::Snatch(const gcry_mpi_t u) {
            gcry_mpi_snatch(m_Handle, u);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPGPMPI::Neg(gcry_mpi_t u) {
            gcry_mpi_neg(m_Handle, u);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPGPMPI::Abs() {
             gcry_mpi_abs(m_Handle);
        }
        //--------------------------------------------------------------------------------------------------------------

        int CPGPMPI::Cmp(const gcry_mpi_t v) const {
            return gcry_mpi_cmp(m_Handle, v);
        }
        //--------------------------------------------------------------------------------------------------------------

        int CPGPMPI::CmpUI(unsigned long v) const {
            return gcry_mpi_cmp_ui(m_Handle, v);
        }
        //--------------------------------------------------------------------------------------------------------------

        int CPGPMPI::IsNeg() const {
            return gcry_mpi_is_neg(m_Handle);
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPGPMPI::Scan(enum gcry_mpi_format format, const unsigned char *buffer, size_t buflen, size_t *nscanned) {
            return !GPGP->CheckForCipherError(gcry_mpi_scan(&m_Handle, format, buffer, buflen, nscanned));
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPGPMPI::Print(enum gcry_mpi_format format, unsigned char *buffer, size_t buflen, size_t *nwritten) const {
            return !GPGP->CheckForCipherError(gcry_mpi_print(format, buffer, buflen, nwritten, m_Handle));
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPGPMPI::APrint(enum gcry_mpi_format format, unsigned char **buffer, size_t *nbytes) const {
            return !GPGP->CheckForCipherError(gcry_mpi_aprint(format, buffer, nbytes, m_Handle));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPGPMPI::Dump() {
            gcry_mpi_dump(m_Handle);
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CPGPSexp --------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CPGPSexp::CPGPSexp() {
            m_Handle = nullptr;
        }
        //--------------------------------------------------------------------------------------------------------------

        CPGPSexp::~CPGPSexp() {
            Release();
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPGPSexp::New(const void *buffer, size_t length, int autodetect) {
            return !GPGP->CheckForCipherError(gcry_sexp_new(&m_Handle, buffer, length, autodetect));
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPGPSexp::Create(void *buffer, size_t length, int autodetect, void (*freefnc)(void *)) {
            return !GPGP->CheckForCipherError(gcry_sexp_create(&m_Handle, buffer, length, autodetect, freefnc));
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPGPSexp::Build(size_t *erroff, const char *format, ...) {
            bool result;

            va_list argList;
            va_start(argList, format);
            result = !GPGP->CheckForCipherError(gcry_sexp_build_array(&m_Handle, erroff, format, (void **) &argList));
            va_end(argList);

            return result;
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPGPSexp::Sscan(size_t *erroff, const char *buffer, size_t length) {
            return !GPGP->CheckForCipherError(gcry_sexp_sscan(&m_Handle, erroff, buffer, length));
        }
        //--------------------------------------------------------------------------------------------------------------

        size_t CPGPSexp::Sprint(int mode, char *buffer, size_t maxlength) {
            return gcry_sexp_sprint(m_Handle, mode, buffer, maxlength);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPGPSexp::Release() {
            if (m_Handle != nullptr)
                gcry_sexp_release(m_Handle);
            m_Handle = nullptr;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPGPSexp::Dump() {
            gcry_sexp_dump(m_Handle);
        }
        //--------------------------------------------------------------------------------------------------------------

        size_t CPGPSexp::CanonLen(const unsigned char *buffer, size_t length, size_t *erroff, gcry_error_t *errcode) {
            return gcry_sexp_canon_len(buffer, length, erroff, errcode);
        }
        //--------------------------------------------------------------------------------------------------------------

        gcry_sexp_t CPGPSexp::FindToken(const char *token, size_t toklen) {
            return gcry_sexp_find_token(m_Handle, token, toklen);
        }
        //--------------------------------------------------------------------------------------------------------------

        int CPGPSexp::Length() {
            return gcry_sexp_length(m_Handle);
        }
        //--------------------------------------------------------------------------------------------------------------

        gcry_sexp_t CPGPSexp::Nth(int number) {
            return gcry_sexp_nth(m_Handle, number);
        }
        //--------------------------------------------------------------------------------------------------------------

        gcry_sexp_t CPGPSexp::Car() {
            return gcry_sexp_car(m_Handle);
        }
        //--------------------------------------------------------------------------------------------------------------

        gcry_sexp_t CPGPSexp::Cdr() {
            return gcry_sexp_cdr(m_Handle);
        }
        //--------------------------------------------------------------------------------------------------------------

        const char *CPGPSexp::NthData(int number, size_t *datalen) {
            return gcry_sexp_nth_data(m_Handle, number, datalen);
        }
        //--------------------------------------------------------------------------------------------------------------

        void *CPGPSexp::NthBuffer(int number, size_t *rlength) {
            return gcry_sexp_nth_buffer(m_Handle, number, rlength);
        }
        //--------------------------------------------------------------------------------------------------------------

        char *CPGPSexp::NthString(int number) {
            return gcry_sexp_nth_string(m_Handle, number);
        }
        //--------------------------------------------------------------------------------------------------------------

        gcry_mpi_t CPGPSexp::NthMPI(int number, int mpifmt) {
            return gcry_sexp_nth_mpi(m_Handle, number, mpifmt);
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CPGPKey ---------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        bool CPGPKey::Encrypt(gcry_sexp_t data, gcry_sexp_t pkey) {
            return !GPGP->CheckForCipherError(gcry_pk_encrypt(&m_Handle, data, pkey));
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPGPKey::Decrypt(gcry_sexp_t data, gcry_sexp_t skey) {
            return !GPGP->CheckForCipherError(gcry_pk_decrypt(&m_Handle, data, skey));
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPGPKey::Sign(gcry_sexp_t data, gcry_sexp_t skey) {
            return !GPGP->CheckForCipherError(gcry_pk_sign(&m_Handle, data, skey));
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPGPKey::Verify(gcry_sexp_t data, gcry_sexp_t pkey) {
            return !GPGP->CheckForCipherError(gcry_pk_verify(m_Handle, data, pkey));
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPGPKey::TestKey() {
            return !GPGP->CheckForCipherError(gcry_pk_testkey(m_Handle));
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPGPKey::AlgoInfo(int algo, int what, void *buffer, size_t *nbytes) {
            return !GPGP->CheckForCipherError(gcry_pk_algo_info(algo, what, buffer, nbytes));
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPGPKey::Ctl(int cmd, void *buffer, size_t buflen) {
            return !GPGP->CheckForCipherError(gcry_pk_ctl(cmd, buffer, buflen));
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPGPKey::GenKey(gcry_sexp_t parms) {
            return !GPGP->CheckForCipherError(gcry_pk_genkey(&m_Handle, parms));
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPGPKey::PubKeyGetSexp(int mode, gcry_ctx_t ctx) {
            return !GPGP->CheckForCipherError(gcry_pubkey_get_sexp(&m_Handle, mode, ctx));
        }
        //--------------------------------------------------------------------------------------------------------------

        const char *CPGPKey::AlgoName(int algo) {
            return gcry_pk_algo_name(algo);
        }
        //--------------------------------------------------------------------------------------------------------------

        int CPGPKey::MapName(const char *name) {
            return gcry_pk_map_name(name);
        }
        //--------------------------------------------------------------------------------------------------------------

        int CPGPKey::TestAlgo(int algo) {
            return gcry_pk_test_algo(algo);
        }
        //--------------------------------------------------------------------------------------------------------------

        unsigned int CPGPKey::GetNBits() {
            return gcry_pk_get_nbits(m_Handle);
        }
        //--------------------------------------------------------------------------------------------------------------

        unsigned char *CPGPKey::KeyGrip(unsigned char *array) {
            return gcry_pk_get_keygrip (m_Handle, array);
        }

        //--------------------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        static void print_mpi(const char *text, const CPGPMPI& Mpi)
        {
            char *buf;
            void *bufaddr = &buf;

            try {
                Mpi.APrint(GCRYMPI_FMT_HEX, (unsigned char **) bufaddr, nullptr);
            } catch (...) {

            }

            if (GPGP->LastError()) {
                GPGP->Verbose("%s=[error printing number: %s]\n",
                              text, gpg_strerror(GPGP->LastError()));
            } else {
                GPGP->Verbose("%s=0x%s\n", text, buf);
                gcry_free (buf);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        static void check_generated_rsa_key(CPGPKey& Key, unsigned long expected_e) {

            CPGPKey Public(Key.FindToken("public-key", 0));
            if (!Public.Handle())
                throw EPGPError("public part missing in return value");

            CPGPSexp List(Public.FindToken("e", 0));
            CPGPMPI Mpi(List.NthMPI(1, 0));

            if (!List.Handle() || !Mpi.Handle()) {
                throw EPGPError("public exponent not found");
            } else if (!expected_e) {
                print_mpi("e", Mpi);
            } else if (Mpi.CmpUI(expected_e)) {
                print_mpi("e", Mpi);
                throw EPGPError("public exponent is not %lu", expected_e);
            }

            Mpi.Release();
            List.Release();
            Public.Release();

            CPGPKey Secret(Key.FindToken("private-key", 0));
            if (!Secret.Handle())
                throw EPGPError("private part missing in return value");

            Secret.TestKey();
            Secret.Release();
        }
        //--------------------------------------------------------------------------------------------------------------

        void check_rsa_keys(COnPGPVerboseEvent && Verbose) {

            CPGPSexp KeyParm;
            CPGPKey Key;

            GPGP->OnVerbose(static_cast<COnPGPVerboseEvent &&>(Verbose));

            /* Check that DSA generation works and that it can grok the qbits
               argument. */
            GPGP->Verbose("Creating 5 1024 bit DSA keys");

            for (int i = 0; i < 5; i++)
            {
                KeyParm.New("(genkey\n"
                            " (dsa\n"
                            "  (nbits 4:1024)\n"
                            " ))", 0, 1);

                Key.GenKey(KeyParm.Handle());

                KeyParm.Release();
                Key.Release();

                GPGP->Verbose("Done");
            }

            GPGP->Verbose("Creating 1536 bit DSA key");

            KeyParm.New("(genkey\n"
                        " (dsa\n"
                        "  (nbits 4:1536)\n"
                        "  (qbits 3:224)\n"
                        " ))", 0, 1);

            Key.GenKey(KeyParm.Handle());
            KeyParm.Release();

            char buffer[20000];
            Key.Sprint(GCRYSEXP_FMT_ADVANCED, buffer, sizeof buffer);
            GPGP->Verbose("=============================\n%s\n"
                          "=============================\n", buffer);

            Key.Release();

            GPGP->Verbose("Creating 1024 bit RSA key");

            KeyParm.New("(genkey\n"
                        " (rsa\n"
                        "  (nbits 4:1024)\n"
                        " ))", 0, 1);

            Key.GenKey(KeyParm.Handle());
            KeyParm.Release();

            check_generated_rsa_key (Key, 65537);
            Key.Release();

            GPGP->Verbose("Creating 512 bit RSA key with e=257");

            KeyParm.New("(genkey\n"
                        " (rsa\n"
                        "  (nbits 3:512)\n"
                        "  (rsa-use-e 3:257)\n"
                        " ))", 0, 1);

            Key.GenKey(KeyParm.Handle());
            KeyParm.Release();

            check_generated_rsa_key (Key, 257);
            Key.Release();

            GPGP->Verbose("Creating 512 bit RSA key with default e");

            KeyParm.New("(genkey\n"
                        " (rsa\n"
                        "  (nbits 3:512)\n"
                        "  (rsa-use-e 1:0)\n"
                        " ))", 0, 1);

            Key.GenKey(KeyParm.Handle());
            KeyParm.Release();

            check_generated_rsa_key (Key, 0); /* We don't expect a constant exponent. */
            Key.Release();
        }

        void get_aes_ctx(const char* passwd, CPGPCipher *Cipher)
        {
            const size_t keylen = 16;
            char passwd_hash[keylen];

            size_t pass_len = strlen(passwd);

            Cipher->Open(GCRY_CIPHER_AES128,GCRY_CIPHER_MODE_CFB, 0);

            gcry_md_hash_buffer(GCRY_MD_MD5, (void*) &passwd_hash,
                                (const void*) passwd, pass_len);

            Cipher->SetKey((const void *) &passwd_hash, keylen);
            Cipher->SetIV((const void *) &passwd_hash, keylen);
        }

        size_t get_keypair_size(int nbits)
        {
            size_t aes_blklen = gcry_cipher_get_algo_blklen(GCRY_CIPHER_AES128);

            size_t keypair_nbits = 4 * (2 * nbits);

            size_t rem = keypair_nbits % aes_blklen;
            return (keypair_nbits + rem) / 8;
        }

        void load_key(const char *KeyFile, const char* Passwd, COnPGPVerboseEvent &&Verbose) {

            CString keyFile;
            keyFile.LoadFromFile(KeyFile);

            /* Grab a key pair password and create an AES context with it. */
            CPGPCipher Cipher;
            get_aes_ctx("Aliens04", &Cipher);

            /* Read and decrypt the key pair from disk. */
            size_t rsa_len = get_keypair_size(2048);

            void* rsa_buf = calloc(1, rsa_len);
            if (!rsa_buf) {
                throw Delphi::Exception::Exception("malloc: could not allocate rsa buffer");
            }

            keyFile.Read(rsa_buf, rsa_len);

            Cipher.Decrypt((unsigned char*) rsa_buf, rsa_len, nullptr, 0);

            /* Load the key pair components into sexps. */
            CPGPSexp KeyPair;
            KeyPair.New(rsa_buf, rsa_len, 0);

            free(rsa_buf);

            CPGPKey Public(KeyPair.FindToken("public-key", 0));
            CPGPKey Secret(KeyPair.FindToken("private-key", 0));

            /* Create a message. */
            CPGPMPI Msg;
            CPGPSexp Data;

            const auto* s = (const unsigned char*) "Hello world.";
            Msg.Scan(GCRYMPI_FMT_USG, s, strlen((const char*) s), nullptr);

            Data.Build(nullptr,"(data (flags raw) (value %m))", Msg.Handle());

            /* Encrypt the message. */
            CPGPKey Ciph;
            Ciph.Encrypt(Data.Handle(), Public.Handle());

            /* Decrypt the message. */
            CPGPKey Plain;
            Plain.Decrypt(Ciph.Handle(), Secret.Handle());

            /* Pretty-print the results. */
            CPGPMPI outMsg(Plain.NthMPI(0, GCRYMPI_FMT_USG));

            GPGP->Verbose("Original:");

            Msg.Dump();

            GPGP->Verbose("Decrypted:");

            outMsg.Dump();

            if (Msg.Cmp(outMsg.Handle())) {
                throw Delphi::Exception::Exception("Data corruption!");
            }

            GPGP->Verbose("Messages match.");

            unsigned char obuf[64] = { 0 };

            outMsg.Print(GCRYMPI_FMT_USG, (unsigned char*) &obuf, sizeof(obuf), nullptr);

            GPGP->Verbose("-> %s", (char *) obuf);
        }
#endif
    }
}
}
