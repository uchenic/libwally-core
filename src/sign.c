#include <include/wally_core.h>
#include <include/wally_crypto.h>
#include "internal.h"
#include "secp256k1/include/secp256k1_schnorr.h"
#include "ccan/ccan/build_assert/build_assert.h"
#include <stdbool.h>

#define EC_FLAGS_TYPES (EC_FLAG_ECDSA | EC_FLAG_SCHNORR)
#define EC_FLAGS_ALL (EC_FLAG_ECDSA | EC_FLAG_SCHNORR)

/* LCOV_EXCL_START */
/* Check assumptions we expect to hold true */
static void assert_assumptions(void)
{
    BUILD_ASSERT(sizeof(secp256k1_ecdsa_signature) == EC_SIGNATURE_LEN);
}
/* LCOV_EXCL_STOP */

static bool is_valid_ec_type(uint32_t flags)
{
    return ((flags & EC_FLAGS_TYPES) == EC_FLAG_ECDSA) ||
           ((flags & EC_FLAGS_TYPES) == EC_FLAG_SCHNORR);
}


int wally_ec_private_key_verify(const unsigned char *priv_key, size_t priv_key_len)
{
    secp256k1_context *ctx;

    if (!priv_key || priv_key_len != EC_PRIVATE_KEY_LEN)
        return WALLY_EINVAL;

    if (!(ctx = (secp256k1_context *)secp_ctx()))
        return WALLY_ENOMEM;

    return secp256k1_ec_seckey_verify(ctx, priv_key) ? WALLY_OK : WALLY_EINVAL;
}

int wally_ec_public_key_from_private_key(const unsigned char *priv_key, size_t priv_key_len,
                                         unsigned char *bytes_out, size_t len)
{
    secp256k1_pubkey pub;
    size_t len_in_out = EC_PUBLIC_KEY_LEN;
    const secp256k1_context *ctx = secp_ctx();
    bool ok;

    ok = priv_key && priv_key_len == EC_PRIVATE_KEY_LEN &&
         bytes_out && len == EC_PUBLIC_KEY_LEN && ctx &&
         pubkey_create(ctx, &pub, priv_key) &&
         pubkey_serialize(ctx, bytes_out, &len_in_out, &pub, PUBKEY_COMPRESSED) &&
         len_in_out == EC_PUBLIC_KEY_LEN;

    if (!ok && bytes_out)
        clear(bytes_out, len);
    clear(&pub, sizeof(pub));
    return ok ? WALLY_OK : WALLY_EINVAL;
}

int wally_ec_sig_from_bytes(const unsigned char *priv_key, size_t priv_key_len,
                            const unsigned char *bytes_in, size_t len_in,
                            uint32_t flags,
                            unsigned char *bytes_out, size_t len)
{
    secp256k1_context *ctx;

    if (!priv_key || priv_key_len != EC_PRIVATE_KEY_LEN ||
        !bytes_in || len_in != EC_MESSAGE_HASH_LEN ||
        !is_valid_ec_type(flags) || flags & ~EC_FLAGS_ALL ||
        !bytes_out || len != EC_SIGNATURE_LEN)
        return WALLY_EINVAL;

    if (!(ctx = (secp256k1_context *)secp_ctx()))
        return WALLY_ENOMEM;

    if (flags & EC_FLAG_SCHNORR)
        return WALLY_EINVAL; /* Not implemented yet */
    else {
        wally_ec_nonce_t nonce_fn = wally_ops()->ec_nonce_fn;
        secp256k1_ecdsa_signature sig;

        if (!secp256k1_ecdsa_sign(ctx, &sig, bytes_in, priv_key, nonce_fn, NULL)) {
            if (secp256k1_ec_seckey_verify(ctx, priv_key))
                return WALLY_ERROR; /* Nonce function failed */
            return WALLY_EINVAL; /* invalid priv_key */
        }

        /* Note this function is documented as never failing */
        secp256k1_ecdsa_signature_serialize_compact(ctx, bytes_out, &sig);
        clear(&sig, sizeof(sig));
    }

    return WALLY_OK;
}

int wally_ec_sig_verify(const unsigned char *pub_key, size_t pub_key_len,
                        const unsigned char *bytes_in, size_t len_in,
                        uint32_t flags,
                        const unsigned char *sig_in, size_t sig_in_len)
{
    secp256k1_context *ctx;

    if (!pub_key || pub_key_len != EC_PUBLIC_KEY_LEN ||
        !bytes_in || len_in != EC_MESSAGE_HASH_LEN ||
        !is_valid_ec_type(flags) || flags & ~EC_FLAGS_ALL ||
        !sig_in || sig_in_len != EC_SIGNATURE_LEN)
        return WALLY_EINVAL;

    if (!(ctx = (secp256k1_context *)secp_ctx()))
        return WALLY_ENOMEM;

    if (flags & EC_FLAG_SCHNORR)
        return WALLY_EINVAL; /* Not implemented yet */
    else {
        secp256k1_pubkey pub;
        secp256k1_ecdsa_signature sig;
        bool ok;

        ok = pubkey_parse(ctx, &pub, pub_key, pub_key_len) &&
             secp256k1_ecdsa_signature_parse_compact(ctx, &sig, sig_in) &&
             secp256k1_ecdsa_verify(ctx, &sig, bytes_in, &pub);

        clear_n(2, &pub, sizeof(pub), &sig, sizeof(sig));
        return ok ? WALLY_OK : WALLY_EINVAL;
    }
}