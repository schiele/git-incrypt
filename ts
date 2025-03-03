#!/bin/bash
TIME=
PATH="$PWD:$PATH"
set -eux
rm -rf crypt tst
mkdir crypt tst
git -C tst init -b master
git ls-remote ~/sysconf/
./fastcrypt i crypt 5A8A11E44AD2A1623B84E5AFC5C0C5C7218D18D7,F2AE0B6CB4089F15A217F12D121F59FB963341A4 \
	"$(git config user.name)" \
	"$(git config user.email)" \
	1740000000 2 \
	"Go away, there is nothing to see here!"
$TIME ./fastcrypt e ~/sysconf/ crypt refs/tags/secrettag
$TIME ./fastcrypt d crypt tst/ refs/tags/secrettag
$TIME ./fastcrypt e ~/sysconf/ crypt refs/heads/master
$TIME ./fastcrypt d crypt tst/ refs/heads/master
$TIME ./fastcrypt e ~/sysconf/ crypt refs/heads/master refs/tags/secrettag
$TIME ./fastcrypt d crypt tst/ refs/heads/master refs/tags/secrettag refs/heads/master~2:refs/heads/master2
$TIME ./fastcrypt e ~/sysconf/ crypt
$TIME ./fastcrypt d crypt tst/
git push incrypt::crypt HEAD:refs/heads/fromthere
./fastcrypt l crypt
git ls-remote tst
git -C tst checkout master
pylint fastcrypt
pycodestyle fastcrypt
