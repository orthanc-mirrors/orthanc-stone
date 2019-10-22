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

#pragma once

#include <string>

namespace OrthancStone
{
  namespace GenericToolbox
  {
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
        else
          return false;
      }
      return true;
    }

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
        int n = 1;
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
      if (*p == 0 || (*p >= '0' && *p <= '9') )
        return true;
      else
        return false;
    }

    inline bool StringToDouble(double& r, const std::string& text)
    {
      return StringToDouble(r, text.c_str());
    }

    template<typename T>
    inline bool StringToInteger(T& r, const char* text)
    {
      if (!LegitDoubleString(text))
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
    "rgb(12,23,255)"  --> red, green, blue and returns true
    "everything else" --> returns false (other values left untouched)
    */
    bool GetRgbValuesFromString(uint8_t& red, uint8_t& green, uint8_t& blue, const char* text);

    /**
    See other overload
    */
    inline bool GetRgbValuesFromString(uint8_t& red, uint8_t& green, uint8_t& blue, const std::string& text)
    {
      return GetRgbValuesFromString(red, green, blue, text.c_str());
    }

    bool GetRgbaValuesFromString(uint8_t& red, uint8_t& green, uint8_t& blue, uint8_t& alpha, const char* text);

    inline bool GetRgbaValuesFromString(uint8_t& red, uint8_t& green, uint8_t& blue, uint8_t& alpha, const std::string& text)
    {
      return GetRgbaValuesFromString(red, green, blue, alpha, text.c_str());
    }
  }
}
