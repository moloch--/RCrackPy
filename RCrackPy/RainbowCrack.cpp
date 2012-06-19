/*
 * rcracki_mt is a multithreaded implementation and fork of the original 
 * RainbowCrack
 *
 * Copyright (C) Zhu Shuanglei <shuanglei@hotmail.com>
 * Copyright Martin Westergaard J�rgensen <martinwj2005@gmail.com>
 * Copyright 2009, 2010  Dani�l Niggebrugge <niggebrugge@fox-it.com>
 * Copyright 2009 James Dickson
 * Copyright 2009, 2010, 2011 James Nobis <quel@quelrod.net>
 * Copyright 2010 uroskn
 * Copyright 2011 Logan Watt <logan.watt@gmail.com>
 * Copyright 2011 Jan Kyska
 * Copyright 2012 Moloch <moloch38@gmail.com>
 *
 * Modified by Martin Westergaard J�rgensen <martinwj2005@gmail.com> to support  * indexed and hybrid tables
 *
 * Modified by neinbrucke to support multi threading and a bunch of other stuff :)
 *
 * 2009-01-04 - <james.dickson@comhem.se> - Slightly modified (or "fulhack" as 
 * we say in sweden)  to support cain .lst files.
 *
 * 2012 - Modified by Moloch to create Python bindings
 *
 * This file is part of rcracki_mt.
 *
 * rcracki_mt is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * rcracki_mt is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with rcracki_mt.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <python2.7/Python.h>
#include <boost/python.hpp>

#include "Public.h"

#include "CrackEngine.h"
#include "lm2ntlm.h"

#ifdef _WIN32
	#include <io.h>
#else
	#include <unistd.h>
	#include <dirent.h>
#endif

#if defined(_WIN32) && !defined(__GNUC__)
	#pragma comment(lib, "libeay32.lib")
#endif

//////////////////////////////////////////////////////////////////////

#ifdef _WIN32
void GetTableList( std::string sWildCharPathName, std::vector<std::string>& vPathName )
{
	std::string sPath;
	std::string::size_type n = sWildCharPathName.find_last_of('\\');

	if ( n == (sWildCharPathName.size() - 1) )
	{
		sWildCharPathName = sWildCharPathName.substr(0, n);
		n = sWildCharPathName.find_last_of('\\');
	}

	if (n != std::string::npos)
		sPath = sWildCharPathName.substr(0, n + 1);

	_finddata_t fd;

	long handle = _findfirst(sWildCharPathName.c_str(), &fd);
	if (handle != -1)
	{
		do
		{
			std::string sName = fd.name;
			if (sName.size()>3) {
				if (sName.substr(sName.size()-3, 3) == ".rt" && !(fd.attrib & _A_SUBDIR))
				{
					std::string sPathName = sPath + sName;
					vPathName.push_back(sPathName);
				}
			}
			if (sName.size()>4) {
				if (sName.substr(sName.size()-4, 4) == ".rti" && !(fd.attrib & _A_SUBDIR))
				{
					std::string sPathName = sPath + sName;
					vPathName.push_back(sPathName);
				}
			}
			if (sName.size()>5) {
				if (sName.substr(sName.size()-5, 5) == ".rti2" && !(fd.attrib & _A_SUBDIR))
				{
					std::string sPathName = sPath + sName;
					vPathName.push_back(sPathName);
				}
			}

			if (sName != "." && sName != ".." && (fd.attrib & _A_SUBDIR))
			{
				std::string sPath_sub = sPath + sName + '\\';
				std::string sWildCharPathName_sub = sPath_sub + '*';
				GetTableList(sWildCharPathName_sub, vPathName);
			}

		} while (_findnext(handle, &fd) == 0);

		_findclose(handle);
	}
	//printf("Found %d rainbowtables (files) in %d sub directories...\n", vPathName.size(), subDir_count);
}
#else
//void GetTableList(int argc, char* argv[], vector<string>& vPathName)
void GetTableList( std::string sWildCharPathName, std::vector<std::string>& vPathName )
{
	struct stat buf;
	if (lstat(sWildCharPathName.c_str(), &buf) == 0)
	{
		if (S_ISDIR(buf.st_mode))
		{
			DIR *dir = opendir(sWildCharPathName.c_str());
			if(dir)
			{
				struct dirent *pDir=NULL;
				while((pDir = readdir(dir)) != NULL)
				{
					std::string filename = "";
					filename += (*pDir).d_name;
					if (filename != "." && filename != "..")
					{
						std::string new_filename = sWildCharPathName + '/' + filename;
						GetTableList(new_filename, vPathName);
					}
				}
				closedir(dir);
			}
		}
		else if (S_ISREG(buf.st_mode))
		{
			if (sWildCharPathName.size()>3)
			{
				if (sWildCharPathName.substr(sWildCharPathName.size()-3, 3) == ".rt")
				{
					vPathName.push_back(sWildCharPathName);
				}
			}
			if (sWildCharPathName.size()>4)
			{
				if (sWildCharPathName.substr(sWildCharPathName.size()-4, 4) == ".rti")
				{
					//string sPathName_sub = sPath_sub + sName_sub;
					vPathName.push_back(sWildCharPathName);
					//printf("sPathName_sub: %s\n", sPathName_sub.c_str());
				}
			}
			if ( sWildCharPathName.size() > 5 )
			{
				if ( sWildCharPathName.substr( sWildCharPathName.size() - 5, 5 ) == ".rti2" )
				{
					vPathName.push_back( sWildCharPathName );
				}
			}
		}
	}
}
#endif

bool NormalizeHash(std::string& sHash)
{
	std::string sNormalizedHash = sHash;

	if (   sNormalizedHash.size() % 2 != 0
		|| sNormalizedHash.size() < MIN_HASH_LEN * 2
		|| sNormalizedHash.size() > MAX_HASH_LEN * 2)
		return false;

	// Make lower
	uint32 i;
	for (i = 0; i < sNormalizedHash.size(); i++)
	{
		if (sNormalizedHash[i] >= 'A' && sNormalizedHash[i] <= 'F')
			sNormalizedHash[i] = (char) sNormalizedHash[i] - 'A' + 'a';
	}

	// Character check
	for (i = 0; i < sNormalizedHash.size(); i++)
	{
		if (   (sNormalizedHash[i] < 'a' || sNormalizedHash[i] > 'f')
			&& (sNormalizedHash[i] < '0' || sNormalizedHash[i] > '9'))
			return false;
	}

	sHash = sNormalizedHash;
	return true;
}

void LoadLMHashFromPwdumpFile( std::string sPathName, std::vector<std::string>& vUserName, std::vector<std::string>& vLMHash, std::vector<std::string>& vNTLMHash )
{
	std::vector<std::string> vLine;
	if (ReadLinesFromFile(sPathName, vLine))
	{
		uint32 i;
		for (i = 0; i < vLine.size(); i++)
		{
			std::vector<std::string> vPart;
			if (SeperateString(vLine[i], "::::", vPart))
			{
				std::string sUserName = vPart[0];
				std::string sLMHash   = vPart[2];
				std::string sNTLMHash = vPart[3];

				if (sLMHash.size() == 32 && sNTLMHash.size() == 32)
				{
					if (NormalizeHash(sLMHash) && NormalizeHash(sNTLMHash))
					{
						vUserName.push_back(sUserName);
						vLMHash.push_back(sLMHash);
						vNTLMHash.push_back(sNTLMHash);
					}
					else
						std::cout << "invalid lm/ntlm hash " << sLMHash.c_str()
							<< ":" << sNTLMHash.c_str() << std::endl;
				}
			}
		}
	}
	else
		std::cout << "can't open " << sPathName.c_str() << std::endl;
}

// 2009-01-04 - james.dickson - Added this so we can load hashes from cain .LST files.
void LoadLMHashFromCainLSTFile( std::string sPathName, std::vector<std::string>& vUserName, std::vector<std::string>& vLMHash, std::vector<std::string>& vNTLMHash )
{
	std::vector<std::string> vLine;
	if (ReadLinesFromFile(sPathName, vLine))
	{
		uint32 i;
		for (i = 0; i < vLine.size(); i++)
		{
			std::vector<std::string> vPart;
			if (SeperateString(vLine[i], "\t\t\t\t\t\t", vPart))
			{
				std::string sUserName = vPart[0];
				std::string sLMHash   = vPart[4];
				std::string sNTLMHash = vPart[5];

				if (sLMHash.size() == 32 && sNTLMHash.size() == 32)
				{
					if (NormalizeHash(sLMHash) && NormalizeHash(sNTLMHash))
					{
						vUserName.push_back(sUserName);
						vLMHash.push_back(sLMHash);
						vNTLMHash.push_back(sNTLMHash);
					}
					else
						std::cout << "invalid lm/ntlm hash " << sLMHash.c_str()
							<< ":" << sNTLMHash.c_str() << std::endl;
				}
			}
		}
	}
	else
		std::cout << "can't open " << sPathName.c_str() << std::endl;
}

bool NTLMPasswordSeek(unsigned char* pLMPassword, int nLMPasswordLen, int nLMPasswordNext,
					  unsigned char* pNTLMHash, std::string& sNTLMPassword)
{
	if (nLMPasswordNext == nLMPasswordLen)
	{
		unsigned char md[MD4_DIGEST_LENGTH];
		MD4_NEW(pLMPassword, nLMPasswordLen * 2, md);

		if (memcmp(md, pNTLMHash, MD4_DIGEST_LENGTH) == 0)
		{
			sNTLMPassword = "";
			int i;
			for (i = 0; i < nLMPasswordLen; i++)
				sNTLMPassword += char(pLMPassword[i * 2]);
			return true;
		}
		else
			return false;
	}

	if (NTLMPasswordSeek(pLMPassword, nLMPasswordLen, nLMPasswordNext + 1, pNTLMHash, sNTLMPassword))
		return true;

	if (   pLMPassword[nLMPasswordNext * 2] >= 'A'
		&& pLMPassword[nLMPasswordNext * 2] <= 'Z')
	{
		pLMPassword[nLMPasswordNext * 2] = (unsigned char) pLMPassword[nLMPasswordNext * 2] - 'A' + 'a';
		if (NTLMPasswordSeek(pLMPassword, nLMPasswordLen, nLMPasswordNext + 1, pNTLMHash, sNTLMPassword))
			return true;
		pLMPassword[nLMPasswordNext * 2] = (unsigned char) pLMPassword[nLMPasswordNext * 2] - 'a' + 'A';
	}

	return false;
}

bool LMPasswordCorrectCase( std::string sLMPassword, unsigned char* pNTLMHash, std::string& sNTLMPassword )
{
	if (sLMPassword.size() == 0)
	{
		sNTLMPassword = "";
		return true;
	}

	unsigned char* pLMPassword = new unsigned char[sLMPassword.size() * 2];
	uint32 i;
	for (i = 0; i < sLMPassword.size(); i++)
	{
		pLMPassword[i * 2    ] = sLMPassword[i];
		pLMPassword[i * 2 + 1] = 0x00;
	}
	bool fRet = NTLMPasswordSeek(pLMPassword, sLMPassword.size(), 0, pNTLMHash, sNTLMPassword);

	delete pLMPassword;

	return fRet;
}

boost::python::dict singleHash(std::string sHash, std::string pathToTables,
		std::string outputFile, std::string sIniPathName,
		std::string sSessionPathName, std::string sProgressPathName,
		std::string sPrecalcPathName, bool debug, bool keepPrecalcFiles,
		int enableGPU, int maxThreads, uint64 maxMem)
{

	CHashSet hashSet;
	bool resumeSession = false; // Sessions not currently supported
	std::vector<std::string> verifiedHashes;
	std::vector<std::string> vPathName;

	/* Setup hashes */
	if (NormalizeHash(sHash)) {
		verifiedHashes.push_back(sHash);
	} else {
		std::ostringstream stringBuilder;
		stringBuilder << "Invalid hash: " << sHash.c_str();
		std::string message = stringBuilder.str();
        PyErr_SetString(PyExc_ValueError, message.c_str());
        throw boost::python::error_already_set();
	}
	for (unsigned int index = 0; index < verifiedHashes.size(); ++index) {
		hashSet.AddHash(verifiedHashes[index]);
	}

	/* Load rainbow tables */
	GetTableList(pathToTables, vPathName);
	if ( debug )
	{
		std::cout << "[Debug]: Found " << vPathName.size() << " rainbowtable file(s)..." << std::endl;
	}
	/* Start cracking! */
	CCrackEngine crackEngine;
	crackEngine.setSession(sSessionPathName, sProgressPathName, sPrecalcPathName, keepPrecalcFiles);
	crackEngine.Run(vPathName, hashSet, maxThreads, maxMem, resumeSession, debug, enableGPU);

	/* Gather results */
	boost::python::dict results;
	for (uint32 i = 0; i < verifiedHashes.size(); i++)
	{
		std::string sPlain;
		std::string sBinary;
		std::string tmpHash = verifiedHashes[i];

		if (!hashSet.GetPlain(tmpHash, sPlain, sBinary))
		{
			sPlain  = "<Not Found>";
			sBinary = "<Not Found>";
		}
		results[verifiedHashes[i].c_str()] = sPlain.c_str();
	}
	return results;
}

/* Python constructor (required) */
void rainbowCrackInit()
{
	NULL; // Do nothing
}

BOOST_PYTHON_MODULE(RainbowCrack)
{
	using namespace boost::python;
	def("RainbowCrack", rainbowCrackInit);
	def("single_hash",
		singleHash,
		(
			arg("sHash"),
			arg("pathToTables"),
			arg("outputFile") = "",
			arg("sIniPathName") = "rcracki_mt.ini",
			arg("sSessionPathName") = "rcracki.session",
			arg("sProgressPathName") = "rcracki.progress",
			arg("sPrecalcPathName") = "rcracki.precalc",
			arg("debug") = false,
			arg("keepPrecalcFiles") = false,
			arg("enableGPU") = 0,
			arg("maxThreads") = 1,
			arg("maxMem") = 0
		),
		"single_hash(): Used to crack any single LM/NTLM/MD5 hash passed as an argument"
	);
	// def("text_file");
	// def("cain_file");
	// def("pwdump_file");
}

