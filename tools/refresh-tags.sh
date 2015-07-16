#!/bin/bash

rm -f TAGS

find . -name *.h -print | grep -v build-ext |while read I
do
  etags --append "$I"
done


find . -name *.cpp -print | grep -v 'moc_' | grep -v build-ext | while read I
do
  etags --append "$I"
done
