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


#pragma once

#include "IEnvironment.h"

namespace OrthancStone
{
  class OracleCallback : public boost::noncopyable
  {
  private:
    IEnvironment&                    environment_;
    boost::weak_ptr<IOracleClient>   client_;
    std::unique_ptr<IOracleCommand>  command_;

  public:
    OracleCallback(IEnvironment& environment,
                   const boost::shared_ptr<IOracleClient>& client,
                   IOracleCommand* command /* takes ownership */);

    void NotifySuccess(IMessage* result);

    void NotifyError(const Orthanc::OrthancException& error);

    const IOracleCommand& GetCommand() const;  // TODO Refactoring - Remove this
  };
}
