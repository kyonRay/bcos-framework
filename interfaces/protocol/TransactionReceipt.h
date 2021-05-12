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
 * @brief interface for TransactionReceipt
 * @file TransactionReceipt.h
 */
#pragma once

#include "../../libutilities/FixedBytes.h"
#include <gsl/span>

namespace bcos
{
namespace protocol
{
class LogEntry;
class TransactionReceipt
{
public:
    using Ptr = std::shared_ptr<TransactionReceipt>;
    using ConstPtr = std::shared_ptr<const TransactionReceipt>;
    TransactionReceipt() = default;
    virtual ~TransactionReceipt() {}

    virtual void decode(bytesConstRef _receiptData) = 0;
    virtual void encode(bytes& _encodedData) = 0;
    virtual bytesConstRef encode(bool _onlyHashFieldData = false) = 0;

    virtual bcos::crypto::HashType const& hash()
    {
        UpgradableGuard l(x_hash);
        if (m_hash != bcos::crypto::HashType())
        {
            return m_hash;
        }
        auto hashFields = encode(true);
        UpgradeGuard ul(l);
        m_hash = m_cryptoSuite->hash(hashFields);
        return m_hash;
    }

    virtual int32_t version() const = 0;
    virtual bcos::crypto::HashType const& stateRoot() const = 0;
    virtual u256 const& gasUsed() const = 0;
    virtual bytesConstRef contractAddress() const = 0;
    virtual LogBloom const& bloom() const = 0;
    virtual int32_t status() const = 0;
    virtual bytesConstRef output() const = 0;
    virtual gsl::span<const LogEntry> logEntries() const = 0;

protected:
    bcos::crypto::CryptoSuite::Ptr m_cryptoSuite;
    bcos::crypto::HashType m_hash;
    SharedMutex x_hash;
};
using Receipts = std::vector<TransactionReceipt::Ptr>;
using ReceiptsPtr = std::shared_ptr<Receipts>;
using ReceiptsConstPtr = std::shared_ptr<const Receipts>;
}  // namespace protocol
}  // namespace bcos
