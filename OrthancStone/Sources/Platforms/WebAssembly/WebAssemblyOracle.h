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

#include "../../OrthancStone.h"

#if !defined(ORTHANC_ENABLE_WASM)
#  error The macro ORTHANC_ENABLE_WASM must be defined
#endif

#if ORTHANC_ENABLE_WASM != 1
#  error This file can only compiled for WebAssembly
#endif

#include "../../Oracle/OracleCallback.h"   // TODO Refactoring

#include "../../Messages/IMessageEmitter.h"
#include "../../Messages/IObservable.h"
#include "../../Oracle/IEnvironment.h"
#include "../../Oracle/IOracle.h"
#include "../../StoneApplication.h"

#if ORTHANC_ENABLE_DCMTK == 1
#  include "../../Toolbox/ParsedDicomCache.h"
#endif

#include <Compatibility.h>  // For ORTHANC_OVERRIDE
#include <WebServiceParameters.h>

#include <Enumerations.h>

namespace OrthancStone
{
  class WebAssemblyOracle :
    public IOracle,   // TODO Refactoring - Remove old flavor
    public IMessageEmitter,  // TODO Refactoring - Remove old flavor
    public New::IOracle
  {
  private:
    typedef std::map<std::string, std::string>  HttpHeaders;
    
    class FetchContext;
    class FetchCommand;

    void SetOrthancUrl(FetchCommand& command,
                       const std::string& uri) const;

    static void ExecuteHttpCommand(FetchCommand& fetch);
    
    void ExecuteOrthancRestApiCommand(FetchCommand& fetch);
    
    void ExecuteGetOrthancImageCommand(FetchCommand& fetch);
    
    void ExecuteGetOrthancWebViewerJpegCommand(FetchCommand& fetch);
    
    void ExecuteParseDicomFromWadoCommand(IOracleCallback* callback);

    StoneApplication::Configuration  configuration_;
    IObservable                      oracleObservable_;

#if ORTHANC_ENABLE_DCMTK == 1
    std::unique_ptr<ParsedDicomCache>  dicomCache_;
#endif

    void ProcessFetchResult(IOracleCallback& callback,
                            const std::string& answer,
                            const HttpHeaders& headers);

    void Submit(IOracleCallback* callback);

  public:
    WebAssemblyOracle(const StoneApplication::Configuration& configuration);
    
    virtual void EmitMessage(boost::weak_ptr<IObserver> observer,
                             const IMessage& message) ORTHANC_OVERRIDE
    {
      oracleObservable_.EmitMessage(observer, message);
    }
    
    virtual bool Schedule(boost::shared_ptr<IObserver> receiver,
                          IOracleCommand* command) ORTHANC_OVERRIDE;

    virtual void Submit(IEnvironment& environment,
                        const boost::shared_ptr<IOracleClient>& client,
                        IOracleCommand* command /* takes ownership */) ORTHANC_OVERRIDE;

    IObservable& GetOracleObservable()
    {
      return oracleObservable_;
    }

    class CachedInstanceAccessor : public boost::noncopyable
    {
    private:
#if ORTHANC_ENABLE_DCMTK == 1
      std::unique_ptr<ParsedDicomCache::Reader>  reader_;
#endif

    public:
      CachedInstanceAccessor(WebAssemblyOracle& oracle,
                             const std::string& sopInstanceUid);

      bool IsValid() const;

#if ORTHANC_ENABLE_DCMTK == 1
      const Orthanc::ParsedDicomFile& GetDicom() const;
#endif

      size_t GetFileSize() const;

      bool HasPixelData() const;
    };    
  };
}
