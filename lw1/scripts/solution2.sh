#!/usr/bin/env bash

set -e

if test -d "out"; then
    echo "The out directory exists, deleting contents..."
    rm -rf out/*
else
    mkdir out
fi

cd out

whoami > me.txt

cp me.txt metoo.txt

man wc > wchelp.txt

echo "Contents of wchelp.txt:"
cat wchelp.txt
echo ""

wc -l wchelp.txt | cut -d ' ' -f1 > wchelp-lines.txt

tac wchelp.txt > wchelp-reversed.txt

cat wchelp.txt wchelp-reversed.txt me.txt metoo.txt wchelp-lines.txt > all.txt

tar -cf result.tar *.txt

gzip result.tar

cd ..

if test -f "result.tar.gz"; then
    echo "File result.tar.gz already exists, deleting..." 
    rm result.tar.gz
fi

mv out/result.tar.gz ./

rm -rf ./out/*
rmdir out