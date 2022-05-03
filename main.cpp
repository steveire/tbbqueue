
#include "oneapi/tbb/concurrent_queue.h"
#include "oneapi/tbb/concurrent_hash_map.h"
#include "oneapi/tbb/task_group.h"
#include "oneapi/tbb/task_arena.h"
#include "oneapi/tbb/global_control.h"

#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <thread>

class DataProcessor
{
    using SignalQueuesType = std::vector<tbb::concurrent_queue<int>>;
    SignalQueuesType signalQueues;
    SignalQueuesType signalRecieved;
    std::vector<std::atomic<bool>> busyIds;

    using LogType = tbb::concurrent_hash_map<std::thread::id, std::vector<std::string>>;
    LogType logs;

    tbb::task_group tg;

    void processData(int sourceId, int value)
    {
        signalRecieved[sourceId].push(value);
        std::this_thread::sleep_for(std::chrono::milliseconds(value));
    }

public:
    DataProcessor()
        : busyIds(200), signalQueues(200), signalRecieved(200)
    {
    }
    ~DataProcessor()
    {
        tg.wait();
    }

    void receiveData(int sourceId, int value)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << sourceId << " S " << value << "\n";
        signalQueues[sourceId].push(value);

        tg.run([this, sourceId, value] {
            std::string buf;
            std::stringstream ss(buf);
            ss << "TASK " << sourceId << " -- " << (busyIds[sourceId].load() ? "B" : "N") << " -- " << value << "\n";
            bool isBusy = false;
            while (!busyIds[sourceId].compare_exchange_strong(isBusy, true))
            {
                isBusy = false;
            }
            ss << "SET TRUE ON " << sourceId << " -- " << isBusy << "\n";
            int nextValue;
            while (signalQueues[sourceId].try_pop(nextValue))
            {
                ss << "START " << sourceId << " -- " << nextValue << "\n";
                processData(sourceId, nextValue);
                ss << "DONE " << sourceId << " -- " << nextValue << "\n";
            }
            // ...
            isBusy = true;
            while (!busyIds[sourceId].compare_exchange_strong(isBusy, false))
            {
                isBusy = true;
            }
            ss << "SET FALSE ON " << sourceId << " -- " << isBusy << "\n";
            LogType::accessor a;
            logs.insert(a, std::this_thread::get_id());
            auto thisLog = ss.str();
            a->second.push_back(thisLog);
        });
    }

    void dumpLogs()
    {
        tg.wait();
        for(auto l : logs)
        {
            int i = 0;
            std::cout << "THREAD " << l.first << "\n";
            for (auto v : l.second)
            {
                std::cout << i << " -- " << v << "\n";
            }
        }

        std::cout << " ---\n";

        for (auto srIdx = 0; srIdx < signalRecieved.size(); ++srIdx)
        {
            if (!signalRecieved[srIdx].empty())
            {
                std::cout << "SR " << srIdx << "\n";
                int v;
                while (signalRecieved[srIdx].try_pop(v))
                {
                    std::cout << "V " << v << "\n";
                }
            }
        }
    }
};

int main(int, char**)
{
    DataProcessor dp;

    oneapi::tbb::global_control limitThreads(tbb::global_control::max_allowed_parallelism, 2);

    std::cout << "NUM THREADS " << oneapi::tbb::global_control::active_value(tbb::global_control::max_allowed_parallelism) << "\n";

    std::thread postDataThread([&dp] {
        dp.receiveData(1, 1001);
        dp.receiveData(2, 304);
        dp.receiveData(2, 311);
        dp.receiveData(2, 312);
        dp.receiveData(2, 305);
        dp.receiveData(2, 316);
        dp.receiveData(2, 317);
        dp.receiveData(1, 1003);
        dp.receiveData(2, 308);
        dp.receiveData(2, 319);
        dp.receiveData(2, 320);
        dp.receiveData(2, 321);
        dp.receiveData(2, 322);
        dp.receiveData(2, 323);
        dp.receiveData(2, 324);
        dp.receiveData(2, 325);

        dp.dumpLogs();

    });

    postDataThread.join();

    return 0;
}
