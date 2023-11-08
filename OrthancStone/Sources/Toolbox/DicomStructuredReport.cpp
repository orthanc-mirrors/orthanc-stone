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


#include "DicomStructuredReport.h"

#include "../Scene2D/ScenePoint2D.h"

#include <OrthancException.h>
#include <SerializationToolbox.h>

#include <dcmtk/dcmdata/dcdeftag.h>
#include <dcmtk/dcmdata/dcsequen.h>
#include <dcmtk/dcmdata/dcfilefo.h>


static std::string FormatTag(const DcmTagKey& key)
{
  OFString s = key.toString();
  return std::string(s.c_str());
}


static std::string GetStringValue(DcmItem& dataset,
                                  const DcmTagKey& key)
{
  const char* value = NULL;
  if (dataset.findAndGetString(key, value).good() &&
      value != NULL)
  {
    return value;
  }
  else
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat,
                                    "Missing tag in DICOM-SR: " + FormatTag(key));
  }
}


static DcmSequenceOfItems& GetSequenceValue(DcmItem& dataset,
                                            const DcmTagKey& key)
{
  DcmSequenceOfItems* sequence = NULL;
  if (dataset.findAndGetSequence(key, sequence).good() &&
      sequence != NULL)
  {
    return *sequence;
  }
  else
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat,
                                    "Missing sequence in DICOM-SR: " + FormatTag(key));
  }
}


static void CheckStringValue(DcmItem& dataset,
                             const DcmTagKey& key,
                             const std::string& expected)
{
  if (GetStringValue(dataset, key) != expected)
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
  }
}


static bool IsDicomTemplate(DcmItem& dataset,
                            const std::string& tid)
{
  DcmSequenceOfItems& sequence = GetSequenceValue(dataset, DCM_ContentTemplateSequence);

  return (sequence.card() == 1 &&
          GetStringValue(*sequence.getItem(0), DCM_MappingResource) == "DCMR" &&
          GetStringValue(*sequence.getItem(0), DCM_TemplateIdentifier) == tid);
}


static bool IsValidConcept(DcmItem& dataset,
                           const DcmTagKey& key,
                           const std::string& scheme,
                           const std::string& concept)
{
  DcmSequenceOfItems& sequence = GetSequenceValue(dataset, key);

  return (sequence.card() == 1 &&
          GetStringValue(*sequence.getItem(0), DCM_CodingSchemeDesignator) == scheme &&
          GetStringValue(*sequence.getItem(0), DCM_CodeValue) == concept);
}


static bool IsDicomConcept(DcmItem& dataset,
                           const std::string& concept)
{
  return IsValidConcept(dataset, DCM_ConceptNameCodeSequence, "DCM", concept);
}


namespace OrthancStone
{
  void DicomStructuredReport::ReferencedInstance::AddFrame(unsigned int frame)
  {
    if (frame == 0)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      frames_.insert(frame - 1);
    }
  }


  class DicomStructuredReport::Structure : public boost::noncopyable
  {
  private:
    std::string   sopInstanceUid_;
    bool          hasFrameNumber_;
    unsigned int  frameNumber_;
    bool          hasProbabilityOfCancer_;
    float         probabilityOfCancer_;

  public:
    Structure(const std::string& sopInstanceUid) :
      sopInstanceUid_(sopInstanceUid),
      hasFrameNumber_(false),
      hasProbabilityOfCancer_(false)
    {
    }

    virtual ~Structure()
    {
    }

    void SetFrameNumber(unsigned int frame)
    {
      if (frame <= 0)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
      else
      {
        hasFrameNumber_ = true;
        frameNumber_ = frame - 1;
      }
    }

    void SetProbabilityOfCancer(float probability)
    {
      if (probability < 0 ||
          probability > 100)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
      else
      {
        hasProbabilityOfCancer_ = true;
        probabilityOfCancer_ = probability;
      }
    }

    bool HasFrameNumber() const
    {
      return hasFrameNumber_;
    }

    bool HasProbabilityOfCancer() const
    {
      return hasProbabilityOfCancer_;
    }

    unsigned int GetFrameNumber() const
    {
      if (hasFrameNumber_)
      {
        return frameNumber_;
      }
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
    }

    float GetProbabilityOfCancer() const
    {
      if (hasProbabilityOfCancer_)
      {
        return probabilityOfCancer_;
      }
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
    }
  };


  class DicomStructuredReport::Point : public Structure
  {
  private:
    ScenePoint2D  point_;

  public:
    Point(const std::string& sopInstanceUid,
          double x,
          double y) :
      Structure(sopInstanceUid),
      point_(x, y)
    {
    }

    const ScenePoint2D& GetPoint() const
    {
      return point_;
    }
  };


  class DicomStructuredReport::Polyline : public Structure
  {
  private:
    std::vector<ScenePoint2D>  points_;

  public:
    Polyline(const std::string& sopInstanceUid,
             const float* points,
             unsigned long pointsCount) :
      Structure(sopInstanceUid)
    {
      if (pointsCount % 2 != 0)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }

      points_.reserve(pointsCount / 2);

      for (unsigned long i = 0; i < pointsCount; i += 2)
      {
        points_.push_back(ScenePoint2D(points[i], points[i + 1]));
      }
    }

    size_t GetSize() const
    {
      return points_.size();
    }

    const ScenePoint2D& GetPoint(size_t i) const
    {
      if (i >= points_.size())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
      else
      {
        return points_[i];
      }
    }
  };


  void DicomStructuredReport::AddStructure(const std::string& sopInstanceUid,
                                           DcmItem& group,
                                           bool hasFrameNumber,
                                           unsigned int frameNumber,
                                           bool hasProbabilityOfCancer,
                                           float probabilityOfCancer)
  {
    const std::string graphicType = GetStringValue(group, DCM_GraphicType);

    const Float32* coords = NULL;
    unsigned long coordsCount = 0;
    if (!group.findAndGetFloat32Array(DCM_GraphicData, coords, &coordsCount).good() ||
        (coordsCount != 0 && coords == NULL))
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat,
                                      "Cannot read coordinates for region in DICOM-SR");
    }

    std::unique_ptr<Structure> structure;

    if (graphicType == "POINT")
    {
      if (coordsCount != 2)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }
      else
      {
        structure.reset(new Point(sopInstanceUid, coords[0], coords[1]));
      }
    }
    else if (graphicType == "POLYLINE")
    {
      structure.reset(new Polyline(sopInstanceUid, coords, coordsCount));
    }
    else
    {
      return;  // Unsupported graphic type
    }

    assert(structure.get() != NULL);

    if (hasFrameNumber)
    {
      structure->SetFrameNumber(frameNumber);
    }

    if (hasProbabilityOfCancer)
    {
      structure->SetProbabilityOfCancer(probabilityOfCancer);
    }

    structures_.push_back(structure.release());
  }


  DicomStructuredReport::DicomStructuredReport(Orthanc::ParsedDicomFile& dicom)
  {
    DcmDataset& dataset = *dicom.GetDcmtkObject().getDataset();

    studyInstanceUid_ = GetStringValue(dataset, DCM_StudyInstanceUID);
    seriesInstanceUid_ = GetStringValue(dataset, DCM_SeriesInstanceUID);
    sopInstanceUid_ = GetStringValue(dataset, DCM_SOPInstanceUID);

    CheckStringValue(dataset, DCM_Modality, "SR");
    CheckStringValue(dataset, DCM_SOPClassUID, "1.2.840.10008.5.1.4.1.1.88.33");  // Comprehensive SR IOD
    CheckStringValue(dataset, DCM_ValueType, "CONTAINER");

    if (!IsDicomConcept(dataset, "126000") /* Imaging measurement report */ ||
        !IsDicomTemplate(dataset, "1500"))
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }

    DcmSequenceOfItems& sequence = GetSequenceValue(dataset, DCM_CurrentRequestedProcedureEvidenceSequence);

    std::list<std::string> tmp;

    for (unsigned long i = 0; i < sequence.card(); i++)
    {
      std::string studyInstanceUid = GetStringValue(*sequence.getItem(i), DCM_StudyInstanceUID);

      DcmSequenceOfItems* referencedSeries = NULL;
      if (!sequence.getItem(i)->findAndGetSequence(DCM_ReferencedSeriesSequence, referencedSeries).good() ||
          referencedSeries == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }

      for (unsigned long j = 0; j < referencedSeries->card(); j++)
      {
        std::string seriesInstanceUid = GetStringValue(*referencedSeries->getItem(j), DCM_SeriesInstanceUID);

        DcmSequenceOfItems* referencedInstances = NULL;
        if (!referencedSeries->getItem(j)->findAndGetSequence(DCM_ReferencedSOPSequence, referencedInstances).good() ||
            referencedInstances == NULL)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
        }

        for (unsigned int k = 0; k < referencedInstances->card(); k++)
        {
          std::string sopClassUid = GetStringValue(*referencedInstances->getItem(k), DCM_ReferencedSOPClassUID);
          std::string sopInstanceUid = GetStringValue(*referencedInstances->getItem(k), DCM_ReferencedSOPInstanceUID);

          if (instancesInformation_.find(sopInstanceUid) == instancesInformation_.end())
          {
            instancesInformation_[sopInstanceUid] = new ReferencedInstance(studyInstanceUid, seriesInstanceUid, sopClassUid);
          }
          else
          {
            throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat,
                                            "Multiple occurrences of the same instance in DICOM-SR: " + sopInstanceUid);
          }

          tmp.push_back(sopInstanceUid);
        }
      }
    }

    orderedInstances_.reserve(tmp.size());

    for (std::list<std::string>::const_iterator it = tmp.begin(); it != tmp.end(); ++it)
    {
      orderedInstances_.push_back(*it);
    }

    sequence = GetSequenceValue(dataset, DCM_ContentSequence);

    for (unsigned long i = 0; i < sequence.card(); i++)
    {
      DcmItem& item = *sequence.getItem(i);

      if (GetStringValue(item, DCM_RelationshipType) == "CONTAINS" &&
          GetStringValue(item, DCM_ValueType) == "CONTAINER" &&
          IsDicomConcept(item, "126010" /* Imaging measurements */))
      {
        DcmSequenceOfItems& measurements = GetSequenceValue(item, DCM_ContentSequence);

        for (unsigned long j = 0; j < measurements.card(); j++)
        {
          DcmItem& measurement = *measurements.getItem(j);

          if (GetStringValue(measurement, DCM_RelationshipType) == "CONTAINS" &&
              GetStringValue(measurement, DCM_ValueType) == "CONTAINER" &&
              IsDicomConcept(measurement, "125007" /* Measurement group */) &&
              IsDicomTemplate(measurement, "1410"))
          {
            DcmSequenceOfItems& groups = GetSequenceValue(measurement, DCM_ContentSequence);

            bool hasProbabilityOfCancer = false;
            float probabilityOfCancer = 0;

            for (unsigned int k = 0; k < groups.card(); k++)
            {
              DcmItem& group = *groups.getItem(k);

              if (GetStringValue(group, DCM_RelationshipType) == "CONTAINS" &&
                  GetStringValue(group, DCM_ValueType) == "NUM" &&
                  IsDicomConcept(group, "111047" /* Probability of cancer */))
              {
                DcmSequenceOfItems& values = GetSequenceValue(group, DCM_MeasuredValueSequence);

                if (values.card() == 1 &&
                    IsValidConcept(*values.getItem(0), DCM_MeasurementUnitsCodeSequence, "UCUM", "%"))
                {
                  std::string value = GetStringValue(*values.getItem(0), DCM_NumericValue);
                  if (Orthanc::SerializationToolbox::ParseFloat(probabilityOfCancer, value))
                  {
                    hasProbabilityOfCancer = true;
                  }
                  else
                  {
                    throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat,
                                                    "Cannot parse float in DICOM-SR: " + value);
                  }
                }
              }
            }

            for (unsigned int k = 0; k < groups.card(); k++)
            {
              DcmItem& group = *groups.getItem(k);

              if (GetStringValue(group, DCM_RelationshipType) == "CONTAINS" &&
                  GetStringValue(group, DCM_ValueType) == "SCOORD" &&
                  IsDicomConcept(group, "111030" /* Image region */))
              {
                DcmSequenceOfItems& regions = GetSequenceValue(group, DCM_ContentSequence);

                for (unsigned int l = 0; l < regions.card(); l++)
                {
                  DcmItem& region = *regions.getItem(l);

                  if (GetStringValue(region, DCM_RelationshipType) == "SELECTED FROM" &&
                      GetStringValue(region, DCM_ValueType) == "IMAGE" &&
                      IsDicomConcept(region, "111040") /* Original source */)
                  {
                    DcmSequenceOfItems& instances = GetSequenceValue(region, DCM_ReferencedSOPSequence);
                    if (instances.card() != 1)
                    {
                      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat,
                                                      "Region cannot reference multiple instances in DICOM-SR");
                    }

                    std::string sopInstanceUid = GetStringValue(*instances.getItem(0), DCM_ReferencedSOPInstanceUID);
                    std::map<std::string, ReferencedInstance*>::iterator instanceInformation = instancesInformation_.find(sopInstanceUid);

                    if (instanceInformation == instancesInformation_.end())
                    {
                      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat,
                                                      "Referencing unknown instance in DICOM-SR: " + sopInstanceUid);
                    }

                    assert(instanceInformation->second != NULL);

                    if (instances.getItem(0)->tagExists(DCM_ReferencedFrameNumber))
                    {
                      std::string frames = GetStringValue(*instances.getItem(0), DCM_ReferencedFrameNumber);
                      std::vector<std::string> tokens;
                      Orthanc::Toolbox::SplitString(tokens, frames, '\\');

                      for (size_t m = 0; m < tokens.size(); m++)
                      {
                        uint32_t frame;
                        if (!Orthanc::SerializationToolbox::ParseUnsignedInteger32(frame, tokens[m]))
                        {
                          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
                        }
                        else
                        {
                          AddStructure(sopInstanceUid, group, true, frame, hasProbabilityOfCancer, probabilityOfCancer);
                          instanceInformation->second->AddFrame(frame);
                        }
                      }
                    }
                    else
                    {
                      AddStructure(sopInstanceUid, group, false, 0, hasProbabilityOfCancer, probabilityOfCancer);
                      instanceInformation->second->AddFrame(1);
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }


  DicomStructuredReport::~DicomStructuredReport()
  {
    for (std::list<Structure*>::iterator it = structures_.begin(); it != structures_.end(); ++it)
    {
      assert(*it != NULL);
      delete *it;
    }

    for (std::map<std::string, ReferencedInstance*>::iterator
           it = instancesInformation_.begin(); it != instancesInformation_.end(); ++it)
    {
      assert(it->second != NULL);
      delete it->second;
    }
  }


  void DicomStructuredReport::GetReferencedInstance(std::string& studyInstanceUid,
                                                    std::string& seriesInstanceUid,
                                                    std::string& sopInstanceUid,
                                                    std::string& sopClassUid,
                                                    size_t i) const
  {
    if (i >= orderedInstances_.size())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    sopInstanceUid = orderedInstances_[i];

    std::map<std::string, ReferencedInstance*>::const_iterator found = instancesInformation_.find(sopInstanceUid);
    if (found == instancesInformation_.end())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }

    assert(found->second != NULL);
    studyInstanceUid = found->second->GetStudyInstanceUid();
    seriesInstanceUid = found->second->GetSeriesInstanceUid();
    sopClassUid = found->second->GetSopClassUid();
  }


  void DicomStructuredReport::ExportReferencedFrames(std::list<ReferencedFrame>& frames) const
  {
    frames.clear();

    for (size_t i = 0; i < orderedInstances_.size(); i++)
    {
      std::map<std::string, ReferencedInstance*>::const_iterator found = instancesInformation_.find(orderedInstances_[i]);
      if (found == instancesInformation_.end())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }

      assert(found->second != NULL);

      for (std::set<unsigned int>::const_iterator frame = found->second->GetFrames().begin();
           frame != found->second->GetFrames().end(); ++frame)
      {
        frames.push_back(ReferencedFrame(found->second->GetStudyInstanceUid(),
                                         found->second->GetSeriesInstanceUid(),
                                         orderedInstances_[i],
                                         found->second->GetSopClassUid(), *frame));
      }
    }
  }
}
