#ifndef ADB_LIB_ADBSTREAMBASE_HPP
#define ADB_LIB_ADBSTREAMBASE_HPP

#include <memory>
#include <queue>
#include <mutex>
#include <atomic>
#include <condition_variable>

#include "APayload.hpp"

class AdbDevice;

class AdbStreamBase {
public:
    using SharedDevice = std::shared_ptr<AdbDevice>;
    using WeakDevice = SharedDevice::weak_type;

    AdbStreamBase(const AdbStreamBase&) = delete;
    AdbStreamBase(AdbStreamBase&&) = delete;
    ~AdbStreamBase();
    [[nodiscard]] bool isOpen() const;

protected: // general
    using Queue = std::deque<APayload>;

    AdbStreamBase(WeakDevice pointer, uint32_t localId, uint32_t remoteId);
    void close();
    SharedDevice lockDeviceIfOpen();

    uint32_t mLocalId;
    uint32_t mRemoteId;
    WeakDevice mDevice;
    std::atomic<bool> mIsOpen;

    friend AdbDevice;

protected: // outgoing
    void send(APayload&& payload);
    void readyToSend();

    bool mReadyToSend;
    Queue mOutgoingQueue;
    std::mutex mOutgoingMutex;

    friend class AdbOStream;

protected: // incoming
    void received(const APayload& payload);
    APayload getPayload();

    std::condition_variable mReceived;
    Queue mIncomingQueue;
    std::mutex mIncomingMutex;

    friend class AdbIStream;
};



#endif //ADB_LIB_ADBSTREAMBASE_HPP
