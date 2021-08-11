#ifndef ADB_LIB_UTILS_HPP
#define ADB_LIB_UTILS_HPP

#include <vector>
#include <string>
#include <string_view>

#include <mbedtls/pk.h>


namespace utils {

    std::vector<std::string_view> tokenize(std::string_view view, const std::string_view& delimiters);

    std::string dataToHex(const unsigned char* payload, size_t size);

    namespace crypto {

        int generateRandomBytes(void* /* ignored */, unsigned char* output, size_t outputLen);

        mbedtls_pk_context* makePkContextFromPem(const std::string& filepath);

        // signs SHA1 hash
        bool sign(mbedtls_pk_context* ctx,
                  const uint8_t* hash, size_t hashLen,
                  uint8_t* signature, size_t signatureLen);

        // verifies signature for SHA1 hash
        bool verify(mbedtls_pk_context* ctx,
                    const uint8_t* hash, size_t hashLen,
                    const uint8_t* signature, size_t signatureLen);

    }
}

#endif //ADB_LIB_UTILS_HPP
