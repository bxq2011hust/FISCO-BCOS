// Library for underlying chaos strategies of Phoenix
// Developed by Fcorleone at 2022-11-14

#pragma once
#include "PhoenixHookPosition.h"
#include <ctime>
#include <string>
#include <vector>

// Signals that tell all the nodes how to act
enum Signal
{
    RANDOMCHAOS,      // this is the normal mode of phoenix, just randomly choose a chaos strategy.
    STOPCHAOS,        // node under this mode will not perform any chaos testing
    DEFINITELYCHAOS,  // node under this mode will perform chaos for 100% percent
    READY,            // the hook position is ready for reproduction
    WAITING,          // the hook position is still waiting for ready for reproduction
};
namespace phoenix
{
// Network related stategies
void network_delay(int duration, int offset, std::string net_interface, int port,
    std::string node_id, HookPosition hookPosition);

void network_drop_packets(int percent, std::string net_interface, int port, std::string node_id,
    HookPosition hookPosition);

void network_duplicate_packets(int correlation, int percent, std::string net_interface, int port,
    std::string node_id, HookPosition hookPosition);

void network_corrupt_packets(int percent, std::string net_interface, int port, std::string node_id,
    HookPosition hookPosition);

bool stop_chaos_blade_strategy(std::string chaos_id);

std::string readReproductionSignal(std::string node_id, HookPosition hookPosition);

void writeReproduceSignal(std::string node_id, HookPosition hookPosition, Signal signal);

std::string executeCMD(const char* cmd);

// write the strategy info into a file
void writeStrategy(std::string info_file, long int timeStamp, std::string node_id,
    HookPosition hookPosition, std::string strategy_id, std::string exec_cmd);

void writeIdToPool(std::string pool_file, std::string node_id);

int getProbability();

void randomNetworkChaos(int p2pPort, std::string node_id, HookPosition hookPosition);
}  // namespace phoenix
