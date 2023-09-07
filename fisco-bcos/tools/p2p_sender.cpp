/**
 * @CopyRight:
 * FISCO-BCOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * FISCO-BCOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FISCO-BCOS.  If not, see <http://www.gnu.org/licenses/>
 * (c) 2016-2018 fisco-dev contributors.
 *
 *
 * @file: p2p_sender.cpp
 * @author: xingqiangbai
 * @date 2023-08-31
 */


#include "libdevcore/FixedHash.h"
#include "libinitializer/GlobalConfigureInitializer.h"
#include "libsync/SyncMsgPacket.h"
#include <libdevcore/TopicInfo.h>
#include <libinitializer/Initializer.h>
#include <libinitializer/P2PInitializer.h>
#include <boost/program_options.hpp>
#include <cstdlib>


using namespace std;
using namespace dev;
using namespace dev::initializer;

namespace po = boost::program_options;

int main(int argc, const char* argv[])
{
    po::options_description global("p2p sender used to send p2p message to FISCO BCOS v2 node");
    global.add_options()("help,h", "help of tool")("config,c",
        boost::program_options::value<std::string>()->default_value("./config.ini"),
        "the config file of node")("nodeID,n",
        boost::program_options::value<std::string>()->default_value(""),
        "send message to the nodeID")("groupID,g",
        boost::program_options::value<int16_t>()->default_value(1),
        "the groupID")("message,m", po::value<std::string>(),
        "message type to send, can be AMOP|Topic|PBFT|Sync|TxPool|Raft|Custom")(
        "subargs", po::value<std::vector<std::string>>(), "parameters for specific message type");

    po::positional_options_description pos;
    pos.add("message", 1).add("subargs", -1);

    po::variables_map vm;

    po::parsed_options parsed = po::command_line_parser(argc, argv)
                                    .options(global)
                                    .positional(pos)
                                    .allow_unregistered()
                                    .run();

    po::store(parsed, vm);
    if (vm.count("help"))
    {
        std::cout << global << std::endl;
        return 0;
    }

    auto configPath = vm["config"].as<std::string>();
    auto groupID = vm["groupID"].as<int16_t>();
    auto toNode = vm["nodeID"].as<std::string>();
    auto toNodeID = dev::h512(0);
    if (!toNode.empty())
    {
        toNodeID = dev::h512(toNode);
    }
    boost::property_tree::ptree pt;
    boost::property_tree::read_ini(configPath, pt);

    /// init log
    auto logInitializer = std::make_shared<LogInitializer>();
    logInitializer->initLog(pt);
    /// init global config. must init before DB, for compatibility
    initGlobalConfig(pt);
    g_BCOSConfig.setiniDir(configPath);

    // init certificates
    auto secureInitializer = std::make_shared<SecureInitializer>();
    secureInitializer->initConfig(pt);

    auto p2pInitializer = std::make_shared<P2PInitializer>();
    p2pInitializer->setSSLContext(secureInitializer->SSLContext(SecureInitializer::Usage::ForP2P));
    p2pInitializer->setKeyPair(secureInitializer->keyPair());
    p2pInitializer->initConfig(pt);

    auto p2pService = p2pInitializer->p2pService();
    // /// p2pService->setMessageFactory(std::make_shared<P2PMessageFactory>());

    std::string messageType = vm["message"].as<std::string>();
    std::cout << "message:" << messageType << std::endl;

    if (messageType == "Sync")
    {
        // Sync command has the following options:
        po::options_description sync_desc("amop message options");
        sync_desc.add_options()("help,h", "print help message")("type,t", po::value<uint8_t>(),
            "the sync message type, 0:status, 1:tx, 2:block, 3:blockRequest, 4:txStatus, "
            "5:txRequest")("parameters,p", po::value<std::vector<std::string>>()->multitoken(),
            "type 0:[blockNumber,genesisHash,latestHash], type 1:[hexTx], type "
            "2:[hexBlock], type 3:[startBlockNumber,length], type 4:[blockNumber,hexTxHash]");

        // Collect all the unrecognized options from the first pass. This will include the
        // (positional) command name, so we need to erase that.
        std::vector<std::string> opts =
            po::collect_unrecognized(parsed.options, po::include_positional);
        opts.erase(opts.begin());
        po::variables_map syncMessageVm;
        // Parse again...
        po::store(po::command_line_parser(opts).options(sync_desc).run(), syncMessageVm);
        uint8_t syncMessageType = syncMessageVm["type"].as<uint8_t>();
        std::cout << "sync message type:" << syncMessageType << std::endl;
        std::vector<std::string> syncMessageParameters =
            syncMessageVm["parameters"].as<std::vector<std::string>>();
        std::cout << "sync message parameters:" << std::endl;
        PROTOCOL_ID syncId = getGroupProtoclID(groupID, ProtocolID::BlockSync);

        for (auto& parameter : syncMessageParameters)
        {
            std::cout << parameter << std::endl;
        }
        switch (syncMessageType)
        {
        case 0:
        {
            if (syncMessageParameters.size() != 4)
            {
                std::cout << "sync message StatusPacket(typ 0) need 4 "
                             "parameters(nodeID,number,genesisHash,latestHash)"
                          << std::endl;
                exit(0);
            }
            auto blockNumber = boost::lexical_cast<int64_t>(syncMessageParameters[0]);
            auto genesisHash = dev::h256(syncMessageParameters[1]);
            auto latestHash = dev::h256(syncMessageParameters[2]);
            std::cout << "blockNumber:" << blockNumber << std::endl;
            std::cout << "genesisHash:" << genesisHash << std::endl;
            std::cout << "latestHash:" << latestHash << std::endl;
            auto syncStatusPacket =
                dev::sync::SyncStatusPacket(p2pService->id(), blockNumber, genesisHash, latestHash);
            syncStatusPacket.encode();
            if (toNodeID != dev::h512(0))
            {
                p2pService->asyncSendMessageByNodeID(toNodeID, syncStatusPacket.toMessage(syncId),
                    CallbackFuncWithSession(), Options());
            }
            else
            {
                p2pService->asyncBroadcastMessage(syncStatusPacket.toMessage(syncId), Options());
            }
            break;
        }
        default:
        {
            std::cout << "sync message type:" << syncMessageType << " not supported" << std::endl;
            exit(0);
        }
        }
    }
    else if (messageType == "Custom")
    {
#if 0
        po::options_description desc("custom p2p message");
        int32_t protocol = 0;
        desc.add_options()("help,h", "print help message")(
            "protocol,p", po::value<int32_t>(), "the protocol id of custom message")(
            "data,d", po::value<std::string>(), "hex encoded payload of message")("parameters,p",
            po::value<std::vector<std::string>>()->multitoken(),
            "type 0:[hexNodeID,blockNumber,genesisHash,latestHash], type 1:[hexTx], type "
            "2:[hexBlock], type 3:[startBlockNumber,length], type 4:[blockNumber,hexTxHash]");

        // Collect all the unrecognized options from the first pass. This will include the
        // (positional) command name, so we need to erase that.
        std::vector<std::string> opts =
            po::collect_unrecognized(parsed.options, po::include_positional);
        opts.erase(opts.begin());
        po::variables_map syncMessageVm;
        // Parse again...
        po::store(po::command_line_parser(opts).options(sync_desc).run(), syncMessageVm);
#endif
    }
    return 0;
}
