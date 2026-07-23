#include "crypto.hpp"

#include <openssl/rand.h>

#include <stdexcept>

namespace core {

uint64_t crypto::generateToken() {
    uint64_t token = 0;

    if (RAND_bytes(reinterpret_cast<unsigned char*>(&token), sizeof(token)) != 1) {
        throw std::runtime_error("OpenSSL: Failed to generate token");
    }

    return token;
}

} // namespace core