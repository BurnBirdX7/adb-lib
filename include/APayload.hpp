#ifndef ADB_LIB_APAYLOAD_HPP
#define ADB_LIB_APAYLOAD_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

class APayload {
public:
    using iterator = uint8_t*;
    using citerator = const uint8_t*;

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

    uint8_t* getBuffer();
    [[nodiscard]] const uint8_t* getBuffer() const;

    [[nodiscard]] std::string toString() const;
    [[nodiscard]] std::string_view toStringView() const;

    iterator begin();
    [[nodiscard]] citerator begin() const;
    iterator end();
    [[nodiscard]] citerator end() const;


    APayload& operator=(APayload&&) noexcept ;
    APayload& operator=(const APayload&);

private:
    uint8_t* mBuffer;
    size_t mBufferSize;
    size_t mDataSize;

};

#endif //ADB_LIB_APAYLOAD_HPP
