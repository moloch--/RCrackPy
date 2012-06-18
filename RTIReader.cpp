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

#include "RTIReader.h"

/// Default constructor
RTIReader::RTIReader()
{
	RTIReaderInit();
}

/** Constructor with filename
 * @param std::string file name on disk
 * index file is std::string + '.index'
 */
RTIReader::RTIReader( std::string fname )
{
	RTIReaderInit();

	this->indexFileName = fname + ".index";

	setFileName( fname );

	if( stat( getFileName().c_str(), &fileStats ) == -1 )
	{
		std::cerr << "ERROR: stat() for file: " << getFileName() << " FAILED! " << std::endl;	
		exit(-1);
	}

	if( stat( indexFileName.c_str(), &indexFileStats ) == -1 )
	{
		std::cerr << "ERROR: stat() for file: " << indexFileName << " FAILED! " << std::endl;
		exit(-1);
	}

	data = fopen( getFileName().c_str(), "rb");
	if( data == NULL )
	{
		std::cerr << "ERROR: could not open table file: " << getFileName() << " EXITING!" << std::endl;
		exit(-1);
	}

	indexFileData = fopen( indexFileName.c_str(), "rb");
	if( indexFileData == NULL )
	{
		std::cerr << "ERROR: could not open index file: " << indexFileName.c_str() << " EXITING!" << std::endl;
		exit(-1);
	}

	// XXX possibly remove if we dont want this to happen automagically
	loadIndex();
}

/**
 * Argument Constructor
 * @param uint32 number of chains in the file
 * @param uint32 size of the chain
 * @param uint32 reduction function index offset
 * @param uint32 start point in the chain
 * @param uint32 end point in the chain
 * @param std::string name of the file on disk
 * @param std::string salt used for hash
 */
RTIReader::RTIReader(uint32 chCount, uint32 chLength, uint32 tblIdx, uint32 stPt, uint32 endPt, std::string fname, std::string slt)
{
	RTIReaderInit();

	this->indexFileName = fname + ".index";

	setFileName( fname );

	if( stat( getFileName().c_str(), &fileStats ) == -1 )
	{
		std::cerr << "ERROR: stat() for file: " << getFileName() << " FAILED! " << std::endl;	
		exit(-1);
	}

	if( stat( indexFileName.c_str(), &indexFileStats ) == -1 )
	{
		std::cerr << "ERROR: stat() for file: " << indexFileName << " FAILED! " << std::endl;
		exit(-1);
	}

	data = fopen( getFileName().c_str(), "rb");
	if( data == NULL )
	{
		std::cerr << "ERROR: could not open table file: " << getFileName() << " EXITING!" << std::endl;
		exit(-1);
	}

	indexFileData = fopen( indexFileName.c_str(), "rb");
	if( indexFileData == NULL )
	{
		std::cerr << "ERROR: could not open index file: " << indexFileName.c_str() << " EXITING!" << std::endl;
		exit(-1);
	}

	// XXX remove this if we don't want it to happen automagically
	loadIndex();

	setChainCount( chCount );
	setChainLength( chLength );
	setTableIndex( tblIdx );
	setStartPointBits( stPt );
	setEndPointBits( endPt );
	setSalt( slt );
}

/// shared init method
void RTIReader::RTIReaderInit()
{
	this->chainSize = 8;
	this->indexSize = 11;

	setStartPointBits(6);
	setEndPointBits(2);
	index = NULL;
}


/// loadIndex
void RTIReader::loadIndex()
{
	if( ( fileStats.st_size % chainSize ) != 0 )
	{
		std::cerr << "ERROR: file length mismatch (" << fileStats.st_size << " bytes) EXITING!" << std::endl;
		exit(-1);
	}

	if( ( indexFileStats.st_size % indexSize ) != 0 )
	{
		std::cerr << "ERROR: index file length mismatch (" << indexFileStats.st_size << " bytes) EXITING!" << std::endl;
		exit(-1);
	}

	if( index != NULL )
	{
		delete index;
		index = NULL;
	}

	index = new (std::nothrow) IndexChain[ indexFileStats.st_size / indexSize ];

	if( index == NULL )
	{
		std::cerr << "ERROR: failed to allocate " << ( indexFileStats.st_size / indexSize / 1024/ 1024 ) << " MB of memory. EXITING!" << std::endl;
		exit(-2);
	}

	memset( index, 0x00, sizeof(IndexChain) * ( indexFileStats.st_size / indexSize ) );
	fseek( indexFileData, 0, SEEK_SET );

	uint32 rows;
	for( rows = 0; (rows * indexSize) < indexFileStats.st_size; rows++ )
	{
		if( fread( &index[rows].nPrefix, 5, 1, indexFileData ) != 1 )
			break;
		if( fread( &index[rows].nFirstChain, 4, 1, indexFileData ) != 1 )
			break;
		if( fread( &index[rows].nChainCount, 2, 1, indexFileData ) != 1)
			break;

		// Index Check
		if( rows != 0 && index[rows].nFirstChain < index[rows - 1].nFirstChain )
		{
			std::cerr << "ERROR: Corrupted index detected (FirstChain is less than previous) EXITING!" << std::endl;
			exit(-1);
		}
		else if( rows != 0 && index[rows].nFirstChain != index[rows - 1].nFirstChain + index[rows - 1].nChainCount )
		{
			std::cerr << "ERROR: Corrupted index detected ( LastChain + nChainCount != FirstChain ) EXITING!" << std::endl;
			exit(-1);
		}
	}

	if( index[rows - 1].nFirstChain + index[rows - 1].nChainCount + 1 <= fileStats.st_size / chainSize )
	{
		std::cerr << "ERROR: Corrupted index detected. Not covering the entire file EXITING!" << std::endl;
		exit(-1);
	}
	
	if( index[rows - 1].nFirstChain + index[rows - 1].nChainCount > fileStats.st_size / chainSize )
	{
		std::cerr << "ERROR: Corrupted index detected. The index is covering more than the file" 
					 << index[rows - 1].nFirstChain + index[rows - 1].nChainCount << " chains of " 
					 << fileStats.st_size / chainSize << "chains) EXITING!"<< std::endl;
		exit(-1);
	}

	// XXX this was in the original RTIReader - i dont know why but putting it
	// here just in case
	indexSize = rows;
}

/// getIndexFileName
std::string RTIReader::getIndexFileName()
{
	return this->indexFileName;
}

/// getChainSize
uint32 RTIReader::getChainSize()
{
	return this->chainSize;
}

/// getIndexSize
uint32 RTIReader::getIndexSize()
{
	return this->indexSize;
}

// XXX used for debugging getIndexFileSize
uint32 RTIReader::getIndexFileSize()
{
	return indexFileStats.st_size;
}

uint32 RTIReader::getDataFileSize()
{
	return fileStats.st_size;
}

FILE* RTIReader::getIndexFileData()
{
	return indexFileData;
}

FILE* RTIReader::getDataFile()
{
	return data;
}

// XXX end debug type functions

/// getChainsLeft
uint32 RTIReader::getChainsLeft()
{
	return ( fileStats.st_size / chainSize ) - chainPosition;
}

/**
 * reads data chains into memory
 * @param uint32 reference to the number of chains to read
 * @param RanbowChain0* pointer in memory to read the chains to
 * @return EXIT_SUCCESS or EXIT_FAILURE
 */
int RTIReader::readChains(uint32 &numChains, RainbowChainO *pData)
{
	// reset data to 0x00 or something bad happens
	memset( pData, 0x00, sizeof( RainbowChainO ) * numChains );
	uint32 readChains = 0;
	uint32 chainsLeft = getChainsLeft();

	for( uint32 i = 0; i < indexSize; i++ )
	{
		if( chainPosition + readChains > index[i].nFirstChain + index[i].nChainCount )
			continue;

		while( chainPosition + readChains < index[i].nFirstChain + index[i].nChainCount )
		{	
			pData[readChains].nIndexE = index[i].nPrefix << 16;
			uint32 endPoint = 0; // have to set to 0
			// XXX start points may not exceed 6 bytes ( 2^48 )
			fread( &pData[readChains].nIndexS, 6, 1, data);
			fread( &endPoint, 2, 1, data);
			pData[readChains].nIndexE += endPoint;
			readChains++;
			
			if( readChains == numChains || readChains == chainsLeft )
				break;
		}
		if( readChains == numChains )
			break;
	}

	if( readChains != numChains )
		numChains = readChains; // update how many chains we read

	chainPosition += readChains;
	std::cout << "Chain Position is now " << chainPosition << std::endl;
	return EXIT_SUCCESS;	
}

uint64 RTIReader::getMinimumStartPoint()
{
	uint64 tmpStartPoint = 0;
	uint64 minimumStartPoint = (uint64)-1;
	uint16 tmpEndPoint;
	long originalFilePos = ftell( data );

	rewind( data );

	while ( !feof( data ) )
	{
		fread( &tmpStartPoint, 6, 1, data );
		fread( &tmpEndPoint, 2, 1, data );

		if ( tmpStartPoint < minimumStartPoint )
			minimumStartPoint = tmpStartPoint;
	}

	fseek( data, originalFilePos, SEEK_SET );
	return minimumStartPoint;
}

void RTIReader::Dump()
{
}

/// Destructor
RTIReader::~RTIReader()
{
	if( indexFileData != NULL )
		fclose( indexFileData );

	if ( index != NULL )
	{
		delete index;
		index = NULL;
	}
}
