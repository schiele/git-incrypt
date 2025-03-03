#!/bin/bash
TIME=
PATH="$PWD:$PATH"
set -eux
rm -rf crypt tst
mkdir crypt
#git -C tst init -b master
git ls-remote ~/sysconf/
REPO=incrypt::$PWD/crypt
./oldif i crypt 5A8A11E44AD2A1623B84E5AFC5C0C5C7218D18D7,F2AE0B6CB4089F15A217F12D121F59FB963341A4 \
	"$(git config user.name)" \
	"$(git config user.email)" \
	1740000000 2 \
	"Go away, there is nothing to see here!"
rm -rf ~/sysconf/.git/incrypt/
git -C ~/sysconf/ push $REPO master~2:refs/heads/master
git clone $REPO tst
git -C ~/sysconf/ push $REPO secrettag
git -C tst pull
git -C ~/sysconf/ push $REPO master
git -C tst pull
git -C ~/sysconf/ push --all $REPO
git -C tst pull
./oldif l crypt
git ls-remote tst

pylint git-incrypt
pycodestyle git-incrypt
