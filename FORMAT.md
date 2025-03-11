# Format

This document descripes the current data format in the encrypted git
repository.

## Metadata

- `refs/heads/_:ver`:
  - format ID: string

  This is a unique version identifier. Once the format reached initial
  stability this will change with every incompatible change to the format.

- `refs/heads/_:key`:
  - gpg encrypted data:
    - key type: NULL-terminated string
    - key: remaining bytes

  This is a unique key format identifier. Whenever we change the algorithm
  that interpretes this key in an incompatible way this will change. This will
  allow for multiple algorithms or ciphers in the future.

- `refs/heads/_:msg`:
  - encrypted data:
    - sha1 of remaining data: 20 bytes
    - commit template: remaining bytes

  This is the complete commit object to be used for all commits in the
  encrypted repository without the tree reference and any parent references.
  While it does not contain any secret information it is still encrypted to
  avoid someone without the key from tampering with this data.

- `refs/heads/_:sig/HASHofBLOB`:
  - encrypted data:
    - sha1 of remaining data: 20 bytes
    - detached gpg signature of cleartext key: remaining bytes

  These are GPG signatures of the cleartext key to prevent an attacker from
  replacing the key with his own and therefore make you leaking content
  towards him. The signature itself is encrypted since there is no reason to
  reveal the identity of signers towards users that do not own the key of this
  repository.

- `refs/heads/_:def`:
  - encrypted data:
    - sha1 of remaining data: 20 bytes
    - default branch: string

  Name of the default branch in the cleartext repository. The default branch
  of the encrypted repository will always point to `refs/heads/_` to avoid
  leaking information about the name of the default branch.

- `refs/heads/_:map`:
  - encrypted data:
    - sha1 of remaining data: 20 bytes
    - list
      - id of clear commit or tag: 20 bytes
      - id of encrypted commit: 20 bytes

  This is a mapping table of all commits and annotated tags in the cleartext
  repository to their representatives in the encrypted repository. Obviously
  this is encrypted since the commit hashes of the cleartext repository are
  sensitive information.

- `refs/heads/_:README.md`:
  - plain text data

  This is a simple readme that explains to an innocent reader of this
  repository that this is an encrypted repository and how to handle it. This
  is purely for informational purposes and never read by the tool itself.

## Content

- `refs/heads/CRYPTREF`:
  - commit from template with parents mapping the clear structure

  Those commits are created from the decrypted commit template at
  `refs/heads/_:msg` with the tree reference and the parent references filled
  in.

- `refs/heads/CRYPTREF:ZERO_OR_HASH`:
  - encrypted data:
    - clear object id: 20 bytes
    - object type: 1 byte
    - object payload: remaining bytes

  `ZERO_OR_HASH` is either `0` for objects that represent commits or tags in
  cleartext repository or the object hash of the encrypted object. The precise
  name is conceptually not important but needs to be unique for trees and
  blobs and commits and tags need to be easily identified for an efficient
  decryption process.

  This object does exist for each object related to the commit or tag in the
  cleartext repository represented by `CRYPTREF`. In the case of representing
  a tag this is only the tag itself. In case of representing a commit this
  includes the commit object itself plus all tree and blob objects recursively
  included in the tree of that commit.
