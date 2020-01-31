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
    public IObservable
  {
  private:
    typedef std::map<std::string, std::string>  HttpHeaders;
    
    class TimeoutContext;
    class Emitter;
    class FetchContext;
    class FetchCommand;    
    
    void Execute(const IObserver& receiver,
                 HttpCommand* command);    
    
    void Execute(const IObserver& receiver,
                 OrthancRestApiCommand* command);    
    
    void Execute(const IObserver& receiver,
                 GetOrthancImageCommand* command);    
    
    void Execute(const IObserver& receiver,
                 GetOrthancWebViewerJpegCommand* command);

    std::string orthancRoot_;

  public:
    WebAssemblyOracle(MessageBroker& broker) :
      IObservable(broker)
    {
    }

    void SetOrthancRoot(const std::string& root)
    {
      orthancRoot_ = root;
    }
    
    virtual void Schedule(const IObserver& receiver,
                          IOracleCommand* command);
  };
}
