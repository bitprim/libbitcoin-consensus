#include <bitcoin/util/elliptic_curve_key.hpp>

#include <algorithm>
#include <iostream>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>

#include <bitcoin/util/assert.hpp>

namespace libbitcoin {

elliptic_curve_key::elliptic_curve_key()
  : key_(nullptr)
{
}

elliptic_curve_key::~elliptic_curve_key()
{
    if (key_ != nullptr)
        EC_KEY_free(key_);
}

bool elliptic_curve_key::set_public_key(const data_chunk& pubkey)
{
    if (!initialize())
        return false;
    const unsigned char* pubkey_bytes = &pubkey[0];
    if (!o2i_ECPublicKey(&key_, &pubkey_bytes, pubkey.size()))
        return false;
    return true;
}

bool elliptic_curve_key::verify(hash_digest hash, const data_chunk& signature)
{
    BITCOIN_ASSERT(key_ != nullptr);
    // SSL likes a reversed hash
    std::reverse(hash.begin(), hash.end());
    // -1 = error, 0 = bad sig, 1 = good
    if (ECDSA_verify(0, hash.data(), hash.size(), 
            &signature[0], signature.size(), key_) == 1)
        return true;
    return false;
}

bool elliptic_curve_key::new_key_pair()
{
    if (!initialize())
        return false;
    if (!EC_KEY_generate_key(key_))
        return false;
    return true;
}

bool elliptic_curve_key::set_private_key(const private_data& privkey)
{
    if (!initialize())
        return false;
    const byte* privkey_begin = &privkey[0];
    if (!d2i_ECPrivateKey(&key_, &privkey_begin, privkey.size()))
        return false;
    return true;
}

private_data elliptic_curve_key::get_private_key() const
{
    size_t length = i2d_ECPrivateKey(key_, NULL);
    if (!length)
        return private_data();
    private_data privkey(length, 0);
    byte* privkey_begin = &privkey[0];
    if (i2d_ECPrivateKey(key_, &privkey_begin) != length)
        return private_data();
    return privkey;
}

data_chunk elliptic_curve_key::sign(hash_digest hash) const
{
    BITCOIN_ASSERT(key_ != nullptr);
    // SSL likes a reversed hash
    std::reverse(hash.begin(), hash.end());
    data_chunk signature;
    signature.resize(10000);
    unsigned int signature_length = signature.size();
    if (!ECDSA_sign(0, hash.data(), hash.size(), 
            &signature[0], &signature_length, key_))
        return data_chunk();
    signature.resize(signature_length);
    return signature;
}

bool elliptic_curve_key::initialize()
{
    // Already initialized
    if (key_ != nullptr)
        return true;
    key_ = EC_KEY_new_by_curve_name(NID_secp256k1);
    return key_ != nullptr;
}

} // libbitcoin

