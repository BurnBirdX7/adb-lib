#ifndef ADB_TEST_APAYLOAD_HPP
#define ADB_TEST_APAYLOAD_HPP

#include <cstddef>
#include <cstdint>


class APayload {
public:
    explicit APayload(size_t bufferSize);
    APayload(APayload&&) noexcept ;
    APayload(const APayload&);
    ~APayload();

    [[nodiscard]] size_t getSize() const;
    [[nodiscard]] size_t getBufferSize() const;
    uint8_t operator[] (size_t index) const;
    uint8_t& operator[] (size_t index);

    void resizeBuffer(size_t newSize);
    void setDataSize(size_t newSize);

    APayload& operator=(APayload&&) noexcept ;
    APayload& operator=(const APayload&);

private:
    uint8_t* mBuffer;
    size_t mBufferSize;
    size_t mDataSize;

};

#endif //ADB_TEST_APAYLOAD_HPP
