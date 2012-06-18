/*
 * freerainbowtables is a project for generating, distributing, and using
 * perfect rainbow tables
 *
 * Copyright 2011 Steve Thomas (Sc00bz)
 * Copyright 2011 James Nobis <quel@quelrod.net>
 *
 * This file is part of freerainbowtables.
 *
 * freerainbowtables is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * freerainbowtables is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with freerainbowtables.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _RTI2COMMON_H
#define _RTI2COMMON_H

#include "Public.h"

struct uint24
{
	uint8 data[3];
};

struct Chain
{
	uint64 startPoint, endPoint;
//	uint64 checkBits;
};

struct CharacterSet
{
	std::vector<uint8>  characterSet1;
	std::vector<uint16> characterSet2;
	std::vector<uint24> characterSet3;
	std::vector<uint32> characterSet4;
};

struct SubKeySpace
{
	uint8 hybridSets;
	std::vector<uint8> passwordLength;
	std::vector<uint8> charSetFlags;
	std::vector<CharacterSet> perPositionCharacterSets;
};

struct RTI20_Header_RainbowTableParameters
{
	uint64 minimumStartPoint;
	uint32 chainLength;
	uint32 tableIndex;
	uint8 algorithm;
		// 0 - Reserved
		// 1 - LM
		// 2 - NTLM
		// 3 - MD2
		// 4 - MD4
		// 5 - MD5
		// 6 - Double MD5
		// 7 - Binary Double MD5
		// 8 - Cisco Pix (96 bit MD5)
		// 9 - SHA1
		// 10 - MySQL SHA1
		// 11 - SHA256
		// 12 - SHA384
		// 13 - SHA512
		// 14 - RipeMD160
		// 15 - MSCache
		// 16 - Half LM Challenge
		// 17 - Second Half LM Challenge
		// 18 - NTLM Challenge
		// 19 - Oracle
	uint8  reductionFunction;
		// 0 - RC   ("RainbowCrack" uses divide - project-rainbowcrack.com)
		// 1 - FPM  ("Fixed Point Multiplication" - tobtu.com)
		// 2 - GRT  ("GPU RT" uses lookup tables - cryptohaze.com)
	std::string salt;
	std::vector<SubKeySpace> subKeySpaces;
	std::vector<uint32> checkPointPositions;
};

struct RTI20_Header
{
	uint8 major, minor; // '2', '0'
	uint8 startPointBits, endPointBits, checkPointBits;
	uint32 fileIndex, files;
	RTI20_Header_RainbowTableParameters rtParams;
};

struct RTI20_Index
{
	uint64 firstPrefix;
	std::vector<uint32> prefixIndex;
};

struct RTI20_File
{
	RTI20_Header header;
	RTI20_Index index;
	struct // RTI20_Data
	{
		uint8 *data;
	};
};

#pragma pack(1)
struct RTI20_File_Header
{
	uint32 tag; // "RTI2"
	uint8 minor; // '0'
	uint8 startPointBits, endPointBits, checkPointBits;
	uint32 fileIndex, files;
	struct
	{
		uint64 minimumStartPoint;
		uint32 chainLength;
		uint32 tableIndex;
		uint8 algorithm;
			// 0 - Custom
			// 1 - LM
			// 2 - NTLM
			// 3 - MD2
			// 4 - MD4
			// 5 - MD5
			// 6 - Double MD5
			// 7 - Binary Double MD5
			// 8 - Cisco Pix (96 bit MD5)
			// 9 - SHA1
			// 10 - MySQL SHA1
			// 11 - SHA256
			// 12 - SHA384
			// 13 - SHA512
			// 14 - RipeMD160
			// 15 - MSCache
			// 16 - Half LM Challenge
			// 17 - Second Half LM Challenge
			// 18 - NTLM Challenge
			// 19 - Oracle
		uint8  reductionFunction;
			// 0 - RC   ("RainbowCrack" uses divide - project-rainbowcrack.com)
			// 1 - FPM  ("Fixed Point Multiplication" - tobtu.com)
			// 2 - GRT  ("GPU RT" uses lookup tables - cryptohaze.com)
	};
};
#pragma pack()

// you can't use pack(0) - it makes VC++ angry

uint8 getAlgorithmId( std::string algorithmName );

#endif
