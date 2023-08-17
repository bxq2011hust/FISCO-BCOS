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
 * @brief block level context
 * @file ExecuteOutputs.h
 * @author: jimmyshi
 * @date: 2022-04-02
 */

#pragma once

#include "bcos-executor/src/Common.h"
#include "bcos-framework/executor/ExecutionMessage.h"
#include <tbb/concurrent_unordered_map.h>
#include <mutex>


namespace bcos
{
namespace executor
{
class ExecuteOutputs
{
public:
    using Ptr = std::shared_ptr<ExecuteOutputs>;

    void add(protocol::ExecutionMessage::UniquePtr result)
    {
        auto contextID = result->contextID();
        x_contextID2Result.lock();
        m_contextID2Result.emplace(contextID, std::move(result));
        x_contextID2Result.unlock();
    }

    std::vector<protocol::ExecutionMessage::UniquePtr> dumpAndClear()
    {
        auto results = std::vector<protocol::ExecutionMessage::UniquePtr>();
        results.reserve(m_contextID2Result.size());
        for (auto it = m_contextID2Result.begin(); it != m_contextID2Result.end(); it++)
        {
            auto contextID = it->first;
            auto& output = it->second;
            results.emplace_back(std::move(output));
        }

        m_contextID2Result.clear();
        return results;
    }

    void clear()
    {
        std::lock_guard<std::mutex> guard(x_contextID2Result);
        m_contextID2Result.clear();
    }

private:
    std::mutex x_contextID2Result;
    std::map<int64_t, protocol::ExecutionMessage::UniquePtr> m_contextID2Result;
};
}  // namespace executor
}  // namespace bcos