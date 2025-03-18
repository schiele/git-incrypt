#!/bin/bash
TIME=
PATH="$PWD:$PATH"
VERBOSE=
set -eux
rm -rf crypt tst
mkdir crypt
git -C crypt init
git ls-remote ~/sysconf/
REPO=incrypt::$PWD/crypt
git incrypt init $REPO 5A8A11E44AD2A1623B84E5AFC5C0C5C7218D18D7
git -C ~/sysconf/ fetch $REPO || git -C ~/sysconf/ incrypt trust $REPO
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
