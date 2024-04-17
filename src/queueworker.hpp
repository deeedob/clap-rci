#pragma once

#include <clap-rci/global.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

CLAP_RCI_BEGIN_NAMESPACE

class QueueWorker {
public:
    bool start();
    bool stop();
    bool tryNotify();

private:
    std::jthread worker;
    std::mutex workerMtx;
    std::condition_variable_any workerCv;
    std::atomic<bool> isReady {false};
};

CLAP_RCI_END_NAMESPACE
