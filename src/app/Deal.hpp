/*++

Library name:

  apostol-core

Module Name:

  Deal.hpp

Notices:

  Apostol Core (Deal)

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef APOSTOL_DEAL_HPP
#define APOSTOL_DEAL_HPP

#include "yaml-cpp/yaml.h"
//----------------------------------------------------------------------------------------------------------------------

#ifdef  __cplusplus
extern "C++" {
#endif // __cplusplus

namespace Apostol {

    namespace Deal {

        CString UTCFormat(const CString& Value);
        CString BTCFormat(const CString& Value);
        //--------------------------------------------------------------------------------------------------------------

        double BTCToDouble(const CString &Value);
        CString DoubleToBTC(const double &Value, const CString &Format = _T("%f BTC"));

        CDateTime StringToDate(const CString &Value, const CString &Format = _T("%04d-%02d-%02d %02d:%02d:%02d"));
        CString DateToString(const CDateTime &Value, const CString &Format = _T("%Y-%m-%d %H:%M:%S UTC"));

        //--------------------------------------------------------------------------------------------------------------

        //-- CRating ---------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        typedef struct CRating {

            int Count;
            int Positive;

            CRating(): Count(0), Positive(0) {

            }

            CRating(const CRating &Rating): CRating() {
                Assign(Rating);
            }

            void Assign(const CRating& Rating) {
                this->Count = Rating.Count;
                this->Positive = Rating.Positive;
            };

            int Compare(const CRating& Rating) const {
                if (this->Count == Rating.Count)
                    return 0;
                if (this->Count > Rating.Count)
                    return 1;
                return -1;
            };

            void ToString(CString &String) const {
                String.Clear();

                int q = 1;
                int c = Count;

                while ((c = div(c, 10).quot > 10))
                    q++;

                c = c * (int) pow(10, q);

                if (Count == c)
                    String.Format("%d, %d%%", c, Positive);
                else
                    String.Format("%d+, %d%%", c, Positive);
            }

            void Parse(const CString &Value);

            CString GetString() const {
                CString S;
                ToString(S);
                return S;
            };

            bool operator== (const CRating& R) const {
                return Compare(R) == 0;
            };

            bool operator!= (const CRating& R) const {
                return Compare(R) != 0;
            };

            bool operator< (const CRating& R) const {
                return Compare(R) < 0;
            };

            bool operator<= (const CRating& R) const {
                return Compare(R) <= 0;
            };

            bool operator> (const CRating& R) const {
                return Compare(R) > 0;
            };

            bool operator>= (const CRating& R) const {
                return Compare(R) >= 0;
            };

            CRating& operator= (const CRating& R) {
                if (this != &R) {
                    Assign(R);
                }
                return *this;
            };

            CRating& operator= (const CString& S) {
                Parse(S);
                return *this;
            };

            CRating& operator<< (const CString& S) {
                Parse(S);
                return *this;
            };

            friend CString& operator>> (const CRating &LS, CString &RS) {
                LS.ToString(RS);
                return RS;
            }

        } CRating;

        //--------------------------------------------------------------------------------------------------------------

        //-- CDealData -------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        enum CDealType { dtPrepayment = 0, dtPostpayment };
        //--------------------------------------------------------------------------------------------------------------

        enum CFeedBackStatus { fsNegative = -1, fsNeutral = 0, fsPositive = 1 };
        //--------------------------------------------------------------------------------------------------------------

        enum CDealOrder { doCreate = 0, doCreated, doPay, doPaid, doComplete, doCompleted, doCancel, doCanceled,
            doExecute, doExecuted, doDelete, doDeleted, doFeedback };
        //--------------------------------------------------------------------------------------------------------------

        typedef struct DealData {

            CDealOrder Order = doCreate;
            CDealType Type = dtPrepayment;

            // Hidden value
            CString Code;

            CString At;
            CString Date;

            struct seller {
                CString Address;
                CRating Rating;
                CString Signature;
            } Seller;

            struct customer {
                CString Address;
                CRating Rating;
                CString Signature;
            } Customer;

            struct payment {
                CString Address;
                CString Until;
                CString Sum;
            } Payment;

            struct feedback {
                CString LeaveBefore;
                CFeedBackStatus Status;
                CString Comments;
            } FeedBack;

            struct transaction {
                CString Fee;
                CString Key;
                CString Hex;
                TList<CString> Signatures;
            } Transaction;

            struct error {
                CString Message;
            } Error;

            CString GetStringData() const;

        } CDealData, *PDealData;

        //--------------------------------------------------------------------------------------------------------------

        //-- CDeal -----------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CDeal {
        private:

            CDealData m_Data;

            CString GetHashData();

        protected:

            std::string get_hash();
            std::string get_code();

            wallet::ec_public to_public_ek(uint8_t version = payment_address::mainnet_p2kh);
            wallet::payment_address to_address_ek(uint8_t version = payment_address::mainnet_p2kh);

            wallet::ec_public to_public_hd(uint64_t prefixes = hd_private::mainnet);
            wallet::payment_address to_address_hd(uint64_t prefixes = hd_private::mainnet);

            std::string get_payment_ek(const std::string &key1, const std::string &key2, std::string &key3,
                                       uint8_t version_key = payment_address::mainnet_p2kh,
                                       uint8_t version_script = payment_address::mainnet_p2sh);

            std::string get_payment_hd(const std::string &key1, const std::string &key2, std::string &key3,
                                       uint64_t version_key = hd_private::mainnet,
                                       uint8_t version_script = payment_address::mainnet_p2sh);

        public:

            CDeal();

            explicit CDeal(const YAML::Node &node);

            void Parse(const YAML::Node &node);

            CString GetHash();
            CString GetCode();

            CString GetPaymentEK(const CString &Key1, const CString &Key2, CString &Key3,
                                 uint8_t version_key = payment_address::mainnet_p2kh,
                                 uint8_t version_script = payment_address::mainnet_p2sh);

            CString GetPaymentHD(const CString &Key1, const CString &Key2, CString &Key3,
                                 uint64_t version_key = hd_private::mainnet,
                                 uint8_t version_script = payment_address::mainnet_p2sh);

            static CDealOrder StringToOrder(const CString &Value);
            static CString OrderToString(CDealOrder Order);

            static CDealType StringToType(const CString &Value);
            static CString TypeToString(CDealType Type);

            static CFeedBackStatus StringToFeedBackStatus(const CString &Value);
            static CString FeedBackStatusToString(CFeedBackStatus Status);

            CDealOrder Order() const { return m_Data.Order; }

            CDealData &Data() { return m_Data; }
            const CDealData &Data() const { return m_Data; }

            static void SetOrder(YAML::Node &Node, CDealOrder Order);

            static void AddFeedback(YAML::Node &Node, CFeedBackStatus Status, const CString &Comments);
            static void AddTransaction(YAML::Node &Node, const transaction &tx, const CPrivateList &Secrets);
            static void AddError(YAML::Node &Node, const CString &Message);

            CDeal& operator<< (const YAML::Node &node) {
                Parse(node);
                return *this;
            }

        };
        //--------------------------------------------------------------------------------------------------------------

    }
}

using namespace Apostol::Deal;
#ifdef  __cplusplus
}
#endif // __cplusplus

#endif //APOSTOL_DEAL_HPP
