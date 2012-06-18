/*
 * freerainbowtables is a project for generating, distributing, and using
 * perfect rainbow tables
 *
 * Copyright 2010, 2011 Martin Westergaard Jørgensen <martinwj2005@gmail.com>
 * Copyright 2010 Daniël Niggebrugge <niggebrugge@fox-it.com>
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

#ifndef _BASERTREADER_H
#define _BASERTREADER_H

#include "Public.h"

/**
 * The foundation for all the RT reader implementations
 */

class BaseRTReader
{
	private:
		/// Members
		uint32 chainCount;		// number of chains in file
		uint32 chainLength;		// size of chain
		uint32 tableIndex;		// reduction function index offset
		uint32 startPointBits;	// start point in the chain
		uint32 endPointBits;		// end point in the chain
		std::string fileName; 	// name of file on disk
		std::string salt;			// salt used for hash

	protected:
		FILE *data;					// binary data
		uint32 chainPosition;
		/// Set Methods
		virtual void setChainCount(uint32);
		virtual void setChainLength(uint32);
		virtual void setTableIndex(uint32);
		virtual void setStartPointBits(uint32);
		virtual void setEndPointBits(uint32);
		virtual void setFileName(std::string);
		virtual void setSalt(std::string);

	public:
		/// Default constructor
		BaseRTReader();

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
		BaseRTReader(uint32, uint32, uint32, uint32, uint32, std::string, std::string);

		/// Default destructor
		virtual ~BaseRTReader();

		/**
		 * reads data chains into memory
		 * @param uint32 reference to the number of chains to read
		 * @param RanbowChain0* pointer in memory to read the chains to
		 * @return EXIT_SUCCESS or EXIT_FAILURE
		 */
		virtual int readChains(uint32 &numChains, RainbowChainO *pData) = 0;
		virtual uint32 getChainsLeft() = 0;
		virtual uint64 getMinimumStartPoint() = 0;

		/// Get Methods
		virtual uint32 getChainCount();
		virtual uint32 getChainLength();
		virtual uint32 getChainPosition();
		virtual uint32 getTableIndex();
		virtual uint32 getStartPointBits();
		virtual uint32 getEndPointBits();
		virtual std::string getFileName();
		virtual std::string getSalt();

		/**
		 * Debugging function
		 */
		virtual void Dump();
};

#endif
