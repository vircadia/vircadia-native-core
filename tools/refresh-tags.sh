#!/bin/bash

rm -f TAGS

find . -name *.h -print | while read I
do
  etags --append "$I"
done


find . -name *.cpp -print | grep -v 'moc_' | while read I
do
  etags --append "$I"
done
