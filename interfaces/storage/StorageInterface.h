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
 * @brief interface of StorageInterface
 * @file StorageInterface.h
 * @author: xingqiangbai
 * @date: 2021-04-07
 * @brief interface of StorageInterface
 * @file StorageInterface.h
 * @author: ancelmo
 * @date: 2021-09-07
 */
#pragma once
#include "../../interfaces/protocol/Block.h"
#include "../../interfaces/protocol/ProtocolTypeDef.h"
#include "../../libutilities/Error.h"
#include "Common.h"
#include "Entry.h"
#include <boost/range.hpp>
#include <boost/range/iterator.hpp>
#include <map>
#include <memory>
#include <string>

namespace bcos
{
namespace storage
{
class Table;
class StorageInterface
{
public:
    static constexpr const char SYS_TABLES[] = "s_tables";
    static constexpr const char SYS_TABLE_VALUE_FIELDS[] = "value_fields,key_field";

    static TableInfo::ConstPtr getSysTableInfo(const std::string_view& tableName);

    using Ptr = std::shared_ptr<StorageInterface>;
    using ConstPtr = std::shared_ptr<const StorageInterface>;

    virtual ~StorageInterface() = default;

    virtual void asyncGetPrimaryKeys(const std::string_view& table,
        const std::optional<Condition const>& _condition,
        std::function<void(Error::UniquePtr, std::vector<std::string>)> _callback) noexcept = 0;

    virtual void asyncGetRow(const std::string_view& table, const std::string_view& _key,
        std::function<void(Error::UniquePtr, std::optional<Entry>)> _callback) noexcept = 0;

    virtual void asyncGetRows(const std::string_view& table,
        const std::variant<const gsl::span<std::string_view const>,
            const gsl::span<std::string const>>& _keys,
        std::function<void(Error::UniquePtr, std::vector<std::optional<Entry>>)>
            _callback) noexcept = 0;

    virtual void asyncSetRow(const std::string_view& table, const std::string_view& key,
        Entry entry, std::function<void(Error::UniquePtr)> callback) noexcept = 0;

    virtual void asyncCreateTable(std::string _tableName, std::string _valueFields,
        std::function<void(Error::UniquePtr, std::optional<Table>)> callback) noexcept;

    virtual void asyncOpenTable(std::string_view tableName,
        std::function<void(Error::UniquePtr, std::optional<Table>)> callback) noexcept;

    virtual TableInfo::ConstPtr getTableInfo(const std::string_view& tableName) noexcept;
};

class TraverseStorageInterface : public StorageInterface
{
public:
    using Ptr = std::shared_ptr<TraverseStorageInterface>;
    using ConstPtr = std::shared_ptr<const TraverseStorageInterface>;

    virtual ~TraverseStorageInterface() = default;

    virtual void parallelTraverse(bool onlyDirty,
        std::function<bool(
            const std::string_view& table, const std::string_view& key, Entry const& entry)>
            callback) const = 0;
};

class TransactionalStorageInterface : public StorageInterface
{
public:
    using Ptr = std::shared_ptr<TransactionalStorageInterface>;
    using ConstPtr = std::shared_ptr<const TransactionalStorageInterface>;

    virtual ~TransactionalStorageInterface() = default;

    struct TwoPCParams
    {
        bcos::protocol::BlockNumber number = 0;
        std::string primaryTableName;
        std::string primaryTableKey;
        uint64_t startTS = 0;
    };

    virtual void asyncPrepare(const TwoPCParams& params,
        const TraverseStorageInterface::ConstPtr& storage,
        std::function<void(Error::Ptr, uint64_t)> callback) noexcept = 0;

    virtual void asyncCommit(
        const TwoPCParams& params, std::function<void(Error::Ptr)> callback) noexcept = 0;

    virtual void asyncRollback(
        const TwoPCParams& params, std::function<void(Error::Ptr)> callback) noexcept = 0;
};

}  // namespace storage
}  // namespace bcos
