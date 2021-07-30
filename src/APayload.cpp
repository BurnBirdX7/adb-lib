#include "APayload.hpp"
#include <memory>
#include <cassert>
#include <algorithm>

APayload::APayload(size_t bufferSize)
    : mBuffer(static_cast<uint8_t*>(std::malloc(bufferSize)))
    , mBufferSize(bufferSize)
    , mDataSize(0)
{
    assert(mBuffer != nullptr && "Couldn't allocate memory for APayload");
}

APayload::APayload(APayload&& other) noexcept
    : mBuffer(other.mBuffer)
    , mBufferSize(other.mBufferSize)
    , mDataSize(other.mDataSize)
{
    other.mBuffer = nullptr;
    other.mDataSize = 0;
    other.mBufferSize = 0;
}

APayload::APayload(const APayload& other)
    : APayload(other.mBufferSize)
{
    // Copy only data and not empty part of the buffer
    std::copy(other.mBuffer, other.mBuffer + other.mDataSize, mBuffer);
    mDataSize = other.mDataSize;
}

APayload::~APayload()
{
    std::free(mBuffer);
}

size_t APayload::getSize() const
{
    return mDataSize;
}

size_t APayload::getBufferSize() const
{
    return mBufferSize;
}

uint8_t APayload::operator[](size_t index) const
{
    assert(index < mDataSize);
    return mBuffer[index];
}

uint8_t& APayload::operator[](size_t index)
{
    assert(index < mDataSize);
    return mBuffer[index];
}

void APayload::resizeBuffer(size_t newSize)
{
    if (newSize == mBufferSize)
        return;

    auto newBuffer = static_cast<uint8_t*>(std::realloc(mBuffer, newSize));
    assert(newBuffer != nullptr && "Couldn't reallocate memory for APaylaod"); // Change to exception?
    mBuffer = newBuffer;
    mBufferSize = newSize;
    if (mBufferSize > mDataSize)
        mDataSize = mBufferSize;
}

void APayload::setDataSize(size_t newSize)
{
    assert(newSize <= mBufferSize && "Data size has to be lesser than buffer size");
    mDataSize = newSize;
}

uint8_t *APayload::getBuffer() {
    return mBuffer;
}

APayload::iterator APayload::begin()
{
    return mBuffer;
}

APayload::citerator APayload::begin() const
{
    return mBuffer;
}

APayload::iterator APayload::end()
{
    return mBuffer + mDataSize;
}

APayload::citerator APayload::end() const
{
    return mBuffer + mDataSize;
}

APayload& APayload::operator=(APayload&& other) noexcept
{
    mBuffer = std::exchange(other.mBuffer, nullptr);
    mBufferSize = std::exchange(other.mBufferSize, 0);
    mDataSize = std::exchange(other.mDataSize, 0);
    return *this;
}

APayload& APayload::operator=(const APayload& other)
{
    if (&other == this) // self-assignment check
        return *this;

    resizeBuffer(other.mBufferSize);
    std::copy(other.mBuffer, other.mBuffer + other.mDataSize, mBuffer);
    setDataSize(other.mDataSize);
    return *this;
}
