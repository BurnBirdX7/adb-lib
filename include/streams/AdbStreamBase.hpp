#ifndef ADB_LIB_ADBSTREAMBASE_HPP
#define ADB_LIB_ADBSTREAMBASE_HPP

#include <memory>
#include <queue>
#include <mutex>
#include <atomic>
#include <condition_variable>

class APayload;
class AdbDevice;

class AdbStreamBase {
public:
    using DevicePointer = std::shared_ptr<AdbDevice>;

    AdbStreamBase(const AdbStreamBase&) = delete;
    AdbStreamBase(AdbStreamBase&&) = delete;
    [[nodiscard]] bool isOpen() const;

protected:
    explicit AdbStreamBase(DevicePointer pointer);
    void close();

protected:
    std::deque<APayload> mQueue;
    std::mutex mQueueMutex;
    DevicePointer mDevice;

    std::atomic<bool> mOpen;
};



#endif //ADB_LIB_ADBSTREAMBASE_HPP
