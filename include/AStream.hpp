#ifndef ADB_LIB_ASTREAM_HPP
#define ADB_LIB_ASTREAM_HPP

#include <memory>
#include <queue>
#include "APayload.hpp"


class AdbDevice;

class AStream {
public:
    AStream(const AStream&) = delete;
    AStream(AStream&&) = delete;

protected:
    AStream() = default;

    void enqueue(APayload payload);
    APayload dequeue();

    std::deque<APayload> mQueue;
    std::weak_ptr<AdbDevice> mDevice;
};



#endif //ADB_LIB_ASTREAM_HPP
