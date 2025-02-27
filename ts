#!/bin/bash
for i in tst crypt; do
       rm -rf $i
       mkdir $i
       git -C $i init
done 
time ./fastcrypt e refs/heads/master ~/sysconf/ crypt
time ./fastcrypt d refs/heads/master crypt tst/
time ./fastcrypt e refs/heads/master ~/sysconf/ crypt
time ./fastcrypt d refs/heads/master crypt tst/
