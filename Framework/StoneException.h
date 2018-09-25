/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2018 Osimis S.A., Belgium
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

#include "Core/OrthancException.h"
#include <boost/lexical_cast.hpp>

namespace OrthancStone
{
  enum ErrorCode
  {
    ErrorCode_Success,
    ErrorCode_OrthancError, // this StoneException is actually an OrthancException with an Orthanc error code
    ErrorCode_ApplicationException, // this StoneException is specific to an application (and should have its own internal error code)
    ErrorCode_NotImplemented, // case not implemented

    ErrorCode_PromiseSingleSuccessHandler, // a Promise can only have a single success handler
    ErrorCode_PromiseSingleFailureHandler, // a Promise can only have a single failure handler

    ErrorCode_CommandJsonInvalidFormat,
    ErrorCode_Last
  };



  class StoneException
  {
  protected:
    OrthancStone::ErrorCode     errorCode_;

  public:
    explicit StoneException(ErrorCode errorCode) :
      errorCode_(errorCode)
    {
    }

    ErrorCode GetErrorCode() const
    {
      return errorCode_;
    }

    virtual const char* What() const
    {
      return "TODO: EnumerationToString for StoneException";
    }
  };

  class StoneOrthancException : public StoneException
  {
  protected:
    Orthanc::OrthancException&  orthancException_;

  public:
    explicit StoneOrthancException(Orthanc::OrthancException& orthancException) :
      StoneException(ErrorCode_OrthancError),
      orthancException_(orthancException)
    {
    }

    Orthanc::ErrorCode GetOrthancErrorCode() const
    {
      return orthancException_.GetErrorCode();
    }

    virtual const char* What() const
    {
      return orthancException_.What();
    }
  };

  class StoneApplicationException : public StoneException
  {
  protected:
    int applicationErrorCode_;

  public:
    explicit StoneApplicationException(int applicationErrorCode) :
      StoneException(ErrorCode_ApplicationException),
      applicationErrorCode_(applicationErrorCode)
    {
    }

    int GetApplicationErrorCode() const
    {
      return applicationErrorCode_;
    }

    virtual const char* What() const
    {
      return boost::lexical_cast<std::string>(applicationErrorCode_).c_str();
    }
  };

}

