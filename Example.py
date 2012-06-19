#!/usr/bin/env python

import RainbowCrack

tables = "/media/data/RainbowTables/MD5/"

md5_hashes = [
	"af5b3d17aa1e2ff2a0f83142d692d701", 
	"5d41402abc4b2a76b9719d911017c592",
	"5f4dcc3b5aa765d61d8327deb882cf99",
]
md5_hash = "af5b3d17aa1e2ff2a0f83142d692d701"

print "[*] Cracking a list of hashes, please wait..."
results = RainbowCrack.crack(len(md5_hashes), md5_hashes, tables, maxThreads = 4)
print "Got:", results

print "[*] Cracking single hash, please wait..."
result = RainbowCrack.single_hash(md5_hash, tables, maxThreads = 4)
print "Got:", result
