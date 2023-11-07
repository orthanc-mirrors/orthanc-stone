/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2023 Osimis S.A., Belgium
 * Copyright (C) 2021-2023 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
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

#if !defined(ORTHANC_ENABLE_DCMTK)
#  error The macro ORTHANC_ENABLE_DCMTK must be defined
#endif

#if ORTHANC_ENABLE_DCMTK != 1
#  error Support for DCMTK must be enabled
#endif

#include <DicomParsing/ParsedDicomFile.h>

#include <dcmtk/dcmdata/dcitem.h>
#include <list>
#include <set>

namespace OrthancStone
{
  class DicomStructuredReport : public boost::noncopyable
  {
  private:
    class Structure;
    class Point;
    class Polyline;

    class ReferencedInstance : public boost::noncopyable
    {
    private:
      std::string  studyInstanceUid_;
      std::string  seriesInstanceUid_;
      std::string  sopClassUid_;
      std::set<unsigned int>  frames_;

    public:
      ReferencedInstance(const std::string& studyInstanceUid,
                         const std::string& seriesInstanceUid,
                         const std::string& sopClassUid) :
        studyInstanceUid_(studyInstanceUid),
        seriesInstanceUid_(seriesInstanceUid),
        sopClassUid_(sopClassUid)
      {
      }

      const std::string& GetStudyInstanceUid() const
      {
        return studyInstanceUid_;
      }

      const std::string& GetSeriesInstanceUid() const
      {
        return seriesInstanceUid_;
      }

      const std::string& GetSopClassUid() const
      {
        return sopClassUid_;
      }

      void AddFrame(unsigned int frame);

      const std::set<unsigned int>& GetFrames() const
      {
        return frames_;
      }
    };


    void AddStructure(const std::string& sopInstanceUid,
                      DcmItem& group,
                      bool hasFrameNumber,
                      unsigned int frameNumber,
                      bool hasProbabilityOfCancer,
                      float probabilityOfCancer);

    std::map<std::string, ReferencedInstance*>  instancesInformation_;
    std::vector<std::string>                    orderedInstances_;
    std::list<Structure*>                       structures_;

  public:
    class Frame
    {
    private:
      std::string  studyInstanceUid_;
      std::string  seriesInstanceUid_;
      std::string  sopInstanceUid_;
      std::string  sopClassUid_;
      unsigned int frameNumber_;

    public:
      Frame(const std::string& studyInstanceUid,
            const std::string& seriesInstanceUid,
            const std::string& sopInstanceUid,
            const std::string& sopClassUid,
            unsigned int frameNumber) :
        studyInstanceUid_(studyInstanceUid),
        seriesInstanceUid_(seriesInstanceUid),
        sopInstanceUid_(sopInstanceUid),
        sopClassUid_(sopClassUid),
        frameNumber_(frameNumber)
      {
      }

      const std::string& GetStudyInstanceUid() const
      {
        return studyInstanceUid_;
      }

      const std::string& GetSeriesInstanceUid() const
      {
        return seriesInstanceUid_;
      }

      const std::string& GetSopInstanceUid() const
      {
        return sopInstanceUid_;
      }

      const std::string& GetSopClassUid() const
      {
        return sopClassUid_;
      }

      unsigned int GetFrameNumber() const
      {
        return frameNumber_;
      }
    };

    DicomStructuredReport(Orthanc::ParsedDicomFile& dicom);

    ~DicomStructuredReport();

    size_t GetReferencedInstancesCount() const
    {
      return orderedInstances_.size();
    }

    void GetReferencedInstance(std::string& studyInstanceUid,
                               std::string& seriesInstanceUid,
                               std::string& sopInstanceUid,
                               std::string& sopClassUid,
                               size_t i) const;

    void ExportOrderedFrames(std::list<Frame>& frames) const;
  };
}
