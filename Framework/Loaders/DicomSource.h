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

#include "../Oracle/IOracleCommand.h"

#include <Core/WebServiceParameters.h>

namespace OrthancStone
{
  enum DicomSourceType
  {
    DicomSourceType_Orthanc,
    DicomSourceType_DicomWeb,
    DicomSourceType_DicomWebThroughOrthanc,
    DicomSourceType_DicomDir
  };


  class DicomSource
  {
  private:
    DicomSourceType                type_;
    Orthanc::WebServiceParameters  webService_;
    std::string                    orthancDicomWebRoot_;
    std::string                    serverName_;
    bool                           hasOrthancWebViewer1_;
    bool                           hasOrthancAdvancedPreview_;
    bool                           hasDicomWebRendered_;

  public:
    DicomSource() :
      hasOrthancWebViewer1_(false),
      hasOrthancAdvancedPreview_(false),
      hasDicomWebRendered_(false)
    {
      SetOrthancSource();
    }

    DicomSourceType GetType() const
    {
      return type_;
    }

    void SetOrthancSource();

    void SetOrthancSource(const Orthanc::WebServiceParameters& parameters);

    const Orthanc::WebServiceParameters& GetOrthancParameters() const;

    void SetDicomDirSource();

    void SetDicomWebSource(const std::string& baseUrl);

    void SetDicomWebSource(const std::string& baseUrl,
                           const std::string& username,
                           const std::string& password);

    void SetDicomWebThroughOrthancSource(const Orthanc::WebServiceParameters& orthancParameters,
                                         const std::string& dicomWebRoot,
                                         const std::string& serverName);
    
    void SetDicomWebThroughOrthancSource(const std::string& serverName);
    
    bool IsDicomWeb() const;

    bool IsOrthanc() const
    {
      return type_ == DicomSourceType_Orthanc;
    }

    bool IsDicomDir() const
    {
      return type_ == DicomSourceType_DicomDir;
    }

    IOracleCommand* CreateDicomWebCommand(const std::string& uri,
                                          const std::map<std::string, std::string>& arguments,
                                          const std::map<std::string, std::string>& headers,
                                          Orthanc::IDynamicObject* payload /* takes ownership */) const;
    
    void AutodetectOrthancFeatures(const std::string& system,
                                   const std::string& plugins);

    void SetOrthancWebViewer1(bool hasPlugin);

    bool HasOrthancWebViewer1() const;

    void SetOrthancAdvancedPreview(bool hasFeature);

    bool HasOrthancAdvancedPreview() const;

    void SetDicomWebRendered(bool hasFeature);

    bool HasDicomWebRendered() const;

    unsigned int GetQualityCount() const;
  };
}