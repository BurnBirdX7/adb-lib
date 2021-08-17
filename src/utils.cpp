#include "utils.hpp"

#include <sstream>
#include <iomanip>
#include <random>
#include <chrono>

std::vector<std::string_view> utils::tokenize(std::string_view view, const std::string_view& delimiters)
{
    std::vector<std::string_view> vector;
    size_t pos = view.find_first_of(delimiters);

    while (pos != std::string_view::npos) {
        vector.push_back(view.substr(0, pos));
        view.remove_prefix(pos + 1);
        pos = view.find_first_of(delimiters);
    }

    if (!view.empty())
        vector.push_back(view);

    return vector;
}

std::string utils::dataToHex(const unsigned char* payload, size_t size)
{
    std::stringstream ss;
    ss << std::setfill('0') << std::hex;
    for (size_t i = 0; i < size; ++i)
        ss << std::setw(2) << static_cast<short>(payload[i]);

    std::string result;
    ss >> result;

    return result;
}

inline unsigned long seed() {
    return std::random_device{}() + std::chrono::system_clock::now().time_since_epoch().count();
}

int utils::crypto::generateRandomBytes(void*, unsigned char* output, size_t outputLen)
{
    static std::mt19937_64 generator(seed());

    for (size_t i = 0; i < outputLen; ++i)
        output[i] = generator();

    return 0;
}


bool utils::crypto::sign(mbedtls_pk_context* ctx,
                         const uint8_t* hash, size_t hashLen,
                         uint8_t* signature, size_t signatureLen)
{
    auto rsa = mbedtls_pk_rsa(*ctx);
    int res = mbedtls_rsa_rsassa_pkcs1_v15_sign(rsa,
                                                generateRandomBytes, nullptr,
                                                MBEDTLS_MD_SHA1,
                                                hashLen, hash,
                                                signature);

    return res == 0;
}


bool utils::crypto::verify(mbedtls_pk_context* ctx,
                           const uint8_t* hash, size_t hashLen,
                           const uint8_t* signature, size_t signatureLen)
{
    auto rsa = mbedtls_pk_rsa(*ctx);
    int res = mbedtls_rsa_rsassa_pkcs1_v15_verify(rsa,
                                                  MBEDTLS_MD_SHA1,
                                                  hashLen, hash,
                                                  signature);

    return res == 0;
}

mbedtls_pk_context* utils::crypto::makePkContextFromPem(const std::string& filepath)
{
    auto ctx = new mbedtls_pk_context;
    mbedtls_pk_init(ctx);

    int res = mbedtls_pk_parse_keyfile(ctx, filepath.c_str(), nullptr, generateRandomBytes, nullptr);

    return (res == 0) ? ctx : nullptr;
}
