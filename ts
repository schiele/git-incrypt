#!/bin/bash
TIME=
PATH="$PWD:$PATH"
VERBOSE=
set -eux
rm -rf crypt tst
mkdir crypt
git ls-remote ~/sysconf/
REPO=incrypt::$PWD/crypt
git -C crypt incrypt init \
	--name "$(git config user.name)" --email "$(git config user.email)" \
	--time 1740000000 --offset 2 \
	--message "Go away, there is nothing to see here!" \
	5A8A11E44AD2A1623B84E5AFC5C0C5C7218D18D7 \
	F2AE0B6CB4089F15A217F12D121F59FB963341A4
rm -rf ~/sysconf/.git/incrypt/
git -C ~/sysconf/ push $VERBOSE $REPO master~2:refs/heads/master
git clone $VERBOSE $REPO tst
git -C ~/sysconf/ push $VERBOSE $REPO secrettag
git -C tst pull $VERBOSE
git -C ~/sysconf/ push $VERBOSE $REPO master
git -C tst pull $VERBOSE
git -C ~/sysconf/ push $VERBOSE --all $REPO
git -C tst pull $VERBOSE
git ls-remote crypt
git -C tst ls-remote $REPO
git ls-remote tst

pylint git-incrypt
pycodestyle git-incrypt
