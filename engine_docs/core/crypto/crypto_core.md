# Crypto Core

## Scope

Low-level cryptographic helpers, hashing primitives, and AES implementation support.

## Entry Points

- `CryptoCore`
- `AESContext`
- `HashingContext`

## Flow Notes

- Low-level helpers should remain independent of high-level resource and editor systems.
- AES and hashing behavior may be used by file encryption, networking, and script-facing APIs.
- Availability can depend on compiled crypto backend support.

## Code Links

- `core/crypto/crypto_core.h`
- `core/crypto/crypto_core.cpp`
- `core/crypto/aes_context.*`
- `core/crypto/hashing_context.*`