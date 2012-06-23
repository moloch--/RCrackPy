#!/bin/bash

sudo apt-get install build-essential python-dev libboost-python-dev

echo ""
echo "[*] Your default Python version is:"
python -V
echo ""
cd ./RCrackPy
make clean
# make install
