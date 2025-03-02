#!/bin/bash
TIME=
set -eux
rm -rf crypt tst
mkdir crypt tst
git -C crypt init -b master --bare
git -C tst init -b master
$TIME ./fastcrypt e ~/sysconf/ crypt refs/tags/secrettag
$TIME ./fastcrypt d crypt tst/ refs/tags/secrettag
$TIME ./fastcrypt e ~/sysconf/ crypt refs/heads/master
$TIME ./fastcrypt d crypt tst/ refs/heads/master
$TIME ./fastcrypt e ~/sysconf/ crypt refs/heads/master refs/tags/secrettag
$TIME ./fastcrypt d crypt tst/ refs/heads/master refs/tags/secrettag refs/heads/master~2:refs/heads/master2
git -C tst checkout master
pylint fastcrypt
