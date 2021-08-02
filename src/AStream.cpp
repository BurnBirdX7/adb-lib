#include "AStream.hpp"

void AStream::enqueue(APayload payload) {
    mQueue.emplace_back(std::move(payload));
}

APayload AStream::dequeue() {
    auto payload = std::move(mQueue.front());
    mQueue.pop_front();
    return payload;
}
