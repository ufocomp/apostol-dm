/*++

Library name:

  apostol-core

Module Name:

  Bitcoin.cpp

Notices:

  Apostol Core

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#include "Core.hpp"
#include "Bitcoin.hpp"

extern "C++" {

namespace Apostol {

    namespace Bech32 {

        typedef std::vector<uint8_t> data;

        /** The Bech32 character set for encoding. */
        const char* charset = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";

        /** The Bech32 character set for decoding. */
        const int8_t charset_rev[128] = {
                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                15, -1, 10, 17, 21, 20, 26, 30,  7,  5, -1, -1, -1, -1, -1, -1,
                -1, 29, -1, 24, 13, 25,  9,  8, 23, -1, 18, 22, 31, 27, 19, -1,
                1,  0,  3, 16, 11, 28, 12, 14,  6,  4,  2, -1, -1, -1, -1, -1,
                -1, 29, -1, 24, 13, 25,  9,  8, 23, -1, 18, 22, 31, 27, 19, -1,
                1,  0,  3, 16, 11, 28, 12, 14,  6,  4,  2, -1, -1, -1, -1, -1
        };

        /** Concatenate two byte arrays. */
        data cat(data x, const data& y) {
            x.insert(x.end(), y.begin(), y.end());
            return x;
        }

        /** Find the polynomial with value coefficients mod the generator as 30-bit. */
        uint32_t polymod(const data& values) {
            uint32_t chk = 1;
            for (size_t i = 0; i < values.size(); ++i) {
                uint8_t top = chk >> 25;
                chk = (chk & 0x1ffffff) << 5 ^ values[i] ^
                      (-((top >> 0) & 1) & 0x3b6a57b2UL) ^
                      (-((top >> 1) & 1) & 0x26508e6dUL) ^
                      (-((top >> 2) & 1) & 0x1ea119faUL) ^
                      (-((top >> 3) & 1) & 0x3d4233ddUL) ^
                      (-((top >> 4) & 1) & 0x2a1462b3UL);
            }
            return chk;
        }

        /** Convert to lower case. */
        unsigned char lc(unsigned char c) {
            return (c >= 'A' && c <= 'Z') ? (c - 'A') + 'a' : c;
        }

        /** Expand a HRP for use in checksum computation. */
        data expand_hrp(const std::string& hrp) {
            data ret;
            ret.resize(hrp.size() * 2 + 1);
            for (size_t i = 0; i < hrp.size(); ++i) {
                unsigned char c = hrp[i];
                ret[i] = c >> 5;
                ret[i + hrp.size() + 1] = c & 0x1f;
            }
            ret[hrp.size()] = 0;
            return ret;
        }

        /** Verify a checksum. */
        bool verify_checksum(const std::string& hrp, const data& values) {
            return polymod(cat(expand_hrp(hrp), values)) == 1;
        }

        /** Create a checksum. */
        data create_checksum(const std::string& hrp, const data& values) {
            data enc = cat(expand_hrp(hrp), values);
            enc.resize(enc.size() + 6);
            uint32_t mod = polymod(enc) ^ 1;
            data ret;
            ret.resize(6);
            for (size_t i = 0; i < 6; ++i) {
                ret[i] = (mod >> (5 * (5 - i))) & 31;
            }
            return ret;
        }

        /** Encode a Bech32 string. */
        std::string encode(const std::string& hrp, const data& values) {
            data checksum = create_checksum(hrp, values);
            data combined = cat(values, checksum);
            std::string ret = hrp + '1';
            ret.reserve(ret.size() + combined.size());
            for (size_t i = 0; i < combined.size(); ++i) {
                ret += charset[combined[i]];
            }
            return ret;
        }

        /** Decode a Bech32 string. */
        std::pair<std::string, data> decode(const std::string& str) {
            bool lower = false, upper = false;
            bool ok = true;
            for (size_t i = 0; ok && i < str.size(); ++i) {
                unsigned char c = str[i];
                if (c < 33 || c > 126) ok = false;
                if (c >= 'a' && c <= 'z') lower = true;
                if (c >= 'A' && c <= 'Z') upper = true;
            }
            if (lower && upper) ok = false;
            size_t pos = str.rfind('1');
            if (ok && str.size() <= 90 && pos != str.npos && pos >= 1 && pos + 7 <= str.size()) {
                data values;
                values.resize(str.size() - 1 - pos);
                for (size_t i = 0; i < str.size() - 1 - pos; ++i) {
                    unsigned char c = str[i + pos + 1];
                    if (charset_rev[c] == -1) ok = false;
                    values[i] = charset_rev[c];
                }
                if (ok) {
                    std::string hrp;
                    for (size_t i = 0; i < pos; ++i) {
                        hrp += lc(str[i]);
                    }
                    if (verify_checksum(hrp, values)) {
                        return std::make_pair(hrp, data(values.begin(), values.end() - 6));
                    }
                }
            }
            return std::make_pair(std::string(), data());
        }
    }

    namespace SegWit {

        /** Convert from one power-of-2 number base to another. */
        template<int frombits, int tobits, bool pad>
        bool convertbits(data& out, const data& in) {
            int acc = 0;
            int bits = 0;
            const int maxv = (1 << tobits) - 1;
            const int max_acc = (1 << (frombits + tobits - 1)) - 1;
            for (size_t i = 0; i < in.size(); ++i) {
                int value = in[i];
                acc = ((acc << frombits) | value) & max_acc;
                bits += frombits;
                while (bits >= tobits) {
                    bits -= tobits;
                    out.push_back((acc >> bits) & maxv);
                }
            }
            if (pad) {
                if (bits) out.push_back((acc << (tobits - bits)) & maxv);
            } else if (bits >= frombits || ((acc << (tobits - bits)) & maxv)) {
                return false;
            }
            return true;
        }

        /** Decode a SegWit address. */
        std::pair<int, data> decode(const std::string& hrp, const std::string& addr) {
            std::pair<std::string, data> dec = Bech32::decode(addr);
            if (dec.first != hrp || dec.second.size() < 1) return std::make_pair(-1, data());
            data conv;
            if (!convertbits<5, 8, false>(conv, data(dec.second.begin() + 1, dec.second.end())) ||
                conv.size() < 2 || conv.size() > 40 || dec.second[0] > 16 || (dec.second[0] == 0 &&
                                                                              conv.size() != 20 && conv.size() != 32)) {
                return std::make_pair(-1, data());
            }
            return std::make_pair(dec.second[0], conv);
        }

        /** Encode a SegWit address. */
        std::string encode(const std::string& hrp, int witver, const data& witprog) {
            data enc;
            enc.push_back(witver);
            convertbits<8, 5, true>(enc, witprog);
            std::string ret = Bech32::encode(hrp, enc);
            if (decode(hrp, ret).first == -1) return "";
            return ret;
        }

        uint32_t checksum(const std::string &hrp, int witver, const std::vector<uint8_t> &witprog) {
            data enc;
            enc.push_back(witver);
            convertbits<8, 5, true>(enc, witprog);
            data checksum = Bech32::create_checksum(hrp, enc);
            const auto checksum_begin = std::begin(checksum);
            return from_little_endian_unsafe<uint32_t>(checksum_begin);
        }
    }

    namespace Bitcoin {

        CBitcoinConfig BitcoinConfig;
        //--------------------------------------------------------------------------------------------------------------

        std::string string_to_hex(const std::string& input)
        {
            static const char* const lut = "0123456789ABCDEF";
            size_t len = input.length();

            std::string output;
            output.reserve(2 * len);
            for (size_t i = 0; i < len; ++i)
            {
                const unsigned char c = input[i];
                output.push_back(lut[c >> 4]);
                output.push_back(lut[c & 15]);
            }
            return output;
        }
        //--------------------------------------------------------------------------------------------------------------

        std::string hex_to_string(const std::string& input)
        {
            static const char* const lut = "0123456789ABCDEF";
            size_t len = input.length();
            if (len & 1) throw std::invalid_argument("odd length");

            std::string output;
            output.reserve(len / 2);
            for (size_t i = 0; i < len; i += 2)
            {
                char a = input[i];
                const char* p = std::lower_bound(lut, lut + 16, a);
                if (*p != a) throw std::invalid_argument("not a hex digit");

                char b = input[i + 1];
                const char* q = std::lower_bound(lut, lut + 16, b);
                if (*q != b) throw std::invalid_argument("not a hex digit");

                output.push_back(((p - lut) << 4) | (q - lut));
            }
            return output;
        }
        //--------------------------------------------------------------------------------------------------------------

        std::string sha1(const std::string &value) {
            const auto& hex = string_to_hex(value);
            const config::base16 data(hex);
            const auto& hash = sha1_hash(data);
            return encode_base16(hash);
        }
        //--------------------------------------------------------------------------------------------------------------

        CString sha1(const CString &Value) {
            const std::string value(Value);
            return sha1(value);
        }
        //--------------------------------------------------------------------------------------------------------------

        std::string sha256(const std::string &value) {
            const auto& hex = string_to_hex(value);
            const config::base16 data(hex);
            const auto& hash = sha256_hash(data);
            return encode_base16(hash);
        }
        //--------------------------------------------------------------------------------------------------------------

        CString sha256(const CString &Value) {
            const std::string value(Value);
            return sha256(value);
        }
        //--------------------------------------------------------------------------------------------------------------

        std::string ripemd160(const std::string &value) {
            const auto& hex = string_to_hex(value);
            const config::base16 data(hex);
            const auto& hash = ripemd160_hash(data);
            return encode_base16(hash);
        }
        //--------------------------------------------------------------------------------------------------------------

        CString ripemd160(const CString &Value) {
            const std::string value(Value);
            return ripemd160(value);
        }
        //--------------------------------------------------------------------------------------------------------------

        static std::vector<uint8_t> segwit_scriptpubkey(int witver, const std::vector<uint8_t>& witprog) {
            std::vector<uint8_t> ret;
            ret.push_back(witver ? (0x80 | witver) : 0);
            ret.push_back(witprog.size());
            ret.insert(ret.end(), witprog.begin(), witprog.end());
            return ret;
        }
        //--------------------------------------------------------------------------------------------------------------

        static bool unwrap(wallet::wrapped_data &data, const data_slice &wrapped) {
            if (!verify_checksum(wrapped))
                return false;

            data.version = wrapped.data()[0];
            const auto payload_begin = std::begin(wrapped) + 1;
            const auto checksum_begin = std::end(wrapped) - checksum_size;
            data.payload.resize(checksum_begin - payload_begin);
            std::copy(payload_begin, checksum_begin, data.payload.begin());
            data.checksum = from_little_endian_unsafe<uint32_t>(checksum_begin);
            return true;
        }
        //--------------------------------------------------------------------------------------------------------------

        static data_chunk wrap(const wallet::wrapped_data& data) {
            auto bytes = to_chunk(data.version);
            extend_data(bytes, data.payload);
            append_checksum(bytes);
            return bytes;
        }
        //--------------------------------------------------------------------------------------------------------------

        uint64_t btc_to_satoshi(double Value) {
            return (uint64_t) (Value * 100000000);
        };
        //--------------------------------------------------------------------------------------------------------------

        double satoshi_to_btc(uint64_t Value) {
            return ((double) Value) / 100000000;
        }
        //--------------------------------------------------------------------------------------------------------------

        bool valid_address(const CString& Address) {
            if (Address.IsEmpty())
                return false;

            const std::string address(Address);

            if (IsLegacyAddress(address) && is_base58(address)) {
                data_chunk data;
                decode_base58(data, address);
                return verify_checksum(data);
            }

            if (IsSegWitAddress(Address)) {
                std::string hrp(address.substr(0, 2));
                std::transform(hrp.begin(), hrp.end(), hrp.begin(), [] (unsigned char c) { return std::tolower(c); });
                const std::pair<int, std::vector<uint8_t>> &segwit = SegWit::decode(hrp, address);
                return segwit.first != -1;
            }

            return false;
        };
        //--------------------------------------------------------------------------------------------------------------

        bool valid_public_key(const std::string& key) {
            return verify(wallet::ec_public(key));
        };
        //--------------------------------------------------------------------------------------------------------------

        bool valid_public_key(const CString& Key){
            return valid_public_key(std::string(Key));
        };
        //--------------------------------------------------------------------------------------------------------------

        bool key_belongs_address(const std::string& key, const std::string& address) {
            const wallet::ec_public public_key(key);

            if (!verify(public_key))
                return false;

            if (IsLegacyAddress(address)) {
                const wallet::payment_address legacy(address);
                const Bitcoin::wrapper wrapped(legacy);
                const wallet::wrapped_data& data = wrapped;

                const auto& addr = wallet::payment_address(public_key, data.version).encoded();
                return address == addr;
            }
#ifdef BITCOIN_VERSION_4
            if (IsSegWitAddress(address)) {
                const auto& addr = wallet::witness_address(public_key).encoded();
                return address == addr;
            }
#endif
            return false;
        };
        //--------------------------------------------------------------------------------------------------------------

        bool key_belongs_address(const CString& Key, const CString& Address) {
            return key_belongs_address(std::string(Key), std::string(Address));
        }
        //--------------------------------------------------------------------------------------------------------------

        bool address_decode(const CString& Str, CString& Result) {
            return address_decode(adtHtml, Str, Result);
        };
        //--------------------------------------------------------------------------------------------------------------

        bool address_decode(CAddressDecodeType type, const CString& Address, CString& Result) {
            const std::string address(Address);
            wallet::wrapped_data data;

            if (IsLegacyAddress(Address)) {
                if (!unwrap_legacy(data, address))
                    return false;
            } else  if (IsSegWitAddress(Address)) {
                if (!unwrap_segwit(data, address))
                    return false;
            } else {
                return false;
            }

            const auto& hex = encode_base16(data.payload);

            switch (type) {
                case adtInfo:
                    Result.Format(" address: %s\n wrapper:\nchecksum: %u\n payload: %s\n version: %u",
                                  address.c_str(),
                                  data.checksum,
                                  hex.c_str(),
                                  data.version);
                    break;
                case adtHtml:
                    Result.Format(" address: <b>%s</b>\n wrapper:\nchecksum: <b>%u</b>\n payload: <b>%s</b>\n version: <b>%u</b>",
                                  address.c_str(),
                                  data.checksum,
                                  hex.c_str(),
                                  data.version);
                    break;
                case adtJson:
                    Result.Format(R"({"address": "%s", "wrapper": {"checksum": %u, "payload": "%s", "version": %u}})",
                                  address.c_str(),
                                  data.checksum,
                                  hex.c_str(),
                                  data.version);
                    break;
            }

            return true;
        }
        //--------------------------------------------------------------------------------------------------------------

        bool address_decode(const CString& Address, CJSON &Result) {
            CString jsonString;
            const bool ret = address_decode(adtJson, Address, jsonString);
            Result << jsonString;
            return ret;
        }
        //--------------------------------------------------------------------------------------------------------------

        bool unwrap_legacy(wallet::wrapped_data &data, const wallet::payment_address &address) {
            if (!address)
                return false;
            const Bitcoin::wrapper wrapped(address);
            data = wrapped;
            return true;
        }
        //--------------------------------------------------------------------------------------------------------------

        bool unwrap_legacy(wallet::wrapped_data &data, const std::string &address) {
            return unwrap_legacy(data, wallet::payment_address(address));
        }
        //--------------------------------------------------------------------------------------------------------------

        bool unwrap_segwit(wallet::wrapped_data &data, const std::string &address) {
            const std::string hrp(address.substr(0, 2));
            const std::pair<int, std::vector<uint8_t>> &segwit = SegWit::decode(hrp, address);

            if (segwit.first == -1)
                return false;

            data.version = segwit.first;
            data.payload = segwit.second;
            data.checksum = SegWit::checksum(hrp, segwit.first, segwit.second);

            //data.payload = segwit_scriptpubkey(segwit.first, segwit.second);
            return true;
        }
        //--------------------------------------------------------------------------------------------------------------

        bool IsLegacyAddress(const std::string& address) {
            if (address.empty()) return false;
            const TCHAR ch = address.at(0);
            return (ch == '1' || ch == '2' || ch == '3' || ch == 'm' || ch == 'n') && (address.length() >= 26 && address.length() <= 35);
        }
        //--------------------------------------------------------------------------------------------------------------

        bool IsLegacyAddress(const CString& Address) {
            if (Address.IsEmpty()) return false;
            const TCHAR ch = Address.at(0);
            return (ch == '1' || ch == '2' || ch == '3' || ch == 'm' || ch == 'n') && (Address.Length() >= 26 && Address.Length() <= 35);
        }
        //--------------------------------------------------------------------------------------------------------------

        bool IsSegWitAddress(const std::string &addr) {
            if (addr.empty()) return false;
            const std::string hrp(addr.substr(0, 3));
            return (hrp == "bc1" || hrp == "tb1") && (addr.length() == 42 || addr.length() == 62);
        }
        //--------------------------------------------------------------------------------------------------------------

        bool IsSegWitAddress(const CString& Addr) {
            if (Addr.IsEmpty()) return false;
            const CString hrp(Addr.SubString(0, 3).Lower());
            return (hrp == "bc1" || hrp == "tb1") && (Addr.Length() == 42 || Addr.Length() == 62);
        }
        //--------------------------------------------------------------------------------------------------------------

        bool IsBitcoinAddress(const std::string &addr) {
            return IsLegacyAddress(addr) || IsSegWitAddress(addr);
        }
        //--------------------------------------------------------------------------------------------------------------

        bool IsBitcoinAddress(const CString& Addr) {
            return IsLegacyAddress(Addr) || IsSegWitAddress(Addr);
        }
        //--------------------------------------------------------------------------------------------------------------

        wallet::ek_token token_new(const std::string &passphrase, const data_chunk &salt) {
#ifdef WITH_ICU
            if (salt.size() < ek_salt_size)
                throw Delphi::Exception::Exception(BX_TOKEN_NEW_SHORT_SALT);

            encrypted_token token = {};
            ek_entropy bytes = {};
            std::copy(salt.begin(), salt.begin() + bytes.size(), bytes.begin());
            create_token(token, passphrase, bytes);

            return ek_token(token);
#else
            throw Delphi::Exception::Exception(BX_TOKEN_NEW_REQUIRES_ICU);
#endif
        }
        //--------------------------------------------------------------------------------------------------------------

        wallet::ek_public ek_public(const ek_token &token, const data_chunk &seed, uint8_t version) {
#ifdef WITH_ICU
            if (seed.size() < ek_seed_size)
                throw Delphi::Exception::Exception(BX_EK_ADDRESS_SHORT_SEED);

            ek_seed bytes = {};
            std::copy(seed.begin(), seed.begin() + ek_seed_size, bytes.begin());
            const bool compressed = true;

            encrypted_private unused1 = {};
            encrypted_public key = {};
            ec_compressed unused2;

            // This cannot fail because the token has been validated.
            /* bool */ create_key_pair(unused1, key, unused2, token, bytes, version, compressed);

            return wallet::ek_public(key);
#else
            throw Delphi::Exception::Exception(BX_TOKEN_NEW_REQUIRES_ICU);
#endif
        }
        //--------------------------------------------------------------------------------------------------------------

        wallet::ec_public ek_public_to_ec(const std::string &passphrase, const wallet::ek_public &key) {
#ifdef WITH_ICU
            bool compressed;
            uint8_t version;
            ec_compressed point;

            if (!decrypt(point, version, compressed, key, passphrase))
                throw Delphi::Exception::Exception(BX_EK_PUBLIC_TO_EC_INVALID_PASSPHRASE);

            return ec_public(point, compressed);
#else
            throw Delphi::Exception::Exception(BX_TOKEN_NEW_REQUIRES_ICU);
#endif
        }
        //--------------------------------------------------------------------------------------------------------------

        wallet::payment_address ek_address(const wallet::ek_token &token, const data_chunk &seed, uint8_t version) {
#ifdef WITH_ICU
            if (seed.size() < ek_seed_size)
                throw Delphi::Exception::Exception(BX_EK_ADDRESS_SHORT_SEED);

            ek_seed bytes = {};
            std::copy(seed.begin(), seed.begin() + ek_seed_size, bytes.begin());
            const bool compressed = true;

            ec_compressed point;
            encrypted_private unused = {};

            // This cannot fail because the token has been validated.
            /* bool */ create_key_pair(unused, point, token, bytes, version, compressed);
            const payment_address address({ point, compressed }, version);

            return address;
#else
            throw Delphi::Exception::Exception(BX_TOKEN_NEW_REQUIRES_ICU);
#endif
        }
        //--------------------------------------------------------------------------------------------------------------

        wallet::hd_private hd_new(const data_chunk &seed, uint64_t prefixes) {
            if (seed.size() < minimum_seed_size)
                throw Delphi::Exception::Exception(BX_HD_NEW_SHORT_SEED);

            const bc::wallet::hd_private private_key(seed, prefixes);
            if (!private_key)
                throw Delphi::Exception::Exception(BX_HD_NEW_INVALID_KEY);

            return private_key;
        }
        //--------------------------------------------------------------------------------------------------------------

        wallet::ec_private hd_to_ec_private(const wallet::hd_key &key, uint64_t prefixes) {
            const auto key_prefixes = from_big_endian_unsafe<uint64_t>(key.begin());
            if (key_prefixes != prefixes)
                throw Delphi::Exception::Exception("HD Key prefixes version error.");

            // Create the private key from hd_key and the public version.
            const auto private_key = bc::wallet::hd_private(key, prefixes);
            if (!private_key)
                throw Delphi::Exception::Exception(BX_HD_NEW_INVALID_KEY);

            return private_key.secret();
        }
        //--------------------------------------------------------------------------------------------------------------

        wallet::ec_public hd_to_ec_public(const wallet::hd_key &key, uint32_t prefix) {
            const auto key_prefix = from_big_endian_unsafe<uint32_t>(key.begin());
            if (key_prefix != prefix)
                throw Delphi::Exception::Exception("HD Key prefix version error.");

            // Create the public key from hd_key and the public version.
            const auto public_key = bc::wallet::hd_public(key, prefix);
            if (!public_key)
                throw Delphi::Exception::Exception(BX_HD_NEW_INVALID_KEY);

            return wallet::ec_public(public_key);
        }
        //--------------------------------------------------------------------------------------------------------------

#ifdef BITCOIN_VERSION_4

        std::string script_to_address(const system::script &script, uint8_t version) {
            const auto& address = payment_address(script, version);
            return address.encoded();
        }
        //--------------------------------------------------------------------------------------------------------------

        std::string script_to_address(const std::string &script, uint8_t version) {
            return script_to_address(system::script(script), version);
        }
        //--------------------------------------------------------------------------------------------------------------

        std::string address_to_key(const wallet::payment_address &address) {
            return encode_base16(sha256_hash(address.output_script().to_data(false)));
        }
        //--------------------------------------------------------------------------------------------------------------

        uint64_t fetch_balance(const wallet::payment_address &address) {
            uint64_t result = 0;

            client::connection_settings connection = {};
            connection.retries = 5;
            connection.server = config::endpoint(BitcoinConfig.endpoint);

            client::obelisk_client client(connection.retries);

            if (!client.connect(connection)) {
                throw Delphi::Exception::Exception(BX_CONNECTION_FAILURE, connection.server);
            }
/*
            auto on_done = [&result](const code& ec, const history::list& rows)
            {
                uint64_t balance = balancer(rows);
                result = encode_base10(balance, 8);
            };
*/
            auto handler = [&result](const code& ec, const history::list& history) {
                if (ec != error::success)
                    throw Delphi::Exception::ExceptionFrm("Failed to retrieve history: %s", ec.message().c_str());
                else
                    for (const auto& row: history)
                        result += row.value;
            };

            const auto key = sha256_hash(address.output_script().to_data(false));

            client.blockchain_fetch_history4(handler, key);
            client.wait();

            return result;
        }
        //--------------------------------------------------------------------------------------------------------------
#else
        chain::script get_witness_script(const ec_public &key1, const ec_public &key2, const ec_public &key3) {
            //make key list
            point_list keys {key1.point(), key2.point(), key3.point()};
            //create 2/3 multisig script
            return chain::script::to_pay_multisig_pattern(2u, keys);
        }
        //--------------------------------------------------------------------------------------------------------------

        chain::script get_redeem_script(const ec_public &key1, const ec_public &key2, const ec_public &key3) {
            //create 2/3 multisig script
            const auto& multisig = get_witness_script(key1, key2, key3);

            //sha256 the script
            const auto& multisig_hash = to_chunk(sha256_hash(multisig.to_data(false)));

            //redeem script
            operation::list redeemscript_ops {operation(opcode(0)), operation(multisig_hash)};

            return script(redeemscript_ops);
        }
        //--------------------------------------------------------------------------------------------------------------

        std::string script_to_address(const chain::script &script, uint8_t version) {
            const auto& address = payment_address(script, version);
            return address.encoded();
        }
        //--------------------------------------------------------------------------------------------------------------

        std::string script_to_address(const std::string &script, uint8_t version) {
            return script_to_address(Bitcoin::script(script), version);
        }
        //--------------------------------------------------------------------------------------------------------------

        void code_to_json(const code& error, CJSON &Result) {
            CString json;
            if (error.message().empty()) {
                json.Format(R"({"error": {"code": %u, "message": null}})", error.value());
            } else {
                json.Format(R"({"error": {"code": %u, "message": "%s"}})", error.value(), error.message().c_str());
            }
            Result << json;
        };
        //--------------------------------------------------------------------------------------------------------------

        uint64_t balancer(const history::list& rows) {
            uint64_t unspent_balance = 0;

            for (const auto& row: rows) {
                // spend unconfirmed (or no spend attempted)
                if (row.spend.hash() == null_hash)
                    unspent_balance += row.value;
            }

            return unspent_balance;
        }
        //--------------------------------------------------------------------------------------------------------------

        uint64_t fetch_balance(const wallet::payment_address &address) {
            uint64_t result = 0;

            client::connection_type connection = {};
            connection.retries = 0;
            connection.timeout_seconds = 5;
            connection.server = config::endpoint(BitcoinConfig.endpoint);

            client::obelisk_client client(connection);

            if (!client.connect(connection)) {
                throw Delphi::Exception::Exception(BX_CONNECTION_FAILURE, connection.server);
            }

            auto on_reply = [&result](const history::list& rows) {
                result = balancer(rows);
            };

            auto on_error = [](const code& error) {
//                if (error != error::success)
//                    throw Delphi::Exception::ExceptionFrm("Failed to retrieve history: %s", message.c_str());
            };

            client.blockchain_fetch_history3(on_error, on_reply, address);
            client.wait();

            return result;
        }
        //--------------------------------------------------------------------------------------------------------------

        void row_to_json(const chain::history& row, CJSONValue &Value) {
            auto& object = Value.Object();

            CJSONValue received;
            CJSONValue spent;

            // missing output implies output cut off by server's history threshold
            if (row.output.hash() != null_hash) {
                received.Object().AddPair("hash", hash256(row.output.hash()).to_string().c_str());

                // zeroized received.height implies output unconfirmed (in mempool)
                if (row.output_height != 0)
                    received.Object().AddPair("height", (int) row.output_height);

                received.Object().AddPair("index", (int) row.output.index());

                object.AddPair("received", received);
            }

            // missing input implies unspent
            if (row.spend.hash() != null_hash)
            {
                spent.Object().AddPair("hash", hash256(row.spend.hash()).to_string().c_str());

                // zeroized input.height implies spend unconfirmed (in mempool)
                if (row.spend_height != 0)
                    spent.Object().AddPair("height", (int) row.spend_height);

                spent.Object().AddPair("index", (int) row.spend.index());
                object.AddPair("spent", spent);
            }

            object.AddPair("value", (int) row.value);
        }
        //--------------------------------------------------------------------------------------------------------------

        void transfers(const history::list& rows, CJSON& Result) {
            CJSONValue Value(jvtArray);
            for (const auto& row: rows) {
                CJSONValue Object;
                row_to_json(row, Object);
                Value.Array().Add(Object);
            }
            Result.Object().AddPair("transfers", Value);
        }
        //--------------------------------------------------------------------------------------------------------------

        void fetch_history(const wallet::payment_address &address, CJSON& Result) {
            client::connection_type connection = {};
            connection.retries = 0;
            connection.timeout_seconds = 5;
            connection.server = config::endpoint(BitcoinConfig.endpoint);

            client::obelisk_client client(connection);

            if (!client.connect(connection)) {
                throw Delphi::Exception::Exception(BX_CONNECTION_FAILURE, connection.server);
            }

            auto on_reply = [&Result](const history::list& rows) {
                transfers(rows, Result);
            };

            auto on_error = [&Result](const code& error) {
                code_to_json(error, Result);
            };

            client.blockchain_fetch_history3(on_error, on_reply, address);
            client.wait();
        }
        //--------------------------------------------------------------------------------------------------------------

        void header_to_json(const header& header, CJSON& Result) {
            const chain::header& block_header = header;

            CJSONValue Header(jvtObject);
            auto& Object = Header.Object();

            Object.AddPair("bits", (int) block_header.bits());
            Object.AddPair("hash", hash256(block_header.hash()).to_string().c_str());
            Object.AddPair("merkle_tree_hash", hash256(block_header.merkle()).to_string().c_str());
            Object.AddPair("nonce", (int) block_header.nonce());
            Object.AddPair("previous_block_hash", hash256(block_header.previous_block_hash()).to_string().c_str());
            Object.AddPair("time_stamp", (int) block_header.timestamp());
            Object.AddPair("version", (int) block_header.version());

            Result.Object().AddPair("header", Header);
        }
        //--------------------------------------------------------------------------------------------------------------

        void fetch_header(uint32_t height, CJSON &Result) {
            client::connection_type connection = {};
            connection.retries = 0;
            connection.timeout_seconds = 5;
            connection.server = config::endpoint(BitcoinConfig.endpoint);

            client::obelisk_client client(connection);

            if (!client.connect(connection)) {
                throw Delphi::Exception::Exception(BX_CONNECTION_FAILURE, connection.server);
            }

            auto on_reply = [&Result](const chain::header& header) {
                header_to_json(header, Result);
            };

            auto on_error = [&Result](const code& error) {
                code_to_json(error, Result);
            };

            client.blockchain_fetch_block_header(on_error, on_reply, height);
            client.wait();
        }
        //--------------------------------------------------------------------------------------------------------------

        void fetch_header(const hash_digest& hash, CJSON &Result) {
            client::connection_type connection = {};
            connection.retries = 0;
            connection.timeout_seconds = 5;
            connection.server = config::endpoint(BitcoinConfig.endpoint);

            client::obelisk_client client(connection);

            if (!client.connect(connection)) {
                throw Delphi::Exception::Exception(BX_CONNECTION_FAILURE, connection.server);
            }

            auto on_reply = [&Result](const chain::header& header) {
                header_to_json(header, Result);
            };

            auto on_error = [&Result](const code& error) {
                code_to_json(error, Result);
            };

            client.blockchain_fetch_block_header(on_error, on_reply, hash);
            client.wait();
        }
        //--------------------------------------------------------------------------------------------------------------

        void send_tx(const chain::transaction& tx, CJSON& Result) {
            client::connection_type connection = {};
            connection.retries = 0;
            connection.timeout_seconds = 5;
            connection.server = config::endpoint(BitcoinConfig.endpoint);

            client::obelisk_client client(connection);

            if (!client.connect(connection)) {
                throw Delphi::Exception::Exception(BX_CONNECTION_FAILURE, connection.server);
            }

            auto on_done = [&Result](const code& error) {
                code_to_json(error, Result);
            };

            auto on_error = [&Result](const code& error) {
                code_to_json(error, Result);
            };

            // This validates the tx, submits it to local tx pool, and notifies peers.
            client.transaction_pool_broadcast(on_error, on_done, tx);
            client.wait();
        }
        //--------------------------------------------------------------------------------------------------------------

        void send_tx(const std::string &hex, CJSON &Result) {
            chain::transaction tx;

            data_chunk data;
            if (!decode_base16(data, hex))
                throw Delphi::Exception::Exception("Invalid converting a hex string into transaction data.");

            if (!tx.from_data(data, true, true))
                throw Delphi::Exception::Exception("Invalid transaction data.");

            send_tx(tx, Result);
        }

#endif

        void CBalance::Clear() {
            m_Transfers = 0;
            m_Height = 0;

            m_Received = 0;
            m_Spent = 0;

            m_TimeStamp = 0;

            m_History.Clear();
            m_Header.Clear();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CBalance::Fetch(const CString &Address) {

            Clear();

            const wallet::payment_address address(std::string(Address.c_str()));

            fetch_history(address, m_History);

            const auto& transfers = m_History["transfers"].Array();
            m_Transfers = transfers.Count();

            if (m_Transfers == 0)
                return;

            for (int i = 0; i < transfers.Count(); i++) {
                const auto& transfer = transfers[i].AsObject();

                if (transfer["received"].ValueType() == jvtObject) {
                    m_Received += transfer["value"].AsInteger();
                    m_Height = transfer["received"]["height"].AsInteger();
                }

                if (transfer["spent"].ValueType() == jvtObject) {
                    m_Spent += transfer["value"].AsInteger();
                }
            }

            if (m_Height != 0) {
                fetch_header(m_Height, m_Header);
                time_t timestamp = m_Header["header"]["time_stamp"].AsInteger();
                m_TimeStamp = SystemTimeToDateTime(localtime(&timestamp), 0);
            }
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CWitness --------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CWitness::CWitness(): CWitness(2u) {

        }
        //--------------------------------------------------------------------------------------------------------------

        CWitness::CWitness(uint8_t signatures): m_signatures(signatures), m_hash() {

        }
        //--------------------------------------------------------------------------------------------------------------

        CWitness::CWitness(const ec_private &key1, const ec_private &key2): CWitness() {
            m_secrets.push_back(key1);
            m_secrets.push_back(key2);

            m_keys.push_back(key1.to_public());
            m_keys.push_back(key2.to_public());

            bind();
        }
        //--------------------------------------------------------------------------------------------------------------

        CWitness::CWitness(const ec_private &key1, const ec_private &key2, const ec_public &key3): CWitness() {
            m_secrets.push_back(key1);
            m_secrets.push_back(key2);

            m_keys.push_back(key1.to_public());
            m_keys.push_back(key2.to_public());
            m_keys.push_back(key3);

            bind();
        }
        //--------------------------------------------------------------------------------------------------------------

        CWitness::CWitness(const ec_public &key1, const ec_public &key2, const ec_public &key3): CWitness() {
            m_keys.push_back(key1);
            m_keys.push_back(key2);
            m_keys.push_back(key3);

            bind();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWitness::bind_script() {
            point_list points;
            for (const ec_public &key : m_keys)
                points.push_back(key.point());
            m_script = chain::script::to_pay_multisig_pattern(m_signatures, points);
            m_hash = sha256_hash(m_script.to_data(false));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWitness::bind_embedded() {
            operation::list operations { operation(opcode::push_size_0), operation(to_chunk(m_hash)) };
            m_embedded = chain::script(operations);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWitness::bind() {
            bind_script();
            bind_embedded();
        }
        //--------------------------------------------------------------------------------------------------------------

        const hash_digest& CWitness::hash() const {
            return m_hash;
        }
        //--------------------------------------------------------------------------------------------------------------

        payment_address CWitness::to_address(uint8_t version) const {
            return payment_address(m_embedded, version);
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- raw -------------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        raw::raw(): value_() {

        }
        //--------------------------------------------------------------------------------------------------------------

        raw::raw(const std::string& hexcode)
        {
            std::stringstream(hexcode) >> *this;
        }
        //--------------------------------------------------------------------------------------------------------------

        raw::raw(const data_chunk& value): value_(value) {

        }
        //--------------------------------------------------------------------------------------------------------------

        raw::raw(const raw& other): raw(other.value_) {

        }
        //--------------------------------------------------------------------------------------------------------------

        raw::operator const data_chunk&() const {
            return value_;
        }
        //--------------------------------------------------------------------------------------------------------------

        raw::operator data_slice() const {
            return value_;
        }
        //--------------------------------------------------------------------------------------------------------------

        std::istream& operator>>(std::istream& input, raw& argument) {
            std::istreambuf_iterator<char> first(input), last;
            argument.value_.assign(first, last);
            return input;
        }
        //--------------------------------------------------------------------------------------------------------------

        std::ostream& operator<<(std::ostream& output, const raw& argument) {
            std::ostreambuf_iterator<char> iterator(output);
            std::copy(argument.value_.begin(), argument.value_.end(), iterator);
            return output;
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- wrapper ---------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        wrapper::wrapper()
                : value_()
        {
        }

        wrapper::wrapper(const std::string& wrapped)
        {
            std::stringstream(wrapped) >> *this;
        }

        wrapper::wrapper(const data_chunk& wrapped)
                : wrapper(encode_base16(wrapped))
        {
        }

        wrapper::wrapper(const wallet::wrapped_data& wrapped)
                : value_(wrapped)
        {
        }

        wrapper::wrapper(const wallet::payment_address& address)
                : wrapper(encode_base16(address.to_payment()))
        {
        }

        wrapper::wrapper(uint8_t version, const data_chunk& payload)
                : wrapper(wallet::wrapped_data{ version, payload, 0 })
        {
        }

        wrapper::wrapper(const wrapper& other)
                : value_(other.value_)
        {
        }

        const data_chunk wrapper::to_data() const
        {
            return wrap(value_);
        }

        wrapper::operator const wallet::wrapped_data&() const
        {
            return value_;
        }

        std::istream& operator>>(std::istream& input, wrapper& argument)
        {
            std::string hexcode;
            input >> hexcode;

            // The checksum is validated here.
            if (!unwrap(argument.value_, base16(hexcode)))
            {
                throw Delphi::Exception::Exception("Error: Payment address is invalid.");
            }

            return input;
        }

        std::ostream& operator<<(std::ostream& output, const wrapper& argument)
        {
            // The checksum is calculated here (value_ checksum is ignored).
            const auto bytes = wrap(argument.value_);
            output << base16(bytes);
            return output;
        }

        //--------------------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        // message_signature format is currently private to bx.
        static bool decode_signature(wallet::message_signature& signature, const std::string& encoded) {
            // There is no bc::decode_base64 array-based override.
            data_chunk decoded;
            if (!decode_base64(decoded, encoded) ||
                (decoded.size() != wallet::message_signature_size))
                return false;

            std::copy(decoded.begin(), decoded.end(), signature.begin());
            return true;
        }
        //--------------------------------------------------------------------------------------------------------------

        static std::string encode_signature(const wallet::message_signature& signature) {
            return encode_base64(signature);
        }
        //--------------------------------------------------------------------------------------------------------------

        signature::signature(): value_() {

        }
        //--------------------------------------------------------------------------------------------------------------

        signature::signature(const std::string& hexcode): value_() {
            std::stringstream(hexcode) >> *this;
        }
        //--------------------------------------------------------------------------------------------------------------

        signature::signature(const wallet::message_signature& value): value_(value) {

        }
        //--------------------------------------------------------------------------------------------------------------

        signature::signature(const signature& other): signature(other.value_) {

        }
        //--------------------------------------------------------------------------------------------------------------

        signature::operator wallet::message_signature&() {
            return value_;
        }
        //--------------------------------------------------------------------------------------------------------------

        signature::operator const wallet::message_signature&() const {
            return value_;
        }
        //--------------------------------------------------------------------------------------------------------------

        std::string signature::encoded() const {
            return encode_signature(value_);
        }
        //--------------------------------------------------------------------------------------------------------------

        std::istream& operator>>(std::istream& input, signature& argument) {
            std::string hexcode;
            input >> hexcode;

            if (!decode_signature(argument.value_, hexcode)) {
                throw Delphi::Exception::Exception("Error: Decode signature.");
            }

            return input;
        }
        //--------------------------------------------------------------------------------------------------------------

        std::ostream& operator<<(std::ostream& output, const signature& argument) {
            output << encode_signature(argument.value_);
            return output;
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- script ----------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        script::script(): value_()
        {
        }

        script::script(const std::string& mnemonic)
        {
            std::stringstream(mnemonic) >> *this;
        }

        script::script(const chain::script& value)
                : value_(value)
        {
        }

        script::script(const data_chunk& value)
        {
            value_.from_data(value, false);
        }

        script::script(const std::vector<std::string>& tokens)
        {
            const auto mnemonic = join(tokens);
            std::stringstream(mnemonic) >> *this;
        }

        script::script(const script& other)
                : script(other.value_)
        {
        }

        const data_chunk script::to_data() const
        {
            return value_.to_data(false);
        }

        const std::string script::to_string() const
        {
            static constexpr auto flags = machine::rule_fork::all_rules;
            return value_.to_string(flags);
        }

        script::operator const chain::script&() const
        {
            return value_;
        }

        std::istream& operator>>(std::istream& input, script& argument)
        {
            std::istreambuf_iterator<char> end;
            std::string mnemonic(std::istreambuf_iterator<char>(input), end);

            // Test for invalid result sentinel.
            if (!argument.value_.from_string(mnemonic) && mnemonic.length() > 0) {
                throw Delphi::Exception::Exception("Error: Invalid script argument.");
            }

            return input;
        }

        std::ostream& operator<<(std::ostream& output, const script& argument)
        {
            static constexpr auto flags = machine::rule_fork::all_rules;
            output << argument.value_.to_string(flags);
            return output;
        }

    } // namespace Bitcoin

} // namespace Apostol
}