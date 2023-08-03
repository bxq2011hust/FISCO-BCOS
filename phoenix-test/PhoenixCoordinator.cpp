// This is a coordinator for chaos testing control and reproduction.
// There is a strategy pool in the coordinator which will be used to manage all the chaos
// strategies. The coordinator also contains a set of orcales which are in charge of checking
// whether there are bugs in the system. Besides, during the reproduction process, the coordinator
// will send signals to each node.

// Developed by Fcorleone at 2022-12-01

#include "PhoenixCoordinator.h"
#include <getopt.h>
#include <json/json.h>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <utility>

using namespace std::chrono;
using namespace std;
using namespace phoenix;
using Clock = std::chrono::steady_clock;

// The constructor of PhoenixStrategy
PhoenixStrategy::PhoenixStrategy(std::string strategyId, std::string timeStamp, std::string node_id,
    std::string exec_cmd, HookPosition hookPosition)
  : strategyId(std::move(strategyId)),
    timeStamp(std::move(timeStamp)),
    nodeId(std::move(node_id)),
    execCmd(std::move(exec_cmd)),
    hookPosition(hookPosition)
{}

// The getter and setter of the PhoenixStrategy
std::string PhoenixStrategy::getStrategyId()
{
    return this->strategyId;
}

std::string PhoenixStrategy::getTimeStamp()
{
    return this->timeStamp;
}

std::string PhoenixStrategy::getNodeId()
{
    return this->nodeId;
}

std::string PhoenixStrategy::getExecCmd()
{
    return this->execCmd;
}

// StrategyType PhoenixStrategy::getStrategyType(){
//     return this->strategyType;
// }

HookPosition PhoenixStrategy::getHookPosition()
{
    return this->hookPosition;
}

void PhoenixStrategy::setStrategyId(std::string _strategyId)
{
    this->strategyId = std::move(_strategyId);
}

void PhoenixStrategy::setTimeStamp(std::string _timeStamp)
{
    this->timeStamp = std::move(_timeStamp);
}

void PhoenixStrategy::setNodeId(std::string _nodeId)
{
    this->strategyId = std::move(_nodeId);
}

void PhoenixStrategy::setExecCmd(std::string _execCmd)
{
    this->timeStamp = std::move(_execCmd);
}

// void PhoenixStrategy::setStrategyType(StrategyType _strategyType){
//     this->strategyType = _strategyType;
// }

void PhoenixStrategy::setHookPosition(HookPosition _hookPosition)
{
    this->hookPosition = _hookPosition;
}

PhoenixCoordinator::PhoenixCoordinator()
{
    this->strategyPool = std::vector<std::shared_ptr<PhoenixStrategy>>();
    this->chaosPhase = 1;
    this->T1_testPhaseTime = 300;   // 300 seconds for T1 by default
    this->T2_checkPhaseTime = 600;  // 600 seconds for T2 by default
}

PhoenixCoordinator::PhoenixCoordinator(int t1, int t2)
{
    this->strategyPool = std::vector<std::shared_ptr<PhoenixStrategy>>();
    this->chaosPhase = 1;
    this->T1_testPhaseTime = t1;
    this->T2_checkPhaseTime = t2;
}

void PhoenixCoordinator::addToStrategyPool(std::shared_ptr<PhoenixStrategy> newStrategy)
{
    this->strategyPool.push_back(newStrategy);
}

std::vector<std::shared_ptr<PhoenixStrategy>> PhoenixCoordinator::getStrategyPool()
{
    return this->strategyPool;
}

bool PhoenixCoordinator::stopAllStrategies()
{
    // first get the strategies into the pool
    this->fillStrategyPool();
    for (auto it = begin(this->strategyPool); it != end(this->strategyPool); ++it)
    {
        std::string cur_id = it->get()->getStrategyId();
        std::cout << "Stop the following strategyid:" << cur_id << std::endl;
        auto success = stop_chaos_blade_strategy(cur_id);
        if (!success)
            return false;
    }
    return true;
}

void PhoenixCoordinator::clearAllStrategies()
{
    this->strategyPool.clear();
}

std::shared_ptr<PhoenixStrategy> PhoenixCoordinator::getStrategyById(std::string strategyId)
{
    for (int i = 0; i < (int)this->strategyPool.size(); i++)
    {
        std::string cur_id = this->strategyPool[i]->getStrategyId();
        if (cur_id == strategyId)
        {
            return this->strategyPool[i];
        }
    }
    // if we cannot find such id, return NULL then
    return NULL;
}

// Write a signal to a file and all the nodes read the signal to act
void PhoenixCoordinator::writeSignal(std::string fileName, Signal signal)
{
    ofstream MyFile(fileName);
    switch (signal)
    {
    case Signal::RANDOMCHAOS:
        MyFile << "Probability:5";
        break;
    case Signal::STOPCHAOS:
        MyFile << "Probability:0";
        break;
    case Signal::DEFINITELYCHAOS:
        MyFile << "Probability:100";
        break;
    default:
        MyFile << "Probability:0";
        break;
    }
    // Close the file
    MyFile.close();
}

int PhoenixCoordinator::getCurrentPhase()
{
    return this->chaosPhase;
}

void PhoenixCoordinator::setCurrentPhase(int newPhase)
{
    this->chaosPhase = newPhase;
}

int PhoenixCoordinator::getT1()
{
    return this->T1_testPhaseTime;
}

void PhoenixCoordinator::setT1(int newT1)
{
    this->T1_testPhaseTime = newT1;
}

int PhoenixCoordinator::getT2()
{
    return this->T2_checkPhaseTime;
}

void PhoenixCoordinator::setT2(int newT2)
{
    this->T2_checkPhaseTime = newT2;
}

void startCoordinator(std::shared_ptr<PhoenixCoordinator> phoenix)
{
    // start a loop to execute the test phase and check phase
    while (true)
    {
        int test_duration = phoenix->getT1();
        // test_duration *= CLOCKS_PER_SEC; // duration is in the unit of seconds
        int check_duration = phoenix->getT2();
        // check_duration *= CLOCKS_PER_SEC; // duration is in the unit of seconds
        auto now = Clock::now();
        // currently, we are in the test phase
        while (duration_cast<milliseconds>(Clock::now() - now).count() / 1000.0 < test_duration)
        {
            if (phoenix->getCurrentPhase() == 1)
            {
                // we set the current phase to the test phase
                phoenix->setCurrentPhase(0);
                // then we write the signal
                phoenix->writeSignal("/tmp/PHOENIX_CHAOS_SIGNAL.txt", Signal::RANDOMCHAOS);
                // ####### DEBUG:
                std::cout << "the signal has been written!" << std::endl;
                // ####### DEBUG:
                std::cout << "the duration is " << phoenix->getT1() << std::endl;
            }
        }
        // ####### DEBUG:
        std::cout << "test_duration has finished!" << std::endl;
        // currently, we are in the check phase
        while (duration_cast<milliseconds>(Clock::now() - now).count() / 1000.0 <
               (test_duration + check_duration))
        {
            // std::cout << "clock is: " << clock() << "now is: " << now << "test_duration is : " <<
            // test_duration << std::endl;
            if (phoenix->getCurrentPhase() == 0)
            {
                // we set the current phase to the check phase
                phoenix->setCurrentPhase(1);
                // then we write the signal
                phoenix->writeSignal("/tmp/PHOENIX_CHAOS_SIGNAL.txt", Signal::STOPCHAOS);
                // We kill all the chaos engineering now
                phoenix->stopAllStrategies();
            }
            bool suc = performChecking();
            // std::cout << "clock is: " << clock() << "now is: " << now << "test_duration is : " <<
            // test_duration << std::endl;
            if (suc)
            {
                // check good! Now we clear the strategies in the pool.
                phoenix->clearAllStrategies();

                // we then write the acutual t2 to the log
                int actual_t2 = duration_cast<milliseconds>(Clock::now() - now).count() / 1000.0 -
                                test_duration;
                std::cout << "The system has already recovered, the recover time t2 is: "
                          << actual_t2 << " seconds" << std::endl;
                break;
            }
            else
            {
                // check again
                continue;
            }
        }
        // double check again
        bool suc = performChecking();
        if (suc)
        {
            continue;
        }
        else
        {
            // we found a resilience issue
            break;
        }
    }
}

bool performChecking()
{
    // todo: perform node checking and data checking
    // sleep(300);
    char check_cmd_node[100];
    sprintf(check_cmd_node, "ps -a | grep fisco | wc -l");
    // std::cout << "The exec cmd is " << network_cmd << std::endl;
    std::string node_checking_result = executeCMD(check_cmd_node);
    int node_res = stoi(node_checking_result);
    // don't check process
    // if(node_res == 10){ // 10 nodes in the group
    // perform the data checking
    char check_cmd_data[100];
    sprintf(check_cmd_data, "/data/chaos/java-sdk-demo/dist/checking_data.sh");
    std::string data_checking_result = executeCMD(check_cmd_data);
    // std::cout << "checking result:" << data_checking_result << std::endl;
    int data_res = stoi(data_checking_result);
    // TODO: add the isolation checking (block height)
    if (data_res == 1)
    {
        // check the block height
        char check_cmd_height[100];
        sprintf(check_cmd_height, "/data/chaos/java-sdk-demo/dist/getHeight.sh");
        std::string height_checking_result = executeCMD(check_cmd_height);
        // std::cout << "checking result:" << height_checking_result << std::endl;
        int height_res = stoi(height_checking_result);
        if (height_res == 1)
        {
            return true;
        }
        else
        {
            std::cout << "checking height failed, checking again" << std::endl;
            return false;
        }
    }
    else
    {
        std::cout << "checking data failed, checking again" << std::endl;
        return false;
    }
    // }
    // else {
    //     std::cout << "checking processes failed, checking again" << std::endl;
    //     return false;
    // }
}

void PhoenixCoordinator::fillStrategyPool()
{  // used in the reproduction process
    std::string path = "/tmp/phoenix";
    for (const auto& entry : std::filesystem::directory_iterator(path))
    {
        // open the strategy storing files
        ifstream MyFile(entry.path());
        std::string strategy;
        while (getline(MyFile, strategy))
        {
            std::stringstream test(strategy);
            std::string segment;
            std::vector<std::string> seglist;
            while (std::getline(test, segment, ','))
            {
                seglist.push_back(segment);
            }
            // MyFile << "Time:" << timeStamp << ",Node:" << node_id <<
            // ",HookPosition:" << hookPosition << ",StrategyId:" << strategy_id
            // << ",exec_cmd:" << exec_cmd << std::endl;
            auto timeStamp = seglist[0];
            auto node_id = seglist[1];
            auto hookPosition = seglist[2];
            auto strategy_id = seglist[3];
            auto exec_cmd = seglist[4];
            std::shared_ptr<PhoenixStrategy> new_strategy = std::make_shared<PhoenixStrategy>(
                strategy_id, timeStamp, node_id, exec_cmd, toEnum(hookPosition));
            this->strategyPool.push_back(new_strategy);
            strategy = "";
        }
    }
}

void PhoenixCoordinator::sortStrategy()
{
    this->strategyPool.clear();  // clear the pool first
    this->fillStrategyPool();
    // cout<<this->getStrategyPool().size()<<endl;
    // a bubble sorting for strategy pool
    int length = this->getStrategyPool().size();
    for (int i = length - 1; i > 0; i--)
    {
        for (int j = 0; j <= i - 1; j++)
        {
            if (this->getStrategyPool()[j]->getTimeStamp() >
                this->getStrategyPool()[j + 1]->getTimeStamp())
            {
                // choose the largest
                std::shared_ptr<PhoenixStrategy> tmp = this->getStrategyPool()[j];
                *this->getStrategyPool()[j] = *this->getStrategyPool()[j + 1];
                *this->getStrategyPool()[j + 1] = *tmp;
            }
        }
    }
}

// start the reproduce process
void startReproduce(std::shared_ptr<PhoenixCoordinator> phoenix)
{
    phoenix->writeSignal("/tmp/PHOENIX_CHAOS_SIGNAL.txt", Signal::STOPCHAOS);
    phoenix->sortStrategy();
    // already test the sorting
    // cout<<phoenix->getStrategyPool()[0]->getTimeStamp()<<endl;
    // cout<<phoenix->getStrategyPool()[24]->getTimeStamp()<<endl;
    // set the n round
    int n = 1;
    while (true)
    {
        cout << "============== Reproduce round " << n << "! ==============" << endl;
        // how many stategies should be collected for this round
        int strategies = pow(2, n);
        vector<std::shared_ptr<PhoenixStrategy>> selected_strategies = phoenix->getStrategyPool();
        if (strategies < (int)phoenix->getStrategyPool().size())
        {
            // we need to select certain strategies in the pool
            selected_strategies.clear();
            int length = phoenix->getStrategyPool().size();
            for (int i = 0; i < strategies; i++)
            {
                selected_strategies.push_back(phoenix->getStrategyPool()[length - i - 1]);
            }
            std::reverse(selected_strategies.begin(), selected_strategies.end());
        }
        cout << "Selected " << selected_strategies.size() << " strategies from the pool!" << endl;
        vector<std::string> this_round_chaos_ids = {};
        // execute all the seleceted strategies
        for (int n = 0; n < (int)selected_strategies.size(); n++)
        {
            std::string cur_exe_cmd = selected_strategies[n]->getExecCmd();
            // char execution_cmd[100];
            // sprintf(execution_cmd, cur_exe_cmd.c_str());
            std::string node_id = selected_strategies[n]->getNodeId();
            HookPosition hook_position = selected_strategies[n]->getHookPosition();
            while (true)
            {
                // execute the command until it is succssful
                std::string signal = readReproductionSignal(node_id, hook_position);
                if (signal == "WAITING")
                {  // Wait until the related hook position is ready
                    continue;
                }
                // cout<<"Start Executin commands"<<endl;
                string chaos_result = executeCMD(cur_exe_cmd.c_str());
                Json::Value json_result;
                Json::Reader reader;
                bool parsingSuccessful = reader.parse(chaos_result, json_result);
                // cout<<"Finish Executin commands"<<endl;
                // The result is like: {"code":200,"success":true,"result":"35719a3de188df83"}
                if (json_result["code"].asInt() == 200 &&
                    json_result["success"].asString() == "true")
                {
                    // This indicates that the json is parsed successfully and the chaos is starting
                    // successfully
                    string chaos_id = json_result["result"].asString();
                    this_round_chaos_ids.push_back(chaos_id);
                    writeReproduceSignal(node_id, hook_position, Signal::WAITING);
                    break;
                }
            }
            cout << "Successfully execute " << n + 1 << " strategy !" << endl;
        }
        // check whether the bug is reproduced
        // here we still need to check whether the bug is the same as the found one. Currently, we
        // did this manually.
        bool suc = performChecking();
        if (suc)
        {
            cout << "This round cannot reproduce the bug! Try next round!!" << endl;
            // stop the current round chaos
            for (int i = 0; i < (int)this_round_chaos_ids.size(); i++)
            {
                auto success = stop_chaos_blade_strategy(this_round_chaos_ids[i]);
                while (!success)
                {
                    success = stop_chaos_blade_strategy(this_round_chaos_ids[i]);
                }
            }
        }
        else
        {
            cout << "The bug occurs! Reproduce Successfully!!!" << endl;
            break;
        }
        n = n + 1;
        this_round_chaos_ids.clear();
    }
}

// the main function of the coordinator
int main(int argc, char* argv[])
{
    int c;
    static struct option long_options[] = {
        {"test", no_argument, NULL, 't'}, {"reproduce", no_argument, NULL, 'r'}};
    int opt_index = 0;

    c = getopt_long(argc, argv, "tr", long_options, &opt_index);

    if (-1 == c)
    {
        return -1;
    }
    int T1 = 600;
    int T2 = 6000;

    switch (c)
    {
    case 't':
    {
        cout << "test mode!!" << endl;
        // clear all the phoenix temp files
        int res0 = system("sudo rm -rf /tmp/PHOENIX*");
        int res1 = system("sudo rm -rf /tmp/phoenix");
        int res2 = system("sudo mkdir /tmp/phoenix");
        std::shared_ptr<PhoenixCoordinator> phoenix = std::make_shared<PhoenixCoordinator>(T1, T2);
        startCoordinator(phoenix);
        break;
    }
    case 'r':
    {
        cout << "reproduce mode!!" << endl;
        std::shared_ptr<PhoenixCoordinator> phoenix = std::make_shared<PhoenixCoordinator>(T1, T2);
        startReproduce(phoenix);
        break;
    }
    }
    return 0;
}