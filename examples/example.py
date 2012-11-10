#!/usr/bin/env python
'''
@author: moloch
@copyright: GPL

Compile RCrackPy and put RainbowCrack.so
onto the PYTHONPATH or in the cwd
'''

import os
import RainbowCrack # This is RainbowCrack.so

# Rainbow tables directory
tables = "/media/data/RainbowTables/MD5/"

print "[*] " + RainbowCrack.version()
if not os.path.exists(tables):
	print '[!] Rainbow tables directory not found'
	os._exit(1)

md5_hashes = [
	"af5b3d17aa1e2ff2a0f83142d692d701", 
	"5d41402abc4b2a76b9719d911017c592",
	"5f4dcc3b5aa765d61d8327deb882cf99",
	]

print "[*] Cracking a list of hashes, please wait..."
results = RainbowCrack.crack(md5_hashes, tables, debug=True, maxThreads=4)
print "[+] Got:", results

