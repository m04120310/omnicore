#ifndef OMNICORE_SP_H
#define OMNICORE_SP_H

#include "omnicore/log.h"
#include "omnicore/omnicore.h"
#include "omnicore/persistence.h"

class CBlockIndex;
class uint256;

#include "serialize.h"

#include <boost/filesystem.hpp>

#include <openssl/sha.h>

#include <stdint.h>
#include <stdio.h>

#include <fstream>
#include <map>
#include <string>
#include <utility>
#include <vector>

/** LevelDB based storage for currencies, smart properties and tokens.
 *
 * DB Schema:
 *
 *  Key:
 *      char 'B'
 *  Value:
 *      uint256 hashBlock
 *
 *  Key:
 *      char 's'
 *      uint32_t propertyId
 *  Value:
 *      CMPSPInfo::Entry info
 *
 *  Key:
 *      char 't'
 *      uint256 hashTxid
 *  Value:
 *      uint32_t propertyId
 *
 *  Key:
 *      char 'b'
 *      uint256 hashBlock
 *      uint32_t propertyId
 *  Value:
 *      CMPSPInfo::Entry info
 */
class CMPSPInfo : public CDBBase
{
public:
    struct Entry {
        // common SP data
        std::string issuer;
        uint16_t prop_type;
        uint32_t prev_prop_id;
        std::string category;
        std::string subcategory;
        std::string name;
        std::string url;
        std::string data;
        int64_t num_tokens;

        // crowdsale generated SP
        uint32_t property_desired;
        int64_t deadline;
        uint8_t early_bird;
        uint8_t percentage;

        // closedearly states, if the SP was a crowdsale and closed due to MAXTOKENS or CLOSE command
        bool close_early;
        bool max_tokens;
        int64_t missedTokens;
        int64_t timeclosed;
        uint256 txid_close;

        // other information
        uint256 txid;
        uint256 creation_block;
        uint256 update_block;
        bool fixed;
        bool manual;

        // For crowdsale properties:
        //   txid -> amount invested, crowdsale deadline, user issued tokens, issuer issued tokens
        // For managed properties:
        //   txid -> granted amount, revoked amount
        std::map<uint256, std::vector<int64_t> > historicalData;

        // For License properties
        uint16_t approve_threshold;
        uint16_t approve_count;
        uint16_t reject_count;
        uint32_t money_application;

        Entry();

        ADD_SERIALIZE_METHODS;

        template <typename Stream, typename Operation>
        inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
            READWRITE(issuer);
            READWRITE(prop_type);
            READWRITE(prev_prop_id);
            READWRITE(category);
            READWRITE(subcategory);
            READWRITE(name);
            READWRITE(url);
            READWRITE(data);
            READWRITE(num_tokens);
            READWRITE(property_desired);
            READWRITE(deadline);
            READWRITE(early_bird);
            READWRITE(percentage);
            READWRITE(close_early);
            READWRITE(max_tokens);
            READWRITE(missedTokens);
            READWRITE(timeclosed);
            READWRITE(txid_close);
            READWRITE(txid);
            READWRITE(creation_block);
            READWRITE(update_block);
            READWRITE(fixed);
            READWRITE(manual);
            READWRITE(historicalData);
            READWRITE(approve_threshold);
            READWRITE(approve_count);
            READWRITE(reject_count);
            READWRITE(money_application);
        }

        bool isDivisible() const;
        void print() const;
    };

private:
    // implied version of OMNI and TOMNI so they don't hit the leveldb
    Entry implied_reward_token;
    Entry implied_test_reward_token;

    uint32_t next_spid;
    uint32_t next_test_spid;

public:
    CMPSPInfo(const boost::filesystem::path& path, bool fWipe);
    virtual ~CMPSPInfo();

    /** Extends clearing of CDBBase. */
    void Clear();

    void init(uint32_t nextSPID = 0x3UL, uint32_t nextTestSPID = TEST_ECO_PROPERTY_1);

    uint32_t peekNextSPID(uint8_t ecosystem) const;
    bool updateSP(uint32_t propertyId, const Entry& info);
    uint32_t putSP(uint8_t ecosystem, const Entry& info);
    bool getSP(uint32_t propertyId, Entry& info) const;
    bool hasSP(uint32_t propertyId) const;
    uint32_t findSPByTX(const uint256& txid) const;

    int64_t popBlock(const uint256& block_hash);

    void setWatermark(const uint256& watermark);
    bool getWatermark(uint256& watermark) const;

    void printAll() const;
};

/** A live crowdsale.
 */
class CMPCrowd
{
private:
    uint32_t propertyId;
    int64_t nValue;

    uint32_t property_desired;
    int64_t deadline;
    uint8_t early_bird;
    uint8_t percentage;

    int64_t u_created;
    int64_t i_created;

    uint256 txid; // NOTE: not persisted as it doesnt seem used

    // Schema:
    //   txid -> amount invested, crowdsale deadline, user issued tokens, issuer issued tokens
    std::map<uint256, std::vector<int64_t> > txFundraiserData;

public:
    CMPCrowd();
    CMPCrowd(uint32_t pid, int64_t nv, uint32_t cd, int64_t dl, uint8_t eb, uint8_t per, int64_t uct, int64_t ict);

    uint32_t getPropertyId() const { return propertyId; }

    int64_t getDeadline() const { return deadline; }
    uint32_t getCurrDes() const { return property_desired; }

    void incTokensUserCreated(int64_t amount) { u_created += amount; }
    void incTokensIssuerCreated(int64_t amount) { i_created += amount; }

    int64_t getUserCreated() const { return u_created; }
    int64_t getIssuerCreated() const { return i_created; }

    void insertDatabase(const uint256& txHash, const std::vector<int64_t>& txData);
    std::map<uint256, std::vector<int64_t> > getDatabase() const { return txFundraiserData; }

    std::string toString(const std::string& address) const;
    void print(const std::string& address, FILE* fp = stdout) const;
    void saveCrowdSale(std::ofstream& file, SHA256_CTX* shaCtx, const std::string& addr) const;
};

// Alliance DB
class AllianceInfo : public CDBBase {
public:
    static const unsigned int ALLIANCE_INFO_STATUS_APPROVED = 0;
    static const unsigned int ALLIANCE_INFO_STATUS_PENDING = 1;
    static const unsigned int ALLIANCE_INFO_STATUS_REJECTED = 2;
    struct Entry {
        std::string address;
        std::string name;
        std::string url;
        std::string data;

        // other information
        uint256 txid;
        uint256 creation_block;
        uint256 update_block;

        // For Alliance vote result
        uint32_t approve_threshold;
        uint32_t approve_count;
        uint32_t reject_count;

        // status
        unsigned int status;

        // Serialization
        ADD_SERIALIZE_METHODS;

        template <typename Stream, typename Operation>
        inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
            READWRITE(address);
            READWRITE(name);
            READWRITE(url);
            READWRITE(data);
            READWRITE(txid);
            READWRITE(creation_block);
            READWRITE(update_block);
            READWRITE(status);
            READWRITE(approve_threshold);
            READWRITE(approve_count);
            READWRITE(reject_count);
        }

        void print();
        std::string getStatusString();
    };

private:
    /* Default to be exodus address. */
    Entry firstAlliance;

public:
    AllianceInfo(const boost::filesystem::path& path, bool fWipe) {
        leveldb::Status status = Open(path, fWipe);
        PrintToConsole("Loading alliance info database: %s\n", status.ToString());
        init();
    }

    virtual ~AllianceInfo() {
        if (msc_debug_persistence)
            PrintToLog("Alliance info is closed\n");
    }

    void init();
    void clear();
    static Entry allianceInfoEntryBuilder(std::string address, std::string name, std::string url, uint16_t approve_threshold,
                                 std::string data, uint256 txid, uint256 blockId);
    bool updateAllianceInfo(std::string address, Entry& info);
    bool putAllianceInfo(std::string address, Entry& info);
    bool getAllianceInfo(std::string address, Entry& info);
    bool hasAllianceInfo(std::string address);
    void getAllAllianceInfo(std::vector<Entry>& infoVec);
    bool deleteAllianceInfo(std::string address);
    bool isAllianceApproved(std::string address);
    uint32_t getApproveThreshold();

    void printAll();
};


// Vote record DB
class VoteRecordDB : public CDBBase {
public:
    VoteRecordDB(const boost::filesystem::path& path, bool fWipe) {
        leveldb::Status status = Open(path, fWipe);
        PrintToConsole("Loading vote record database: %s\n", status.ToString());
    }

    virtual ~VoteRecordDB() {
        if (msc_debug_persistence)
            PrintToLog("VoteRecordDB is closed\n");
    }
    void clear();
    bool updateVoteRecord(std::string address, unsigned int txType, std::string voteTarget, std::string& voteType);
    bool putVoteRecord(std::string address, unsigned int txType, std::string voteTarget, std::string voteType);
    bool getVoteRecord(std::string address, unsigned int txType, std::string voteTarget, std::string& voteType);
    bool hasVoteRecord(std::string address, unsigned int txType, std::string voteTarget);
    bool deleteVoteRecord(std::string address, unsigned int txType, std::string voteTarget);

    void printAll();
};

// BTC tx record DB
class BTCTxRecordDB : public CDBBase {
public:
    BTCTxRecordDB(const boost::filesystem::path& path, bool fWipe) {
        leveldb::Status status = Open(path, fWipe);
        PrintToConsole("Loading btc tx record database: %s\n", status.ToString());
    }

    virtual ~BTCTxRecordDB() {
        if (msc_debug_persistence)
            PrintToLog("BTCTxRecordDB is closed\n");
    }
    void clear();
    bool getBTCTxRecord(std::string address, uint32_t pid, std::string& txid);
    bool putBTCTxRecord(std::string address, uint32_t pid, std::string txid);
    bool hasBTCTxRecord(std::string address, uint32_t pid);
    bool deleteBTCTxRecord(std::string address, uint32_t pid);
};

namespace mastercore
{
typedef std::map<std::string, CMPCrowd> CrowdMap;

extern CMPSPInfo* _my_sps;
extern CrowdMap my_crowds;
extern AllianceInfo* allianceInfoDB;
extern VoteRecordDB* voteRecordDB;
extern BTCTxRecordDB* btcTxRecordDB;

std::string strPropertyType(uint16_t propertyType);
std::string strEcosystem(uint8_t ecosystem);

std::string getPropertyName(uint32_t propertyId);
bool isPropertyDivisible(uint32_t propertyId);

CMPCrowd* getCrowd(const std::string& address);

bool isCrowdsaleActive(uint32_t propertyId);
bool isCrowdsalePurchase(const uint256& txid, const std::string& address, int64_t* propertyId, int64_t* userTokens, int64_t* issuerTokens);

/** Calculates missing bonus tokens, which are credited to the crowdsale issuer. */
int64_t GetMissedIssuerBonus(const CMPSPInfo::Entry& sp, const CMPCrowd& crowdsale);

/** Calculates amounts credited for a crowdsale purchase. */
void calculateFundraiser(bool inflateAmount, int64_t amtTransfer, uint8_t bonusPerc,
        int64_t fundraiserSecs, int64_t currentSecs, int64_t numProps, uint8_t issuerPerc, int64_t totalTokens,
        std::pair<int64_t, int64_t>& tokens, bool& close_crowdsale);

void eraseMaxedCrowdsale(const std::string& address, int64_t blockTime, int block);

unsigned int eraseExpiredCrowdsale(const CBlockIndex* pBlockIndex);
}


#endif // OMNICORE_SP_H
