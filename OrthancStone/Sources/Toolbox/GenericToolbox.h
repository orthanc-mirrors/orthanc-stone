/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2020 Osimis S.A., Belgium
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

#pragma once

#include <Compatibility.h>
#include <OrthancException.h>

#include <boost/shared_ptr.hpp>

#include <string>
#include <stdint.h>
#include <math.h>

#include <memory>

namespace OrthancStone
{
  namespace GenericToolbox
  {
    /**
    Fast floating point string validation.
    No trimming applied, so the input must match regex 
    /^[-]?[0-9]*\.?[0-9]*([eE][-+]?[0-9]+)?$/
    The following are allowed as edge cases: "" and "-"
    */
    inline bool LegitDoubleString(const char* text)
    {
      const char* p = text;
      if(*p == '-')
        p++;
      size_t period = 0;
      while(*p != 0)
      {
        if (*p >= '0' && *p <= '9')
          ++p;
        else if(*p == '.')
        {
          if(period > 0)
            return false;
          else
            period++;
        ++p;
        }
        else if (*p == 'e' || *p == 'E')
        {
          ++p;
          if (*p == '-' || *p == '+')
            ++p;
          // "e+"/"E+" "e-"/"E-" or "e"/"E" must be followed by a number
          if (!(*p >= '0' && *p <= '9'))
            return false;

          // these must be the last in the string
          while(*p >= '0' && *p <= '9')
            ++p;

          return (*p == 0);
        }
        else
        {
          return false;
        }
      }
      return true;
    }

    /**
    Fast integer string validation.
    No trimming applied, so the input must match regex /^-?[0-9]*$/
    The following are allowed as edge cases: "" and "-"
    */
    inline bool LegitIntegerString(const char* text)
    {
      const char* p = text;
      if (*p == '-')
        p++;
      while (*p != 0)
      {
        if (*p >= '0' && *p <= '9')
          ++p;
        else
          return false;
      }
      return true;
    }

    /*
      Fast string --> double conversion.
      Must pass the LegitDoubleString test

      String to doubles with at most 18 digits
    */
    inline bool StringToDouble(double& r, const char* text)
    {
      if(!LegitDoubleString(text))
        return false;

      static const double FRAC_FACTORS[] = 
      {
        1.0,
        0.1,
        0.01,
        0.001,
        0.0001,
        0.00001,
        0.000001,
        0.0000001,
        0.00000001,
        0.000000001,
        0.0000000001,
        0.00000000001,
        0.000000000001,
        0.0000000000001,
        0.00000000000001,
        0.000000000000001,
        0.0000000000000001,
        0.00000000000000001,
        0.000000000000000001,
        0.0000000000000000001
      };
      const size_t FRAC_FACTORS_LEN = sizeof(FRAC_FACTORS)/sizeof(double);

      r = 0.0;
      double neg = 1.0;
      const char* p = text;

      if (*p == '-')
      {
        neg = -1.0;
        ++p;
      }
      // 12345.67890
      while (*p >= '0' && *p <= '9')
      {
          r = (r*10.0) + (*p - '0'); // 1 12 123 123 12345
        ++p;
      }
      if (*p == '.')
      {
        double f = 0.0;
        size_t n = 1;
        ++p;
        while (*p >= '0' && *p <= '9' && n < FRAC_FACTORS_LEN)
        {
          f += (*p - '0') * FRAC_FACTORS[n];
          ++p;
          ++n;
        }
        r += f;
      }
      r *= neg;

      // skip the remaining numbers until we reach not-a-digit (either the 
      // end of the string OR the scientific notation symbol)
      while ((*p >= '0' && *p <= '9'))
        ++p;

      if (*p == 0 )
      {
        return true;
      }
      else if ((*p == 'e') || (*p == 'E'))
      {
        // process the scientific notation
        double sign; // no init is safe (read below)
        ++p;
        if (*p == '-')
        {
          sign = -1.0;
          // point to first number
          ++p;
        }
        else if (*p == '+')
        {
          sign = 1.0;
          // point to first number
          ++p;
        }
        else if (*p >= '0' && *p <= '9')
        {
          sign = 1.0;
        }
        else
        {
          // only a sign char or a number is allowed
          return false;
        }
        // now p points to the absolute value of the exponent
        double exp = 0;
        while (*p >= '0' && *p <= '9')
        {
          exp = (exp * 10.0) + static_cast<double>(*p - '0'); // 1 12 123 123 12345
          ++p;
        }
        // now we have our exponent. put a sign on it.
        exp *= sign;
        double scFac = ::pow(10.0, exp);
        r *= scFac;

        // only allowed symbol here is EOS
        return (*p == 0);
      }
      else
      {
        // not allowed
        return false;
      }
    }

    inline bool StringToDouble(double& r, const std::string& text)
    {
      return StringToDouble(r, text.c_str());
    }

    /**
    Fast string to integer conversion. Leading zeroes and minus are accepted,
    but a leading + sign is NOT.
    Must pass the LegitIntegerString function test.
    In addition, an empty string (or lone minus sign) yields 0.
    */

    template<typename T>
    inline bool StringToInteger(T& r, const char* text)
    {
      if (!LegitIntegerString(text))
        return false;

      r = 0;
      T neg = 1;
      const char* p = text;

      if (*p == '-')
      {
        neg = -1;
        ++p;
      }
      while (*p >= '0' && *p <= '9')
      {
        r = (r * 10) + (*p - '0'); // 1 12 123 123 12345
        ++p;
      }
      r *= neg;
      if (*p == 0)
        return true;
      else
        return false;
    }

    template<typename T>
    inline bool StringToInteger(T& r, const std::string& text)
    {
      return StringToInteger<T>(r, text.c_str());
    }

    /**
    if input is "rgb(12,23,255)"  --> function fills `red`, `green` and `blue` and returns true
    else ("everything else")      --> function returns false and leaves all values untouched
    */
    bool GetRgbValuesFromString(uint8_t& red, uint8_t& green, uint8_t& blue, const char* text);

    /**
    See main overload
    */
    inline bool GetRgbValuesFromString(uint8_t& red, uint8_t& green, uint8_t& blue, const std::string& text)
    {
      return GetRgbValuesFromString(red, green, blue, text.c_str());
    }

    /**
    Same as GetRgbValuesFromString
    */
    bool GetRgbaValuesFromString(uint8_t& red,
                                 uint8_t& green,
                                 uint8_t& blue,
                                 uint8_t& alpha,
                                 const char* text);

    /**
    Same as GetRgbValuesFromString
    */
    inline bool GetRgbaValuesFromString(uint8_t& red, 
                                        uint8_t& green, 
                                        uint8_t& blue, 
                                        uint8_t& alpha, 
                                        const std::string& text)
    {
      return GetRgbaValuesFromString(red, green, blue, alpha, text.c_str());
    }
  }
}
