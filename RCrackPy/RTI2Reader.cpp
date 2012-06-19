/*
 * freerainbowtables is a project for generating, distributing, and using
 * perfect rainbow tables
 *
 * Copyright 2010, 2011 Martin Westergaard Jørgensen <martinwj2005@gmail.com>
 * Copyright 2010, 2011 James Nobis <quel@quelrod.net>
 * Copyright 2011 Logan Watt <logan.watt@gmail.com>
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

#include "RTI2Reader.h"

RTI2Reader::RTI2Reader( std::string filename )
{
	SubKeySpace subKeySpace;
	CharacterSet charSet;
	char *str;
	uint8 *indexTmp;
	uint32 a, b, count, sum = 0, subKeySpacesCount = 0, hybridSets = 0, passwordLength = 0;
	int ret;
	uint8 characterSetFlags;
	chainPosition = 0;
	// XXX for reading straight from data in memory
	//indexOffset = 0;

	fin.open( filename.c_str(), std::ios_base::binary | std::ios_base::in );

	if( !fin.seekg( 0, std::ios_base::beg ).good() 
		|| !fin.read( (char*) (&header), sizeof(header) ).good() )
	{
		std::cerr << "readHeader fin.read() error" << std::endl;
		exit( 1 ); // file error
	}

	if ( header.tag != 0x32495452 ) // RTI2
	{
		std::cerr << "readHeader bad tag - this is not a RTI2 file" << std::endl;
		exit( 3 ); // bad tag
	}

	if ( header.minor != 0 )
	{
		std::cerr << "readHeader bad minor version" << std::endl;
		exit( 4 ); // bad minor version
	}

	if ( header.startPointBits == 0 || header.startPointBits > 64 )
	{
		std::cerr << "readHeader parsing error" << std::endl;
		std::cerr << "header.startPointBits: " << header.startPointBits << std::endl;
		exit( 2 ); // invalid header
	}

	if ( header.endPointBits == 0 || header.endPointBits > 64 )
	{
		std::cerr << "readHeader parsing error" << std::endl;
		std::cerr << "header.endPointBits: " << header.endPointBits << std::endl;
		exit( 2 ); // invalid header
	}

	if ( header.checkPointBits > 64 )
	{
		std::cerr << "readHeader parsing error" << std::endl;
		std::cerr << "header.checkPointBits: " << header.checkPointBits << std::endl;
		exit( 2 ); // invalid header
	}

	if ( header.startPointBits + header.checkPointBits + header.endPointBits
		> 64 )
	{
		std::cerr << "readHeader parsing error" << std::endl;
		std::cerr << "header.startPointBits + header.checkPointBits + header.endPointBits > 64" << std::endl;
		std::cerr << "header.startPointBits: " << header.startPointBits << std::endl;
		std::cerr << "header.endPointBits: " << header.endPointBits << std::endl;
		std::cerr << "header.checkPointBits: " << header.checkPointBits << std::endl;
		exit( 2 ); // invalid header
	}

	if ( header.fileIndex > header.files )
	{
		std::cerr << "readHeader parsing error" << std::endl;
		std::cerr << "fileIndex: " << header.fileIndex << std::endl;
		std::cerr << "files: " << header.files << std::endl;
		exit( 2 ); // invalid header
	}

	if ( header.algorithm > 19 )
	{
		std::cerr << "readHeader parsing error" << std::endl;
		std::cerr << "undefined algorithm: " << header.algorithm << std::endl;
		exit( 2 ); // invalid header
	}

	if (  header.reductionFunction > 2 
		|| (header.reductionFunction <  1 && header.tableIndex > 65535)
		|| (header.reductionFunction == 1 && header.tableIndex >   255))
	{
		std::cerr << "readHeader parsing error" << std::endl;
		std::cerr << "invalid reductionFunction parameters" << std::endl;
		std::cerr << "header.reductionFunction: " << header.reductionFunction << std::endl;
		std::cerr << "header.tableIndex: " << header.tableIndex << std::endl;
		exit( 2 ); // invalid header
	}

	if ( header.tableIndex != 0
		&&  ((header.reductionFunction < 1 && header.chainLength - 2 > header.tableIndex << 16)
			|| (header.reductionFunction == 2 && header.chainLength > header.tableIndex))) // I think this might be "header.chainLength - 2 > header.tableIndex" need to double check
	{
		// Sc00bz remarks "(these tables suck)"
		std::cerr << "WARNING: Table index is not large enough for this chain length"
			<< std::endl;
	}

	str = new char[1024];

	if ( header.algorithm == 0 )
	{
		std::cerr << "readHeader fin.read() error, reserved algorithm"
			<< std::endl;
		delete [] str;
		exit( 2 );
	}
	
	// Salt
	setSalt( "" );

	if ( header.algorithm == 0
		|| ( header.algorithm >= 15 && header.algorithm <= 19 ))
	{
		ret = readRTI2String( fin, str );

		if ( ret < 0 )
		{
			std::cerr << "readHeader fin.read() error, salt" << std::endl;
			delete [] str;
			exit( -ret );
		}

		str[ret] = 0;
		setSalt( str );
	}
	
	// Sub keyspaces
	subKeySpaces.clear();

	subKeySpacesCount = 0;

	if ( !fin.read( (char*) (&subKeySpacesCount), 1 ).good() )
	{
		std::cerr << "readHeader fin.read() error" << std::endl;
		delete [] str;
		exit( 1 ); // file error
	}

	if  ( subKeySpacesCount == 0 )
	{
		std::cerr << "readHeader fin.read() error" << std::endl;
		std::cerr << "subKeySpaces missing from header" << std::endl;
		delete [] str;
		exit( 2 ); // invalid header
	}

	for ( a = 0; a < subKeySpacesCount; a++ )
	{
		subKeySpace.perPositionCharacterSets.clear();
		subKeySpace.passwordLength.clear();

		if ( !fin.read( (char*) (&hybridSets), 1 ).good() )
		{
			std::cerr << "readHeader fin.read() error" << std::endl;
			delete [] str;
			exit( 1 ); // file error
		}
	
		if ( hybridSets == 0 )
		{
			std::cerr << "readHeader fin.read() error, hybridSets is 0"
				<< std::endl;
			delete [] str;
			exit( 2 ); // invalid header
		}

		subKeySpace.hybridSets = hybridSets;

		// Hybrid sets
		for ( b = 0; b < hybridSets; b++ )
		{
			// Password length
			if ( !fin.read( (char*) (&passwordLength), 1 ).good() )
			{
				std::cerr << "readHeader fin.read() error" << std::endl;
				delete [] str;
				exit( 1 ); // file error
			}

			if ( passwordLength == 0 )
			{
				std::cerr << "readHeader fin.read() error, passwordLength is 0"
					<< std::endl;
				delete [] str;
				exit( 2 ); //invalid header
			}

			subKeySpace.passwordLength.push_back( passwordLength );

			// Character set flags
			if ( !fin.read( (char*) (&characterSetFlags), 1 ).good() )
			{
				std::cerr << "readHeader fin.read() error" << std::endl;
				delete [] str;
				exit( 1 ); // file error
			}

			if ( characterSetFlags == 0 )
			{
				std::cerr << "readHeader fin.read() error, characterSetFlags is 0"
					<< std::endl;
				delete [] str;
				exit( 2 ); // invalid header
			}

			subKeySpace.charSetFlags.push_back( characterSetFlags );

			// Character set
			charSet.characterSet1.clear();
			charSet.characterSet2.clear();
			charSet.characterSet3.clear();
			charSet.characterSet4.clear();

			if ( characterSetFlags & 1 )
			{
				ret = readRTI2String( fin, str, 1 );
				if ( ret < 0 )
				{
					delete [] str;
					exit( -ret );
				}

				charSet.characterSet1.assign((uint8*) str, ((uint8*) str) + ret );
			}

			if ( characterSetFlags & 2 )
			{
				ret = readRTI2String( fin, str, 2 );

				if ( ret < 0 )
				{
					delete [] str;
					exit( -ret );
				}

				charSet.characterSet2.assign((uint16*) str, ((uint16*) str) + ret );
			}

			if ( characterSetFlags & 4 )
			{
				ret = readRTI2String( fin, str, 3 );

				if ( ret < 0 )
				{
					delete [] str;
					exit( -ret );
				}

				charSet.characterSet3.assign((uint24*) str, ((uint24*) str) + ret );
			}

			if ( characterSetFlags & 8 )
			{
				ret = readRTI2String( fin, str, 4 );

				if ( ret < 0 )
				{
					delete [] str;
					exit( -ret );
				}

				charSet.characterSet4.assign((uint32*) str, ((uint32*) str) + ret );
			}

			subKeySpace.perPositionCharacterSets.push_back(charSet);
		}
	
		subKeySpaces.push_back(subKeySpace);
	}

	// Check point positions
	if ( !fin.read( str, 4 * header.checkPointBits ).good() )
	{
		std::cerr << "readHeader fin.read() error" << std::endl;
		delete [] str;
		exit( 1 ); // file error
	}

	checkPointPositions.assign((uint32*) str, ((uint32*) str) + header.checkPointBits);
	delete [] str;
	
	// *** Index ***
	if ( !fin.read( (char*) (&index.firstPrefix), 8 ).good() )
	{
		std::cerr << "readIndex fin.read() error" << std::endl;
		exit( 1 ); // file error
	}

	if ( !fin.read((char*) (&count), 4 ).good() )
	{
		std::cerr << "readIndex fin.read() error" << std::endl;
		exit( 1 ); // file error
	}

	if ( count == 0 )
	{
		std::cerr << "readIndex fin.read() error, prefixIndex count is 0"
			<< std::endl;
		exit( 1 ); // file error
	}

	indexTmp = new uint8[count];

	if ( !fin.read( (char*) indexTmp, count ).good() )
	{
		std::cerr << "readIndex fin.read() error" << std::endl;
		delete [] indexTmp;
		exit( 1 ); // file error
	}

	index.prefixIndex.reserve(count + 1);
	index.prefixIndex.push_back(sum);

	for (a = 0; a < count; a++)
	{
		sum += indexTmp[a];
		index.prefixIndex.push_back(sum);
	}

	delete [] indexTmp;

	// *** Data ***
	chainSizeBytes = (header.startPointBits + header.checkPointBits + header.endPointBits + 7) >> 3;
	chainCount = sum;

/*
 * reads everything at once...doesn't work for memory constrained env
	data = new uint8[chainSizeBytes * sum + 8 - chainSizeBytes]; // (8 - chainSizeBytes) to avoid "reading past the end of the array" error

	if ( !fin.read( (char*) (data), chainSizeBytes * sum ).good() )
	{
		std::cerr << "readData fin.read() error" << std::endl;
		delete [] data;
		data = NULL;
		exit( 1 ); // file error
	}
*/
}

RTI2Reader::~RTI2Reader()
{
	if ( fin.good() )
		fin.close();

/*
	if ( data != NULL )
		delete [] data;

	data = NULL;
*/
}

int RTI2Reader::readRTI2String( std::ifstream &fin, void *str, uint32 charSize )
{
/*
 * String length (1 byte)
 * String (String length bytes or (charSize * (String length + 1)) bytes)
*/
	uint32 size = 0;
	int ret = 0;

	if ( !fin.read( (char*) (&ret), 1 ).good() )
	{
		std::cerr << "readHeader find.read() error" << std::endl;
		return -1; // file error
	}

	if ( charSize )
	{
		ret++;
		size = charSize * ret;
	}

	if ( size > 0 )
	{
		if ( !fin.read( (char*) str, size ).good() )
		{
			std::cerr << "readHeader fin.read() error" << std::endl;
			return -1; //file error
		}
	}

	return ret;
}

uint32 RTI2Reader::getChainsLeft()
{
	return chainCount - chainPosition;
}

void RTI2Reader::Dump()
{
	std::cout << "header.tag: " << header.tag << std::endl;
	std::cout << "header.minor: " << (uint32)header.minor << std::endl;
	std::cout << "header.startPointBits: " << (uint32)header.startPointBits
		<< std::endl;
	std::cout << "header.endPointBits: " << (uint32)header.endPointBits
		<< std::endl;
	std::cout << "header.checkPointBits: "	<< (uint32)header.checkPointBits
		<< std::endl;
	std::cout << "header.fileIndex: " << (uint32)header.fileIndex << std::endl;
	std::cout << "header.files: " << (uint32)header.files << std::endl;
	std::cout << "header.minimumStartPoint: "
		<< (uint64)header.minimumStartPoint << std::endl;
	std::cout << "header.chainLength: "
		<< (uint32)header.chainLength << std::endl;
	std::cout << "header.tableIndex: "
		<< (uint16)header.tableIndex << std::endl;
	std::cout << "header.algorithm: "
		<< (uint32)header.algorithm << std::endl;
	std::cout << "header.reductionFunction: "
		<< (uint32)header.reductionFunction << std::endl;

	if ( getSalt().size() > 0 )
		std::cout << "header.salt: " << getSalt() << std::endl;

	std::cout << "subKeySpaces Count: " << subKeySpaces.size() << std::endl;

	for ( uint32 i = 0; i < subKeySpaces.size(); i++ )
	{
		std::cout << "subKeySpace " << i + 1 << std::endl;
		std::cout << "Number of hybrid sets: "
			<< (uint32)subKeySpaces[i].hybridSets << std::endl;

		for ( uint32 j = 0; j < (uint32)subKeySpaces[i].hybridSets; j++ )
		{
			std::cout << "Hybrid set " << j + 1 << std::endl;
			std::cout << "Password length: "
				<< (uint32)subKeySpaces[i].passwordLength[j] << std::endl;
			std::cout << "charSetFlags: "
				<< (uint32)subKeySpaces[i].charSetFlags[j] << std::endl;

			if ( subKeySpaces[i].charSetFlags[j] & 1 )
			{
				std::cout << "characterSet1: ";
				
				for ( uint32 k = 0; k < subKeySpaces[i].perPositionCharacterSets[j].characterSet1.size(); k++ )
				{
					std::cout << subKeySpaces[i].perPositionCharacterSets[j].characterSet1[k];
				}
				std::cout << std::endl;
			}
		}
		
	}

	std::cout << "header.checkPointBits: " << (uint32)header.checkPointBits
		<< std::endl;

	if ( (uint32)header.checkPointBits > 0 )
	{
		std::cout << "checkPointPositions:";

		for ( uint32 i = 0; i < (uint32)header.checkPointBits; i++ )
		{
			std::cout << " " << checkPointPositions[i];
		}

		std::cout << std::endl;
	}

	std::cout << "index.firstPrefix: " << index.firstPrefix << std::endl;
	std::cout << "index.prefixIndex.size(): "
		<< index.prefixIndex.size() << std::endl;

	/*
	for ( uint32 i = 0; i < index.prefixIndex.size(); i++ )
	{
		std::cout << "index.prefixIndex[" << i << "]: "
			<< index.prefixIndex[i] << std::endl;
	}
	*/

	// XXX data
}

// convert in memory rti2 to rt reading from the file as needed
int RTI2Reader::readChains(unsigned int &numChains, RainbowChainO *pData)
{
	unsigned int readChains = 0;
	unsigned int chainsleft = getChainsLeft();

	uint64 endPointMask = (( (uint64) 1 ) << header.endPointBits ) - 1;
	uint64 startPointMask = (( (uint64) 1 ) << header.startPointBits ) - 1;
	uint32 startPointShift = header.endPointBits;
/*	
	uint64 checkPointMask = (( (uint64) ) << header.checkPointBits ) - 1;
	uint32 checkPointShift = header.endPointBits + header.StartPointBits;
*/

	uint64 chainrow = 0;
	uint32 i=1;
	uint8 str[8] = {0};
	
	// Fast forward to current position
	// XXX for chainPosition != 0 use a binary search
	// XXX get rid of this whole monstronsity for a nice pointer
	
	for( i = 1; i < index.prefixIndex.size(); i++ )
	{
		if ( i == 1 && index.prefixIndex[i] > 0 && chainPosition == 0 )
		{
			break;
		}

		if ( ( chainPosition + readChains ) > index.prefixIndex[i] )
		{
			i++;
			break;
		}
	}

	for( ; i < index.prefixIndex.size(); i++ )
	{
		/* XXX this is completely redundant
		 * try the next index position

		if (  (chainPosition + readChains ) > index.prefixIndex[i] )
			continue;
		*/
		
		while ( (chainPosition + readChains ) < index.prefixIndex[i] )
		{
			// XXX for reading straight from data in memory
			//chainrow = *((uint64*) ( data + ( ( index.prefixIndex[i-1] + indexOffset ) * chainSizeBytes ) ));
			if ( !fin.read( (char*) (str), chainSizeBytes ).good() )
			{
				std::cerr << "readData fin.read() error" << std::endl;
				exit( 1 ); // file error
			}

			chainrow = *((uint64*) str);

			// end point prefix
			pData[readChains].nIndexE = ( index.firstPrefix + i - 1 ) << header.endPointBits;
			// end point suffix
			pData[readChains].nIndexE |= chainrow & endPointMask;

			pData[readChains].nIndexS = ((chainrow >> startPointShift) & startPointMask) + header.minimumStartPoint;

			readChains++;
			// XXX for reading straight from data in memory
			//indexOffset++;

			if ( readChains == numChains || readChains == chainsleft )
				break;
		}

		if ( readChains == numChains )
			break;

		// XXX for reading straight from data in memory
		//indexOffset = 0;
	}

	if ( readChains != numChains )
		numChains = readChains; // Update how many chains we read

	chainPosition += readChains;
	return 0;
}

void RTI2Reader::setMinimumStartPoint( uint64 minimumStartPoint )
{
	header.minimumStartPoint = minimumStartPoint;
}

/// getChainSizeBytes
uint32 RTI2Reader::getChainSizeBytes()
{
	return this->chainSizeBytes;
}

/// getMinimumStartPoint
uint64 RTI2Reader::getMinimumStartPoint()
{
	return header.minimumStartPoint;
}
