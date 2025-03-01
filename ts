#!/bin/bash
TIME=
set -eux
rm -rf crypt tst
mkdir crypt tst
git -C crypt init -b master --bare
git -C tst init -b master
$TIME ./fastcrypt e refs/tags/secrettag ~/sysconf/ crypt
$TIME ./fastcrypt d refs/tags/secrettag crypt tst/
$TIME ./fastcrypt e refs/heads/master ~/sysconf/ crypt
$TIME ./fastcrypt d refs/heads/master crypt tst/
$TIME ./fastcrypt e refs/heads/master ~/sysconf/ crypt
$TIME ./fastcrypt d refs/heads/master crypt tst/
git -C tst checkout master
