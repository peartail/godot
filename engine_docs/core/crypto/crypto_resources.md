# Crypto Resources

## Scope

Script-facing crypto API, crypto resource formats, and compressed certificate data.

## Entry Points

- `Crypto`
- `CryptoKey`
- `X509Certificate`
- `CryptoResourceFormatLoader`
- `CryptoResourceFormatSaver`

## Flow Notes

- `Crypto` exposes higher-level key, certificate, random, and signing helpers.
- Resource format handlers load/save crypto resources.
- Certificate data is generated into compressed core IO data.

## Code Links

- `core/crypto/crypto.*`
- `core/crypto/crypto_resource_format.*`
- `core/io/certs_compressed.gen.h`