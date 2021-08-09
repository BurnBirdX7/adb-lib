#ifndef ADB_LIB_ADBSTREAMBASE_HPP
#define ADB_LIB_ADBSTREAMBASE_HPP

#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "APayload.hpp"


class AdbDevice;

class AdbStreamBase {
public:
    using DevicePointer = std::shared_ptr<AdbDevice>;

    AdbStreamBase(const AdbStreamBase&) = delete;
    AdbStreamBase(AdbStreamBase&&) = delete;

protected:
    explicit AdbStreamBase(DevicePointer pointer);

    std::deque<APayload> mQueue;
    std::mutex mQueueMutex;
    DevicePointer mDevice;
};



#endif //ADB_LIB_ADBSTREAMBASE_HPP
