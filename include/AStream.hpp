#ifndef ADB_LIB_ASTREAM_HPP
#define ADB_LIB_ASTREAM_HPP

#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "APayload.hpp"


class AdbDevice;

class AStream {
public:
    using DevicePointer = std::shared_ptr<AdbDevice>;

    AStream(const AStream&) = delete;
    AStream(AStream&&) = delete;

protected:
    explicit AStream(DevicePointer pointer);

    void enqueue(APayload payload);
    APayload dequeue();

    std::deque<APayload> mQueue;
    std::mutex mQueueMutex;
    DevicePointer mDevice;
};


class AIStream
        : public AStream
{
public:
    AIStream& operator>> (std::string& string);
    AIStream& operator>> (APayload& payload);

protected:
    friend AdbDevice;
    std::condition_variable mReceived;

    explicit AIStream(DevicePointer pointer);

    void received(const APayload& payload);
};

class AOStream
        : public AStream
{
public:
    AOStream& operator<< (const std::string_view& string);
    AOStream& operator<< (const APayload& payload);
    AOStream& operator<< (APayload&& payload);

protected:
    friend AdbDevice;

    AOStream(DevicePointer pointer, uint32_t localId, uint32_t remoteId);

    void ready();

private:
    bool mReady;
    uint32_t mLocalId;
    uint32_t mRemoteId;

    void send(APayload&& payload);
};



#endif //ADB_LIB_ASTREAM_HPP
