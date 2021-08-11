#include <iostream>
#include "utils.hpp"
#include <cstring>

#include <mbedtls/md.h>
#include <mbedtls/pk.h>
#include <mbedtls/rsa.h>

int main() {

    std::string to_tokenize = "system_type:serial-serial:prop1;prop2";

    auto vec = utils::tokenize(to_tokenize, ":");

    for (const auto& str : vec)
        std::cout << str << std::endl;


    auto text = "hello";
    auto web_hash = "aaf4c61ddcc5e8a2dabede0f3b482cd9aea9434d";
    unsigned char hash[20];
    memset(hash, '\0', 20);

    mbedtls_md_context_t md;
    mbedtls_md_init(&md);
    mbedtls_md_setup(&md, mbedtls_md_info_from_type(MBEDTLS_MD_SHA1), 0);

    mbedtls_md_starts(&md);
    mbedtls_md_update(&md, reinterpret_cast<const unsigned char*>(text), strlen(text));
    mbedtls_md_finish(&md, hash);

    std::cout << web_hash << std::endl;
    std::cout << utils::dataToHex(hash, 20) << std::endl;

    auto pk = utils::crypto::makePkContextFromPem("/home/user/.android/adbkey");

    unsigned char signature[256];
    int res = utils::crypto::sign(pk, hash, 20, signature, 256);
    std::cout << res << std::endl;

    res = utils::crypto::verify(pk, hash, 20, signature, 256);
    std::cout << res << std::endl;


    return 0;
}