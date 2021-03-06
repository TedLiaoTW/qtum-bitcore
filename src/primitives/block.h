// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIMITIVES_BLOCK_H
#define BITCOIN_PRIMITIVES_BLOCK_H

#include "primitives/transaction.h"
#include "serialize.h"
#include "uint256.h"

/** Nodes collect new transactions into a block, hash them into a hash tree,
 * and scan through nonce values to make the block's hash satisfy proof-of-work
 * requirements.  When they solve the proof-of-work, they broadcast the block
 * to everyone and the block is added to the block chain.  The first transaction
 * in the block is a special one that creates a new coin owned by the creator
 * of the block.
 */
class CBlockHeader
{
public:
    // header
    int32_t nVersion;
    uint256 hashPrevBlock;
    uint256 hashMerkleRoot;
    uint32_t nTime;
    uint32_t nBits;
    uint32_t nNonce;
    uint256 hashStateRoot; // qtum
    std::vector<unsigned char> vchBlockSig;
    bool fStake;
    // proof-of-stake specific fields
    COutPoint prevoutStake;
    uint32_t nStakeTime; // qtum

    CBlockHeader()
    {
        SetNull();
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(this->nVersion);
        READWRITE(hashPrevBlock);
        READWRITE(hashMerkleRoot);
        READWRITE(nTime);
        READWRITE(nBits);
        READWRITE(nNonce);
        READWRITE(hashStateRoot); // qtum
        READWRITE(vchBlockSig);
        fStake = IsProofOfStake();
        prevoutStake = PrevoutStake();
        nStakeTime = StakeTime();
        READWRITE(fStake);
        READWRITE(prevoutStake);
        READWRITE(nStakeTime); // qtum
    }

    void SetNull()
    {
        nVersion = 0;
        hashPrevBlock.SetNull();
        hashMerkleRoot.SetNull();
        nTime = 0;
        nBits = 0;
        nNonce = 0;
        hashStateRoot.SetNull(); // qtum
        vchBlockSig.clear();
        fStake = 0;
        prevoutStake.SetNull();
        nStakeTime = 0; // qtum
    }

    bool IsNull() const
    {
        return (nBits == 0);
    }

    uint256 GetHash() const;

    int64_t GetBlockTime() const
    {
        return (int64_t)nTime;
    }
    
    // ppcoin: two types of block: proof-of-work or proof-of-stake
    virtual bool IsProofOfStake() const //qtum
    {
        return fStake;
    }

    virtual bool IsProofOfWork() const
    {
        return !IsProofOfStake();
    }
    
    virtual COutPoint PrevoutStake() const
    {
        return prevoutStake;
    }

    virtual uint32_t StakeTime() const
    {
        return nStakeTime;
    }

    CBlockHeader& operator=(const CBlockHeader& other) //qtum
    {
        if (this != &other)
        {
            this->nVersion       = other.nVersion;
            this->hashPrevBlock  = other.hashPrevBlock;
            this->hashMerkleRoot = other.hashMerkleRoot;
            this->nTime          = other.nTime;
            this->nBits          = other.nBits;
            this->nNonce         = other.nNonce;
            this->hashStateRoot  = other.hashStateRoot;
            this->vchBlockSig    = other.vchBlockSig;
            this->fStake         = other.IsProofOfStake();
            this->prevoutStake   = other.PrevoutStake();
            this->nStakeTime     = other.StakeTime();
        }
        return *this;
    }
};


class CBlock : public CBlockHeader
{
public:
    // network and disk
    std::vector<CTransactionRef> vtx;

    // memory only
    mutable bool fChecked;

    CBlock()
    {
        SetNull();
    }

    CBlock(const CBlockHeader &header)
    {
        SetNull();
        *((CBlockHeader*)this) = header;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(*(CBlockHeader*)this);
        READWRITE(vtx);
    }

    void SetNull()
    {
        CBlockHeader::SetNull();
        vtx.clear();
        fChecked = false;
    }

    // ppcoin: two types of block: proof-of-work or proof-of-stake
    bool IsProofOfStake() const //qtum
    {
        if(vtx.size() == 0) return fStake;

        return (vtx.size() > 1 && vtx[1]->IsCoinStake());
    }

    COutPoint PrevoutStake() const
    {
        if(vtx.size() == 0) return prevoutStake;

        COutPoint ret;
        if(IsProofOfStake())
        {
            ret = vtx[1]->vin[0].prevout;
        }
        return ret;
    }
    
    uint32_t StakeTime() const
    {
        if(vtx.size() == 0) return nStakeTime;

        uint32_t ret = 0;
        if(IsProofOfStake())
        {
            ret = vtx[1]->nTime;;
        }
        return ret;
    }
    
    std::pair<COutPoint, unsigned int> GetProofOfStake() const //qtum
    {
        return IsProofOfStake()? std::make_pair(vtx[1]->vin[0].prevout, vtx[1]->nTime) : std::make_pair(COutPoint(), (unsigned int)0);
    }
    
    CBlockHeader GetBlockHeader() const
    {
        CBlockHeader block;
        block.nVersion       = nVersion;
        block.hashPrevBlock  = hashPrevBlock;
        block.hashMerkleRoot = hashMerkleRoot;
        block.nTime          = nTime;
        block.nBits          = nBits;
        block.nNonce         = nNonce;
        block.hashStateRoot  = hashStateRoot; // qtum
        block.vchBlockSig    = vchBlockSig;
        block.fStake         = IsProofOfStake();
        block.prevoutStake   = PrevoutStake();
        block.nStakeTime     = StakeTime(); // qtum
        return block;
    }

    std::string ToString() const;
};

/** Describes a place in the block chain to another node such that if the
 * other node doesn't have the same branch, it can find a recent common trunk.
 * The further back it is, the further before the fork it may be.
 */
struct CBlockLocator
{
    std::vector<uint256> vHave;

    CBlockLocator() {}

    CBlockLocator(const std::vector<uint256>& vHaveIn)
    {
        vHave = vHaveIn;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        int nVersion = s.GetVersion();
        if (!(s.GetType() & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(vHave);
    }

    void SetNull()
    {
        vHave.clear();
    }

    bool IsNull() const
    {
        return vHave.empty();
    }
};

/** Compute the consensus-critical block weight (see BIP 141). */
int64_t GetBlockWeight(const CBlock& tx);

#endif // BITCOIN_PRIMITIVES_BLOCK_H
