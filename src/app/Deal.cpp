/*++

Program name:

  BitDeals

Module Name:

  Deal.cpp

Notices:

  Apostol Core (Deal)

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#include "Core.hpp"
#include "Deal.hpp"
//----------------------------------------------------------------------------------------------------------------------

#ifdef  __cplusplus
extern "C++" {
#endif // __cplusplus

namespace Apostol {

    namespace Deal {

        CString UTCFormat(const CString& Value) {
            return Value.SubString(0, 19) << " UTC";
        }
        //--------------------------------------------------------------------------------------------------------------

        CString BTCFormat(const CString& Value) {
            if (Value.Find(BitcoinConfig.Symbol) == CString::npos)
                return Value.TrimRight('0') << " " << BitcoinConfig.Symbol;
            return Value.TrimRight('0');
        }
        //--------------------------------------------------------------------------------------------------------------

        double BTCToDouble(const CString &Value) {
            const size_t Pos = Value.Find(BitcoinConfig.Symbol);
            if (Pos == CString::npos)
                return StrToDouble(Value.c_str());
            return StrToDouble(Value.SubString(0, Pos).TrimRight().c_str());
        }
        //--------------------------------------------------------------------------------------------------------------

        CString DoubleToBTC(const double &Value, const CString &Format) {
            TCHAR Buffer[25] = {0};
            FloatToStr(Value, Buffer, sizeof(Buffer), Format.c_str());
            return Buffer;
        }
        //--------------------------------------------------------------------------------------------------------------

        CDateTime StringToDate(const CString &Value, const CString &Format) {
            return StrToDateTimeDef(Value.c_str(), 0, Format.c_str());
        }
        //--------------------------------------------------------------------------------------------------------------

        CString DateToString(const CDateTime &Value, const CString &Format) {
            TCHAR Buffer[25] = {0};
            DateTimeToStr(Value, Buffer, sizeof(Buffer), Format.c_str());
            return Buffer;
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- DealData --------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CString DealData::GetStringData() const {
            CString Data;

            Data.Format("Type: %d;", (int) Type);
            Data.Format("URL: %s;", At.c_str());
            Data.Format("Date: %s;", Date.c_str());
            Data.Format("Seller: %s;", Seller.Address.c_str());
            Data.Format("Customer: %s;", Customer.Address.c_str());
            Data.Format("Until: %s;", Payment.Until.c_str());
            Data.Format("Sum: %s;", Payment.Sum.c_str());
            Data.Format("LeaveBefore: %s", FeedBack.LeaveBefore.c_str());

            return Data;
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CRating ---------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        void CRating::Parse(const CString &Value) {
            size_t Pos = 0;

            TCHAR ch = Value.at(Pos++);
            if (Value.Size() < 4 || !IsNumeral(ch))
                throw ExceptionFrm("Invalid rating value.");

            CString Item;
            while (!IsCtl(ch)) {
                if (ch == '+' && !Item.IsEmpty()) {
                    Count = StrToIntDef(Item.c_str(), 0);
                    Count++;
                    Item.Clear();
                } else if (ch == ',' && !Item.IsEmpty()) {
                    Count = StrToIntDef(Item.c_str(), 0);
                    Item.Clear();
                } else if (ch == '%' && !Item.IsEmpty()) {
                    Positive = StrToIntDef(Item.c_str(), 0);
                    Item.Clear();
                } else {
                    if (!(ch == ' ' || ch == ',')) {
                        if (!IsNumeral(ch))
                            throw ExceptionFrm("Invalid rating value.");
                        Item << ch;
                    }
                }

                ch = Value.at(Pos++);
            }
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CDeal -----------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CDeal::CDeal() {
            m_Data.Order = doCreate;
            m_Data.FeedBack.Status = fsPositive;
        }
        //--------------------------------------------------------------------------------------------------------------

        CDeal::CDeal(const YAML::Node &node) : CDeal() {
            Parse(node);
        }
        //--------------------------------------------------------------------------------------------------------------

        CDealOrder CDeal::StringToOrder(const CString &Value) {
            const CString &S = Value.Lower();

            if (S == "create") {
                return doCreate;
            } else if (S == "created") {
                return doCreated;
            } else if (S == "pay") {
                return doPay;
            } else if (S == "paid") {
                return doPaid;
            } else if (S == "complete") {
                return doComplete;
            } else if (S == "completed") {
                return doCompleted;
            } else if (S == "cancel") {
                return doCancel;
            } else if (S == "canceled") {
                return doCanceled;
            } else if (S == "execute") {
                return doExecute;
            } else if (S == "executed") {
                return doExecuted;
            } else if (S == "delete") {
                return doDelete;
            } else if (S == "deleted") {
                return doDeleted;
            } else if (S == "fail") {
                return doFail;
            } else if (S == "failed") {
                return doFailed;
            } else if (S == "feedback") {
                return doFeedback;
            } else {
                throw ExceptionFrm("Invalid deal order value: \"%s\".", Value.c_str());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CDeal::OrderToString(CDealOrder Order) {
            switch (Order) {
                case doCreate:
                    return "Create";
                case doCreated:
                    return "Created";
                case doPay:
                    return "Pay";
                case doPaid:
                    return "Paid";
                case doComplete:
                    return "Complete";
                case doCompleted:
                    return "Completed";
                case doCancel:
                    return "Cancel";
                case doCanceled:
                    return "Canceled";
                case doExecute:
                    return "Execute";
                case doExecuted:
                    return "Executed";
                case doDelete:
                    return "Delete";
                case doDeleted:
                    return "Deleted";
                case doFail:
                    return "Fail";
                case doFailed:
                    return "Failed";
                case doFeedback:
                    return "Feedback";
                default:
                    return "Unknown";
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        CDealType CDeal::StringToType(const CString &Value) {
            const auto &status = Value.Lower();

            if (status.Find("prepaid") != CString::npos || status.Find("prepayment") != CString::npos) {
                return dtPrepaid;
            } else if (status.Find("postpaid") != CString::npos || status.Find("postpayment") != CString::npos) {
                return dtPostpaid;
            } else {
                throw ExceptionFrm(R"(Invalid order type value: "%s".)", status.c_str());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CDeal::TypeToString(CDealType Type) {
            switch (Type) {
                case dtPrepaid:
                    return "Prepaid";
                case dtPostpaid:
                    return "Postpaid";
                default:
                    return "Unknown";
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CDeal::TypeCodeToString(const CString &Code) {
            if (Code == "prepaid.deal") {
                return CString("Prepaid");
            } else if (Code == "postpaid.deal") {
                return CString("Postpaid");
            } else {
                throw ExceptionFrm(R"(Invalid type code value: "%s".)", Code.c_str());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        CFeedBackStatus CDeal::StringToFeedBackStatus(const CString &Value) {
            const CString &Status = Value.Lower();

            if (Status == "negative") {
                return fsNegative;
            } else if (Status == "neutral") {
                return fsNeutral;
            } else if (Status == "positive") {
                return fsPositive;
            } else {
                throw ExceptionFrm(R"(Invalid feedback status value: "%s".)", Status.c_str());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CDeal::FeedBackStatusToString(CFeedBackStatus Status) {
            switch (Status) {
                case fsNegative:
                    return "Negative";
                case fsNeutral:
                    return "Neutral";
                case fsPositive:
                    return "Positive";
                default:
                    return "Unknown";
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CDeal::GetHashData() {
            return m_Data.GetStringData();
        }
        //--------------------------------------------------------------------------------------------------------------

        std::string CDeal::get_hash() {
            const std::string str(GetHashData().c_str());
            return sha256(str);
        }
        //--------------------------------------------------------------------------------------------------------------

        std::string CDeal::get_code() {
            const std::string value(GetHashData().c_str());
            const auto& hex = string_to_hex(value);
            const config::base16 data(hex);
            const auto& hash = ripemd160_hash(sha256_hash(data));
            return encode_base16(hash);
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CDeal::GetCode() {
            return get_code();
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CDeal::GetHash() {
            return sha256(GetHashData());
        }
        //--------------------------------------------------------------------------------------------------------------

        wallet::ec_public CDeal::to_public_ek(uint8_t version) {
            const auto& hash = get_hash();
            const std::string seed(hash.substr(0, 48));
            const std::string salt(hash.substr(48, 16));
            const auto& token = Bitcoin::token_new(hash, base16(salt));
            const auto& key = Bitcoin::ek_public(token, base16(seed), version);
            return ek_public_to_ec(hash, key);
        }
        //--------------------------------------------------------------------------------------------------------------

        wallet::payment_address CDeal::to_address_ek(uint8_t version) {
            const auto& hash = get_hash();
            const std::string seed(hash.substr(0, 48));
            const std::string salt(hash.substr(48, 16));
            const auto& token = Bitcoin::token_new(hash, base16(salt));
            return Bitcoin::ek_address(token, base16(seed), version);
        }
        //--------------------------------------------------------------------------------------------------------------

        wallet::ec_public CDeal::to_public_hd(uint64_t prefixes) {
            const auto& hash = get_hash();
            const auto& key = hd_new(base16(hash), prefixes);
            const auto& public_key = key.to_public();
            return public_key.point();
        }
        //--------------------------------------------------------------------------------------------------------------

        wallet::payment_address CDeal::to_address_hd(uint64_t prefixes) {
            const auto& key = to_public_hd(prefixes);
            return key.to_payment_address();
        }
        //--------------------------------------------------------------------------------------------------------------

        std::string CDeal::get_payment_ek(const std::string &key1, const std::string &key2, std::string &key3,
                uint8_t version_key, uint8_t version_script) {

            CWitness Witness(ec_public(key1), ec_public(key2), key3.empty() ? to_public_ek(version_key) : ec_public(key3));

            if (key3.empty())
                key3 = Witness.keys()[2].encoded();

            const auto& address = Witness.to_address(version_script);

            return address.encoded();
        }
        //--------------------------------------------------------------------------------------------------------------

        std::string CDeal::get_payment_hd(const std::string &key1, const std::string &key2, std::string &key3,
                uint64_t version_key, uint8_t version_script) {

            CWitness Witness(ec_public(key1), ec_public(key2), key3.empty() ? to_public_hd(version_key) : ec_public(key3));

            if (key3.empty())
                key3 = Witness.keys()[2].encoded();

            const auto& address = Witness.to_address(version_script);

            return address.encoded();
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CDeal::GetPaymentEK(const CString &Key1, const CString &Key2, CString &Key3,
                uint8_t version_key, uint8_t version_script) {

            std::string key3(Key3);

            const auto& payment = get_payment_ek(Key1.c_str(), Key2.c_str(), key3, version_key, version_script);

            Key3 = key3;

            return payment;
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CDeal::GetPaymentHD(const CString &Key1, const CString &Key2, CString &Key3, uint64_t version_key,
                uint8_t version_script) {

            std::string key3(Key3);

            const auto& payment = get_payment_hd(Key1.c_str(), Key2.c_str(), key3, version_key, version_script);

            Key3 = key3;

            return payment;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CDeal::Parse(const YAML::Node &node) {

            const auto& deal = node["deal"];

            if (deal) {
                m_Data.Order = StringToOrder(deal["order"].as<std::string>());
                m_Data.Type = StringToType(deal["type"].as<std::string>());

                m_Data.At = deal["at"].as<std::string>();
                m_Data.Date = UTCFormat(deal["date"].as<std::string>());

                const auto& seller = deal["seller"];
                m_Data.Seller.Address = seller["address"].as<std::string>();

                if (!valid_address(m_Data.Seller.Address))
                    throw ExceptionFrm("Invalid seller address: %s.", m_Data.Seller.Address.c_str());

                if (seller["rating"])
                    m_Data.Seller.Rating = seller["rating"].as<std::string>();

                if (seller["signature"])
                    m_Data.Seller.Signature = seller["signature"].as<std::string>();

                const auto& customer = deal["customer"];
                m_Data.Customer.Address = customer["address"].as<std::string>();

                if (!valid_address(m_Data.Customer.Address))
                    throw ExceptionFrm("Invalid customer address: %s.", m_Data.Customer.Address.c_str());

                if (customer["rating"])
                    m_Data.Customer.Rating = customer["rating"].as<std::string>();

                if (customer["signature"])
                    m_Data.Customer.Signature = customer["signature"].as<std::string>();

                const auto& payment = deal["payment"];
                if (payment) {
                    if (payment["address"])
                        m_Data.Payment.Address = payment["address"].as<std::string>();

                    if (payment["until"])
                        m_Data.Payment.Until = UTCFormat(payment["until"].as<std::string>());

                    m_Data.Payment.Sum = BTCFormat(payment["sum"].as<std::string>());
                }

                const auto& feedback = deal["feedback"];
                if (feedback) {
                    if (feedback["leave-before"])
                        m_Data.FeedBack.LeaveBefore = UTCFormat(feedback["leave-before"].as<std::string>());

                    if (feedback["status"])
                        m_Data.FeedBack.Status = StringToFeedBackStatus(feedback["status"].as<std::string>());

                    if (feedback["refund"])
                        m_Data.FeedBack.Refund = feedback["refund"].as<std::string>();

                    if (feedback["comments"])
                        m_Data.FeedBack.Comments = feedback["comments"].as<std::string>();
                }

                const auto& transaction = deal["transaction"];
                if (transaction) {
                    m_Data.Transaction.Hex = transaction["hex"].as<std::string>();
                }

                const auto& error = deal["error"];
                if (error) {
                    m_Data.Error.Message = error["message"].as<std::string>();
                }

                const CDateTime Date = StringToDate(m_Data.Date);

                if (m_Data.Payment.Until.IsEmpty())
                    m_Data.Payment.Until = DateToString(Date + (CDateTime) 3600 / SecsPerDay); // 60 min

                if (m_Data.FeedBack.LeaveBefore.IsEmpty())
                    m_Data.FeedBack.LeaveBefore = DateToString(Date + 1);

                m_Data.Code = GetCode();
            } else
                throw ExceptionFrm("Invalid YAML format: Need node \"deal\".");
        }
        //--------------------------------------------------------------------------------------------------------------

        void CDeal::SetOrder(YAML::Node &Node, CDealOrder Order) {
            Node["deal"]["order"] = CDeal::OrderToString(Order).c_str();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CDeal::AddFeedback(YAML::Node &Node, CFeedBackStatus Status, const CString &Comments) {
            YAML::Node Deal = Node["deal"];
            YAML::Node Feedback = Deal["feedback"];
            Feedback["status"] = CDeal::FeedBackStatusToString(Status).c_str();
            if (!Comments.IsEmpty())
                Feedback["comments"] = Comments.c_str();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CDeal::AddTransaction(YAML::Node &Node, const transaction &tx) {

            YAML::Node Deal = Node["deal"];
            YAML::Node Transaction = Deal["transaction"];

            const auto& hex = encode_base16(tx.to_data(true, true));

            Transaction["hex"] = hex;

            DebugMessage("tx: %s\n", hex.c_str());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CDeal::AddError(YAML::Node &Node, const CString &Message) {
            YAML::Node Deal = Node["deal"];
            YAML::Node Error = Deal["error"];
            Error["message"] = Message.c_str();
        }

    }
}

using namespace Apostol::Deal;
#ifdef  __cplusplus
}
#endif // __cplusplus
