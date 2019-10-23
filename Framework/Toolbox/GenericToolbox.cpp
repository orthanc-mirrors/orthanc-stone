/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2019 Osimis S.A., Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


#include "GenericToolbox.h"

#include <boost/regex.hpp>

namespace OrthancStone
{
  namespace GenericToolbox
  {
    bool GetRgbaValuesFromString(uint8_t& red, uint8_t& green, uint8_t& blue, uint8_t& alpha, const char* text)
    {
      boost::regex pattern("\\s*rgb\\s*\\(\\s*([0-9]+)\\s*,\\s*([0-9]+)\\s*,\\s*([0-9]+)\\s*,\\s*([0-9]+)\\s*\\)\\s*");

      boost::cmatch what;

      if (boost::regex_match(text, what, pattern))
      {
        {
          std::string redStr = what[1];
          bool ok = StringToInteger<uint8_t>(red, redStr);
          if (!ok)
            return false;
        }
        {
          std::string greenStr = what[2];
          bool ok = StringToInteger<uint8_t>(green, greenStr);
          if (!ok)
            return false;
        }
        {
          std::string blueStr = what[3];
          bool ok = StringToInteger<uint8_t>(blue, blueStr);
          if (!ok)
            return false;
        }
        {
          std::string alphaStr = what[4];
          bool ok = StringToInteger<uint8_t>(alpha, alphaStr);
          if (!ok)
            return false;
        }
        return true;
      }
      else
      {
        return false;
      }
    }
    bool GetRgbValuesFromString(uint8_t& red, uint8_t& green, uint8_t& blue, const char* text)
    {
      boost::regex pattern("\\s*rgb\\s*\\(\\s*([0-9]+)\\s*,\\s*([0-9]+)\\s*,\\s*([0-9]+)\\s*\\)\\s*");

      boost::cmatch what;

      if (boost::regex_match(text, what, pattern))
      {
        {
          std::string redStr = what[1];
          bool ok = StringToInteger<uint8_t>(red, redStr);
          if (!ok)
            return false;
        }
        {
          std::string greenStr = what[2];
          bool ok = StringToInteger<uint8_t>(green, greenStr);
          if (!ok)
            return false;
        }
        {
          std::string blueStr = what[3];
          bool ok = StringToInteger<uint8_t>(blue, blueStr);
          if (!ok)
            return false;
        }
        return true;
      }
      else
      {
        return false;
      }
    }

  }
}
