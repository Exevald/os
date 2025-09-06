#!/usr/bin/env bash

set -e

tar -czf proj.tar.gz proj/

if [ -d "out" ]; then
    read -p "Directory already exists. Recreate? (y/N): " answer
    if [ "$answer" = "y" ] || [ "$answer" = "Y" ]; then
        rm -rf out
        mkdir out
        echo "The out directory has been recreated."
    else
        echo "Cancelled."
        exit 1
    fi
else
    mkdir out
fi

cp proj.tar.gz out/

cd out
tar -xzf proj.tar.gz
rm proj.tar.gz

mkdir -p includ;e src build

mv ./proj/*.h include/
mv ./proj/*.cpp src/ 

g++ -I ./include -o build/main src/*.cpp

echo "30" > input.txt
echo "12" >> input.txt
./build/main < input.txt > stdout.txt
rm input.txt

cd ..