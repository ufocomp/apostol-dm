/*++

Library name:

  apostol-core

Module Name:

  Bitcoin.hpp

Notices:

  Apostol Core

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef APOSTOL_BITCOIN_BITCOIN_HPP
#define APOSTOL_BITCOIN_BITCOIN_HPP

#ifdef BITCOIN_VERSION_4

//#include <bitcoin/system.hpp>

#include <bitcoin/system/formats/base_10.hpp>
#include <bitcoin/system/formats/base_16.hpp>
#include <bitcoin/system/formats/base_32.hpp>
#include <bitcoin/system/formats/base_58.hpp>
#include <bitcoin/system/formats/base_64.hpp>
#include <bitcoin/system/formats/base_85.hpp>

#include <bitcoin/system/config/base16.hpp>
#include <bitcoin/system/wallet/message.hpp>
#include <bitcoin/system/wallet/payment_address.hpp>
#include <bitcoin/system/wallet/witness_address.hpp>
#include <bitcoin/system/wallet/ek_token.hpp>
#include <bitcoin/system/wallet/ek_public.hpp>
#include <bitcoin/system/wallet/encrypted_keys.hpp>

#include <bitcoin/system/config/script.hpp>

#include <bitcoin/client/history.hpp>
#include <bitcoin/client/stealth.hpp>

using namespace bc;
using namespace bc::system;
using namespace bc::system::config;
using namespace bc::system::wallet;
using namespace bc::client;

#else

#include <bitcoin/bitcoin.hpp>
#include <bitcoin/protocol.hpp>

#include <bitcoin/client/define.hpp>
#include <bitcoin/client/obelisk_client.hpp>
#include <bitcoin/client/version.hpp>

using namespace bc;
using namespace bc::chain;
using namespace bc::machine;
using namespace bc::config;
using namespace bc::wallet;
using namespace bc::client;

#endif

//----------------------------------------------------------------------------------------------------------------------

#define BX_EK_PUBLIC_TO_EC_INVALID_PASSPHRASE \
    "The passphrase is incorrect."
#define BX_EK_ADDRESS_SHORT_SEED \
    "The seed is less than 192 bits long."
#define BX_TOKEN_NEW_SHORT_SALT \
    "The salt is less than 32 bits long."
#define BX_TOKEN_NEW_REQUIRES_ICU \
    "The command requires an ICU build."
#define BX_CONNECTION_FAILURE \
    "Could not connect to server: %1%"
#define BX_HD_NEW_SHORT_SEED \
    "The seed is less than 128 bits long."
#define BX_HD_NEW_INVALID_KEY \
    "The seed produced an invalid key."
//----------------------------------------------------------------------------------------------------------------------

#define BX_NO_TRANSFERS_FOUND "No transfers found on account: %s."
#define BX_INSUFFICIENT_FUNDS "Insufficient funds on account: %s."
#define BX_INSUFFICIENT_FEE "Insufficient transaction fee on account: %s."
//----------------------------------------------------------------------------------------------------------------------

/**
 * The minimum safe length of a seed in bits (multiple of 8).
 */
BC_CONSTEXPR size_t minimum_seed_bits = 128;

/**
 * The minimum safe length of a seed in bytes (16).
 */
BC_CONSTEXPR size_t minimum_seed_size = minimum_seed_bits / bc::byte_bits;
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Bech32
    {
        /** Encode a Bech32 string. Returns the empty string in case of failure. */
        std::string encode(const std::string& hrp, const std::vector<uint8_t>& values);

        /** Decode a Bech32 string. Returns (hrp, data). Empty hrp means failure. */
        std::pair<std::string, std::vector<uint8_t> > decode(const std::string& str);
    }

    namespace SegWit
    {
        /** Decode a SegWit address. Returns (witver, witprog). witver = -1 means failure. */
        std::pair<int, std::vector<uint8_t> > decode(const std::string& hrp, const std::string& addr);

        /** Encode a SegWit address. Empty string means failure. */
        std::string encode(const std::string& hrp, int witver, const std::vector<uint8_t>& witprog);

        uint32_t checksum(const std::string& hrp, int witver, const std::vector<uint8_t>& witprog);
    }

    namespace Bitcoin {

        typedef TList<ec_private> CPrivateList;
        //--------------------------------------------------------------------------------------------------------------

        typedef struct CBitcoinConfig {
            uint64_t VersionHD = hd_private::mainnet;
            uint16_t VersionEC = ec_private::mainnet;
            uint8_t VersionKey = payment_address::mainnet_p2kh;
            uint8_t VersionScript = payment_address::mainnet_p2sh;
            std::string endpoint = "tcp://mainnet.libbitcoin.net:9091";
            CString Symbol = "BTC";
        } CBitcoinConfig, *PBitcoinConfig;
        //--------------------------------------------------------------------------------------------------------------

        extern CBitcoinConfig BitcoinConfig;
        //--------------------------------------------------------------------------------------------------------------

        enum CAddressDecodeType { adtInfo, adtHtml, adtJson };

        std::string string_to_hex(const std::string& input);
        std::string hex_to_string(const std::string& input);
        //--------------------------------------------------------------------------------------------------------------

        std::string sha1(const std::string &value);
        CString sha1(const CString &Value);
        //--------------------------------------------------------------------------------------------------------------

        std::string sha256(const std::string &value);
        CString sha256(const CString &Value);
        //--------------------------------------------------------------------------------------------------------------

        std::string ripemd160(const std::string &value);
        CString ripemd160(const CString &Value);
        //--------------------------------------------------------------------------------------------------------------

        double satoshi_to_btc(uint64_t Value);
        uint64_t btc_to_satoshi(double Value);
        //--------------------------------------------------------------------------------------------------------------

        bool valid_address(const CString& Address);
        //--------------------------------------------------------------------------------------------------------------

        bool valid_public_key(const std::string& key);
        bool valid_public_key(const CString& Key);
        //--------------------------------------------------------------------------------------------------------------

        bool key_belongs_address(const std::string& key, const std::string& address);
        bool key_belongs_address(const CString& Key, const CString& Address);
        //--------------------------------------------------------------------------------------------------------------

        bool address_decode(CAddressDecodeType type, const CString& Address, CString& Result);
        //--------------------------------------------------------------------------------------------------------------

        bool address_decode(const CString& Address, CJSON& Result);
        bool address_decode(const CString& Address, CString& Result);
        //--------------------------------------------------------------------------------------------------------------

        bool unwrap_legacy(wallet::wrapped_data &data, const wallet::payment_address &address);
        bool unwrap_legacy(wallet::wrapped_data &data, const std::string &address);
        //--------------------------------------------------------------------------------------------------------------

        bool unwrap_segwit(wallet::wrapped_data &data, const std::string &address);
        //--------------------------------------------------------------------------------------------------------------

        bool IsLegacyAddress(const std::string& address);
        bool IsLegacyAddress(const CString& Address);

        bool IsSegWitAddress(const std::string& addr);
        bool IsSegWitAddress(const CString& Addr);

        bool IsBitcoinAddress(const std::string& addr);
        bool IsBitcoinAddress(const CString& Addr);
        //--------------------------------------------------------------------------------------------------------------

        wallet::ek_token token_new(const std::string &passphrase, const data_chunk& salt);
        wallet::ek_public ek_public(const wallet::ek_token &token, const data_chunk& seed, uint8_t version = payment_address::mainnet_p2kh);
        wallet::ec_public ek_public_to_ec(const std::string &passphrase, const wallet::ek_public &key);
        wallet::payment_address ek_address(const wallet::ek_token &token, const data_chunk& seed, uint8_t version = payment_address::mainnet_p2kh);
        //--------------------------------------------------------------------------------------------------------------

        wallet::hd_private hd_new(const data_chunk& seed, uint64_t prefixes = hd_private::mainnet);
        wallet::ec_private hd_to_ec_private(const wallet::hd_key& key, uint64_t prefixes = hd_private::mainnet);
        wallet::ec_public hd_to_ec_public(const wallet::hd_key& key, uint32_t prefix = hd_public::mainnet);
        //--------------------------------------------------------------------------------------------------------------

        bool verify_message(const std::string& msg, const std::string& addr, const std::string& sig);
        bool VerifyMessage(const CString& Message, const CString& Address, const CString& Signature);
        //--------------------------------------------------------------------------------------------------------------

#ifdef BITCOIN_VERSION_4
        std::string address_to_key(const wallet::payment_address &address);
#endif
        chain::script get_witness_script(const ec_public& key1, const ec_public& key2, const ec_public& key3);
        chain::script get_redeem_script(const ec_public& key1, const ec_public& key2, const ec_public& key3);

        std::string script_to_address(const chain::script &script, uint8_t version = payment_address::mainnet_p2sh);
        std::string script_to_address(const std::string &script, uint8_t version = payment_address::mainnet_p2sh);
        //--------------------------------------------------------------------------------------------------------------
#ifdef WITH_BITCOIN_CLIENT
        uint64_t fetch_balance(const wallet::payment_address &address);
        //--------------------------------------------------------------------------------------------------------------

        void fetch_history(const wallet::payment_address &address, CJSON& Result);
        void fetch_header(uint32_t height, CJSON& Result);
        void fetch_header(const hash_digest& hash, CJSON& Result);

        void send_tx(const chain::transaction& tx, CJSON& Result);
        void send_tx(const std::string& hex, CJSON& Result);

        //--------------------------------------------------------------------------------------------------------------

        //-- CBalance --------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CBalance {
        private:

            uint32_t m_Transfers;
            uint32_t m_Height;

            uint32_t m_Received;
            uint32_t m_Spent;

            CDateTime m_TimeStamp;

        protected:

            CJSON m_History;
            CJSON m_Header;

        public:

            CBalance() {
                m_Transfers = 0;
                m_Height = 0;

                m_Received = 0;
                m_Spent = 0;

                m_TimeStamp = 0;
            }

            void Clear();

            void Fetch(const CString &Address);

            const CJSON &History() const { return m_History; };
            const CJSON &Header() const { return m_Header; };

            uint32_t Transfers() const { return m_Transfers; }

            uint32_t Height() const { return m_Height; }
            CDateTime TimeStamp() const { return m_TimeStamp; }

            uint32_t Received() const { return m_Received; }
            uint32_t Spent() const { return m_Spent; }

            uint32_t Balance() const {
                return m_Received - m_Spent;
            }

        };
#endif

        //--------------------------------------------------------------------------------------------------------------

        //-- CWitness --------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CWitness {
        private:

            std::vector<ec_private> m_secrets;
            std::vector<ec_public> m_keys;

            uint8_t m_signatures;

            hash_digest m_hash;

            chain::script m_script;
            chain::script m_embedded;

            void bind_script();
            void bind_embedded();

        public:

            CWitness();
            explicit CWitness(uint8_t signatures);

            CWitness(const ec_private& key1, const ec_private& key2);
            CWitness(const ec_private& key1, const ec_private& key2, const ec_public& key3);
            CWitness(const ec_public& key1, const ec_public& key2, const ec_public& key3);

            void bind();

            const hash_digest& hash() const;

            const chain::script& script() const { return m_script; };
            const chain::script& embedded() const {return m_embedded; };

            payment_address to_address(uint8_t version = payment_address::mainnet_p2sh) const;

            const std::vector<ec_private> &secrets() const { return m_secrets; };
            const std::vector<ec_public> &keys() const { return m_keys; };

        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CRecipient ------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        typedef struct CRecipient {

            CString Address;
            uint64_t Amount;

            CRecipient() {
                Amount = 0;
            }

            CRecipient(const CRecipient &Other) {
                if (this != &Other) {
                    this->Address = Other.Address;
                    this->Amount = Other.Amount;
                }
            };

            CRecipient(const CString &Address, uint64_t Amount) {
                this->Address = Address;
                this->Amount = Amount;
            }

            CRecipient &operator=(const CRecipient &Other) {
                if (this != &Other) {
                    this->Address = Other.Address;
                    this->Amount = Other.Amount;
                }
                return *this;
            };

            inline bool operator!=(const CRecipient &Value) { return Address != Value.Address; };

            inline bool operator==(const CRecipient &Value) { return Address == Value.Address; };

        } CRecipient, *PRecipient;
        //--------------------------------------------------------------------------------------------------------------

        typedef TList<CRecipient> CRecipients;

        //--------------------------------------------------------------------------------------------------------------

        //-- raw -------------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        /**
         * Serialization helper to convert between a byte stream and data_chunk.
         */
        class raw
        {
        public:

            /**
             * Default constructor.
             */
            raw();

            /**
             * Initialization constructor.
             * @param[in]  text  The value to initialize with.
             */
            raw(const std::string& text);

            /**
             * Initialization constructor.
             * @param[in]  value  The value to initialize with.
             */
            raw(const data_chunk& value);

            /**
             * Copy constructor.
             * @param[in]  other  The object to copy into self on construct.
             */
            raw(const raw& other);

            /**
             * Overload cast to internal type.
             * @return  This object's value cast to internal type.
             */
            operator const data_chunk&() const;

            /**
             * Overload cast to generic data reference.
             * @return  This object's value cast to a generic data reference.
             */
            operator data_slice() const;

            /**
             * Overload stream in. Throws if input is invalid.
             * @param[in]   input     The input stream to read the value from.
             * @param[out]  argument  The object to receive the read value.
             * @return                The input stream reference.
             */
            friend std::istream& operator>>(std::istream& input,
                                            raw& argument);

            /**
             * Overload stream out.
             * @param[in]   output    The output stream to write the value to.
             * @param[out]  argument  The object from which to obtain the value.
             * @return                The output stream reference.
             */
            friend std::ostream& operator<<(std::ostream& output,
                                            const raw& argument);

        private:

            /**
             * The state of this object's raw data.
             */
            data_chunk value_;
        };

        class wrapper
        {
        public:

            /**
             * Default constructor.
             */
            wrapper();

            /**
             * Initialization constructor.
             *
             * @param[in]  wrapped  The value to initialize with.
             */
            wrapper(const std::string& wrapped);

            /**
             * Initialization constructor.
             * @param[in]  wrapped  The wrapped value to initialize with.
             */
            wrapper(const data_chunk& wrapped);

            /**
             * Initialization constructor.
             * @param[in]  wrapped  The wrapped value to initialize with.
             */
            wrapper(const wallet::wrapped_data& wrapped);

            /**
             * Initialization constructor.
             * @param[in]  address  The payment address to initialize with.
             */
            wrapper(const wallet::payment_address& address);

            /**
             * Initialization constructor.
             * @param[in]  version  The version for the new wrapped value.
             * @param[in]  payload  The payload for the new wrapped value.
             */
            wrapper(uint8_t version, const data_chunk& payload);

            /**
             * Copy constructor.
             * @param[in]  other  The object to copy into self on construct.
             */
            wrapper(const wrapper& other);

            /**
             * Serialize the wrapper to bytes according to the wire protocol.
             * @return  The byte serialized copy of the wrapper.
             */
            const data_chunk to_data() const;

            /**
             * Overload cast to internal type.
             * @return  This object's value cast to internal type.
             */
            operator const wallet::wrapped_data&() const;

            /**
             * Overload stream in. Throws if input is invalid.
             * @param[in]   input     The input stream to read the value from.
             * @param[out]  argument  The object to receive the read value.
             * @return                The input stream reference.
             */
            friend std::istream& operator>>(std::istream& input,
                                            wrapper& argument);

            /**
             * Overload stream out.
             * @param[in]   output    The output stream to write the value to.
             * @param[out]  argument  The object from which to obtain the value.
             * @return                The output stream reference.
             */
            friend std::ostream& operator<<(std::ostream& output,
                                            const wrapper& argument);

        private:

            /**
             * The state of this object's data.
             */
            wallet::wrapped_data value_;
        };

        /**
         * Serialization helper to convert between string and message_signature.
         */
        class signature
        {
        public:

            /**
             * Default constructor.
             */
            signature();

            /**
             * Initialization constructor.
             * @param[in]  hexcode  The value to initialize with.
             */
            signature(const std::string& hexcode);

            /**
             * Initialization constructor.
             * @param[in]  value  The value to initialize with.
             */
            signature(const wallet::message_signature& value);

            /**
             * Copy constructor.
             * @param[in]  other  The object to copy into self on construct.
             */
            signature(const signature& other);

            /**
             * Overload cast to internal type.
             * @return  This object's value cast to internal type.
             */
            operator wallet::message_signature&();

            /**
             * Overload cast to internal type.
             * @return  This object's value cast to internal type.
             */
            operator const wallet::message_signature&() const;

            /// Serializer.
            std::string encoded() const;

            /**
             * Overload stream in. If input is invalid sets no bytes in argument.
             * @param[in]   input     The input stream to read the value from.
             * @param[out]  argument  The object to receive the read value.
             * @return                The input stream reference.
             */
            friend std::istream& operator>>(std::istream& input, signature& argument);

            /**
             * Overload stream out.
             * @param[in]   output    The output stream to write the value to.
             * @param[out]  argument  The object from which to obtain the value.
             * @return                The output stream reference.
             */
            friend std::ostream& operator<<(std::ostream& output, const signature& argument);

        private:

            /**
             * The state of this object.
             */
            wallet::message_signature value_;
        };

        /**
         * Serialization helper to convert between base16/raw script and script_type.
        */
        class script
        {
        public:

            /**
             * Default constructor.
             */
            script();

            /**
             * Initialization constructor.
             * @param[in]  mnemonic  The value to initialize with.
             */
            script(const std::string& mnemonic);

            /**
             * Initialization constructor.
             * @param[in]  value  The value to initialize with.
             */
            script(const chain::script& value);

            /**
             * Initialization constructor.
             * @param[in]  value  The value to initialize with.
             */
            script(const data_chunk& value);

            /**
             * Initialization constructor.
             * @param[in]  tokens  The mnemonic tokens to initialize with.
             */
            script(const std::vector<std::string>& tokens);

            /**
             * Copy constructor.
             * @param[in]  other  The object to copy into self on construct.
             */
            script(const script& other);

            /**
             * Serialize the script to bytes according to the wire protocol.
             * @return  The byte serialized copy of the script.
             */
            const bc::data_chunk to_data() const;

            /**
             * Return a pretty-printed copy of the script.
             * @return  A mnemonic-printed copy of the internal script.
             */
            const std::string to_string() const;

            /**
             * Overload cast to internal type.
             * @return  This object's value cast to internal type.
             */
            operator const chain::script&() const;

            /**
             * Overload stream in. Throws if input is invalid.
             * @param[in]   input     The input stream to read the value from.
             * @param[out]  argument  The object to receive the read value.
             * @return                The input stream reference.
             */
            friend std::istream& operator>>(std::istream& input,
                                            script& argument);

            /**
             * Overload stream out.
             * @param[in]   output    The output stream to write the value to.
             * @param[out]  argument  The object from which to obtain the value.
             * @return                The output stream reference.
             */
            friend std::ostream& operator<<(std::ostream& output,
                                            const script& argument);

        private:

            /**
             * The state of this object.
             */
            chain::script value_;
        };

    } // namespace Bitcoin

} // namespace Apostol

using namespace Apostol::Bitcoin;
using namespace Apostol::Bech32;
using namespace Apostol::SegWit;
}
#endif //APOSTOL_BITCOIN_BITCOIN_HPP
