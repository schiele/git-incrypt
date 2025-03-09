# Format

This document descripes the current data format in the encrypted git
repository in the first section. The second section describes the format
planned in the future.

## Current Format

### Metadata

`refs/heads/key:0`:
- format ID: 4 bytes
- gpg encypted data: remaining bytes:
  - key (iv+key): 48 bytes
  - name: NULL-terminated string
  - email: NULL-terminated string
  - time: NULL-terminated string
  - offset: NULL-terminated string
  - message: NULL-terminated string

`refs/heads/map:0`:
- encrypted data:
  - sha1 of remaining data: 20 bytes
  - list
    - id of clear commit or tag: 20 bytes
    - id of encrypted commit: 20 bytes

### Content

`refs/heads/CRYPTREF`:
- commit from template with parents mapping the clear structure

`refs/heads/CRYPTREF:NUMBER`:
- encrypted data:
  - clear commit id: 20 bytes
  - object type: 1 byte
  - object payload: remaining bytes

## Planned Format

### Metadata

`refs/heads/_:key`:
- gpg encrypted data:
  - key type: NULL-terminated string
  - key: remaining bytes

`refs/heads/_:msg`:
- encrypted data:
  - sha1 of remaining data: 20 bytes
  - commit template: remaining bytes

`refs/heads/_:sig/HASHofBLOB`:
- encrypted data:
  - sha1 of remaining data: 20 bytes
  - detached gpg signature of cleartext key: remaining bytes

`refs/heads/_:map`:
- encrypted data:
  - sha1 of remaining data: 20 bytes
  - list
    - id of clear commit or tag: 20 bytes
    - id of encrypted commit: 20 bytes

### Content

Unchanged from current format.
