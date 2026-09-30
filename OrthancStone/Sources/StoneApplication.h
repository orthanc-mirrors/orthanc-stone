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

#include "Messages/IObservable.h"  // TODO Refactoring - Remove this

#include "Messages/IMessageEmitter.h"
#include "Oracle/IEnvironment.h"
#include "Oracle/IOracle.h"

#include <WebServiceParameters.h>

#include <MultiThreading/Mutex.h>  // TODO Refactoring - Remove this
#include <list>  // TODO Refactoring - Remove this


namespace OrthancStone
{
  class OracleScheduler;

  class StoneApplication : public boost::noncopyable
  {
  public:
    class Configuration
    {
    private:
      bool                            isLocalOrthanc_;
      Orthanc::WebServiceParameters   remoteOrthanc_;
      std::string                     localOrthancRoot_;
      std::string                     rootDirectory_;
      unsigned int                    oracleThreadsCount_;
      unsigned int                    workersTimeResolution_;
      size_t                          dicomCacheSize_;

    public:
      Configuration();

      bool IsLocalOrthanc() const
      {
        return isLocalOrthanc_;
      }

      void SetRemoteOrthancParameters(const Orthanc::WebServiceParameters& orthanc);

      const Orthanc::WebServiceParameters& GetRemoteOrthancParameters() const;

      // Using a local Orthanc only makes sense for WebAssembly, if it
      // is Orthanc that serves the Web application
      void SetLocalOrthancRoot(const std::string& root);

      const std::string& GetLocalOrthancRoot() const;

      void SetRootDirectory(const std::string& root)
      {
        rootDirectory_ = root;
      }

      const std::string& GetRootDirectory() const
      {
        return rootDirectory_;
      }

      void SetOracleThreadsCount(unsigned int count);

      unsigned int GetOracleThreadsCount() const
      {
        return oracleThreadsCount_;
      }

      void SetWorkersTimeResolution(unsigned int milliseconds);

      unsigned int GetWorkersTimeResolution() const
      {
        return workersTimeResolution_;
      }

      // Setting the cache size to zero disables it
      void SetDicomCacheSize(size_t size)
      {
        dicomCacheSize_ = size;
      }

      size_t GetDicomCacheSize() const
      {
        return dicomCacheSize_;
      }
    };


    class Context : public IMessageEmitter  // TODO Refactoring - Remove this
    {
      friend class StoneApplication;

    private:
      class Emitter;  // TODO Refactoring - Remove this

      class PImpl;
      PImpl* pimpl_;

      boost::shared_ptr<OracleScheduler>  oracleScheduler_;

      Orthanc::Mutex   loadersMutex_;  // TODO Refactoring - Remove this
      std::list< boost::shared_ptr<IObserver> >  loaders_;  // TODO Refactoring - Remove this

    public:
      Context(const Configuration& configuration);

      ~Context();

      IEnvironment& GetEnvironment();

      IOracle& GetOracle();

      virtual void EmitMessage(boost::weak_ptr<IObserver> observer,
                               const IMessage& message) ORTHANC_OVERRIDE;

      IObservable& GetOracleObservable();  // TODO Refactoring - Remove this

      void Schedule(boost::shared_ptr<IObserver> receiver,
                    int priority,
                    IOracleCommand* command /* Takes ownership */);  // TODO Refactoring - Remove this

      /**
       * Add a reference to the given observer in the Stone loaders
       * context. This can be used to match the lifetime of a loader
       * with the lifetime of the Stone context: This is useful if
       * your Stone application does not keep a reference to the
       * loader by itself (typically in global promises), which would
       * make the loader disappear as soon as the scope of the
       * variable is left.
       **/
      void AddLoader(const boost::shared_ptr<IObserver>& loader);  // TODO Refactoring - Remove this
    };


  private:
    Configuration  configuration_;

  protected:
    virtual void RunInternal(const boost::shared_ptr<Context>& context) = 0;

  public:
    StoneApplication(const Configuration& configuration) :
      configuration_(configuration)
    {
    }

    virtual ~StoneApplication()
    {
    }

    bool Run();

    static void Initialize(const Configuration& configuration);  // TODO Refactoring - Remove this

    static StoneApplication::Context& GetInstance();  // TODO Refactoring - Remove this

    static void Finalize();  // TODO Refactoring - Remove this
  };
}
