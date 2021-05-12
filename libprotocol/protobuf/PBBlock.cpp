/*
 *  Copyright (C) 2021 FISCO BCOS.
 *  SPDX-License-Identifier: Apache-2.0
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * @brief protobuf implementation of Block
 * @file PBBlock.cpp
 * @author: yujiechen
 * @date: 2021-03-23
 */
#include "PBBlock.h"
#include "interfaces/protocol/Exceptions.h"
#include "libprotocol/Common.h"
#include "libprotocol/ParallelMerkleProof.h"
#include <tbb/parallel_invoke.h>

using namespace bcos;
using namespace bcos::protocol;
using namespace bcos::crypto;

void PBBlock::decode(bytesConstRef _data, bool _calculateHash, bool _checkSig)
{
    decodePBObject(m_pbRawBlock, _data);
    tbb::parallel_invoke(
        [this]() {
            // decode blockHeader
            auto blockHeaderData = m_pbRawBlock->mutable_header();
            if (blockHeaderData->size() > 0)
            {
                m_blockHeader = m_blockHeaderFactory->createBlockHeader(
                    bytesConstRef((byte const*)blockHeaderData->data(), blockHeaderData->size()));
            }
        },
        [this, _calculateHash, _checkSig]() {
            // decode transactions
            decodeTransactions(_calculateHash, _checkSig);
        },
        [this, _calculateHash]() {
            // decode receipts
            decodeReceipts(_calculateHash);
        },
        [this]() {
            // decode txsHashList
            decodeTxsHashList();
        },
        [this]() {
            // decode ReceiptHashList
            decodeReceiptsHashList();
        },
        [this]() { decodeNonceList(); });
}

void PBBlock::encode(bytes& _encodedData) const
{
    tbb::parallel_invoke(
        [this]() {
            // encode blockHeader
            if (!m_blockHeader)
            {
                return;
            }
            auto encodedData = std::make_shared<bytes>();
            m_blockHeader->encode(*encodedData);
            m_pbRawBlock->set_header(encodedData->data(), encodedData->size());
        },
        [this]() {
            // encode transactions
            encodeTransactions();
        },
        [this]() {
            // encode receipts
            encodeReceipts();
        },
        [this]() {
            // encode transactions hash
            encodeTransactionsHash();
        },
        [this]() {
            // encode receipts hash
            encodeReceiptsHash();
        },
        [this]() {
            // encode the nonceList
            encodeNonceList();
        });
    auto data = encodePBObject(m_pbRawBlock);
    _encodedData = *data;
}

void PBBlock::decodeTxsHashList()
{
    auto txsHashNum = m_pbRawBlock->transactionshash_size();
    if (txsHashNum == 0)
    {
        return;
    }
    // decode the transactionsHash
    for (auto i = 0; i < txsHashNum; i++)
    {
        m_transactionsHash->push_back(
            HashType((byte const*)m_pbRawBlock->transactionshash(i).data(),
                HashType::ConstructorType::FromPointer));
    }
}

void PBBlock::decodeReceiptsHashList()
{
    auto receiptsHashNum = m_pbRawBlock->receiptshash_size();
    if (receiptsHashNum == 0)
    {
        return;
    }
    // decode the transactionsHash
    for (auto i = 0; i < receiptsHashNum; i++)
    {
        m_receiptsHash->push_back(HashType((byte const*)m_pbRawBlock->receiptshash(i).data(),
            HashType::ConstructorType::FromPointer));
    }
}

void PBBlock::decodeNonceList()
{
    auto noncesNum = m_pbRawBlock->noncelist_size();
    if (noncesNum == 0)
    {
        return;
    }
    // decode the transactionsHash
    m_nonceList->clear();
    for (auto i = 0; i < noncesNum; i++)
    {
        auto const& nonceData = m_pbRawBlock->noncelist(i);
        auto nonceValue =
            fromBigEndian<u256>(bytesConstRef((const byte*)nonceData.data(), nonceData.size()));
        m_nonceList->push_back(nonceValue);
    }
}

void PBBlock::decodeTransactions(bool _calculateHash, bool _checkSig)
{
    // Does not contain transaction fields
    if (m_pbRawBlock->mutable_transactions()->size() == 0)
    {
        return;
    }
    // Parallel decode transaction
    m_transactions->clear();
    int txsNum = m_pbRawBlock->transactions_size();
    m_transactions->resize(txsNum);
    tbb::parallel_for(tbb::blocked_range<int>(0, txsNum), [&](const tbb::blocked_range<int>& _r) {
        for (auto i = _r.begin(); i < _r.end(); i++)
        {
            auto const& txData = m_pbRawBlock->transactions(i);
            (*m_transactions)[i] = m_transactionFactory->createTransaction(
                bytesConstRef((byte const*)txData.data(), txData.size()), _checkSig);
            if (_calculateHash)
            {
                ((*m_transactions)[i])->hash();
            }
        }
    });
}

void PBBlock::decodeReceipts(bool _calculateHash)
{
    // Does not contain receipts fields
    if (m_pbRawBlock->mutable_receipts()->size() == 0)
    {
        return;
    }
    // Parallel decode receipt
    m_receipts->clear();
    int receiptsNum = m_pbRawBlock->receipts_size();
    m_receipts->resize(receiptsNum);
    tbb::parallel_for(
        tbb::blocked_range<int>(0, receiptsNum), [&](const tbb::blocked_range<int>& _r) {
            for (auto i = _r.begin(); i < _r.end(); i++)
            {
                auto const& receiptData = m_pbRawBlock->receipts(i);
                (*m_receipts)[i] = m_receiptFactory->createReceipt(
                    bytesConstRef((byte const*)receiptData.data(), receiptData.size()));
                if (_calculateHash)
                {
                    ((*m_receipts)[i])->hash();
                }
            }
        });
}

void PBBlock::encodeTransactions() const
{
    auto txsNum = m_transactions->size();
    if (txsNum == 0)
    {
        return;
    }
    // hit the transaction cache
    if (m_pbRawBlock->transactions_size() > 0)
    {
        return;
    }
    // extend transactions
    for (size_t i = 0; i < txsNum; i++)
    {
        m_pbRawBlock->add_transactions();
    }
    // parallel encode transactions
    tbb::parallel_for(tbb::blocked_range<int>(0, txsNum), [&](const tbb::blocked_range<int>& _r) {
        for (auto i = _r.begin(); i < _r.end(); i++)
        {
            auto encodedData = std::make_shared<bytes>();
            (*m_transactions)[i]->encode(*encodedData);
            m_pbRawBlock->set_transactions(i, encodedData->data(), encodedData->size());
        }
    });
}
void PBBlock::encodeReceipts() const
{
    auto receiptsNum = m_receipts->size();
    if (receiptsNum == 0)
    {
        return;
    }
    // hit the receipts cache
    if (m_pbRawBlock->receipts_size() > 0)
    {
        return;
    }
    // extend transactions
    for (size_t i = 0; i < receiptsNum; i++)
    {
        m_pbRawBlock->add_receipts();
    }
    // parallel encode transactions
    tbb::parallel_for(
        tbb::blocked_range<int>(0, receiptsNum), [&](const tbb::blocked_range<int>& _r) {
            for (auto i = _r.begin(); i < _r.end(); i++)
            {
                auto encodedData = std::make_shared<bytes>();
                (*m_receipts)[i]->encode(*encodedData);
                m_pbRawBlock->set_receipts(i, encodedData->data(), encodedData->size());
            }
        });
}

void PBBlock::encodeTransactionsHash() const
{
    auto txsHashNum = m_transactionsHash->size();
    if (txsHashNum == 0)
    {
        return;
    }
    // hit the cache
    if (m_pbRawBlock->transactionshash_size() > 0)
    {
        return;
    }
    // extend transactionshash
    for (size_t i = 0; i < txsHashNum; i++)
    {
        m_pbRawBlock->add_transactionshash();
    }
    int index = 0;
    for (auto const& txHash : *m_transactionsHash)
    {
        m_pbRawBlock->set_transactionshash(index++, txHash.data(), HashType::size);
    }
}

void PBBlock::encodeReceiptsHash() const
{
    auto receiptsHashNum = m_receiptsHash->size();
    if (receiptsHashNum == 0)
    {
        return;
    }
    // hit the cache
    if (m_pbRawBlock->receiptshash_size() > 0)
    {
        return;
    }
    // extend receiptshash
    for (size_t i = 0; i < receiptsHashNum; i++)
    {
        m_pbRawBlock->add_receiptshash();
    }
    int index = 0;
    for (auto const& receiptHash : *m_receiptsHash)
    {
        m_pbRawBlock->set_receiptshash(index++, receiptHash.data(), HashType::size);
    }
}

void PBBlock::encodeNonceList() const
{
    auto nonceNum = m_nonceList->size();
    if (nonceNum == 0)
    {
        return;
    }
    if (m_pbRawBlock->noncelist_size() > 0)
    {
        return;
    }
    for (size_t i = 0; i < nonceNum; i++)
    {
        m_pbRawBlock->add_noncelist();
    }
    int index = 0;
    for (auto const& nonce : *m_nonceList)
    {
        auto nonceBytes = toBigEndian(nonce);
        m_pbRawBlock->set_noncelist(index++, nonceBytes.data(), nonceBytes.size());
    }
}

// getNonces of the current block
NonceListPtr PBBlock::nonces()
{
    auto nonceList = std::make_shared<NonceList>();
    if (m_transactions->size() == 0)
    {
        return nonceList;
    }
    for (auto transaction : *m_transactions)
    {
        nonceList->push_back(transaction->nonce());
    }
    return nonceList;
}

Transaction::ConstPtr PBBlock::transaction(size_t _index) const
{
    if (m_transactions->size() < _index)
    {
        return nullptr;
    }
    return (*m_transactions)[_index];
}

HashType const& PBBlock::transactionHash(size_t _index) const
{
    return (*m_transactionsHash)[_index];
}

TransactionReceipt::ConstPtr PBBlock::receipt(size_t _index) const
{
    if (m_receipts->size() < _index)
    {
        return nullptr;
    }
    return (*m_receipts)[_index];
}

HashType const& PBBlock::receiptHash(size_t _index) const
{
    return (*m_receiptsHash)[_index];
}

HashType PBBlock::calculateTransactionRoot(bool _updateHeader) const
{
    auto txsRoot = HashType();
    // with no transactions
    if (m_transactions->size() == 0)
    {
        updateTxsRootForHeader(_updateHeader, txsRoot);
        return txsRoot;
    }
    // hit the cache
    UpgradableGuard l(x_txsRootCache);
    if (m_txsRootCache != HashType())
    {
        updateTxsRootForHeader(_updateHeader, m_txsRootCache);
        return m_txsRootCache;
    }
    // miss the cache or the cache has been cleared
    std::vector<bytes> transactionsList;
    encodeToCalculateRoot(transactionsList, m_transactions);
    txsRoot = calculateMerkleProofRoot(m_transactionFactory->cryptoSuite(), transactionsList);
    UpgradeGuard ul(l);
    m_txsRootCache = txsRoot;
    updateTxsRootForHeader(_updateHeader, txsRoot);
    return txsRoot;
}

HashType PBBlock::calculateReceiptRoot(bool _updateHeader) const
{
    auto receiptsRoot = HashType();
    // with no receipts
    if (m_receipts->size() == 0)
    {
        updateReceiptRootForHeader(_updateHeader, receiptsRoot);
        return receiptsRoot;
    }
    // hit the cache
    UpgradableGuard l(x_receiptRootCache);
    if (m_receiptRootCache != HashType())
    {
        updateReceiptRootForHeader(_updateHeader, m_receiptRootCache);
        return m_receiptRootCache;
    }
    // miss the cache or the cache has been cleared
    std::vector<bytes> receiptsList;
    encodeToCalculateRoot(receiptsList, m_receipts);
    receiptsRoot = calculateMerkleProofRoot(m_receiptFactory->cryptoSuite(), receiptsList);
    UpgradeGuard ul(l);
    m_receiptRootCache = receiptsRoot;
    updateReceiptRootForHeader(_updateHeader, receiptsRoot);
    return receiptsRoot;
}