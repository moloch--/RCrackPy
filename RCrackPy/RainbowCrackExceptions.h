/*
 * RainbowCrackExceptions.cpp
 *
 *  Created on: Nov 10, 2012
 *      Author: moloch
 */

#include <exception>

class CharSetIOError: public std::exception
{
  virtual const char* what() const throw()
  {
    return "Cannot open charset configuration file";
  }
} CharSetIOError;


