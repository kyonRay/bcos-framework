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
 */
#pragma once
#include "../../interfaces/protocol/Block.h"
#include "../../libutilities/Error.h"
#include "Common.h"
#include <map>
#include <memory>
#include <string>

namespace bcos
{
namespace storage
{
class TableFactory;
class StorageInterface : public std::enable_shared_from_this<StorageInterface>
{
public:
    enum ErrorCode
    {
        DataBase_Unavailable = -50000,
    };
    using Ptr = std::shared_ptr<StorageInterface>;
    StorageInterface() = default;
    virtual ~StorageInterface() = default;
    virtual std::vector<std::string> getPrimaryKeys(
        std::shared_ptr<TableInfo> _tableInfo, std::shared_ptr<Condition> _condition) const = 0;
    virtual std::shared_ptr<Entry> getRow(
        std::shared_ptr<TableInfo> _tableInfo, const std::string_view& _key) = 0;
    virtual std::map<std::string, std::shared_ptr<Entry> > getRows(
        std::shared_ptr<TableInfo> _tableInfo, const std::vector<std::string>& _keys) = 0;
    virtual size_t commitTables(const std::vector<std::shared_ptr<TableInfo> > _tableInfos,
        std::vector<std::shared_ptr<std::map<std::string, std::shared_ptr<Entry> > > >&
            _tableDatas) = 0;

    virtual void asyncGetPrimaryKeys(std::shared_ptr<TableInfo> _tableInfo,
        std::shared_ptr<Condition> _condition,
        std::function<void(Error, std::vector<std::string>)> _callback) = 0;
    virtual void asyncGetRow(std::shared_ptr<TableInfo> _tableInfo,
        std::shared_ptr<std::string> _key,
        std::function<void(Error, std::shared_ptr<Entry>)> _callback) = 0;
    virtual void asyncGetRows(std::shared_ptr<TableInfo> _tableInfo,
        std::shared_ptr<std::vector<std::string> > _keys,
        std::function<void(Error, std::map<std::string, std::shared_ptr<Entry> >)> _callback) = 0;
    virtual void asyncCommitTables(
        std::shared_ptr<std::vector<std::shared_ptr<TableInfo> > > _infos,
        std::shared_ptr<std::vector<std::shared_ptr<std::map<std::string, Entry::Ptr> > > >& _datas,
        std::function<void(Error, size_t)> _callback) = 0;

    // cache TableFactory
    virtual void asyncAddStateCache(protocol::BlockNumber _blockNumber, protocol::Block::Ptr _block,
        std::shared_ptr<TableFactory> _tablefactory, std::function<void(Error)> _callback) = 0;
    virtual void asyncDropStateCache(
        protocol::BlockNumber _blockNumber, std::function<void(Error)> _callback) = 0;
    virtual void asyncGetBlock(protocol::BlockNumber _blockNumber,
        std::function<void(Error, protocol::Block::Ptr)> _callback) = 0;
    virtual void asyncGetStateCache(protocol::BlockNumber _blockNumber,
        std::function<void(Error, std::shared_ptr<TableFactory>)> _callback) = 0;
    virtual protocol::Block::Ptr getBlock(protocol::BlockNumber _blockNumber) = 0;
    virtual std::shared_ptr<TableFactory> getStateCache(protocol::BlockNumber _blockNumber) = 0;
    virtual void dropStateCache(protocol::BlockNumber _blockNumber) = 0;
    virtual void addStateCache(protocol::BlockNumber _blockNumber, protocol::Block::Ptr _block,
        std::shared_ptr<TableFactory> _tablefactory) = 0;
    // KV store in split database, used to store data off-chain
    virtual bool put(const std::string& columnFamily, const std::string_view& key,
        const std::string_view& value) = 0;
    virtual std::string get(const std::string& columnFamily, const std::string_view& key) = 0;
    virtual void asyncGetBatch(const std::string& columnFamily,
        std::shared_ptr<std::vector<std::string_view> > keys,
        std::function<void(Error, std::shared_ptr<std::vector<std::string> >)> callback) = 0;
};

}  // namespace storage
}  // namespace bcos
