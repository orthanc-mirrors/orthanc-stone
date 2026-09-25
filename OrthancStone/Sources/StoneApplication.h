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

#include "Oracle/IEnvironment.h"

#include <WebServiceParameters.h>


namespace OrthancStone
{
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

  private:
    class PImpl;
    PImpl* pimpl_;

    StoneApplication(const Configuration& configuration);

  public:
    static void Initialize(const Configuration& configuration);

    static StoneApplication& GetInstance();

    static void Finalize();

    ~StoneApplication();

    IEnvironment& GetEnvironment();

    void Submit(const boost::shared_ptr<IOracleClient>& client,
                IOracleCommand* command /* takes ownership */);
  };
}
