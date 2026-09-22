/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2023 Osimis S.A., Belgium
 * Copyright (C) 2021-2026 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 **/


#include "OracleCallback.h"


namespace OrthancStone
{
  OracleCallback::OracleCallback(IEnvironment& environment,
                                 const boost::shared_ptr<IOracleClient>& client,
                                 IOracleCommand* command /* takes ownership */) :
    environment_(environment),
    client_(client),
    command_(command)
  {
    if (!client ||
        command == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }
  }


  void OracleCallback::NotifySuccess(IMessage* result)
  {
    std::unique_ptr<IMessage> protection(result);

    if (command_.get() == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      environment_.NotifyOracleSuccess(client_, command_.release(), protection.release());
    }
  }


  void OracleCallback::NotifyError(const Orthanc::OrthancException& error)
  {
    if (command_.get() == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      environment_.NotifyOracleError(client_, command_.release(), error);
    }
  }


  const IOracleCommand& OracleCallback::GetCommand() const
  {
    if (command_.get() == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      return *command_;
    }
  }
}
