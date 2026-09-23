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

#include "../Messages/IMessageEmitter.h"   // TODO Refactoring - Remove this


namespace OrthancStone
{
  class IOracleCallback : public Orthanc::IDynamicObject  // TODO Refactoring - Remove this
  {
  public:
    virtual void NotifySuccess(IMessage* result) = 0;

    virtual void NotifyError(const Orthanc::OrthancException& error) = 0;

    virtual const IOracleCommand& GetCommand() const = 0;
  };


  class OracleCallback : public IOracleCallback
  {
  private:
    IEnvironment&                    environment_;
    boost::weak_ptr<IOracleClient>   client_;
    std::unique_ptr<IOracleCommand>  command_;

  public:
    OracleCallback(IEnvironment& environment,
                   const boost::shared_ptr<IOracleClient>& client,
                   IOracleCommand* command /* takes ownership */);

    virtual void NotifySuccess(IMessage* result) ORTHANC_OVERRIDE;

    virtual void NotifyError(const Orthanc::OrthancException& error) ORTHANC_OVERRIDE;

    virtual const IOracleCommand& GetCommand() const ORTHANC_OVERRIDE  // TODO Refactoring - Remove this
    {
      return *command_;
    }
  };


  class OldOracleCallback : public IOracleCallback  // TODO Refactoring - Remove this
  {
  private:
    std::unique_ptr<IOracleCommand>  command_;
    boost::weak_ptr<IObserver>       receiver_;
    IMessageEmitter&                 emitter_;

  public:
    OldOracleCallback(IOracleCommand* command /* takes ownership */,
                      boost::weak_ptr<IObserver> receiver,
                      IMessageEmitter& emitter);

    virtual void NotifySuccess(IMessage* message /* takes ownership */) ORTHANC_OVERRIDE;

    virtual void NotifyError(const Orthanc::OrthancException& error) ORTHANC_OVERRIDE;

    virtual const IOracleCommand& GetCommand() const ORTHANC_OVERRIDE;

    const boost::weak_ptr<IObserver>& GetReceiver() const
    {
      return receiver_;
    }
  };
}
