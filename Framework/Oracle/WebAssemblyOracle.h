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

#if !defined(ORTHANC_ENABLE_WASM)
#  error The macro ORTHANC_ENABLE_WASM must be defined
#endif

#if ORTHANC_ENABLE_WASM != 1
#  error This file can only compiled for WebAssembly
#endif

#include "../Messages/IObservable.h"
#include "GetOrthancImageCommand.h"
#include "GetOrthancWebViewerJpegCommand.h"
#include "HttpCommand.h"
#include "IOracle.h"
#include "OrthancRestApiCommand.h"


namespace OrthancStone
{
  class WebAssemblyOracle :
    public IOracle,
    public IObservable,
    public IMessageEmitter
  {
  private:
    typedef std::map<std::string, std::string>  HttpHeaders;
    
    class TimeoutContext;
    class FetchContext;
    class FetchCommand;    
    
    void Execute(boost::weak_ptr<IObserver> receiver,
                 HttpCommand* command);    
    
    void Execute(boost::weak_ptr<IObserver> receiver,
                 OrthancRestApiCommand* command);    
    
    void Execute(boost::weak_ptr<IObserver> receiver,
                 GetOrthancImageCommand* command);    
    
    void Execute(boost::weak_ptr<IObserver> receiver,
                 GetOrthancWebViewerJpegCommand* command);

    IObservable oracleObservable_;
    std::string orthancRoot_;

  public:
    virtual void EmitMessage(boost::weak_ptr<IObserver> observer,
                             const IMessage& message) ORTHANC_OVERRIDE
    {
      oracleObservable_.EmitMessage(observer, message);
    }
    
    virtual bool Schedule(boost::shared_ptr<IObserver> receiver,
                          IOracleCommand* command) ORTHANC_OVERRIDE;

    IObservable& GetOracleObservable()
    {
      return oracleObservable_;
    }

    void SetOrthancRoot(const std::string& root)
    {
      orthancRoot_ = root;
    }
  };
}
