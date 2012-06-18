/*
 * freerainbowtables is a project for generating, distributing, and using
 * perfect rainbow tables
 *
 * Copyright 2009, 2010 Daniël Niggebrugge <niggebrugge@fox-it.com>
 * Copyright 2009, 2010, 2011 James Nobis <quel@quelrod.net>
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

#ifndef _GLOBAL_H
#define _GLOBAL_H

/*
 * XXX
 * add universal support for unsigned 64-bit int values like the MAX
 * 0xFFFFFFFFFFFFFFFFI64 (_UI64_MAX)
 * 0xFFFFFFFFFFFFFFFFllu
 */

#if defined(_WIN32) && !defined(__GNUC__)
	#define uint64 unsigned __int64
#else
	#ifndef u_int64_t
		#define uint64 unsigned long long
	#else
		#define uint64 u_int64_t
	#endif
#endif

#if defined(_WIN32) && !defined(__GNUC__)
	#define uint32 unsigned __int32
#else
	#ifndef u_int32_t
		#define uint32 unsigned int
	#else
		#define uint32 u_int32_t
	#endif
#endif

#if defined(_WIN32) && !defined(__GNUC__)
	#define uint16 unsigned short
#else
	#ifndef u_int16_t
		#define uint16 unsigned short
	#else
		#define uint16 u_int16_t
	#endif
#endif

#if defined(_WIN32) && !defined(__GNUC__)
	#define uint8 unsigned char
#else
	#ifndef u_int8_t
		#define uint8 unsigned char
	#else
		#define uint8 u_int8_t
	#endif
#endif

#endif /* !GLOBAL_H */
