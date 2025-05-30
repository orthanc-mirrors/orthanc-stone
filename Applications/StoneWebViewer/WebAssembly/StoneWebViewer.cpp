/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2023 Osimis S.A., Belgium
 * Copyright (C) 2021-2025 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
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


#include "IStoneWebViewerContext.h"

#include <EmbeddedResources.h>
#include <emscripten.h>


// Orthanc framework includes
#include <Cache/MemoryObjectCache.h>
#include <DicomFormat/DicomArray.h>
#include <DicomParsing/ParsedDicomFile.h>
#include <Images/Image.h>
#include <Images/ImageProcessing.h>
#include <Images/JpegReader.h>
#include <Logging.h>

// Stone includes
#include "../../../OrthancStone/Sources/Loaders/DicomResourcesLoader.h"
#include "../../../OrthancStone/Sources/Loaders/SeriesMetadataLoader.h"
#include "../../../OrthancStone/Sources/Loaders/SeriesThumbnailsLoader.h"
#include "../../../OrthancStone/Sources/Messages/ObserverBase.h"
#include "../../../OrthancStone/Sources/Oracle/ParseDicomFromWadoCommand.h"
#include "../../../OrthancStone/Sources/Oracle/ParseDicomSuccessMessage.h"
#include "../../../OrthancStone/Sources/Scene2D/AnnotationsSceneLayer.h"
#include "../../../OrthancStone/Sources/Scene2D/ArrowSceneLayer.h"
#include "../../../OrthancStone/Sources/Scene2D/ColorTextureSceneLayer.h"
#include "../../../OrthancStone/Sources/Scene2D/FloatTextureSceneLayer.h"
#include "../../../OrthancStone/Sources/Scene2D/MacroSceneLayer.h"
#include "../../../OrthancStone/Sources/Scene2D/OsiriXLayerFactory.h"
#include "../../../OrthancStone/Sources/Scene2D/PolylineSceneLayer.h"
#include "../../../OrthancStone/Sources/Scene2D/TextSceneLayer.h"
#include "../../../OrthancStone/Sources/Scene2DViewport/ViewportController.h"
#include "../../../OrthancStone/Sources/StoneException.h"
#include "../../../OrthancStone/Sources/Toolbox/DicomInstanceParameters.h"
#include "../../../OrthancStone/Sources/Toolbox/DicomStructuredReport.h"
#include "../../../OrthancStone/Sources/Toolbox/GeometryToolbox.h"
#include "../../../OrthancStone/Sources/Toolbox/OsiriX/CollectionOfAnnotations.h"
#include "../../../OrthancStone/Sources/Toolbox/SortedFrames.h"
#include "../../../OrthancStone/Sources/Viewport/DefaultViewportInteractor.h"

// WebAssembly includes
#include "../../../OrthancStone/Sources/Platforms/WebAssembly/WebAssemblyCairoViewport.h"
#include "../../../OrthancStone/Sources/Platforms/WebAssembly/WebAssemblyLoadersContext.h"
#include "../../../OrthancStone/Sources/Platforms/WebAssembly/WebGLViewport.h"


#include <algorithm>
#include <boost/make_shared.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/math/special_functions/round.hpp>
#include <stdio.h>

static const double PI = boost::math::constants::pi<double>();


#if !defined(STONE_WEB_VIEWER_EXPORT)
// We are not running ParseWebAssemblyExports.py, but we're compiling the wasm
#  define STONE_WEB_VIEWER_EXPORT
#endif


#define FIX_LSD_479  1


enum STONE_WEB_VIEWER_EXPORT ThumbnailType
{
  ThumbnailType_Image,
    ThumbnailType_NoPreview,
    ThumbnailType_Pdf,
    ThumbnailType_Video,
    ThumbnailType_Loading,
    ThumbnailType_StructuredReport,
    ThumbnailType_Unknown,
    ThumbnailType_Unavailable
    };


enum STONE_WEB_VIEWER_EXPORT DisplayedFrameQuality
{
  DisplayedFrameQuality_None,
    DisplayedFrameQuality_Low,
    DisplayedFrameQuality_High
    };


enum STONE_WEB_VIEWER_EXPORT WebViewerAction
{
  WebViewerAction_None,
    
    WebViewerAction_Windowing,
    WebViewerAction_Zoom,
    WebViewerAction_Pan,
    WebViewerAction_Rotate,
    WebViewerAction_Crosshair,
    WebViewerAction_MagnifyingGlass,       // New in 2.4
    
    WebViewerAction_CreateAngle,
    WebViewerAction_CreateCircle,
    WebViewerAction_CreateLength,
    WebViewerAction_RemoveMeasure,
    WebViewerAction_CreatePixelProbe,      // New in 2.4
    WebViewerAction_CreateEllipseProbe,    // New in 2.4
    WebViewerAction_CreateRectangleProbe,  // New in 2.4
    WebViewerAction_CreateTextAnnotation   // New in 2.4
    };
  

enum FramesCollectionType
{
  FramesCollectionType_None,
  FramesCollectionType_DicomSR
};


static OrthancStone::MouseAction ConvertWebViewerAction(int action)
{
  switch (action)
  {
    case WebViewerAction_Windowing:
      return OrthancStone::MouseAction_GrayscaleWindowing;
      
    case WebViewerAction_Zoom:
      return OrthancStone::MouseAction_Zoom;
      
    case WebViewerAction_Pan:
      return OrthancStone::MouseAction_Pan;
      
    case WebViewerAction_Rotate:
      return OrthancStone::MouseAction_Rotate;
      
    case WebViewerAction_MagnifyingGlass:
      return OrthancStone::MouseAction_MagnifyingGlass;
      
    case WebViewerAction_None:
    case WebViewerAction_Crosshair:
    case WebViewerAction_CreateAngle:
    case WebViewerAction_CreateCircle:
    case WebViewerAction_CreateLength:
    case WebViewerAction_RemoveMeasure:
    case WebViewerAction_CreatePixelProbe:
    case WebViewerAction_CreateEllipseProbe:
    case WebViewerAction_CreateRectangleProbe:
    case WebViewerAction_CreateTextAnnotation:
      return OrthancStone::MouseAction_None;

    default:
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
  }
}



static const int PRIORITY_HIGH = -100;
static const int PRIORITY_LOW = 100;
static const int PRIORITY_NORMAL = 0;

static const unsigned int QUALITY_JPEG = 0;
static const unsigned int QUALITY_FULL = 1;

static const unsigned int DEFAULT_CINE_RATE = 30;



enum WindowingState
{
  WindowingState_None = 1,
  WindowingState_Fallback = 2,
  WindowingState_GlobalPreset = 3,
  WindowingState_FramePreset = 4,
  WindowingState_User = 5
};


class WindowingTracker
{
private:
  WindowingState           state_;
  OrthancStone::Windowing  windowing_;

public:
  WindowingTracker() :
    state_(WindowingState_None)
  {
  }

  WindowingState GetState() const
  {
    return state_;
  }

  const OrthancStone::Windowing& GetWindowing() const
  {
    return windowing_;
  }

  void Reset()
  {
    state_ = WindowingState_None;
    windowing_ = OrthancStone::Windowing();
  }

  // Returns "true" iif. the windowing needed an update
  bool Update(WindowingState newState,
              const OrthancStone::Windowing& newWindowing)
  {
    if (newState == WindowingState_None)
    {
      // "Reset()" should have been called
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    if (newState >= state_)
    {
      state_ = newState;

      if (windowing_.IsNear(newWindowing))
      {
        return false;
      }
      else
      {
        windowing_ = newWindowing;
        return true;
      }
    }
    else
    {
      return false;
    }
  }
};


class IFramesCollection : public boost::noncopyable
{
public:
  virtual ~IFramesCollection()
  {
  }

  virtual FramesCollectionType GetType() const = 0;

  virtual size_t GetFramesCount() const = 0;

  virtual const OrthancStone::DicomInstanceParameters& GetInstanceOfFrame(size_t frameIndex) const = 0;

  virtual unsigned int GetFrameNumberInInstance(size_t frameIndex) const = 0;

  virtual bool LookupFrame(size_t& frameIndex,
                           const std::string& sopInstanceUid,
                           unsigned int frameNumber) const = 0;

  virtual bool FindClosestFrame(size_t& frameIndex,
                                const OrthancStone::Vector& point,
                                double maximumDistance) const = 0;

  static OrthancStone::CoordinateSystem3D GetFrameGeometry(const IFramesCollection& frames,
                                                           size_t frameIndex)
  {
    return frames.GetInstanceOfFrame(frameIndex).GetFrameGeometry(frames.GetFrameNumberInInstance(frameIndex));
  }
};


class SortedFramesCollection : public IFramesCollection
{
private:
  std::unique_ptr<OrthancStone::SortedFrames>  frames_;

public:
  explicit SortedFramesCollection(OrthancStone::SortedFrames* frames)
  {
    if (frames == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }
    else
    {
      frames_.reset(frames);
    }
  }

  virtual FramesCollectionType GetType() const ORTHANC_OVERRIDE
  {
    return FramesCollectionType_None;
  }

  virtual size_t GetFramesCount() const ORTHANC_OVERRIDE
  {
    return frames_->GetFramesCount();
  }

  const OrthancStone::DicomInstanceParameters& GetInstanceOfFrame(size_t frameIndex) const ORTHANC_OVERRIDE
  {
    return frames_->GetInstanceOfFrame(frameIndex);
  }

  virtual unsigned int GetFrameNumberInInstance(size_t frameIndex) const ORTHANC_OVERRIDE
  {
    return frames_->GetFrameNumberInInstance(frameIndex);
  }

  virtual bool LookupFrame(size_t& frameIndex,
                           const std::string& sopInstanceUid,
                           unsigned int frameNumber) const ORTHANC_OVERRIDE
  {
    return frames_->LookupFrame(frameIndex, sopInstanceUid, frameNumber);
  }

  virtual bool FindClosestFrame(size_t& frameIndex,
                                const OrthancStone::Vector& point,
                                double maximumDistance) const ORTHANC_OVERRIDE
  {
    return frames_->FindClosestFrame(frameIndex, point, maximumDistance);
  }
};


class DicomStructuredReportFrames : public IFramesCollection
{
private:
  class Frame : public boost::noncopyable
  {
  private:
    OrthancStone::DicomStructuredReport::ReferencedFrame    info_;
    Orthanc::DicomMap                                       tags_;
    std::unique_ptr<OrthancStone::DicomInstanceParameters>  parameters_;

  public:
    Frame(const OrthancStone::DicomStructuredReport::ReferencedFrame& info,
          const OrthancStone::LoadedDicomResources& instances) :
      info_(info)
    {
      if (!instances.LookupResource(tags_, info.GetSopInstanceUid()))
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InexistentItem);
      }

      parameters_.reset(new OrthancStone::DicomInstanceParameters(tags_));
    }

    const OrthancStone::DicomStructuredReport::ReferencedFrame& GetInformation() const
    {
      return info_;
    }

    const Orthanc::DicomMap& GetTags() const
    {
      return tags_;
    }

    const OrthancStone::DicomInstanceParameters& GetParameters() const
    {
      return *parameters_;
    }
  };

  std::unique_ptr<OrthancStone::DicomStructuredReport>  sr_;
  OrthancStone::DicomInstanceParameters                 parameters_;
  std::vector<Frame*>                                   frames_;

  void Finalize()
  {
    for (size_t i = 0; i < frames_.size(); i++)
    {
      if (frames_[i] != NULL)
      {
        delete frames_[i];
      }
    }

    frames_.clear();
  }

  const Frame& GetFrame(size_t index) const
  {
    if (index >= frames_.size())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      assert(frames_[index] != NULL);
      return *frames_[index];
    }
  }

  static OrthancStone::Color& GetColorInternal()
  {
    static OrthancStone::Color color_(0, 255, 0);  // Default color: green
    return color_;
  }

public:
  DicomStructuredReportFrames(const OrthancStone::DicomStructuredReport& sr,
                              const OrthancStone::LoadedDicomResources& instances) :
    sr_(new OrthancStone::DicomStructuredReport(sr)),
    parameters_(sr.GetMainDicomTags())
  {
    std::list<OrthancStone::DicomStructuredReport::ReferencedFrame> tmp;
    sr_->ExportReferencedFrames(tmp);

    frames_.reserve(tmp.size());
    for (std::list<OrthancStone::DicomStructuredReport::ReferencedFrame>::const_iterator
           it = tmp.begin(); it != tmp.end(); ++it)
    {
      try
      {
        frames_.push_back(new Frame(*it, instances));
      }
      catch (Orthanc::OrthancException&)
      {
        // An instance is not loaded yet
        Finalize();
        throw;
      }
    }
  }

  virtual ~DicomStructuredReportFrames()
  {
    Finalize();
  }

  virtual FramesCollectionType GetType() const ORTHANC_OVERRIDE
  {
    return FramesCollectionType_DicomSR;
  }

  virtual size_t GetFramesCount() const ORTHANC_OVERRIDE
  {
    return frames_.size() + 1;
  }

  virtual const OrthancStone::DicomInstanceParameters& GetInstanceOfFrame(size_t frameIndex) const ORTHANC_OVERRIDE
  {
    if (frameIndex == frames_.size())
    {
      return parameters_;
    }
    else
    {
      return GetFrame(frameIndex).GetParameters();
    }
  }

  virtual unsigned int GetFrameNumberInInstance(size_t frameIndex) const ORTHANC_OVERRIDE
  {
    if (frameIndex == frames_.size())
    {
      return 0;
    }
    else
    {
      return GetFrame(frameIndex).GetInformation().GetFrameNumber();
    }
  }

  virtual bool LookupFrame(size_t& frameIndex,
                           const std::string& sopInstanceUid,
                           unsigned int frameNumber) const ORTHANC_OVERRIDE
  {
    if (sopInstanceUid == sr_->GetSopInstanceUid() &&
        frameNumber == 0)
    {
      frameIndex = frames_.size();
      return true;
    }

    // TODO - Could be speeded up with an additional index
    for (size_t i = 0; i < frames_.size(); i++)
    {
      if (frames_[i]->GetInformation().GetSopInstanceUid() == sopInstanceUid &&
          frames_[i]->GetInformation().GetFrameNumber() == frameNumber)
      {
        frameIndex = i;
        return true;
      }
    }

    return false;
  }

  virtual bool FindClosestFrame(size_t& frameIndex,
                                const OrthancStone::Vector& point,
                                double maximumDistance) const ORTHANC_OVERRIDE
  {
    bool found = false;

    for (size_t i = 0; i < GetFramesCount(); i++)
    {
      double distance = GetFrameGeometry(*this, i).ComputeDistance(point);
      if (distance <= maximumDistance)
      {
        found = true;
        frameIndex = i;
      }
    }

    return found;
  }

  OrthancStone::ISceneLayer* ExtractAnnotations(const std::string& sopInstanceUid,
                                                unsigned int frameNumber,
                                                double originX,
                                                double originY,
                                                double pixelSpacingX,
                                                double pixelSpacingY) const
  {
    std::unique_ptr<OrthancStone::MacroSceneLayer> layer(new OrthancStone::MacroSceneLayer);

    const double x = originX - pixelSpacingX / 2.0;
    const double y = originY - pixelSpacingY / 2.0;

    for (size_t i = 0; i < sr_->GetStructuresCount(); i++)
    {
      const OrthancStone::DicomStructuredReport::Structure& structure = sr_->GetStructure(i);
      if (structure.GetSopInstanceUid() == sopInstanceUid &&
          (!structure.HasFrameNumber() ||
           structure.GetFrameNumber() == frameNumber))
      {
#if 1
        const OrthancStone::Color& color = GetColorInternal();
#else
        OrthancStone::Color color(GetColorInternal());

        if (structure.HasProbabilityOfCancer())
        {
          if (structure.GetProbabilityOfCancer() > 50.0f)
          {
            color = OrthancStone::Color(255, 0, 0);
          }
          else
          {
            color = OrthancStone::Color(0, 255, 0);
          }
        }
#endif

        switch (structure.GetType())
        {
          case OrthancStone::DicomStructuredReport::StructureType_Point:
            // TODO
            break;

          case OrthancStone::DicomStructuredReport::StructureType_Polyline:
          {
            const OrthancStone::DicomStructuredReport::Polyline& source = dynamic_cast<const OrthancStone::DicomStructuredReport::Polyline&>(structure);

            if (source.GetSize() > 1)
            {
              std::unique_ptr<OrthancStone::PolylineSceneLayer> target(new OrthancStone::PolylineSceneLayer);

              OrthancStone::PolylineSceneLayer::Chain chain;
              chain.resize(source.GetSize());
              for (size_t i = 0; i < source.GetSize(); i++)
              {
                chain[i] = OrthancStone::ScenePoint2D(x + source.GetPoint(i).GetX() * pixelSpacingX,
                                                      y + source.GetPoint(i).GetY() * pixelSpacingY);
              }

              target->AddChain(chain, false, color.GetRed(), color.GetGreen(), color.GetBlue());
              layer->AddLayer(target.release());
            }
            break;
          }

          default:
            break;
        }
      }
    }

    return layer.release();
  }


  static const OrthancStone::Color& GetAnnotationsColor()
  {
    return GetColorInternal();
  }

  static void SetAnnotationsColor(const OrthancStone::Color& color)
  {
    GetColorInternal() = color;
  }
};


class VirtualSeries : public boost::noncopyable
{
private:
  class Item : public boost::noncopyable
  {
  private:
    std::string             seriesInstanceUid_;
    std::list<std::string>  sopInstanceUids_;

  public:
    Item(const std::string& seriesInstanceUid,
         const std::list<std::string>& sopInstanceUids) :
      seriesInstanceUid_(seriesInstanceUid),
      sopInstanceUids_(sopInstanceUids)
    {
    }

    const std::string& GetSeriesInstanceUid() const
    {
      return seriesInstanceUid_;
    }

    const std::list<std::string>& GetSopInstanceUids() const
    {
      return sopInstanceUids_;
    }
  };

  typedef std::map<std::string, Item*>  Content;

  Content  content_;

  const Item& GetItem(const std::string& id) const
  {
    Content::const_iterator found = content_.find(id);
    
    if (found == content_.end())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      assert(found->second != NULL);
      return *found->second;
    }
  }
  
public:
  ~VirtualSeries()
  {
    for (Content::iterator it = content_.begin(); it != content_.end(); ++it)
    {
      assert(it->second != NULL);
      delete it->second;
    }
  }
  
  std::string AddSingleInstance(const std::string& seriesInstanceUid,
                                const std::string& sopInstanceUid)
  {
    std::list<std::string> sopInstanceUids;
    sopInstanceUids.push_back(sopInstanceUid);
    return AddMultipleInstances(seriesInstanceUid, sopInstanceUids);
  }

  std::string AddMultipleInstances(const std::string& seriesInstanceUid,
                                   const std::list<std::string>& sopInstanceUids)
  {
    // Generate a unique identifier for this virtual series
    const std::string virtualSeriesId = "virtual-" + boost::lexical_cast<std::string>(content_.size());
    
    if (content_.find(virtualSeriesId) != content_.end())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      content_.insert(std::make_pair(virtualSeriesId, new Item(seriesInstanceUid, sopInstanceUids)));
      return virtualSeriesId;
    }
  }

  const std::string& GetSeriesInstanceUid(const std::string& id) const
  {
    return GetItem(id).GetSeriesInstanceUid();
  }

  const std::list<std::string>& GetSopInstanceUids(const std::string& id) const
  {
    return GetItem(id).GetSopInstanceUids();
  }
};



class ResourcesLoader : public OrthancStone::ObserverBase<ResourcesLoader>
{
public:
  class IObserver : public boost::noncopyable
  {
  public:
    virtual ~IObserver()
    {
    }

    virtual void SignalResourcesLoaded() = 0;

    virtual void SignalSeriesThumbnailLoaded(const std::string& studyInstanceUid,
                                             const std::string& seriesInstanceUid) = 0;

    virtual void SignalSeriesMetadataLoaded(const std::string& studyInstanceUid,
                                            const std::string& seriesInstanceUid) = 0;

    virtual void SignalSeriesPdfLoaded(const std::string& studyInstanceUid,
                                       const std::string& seriesInstanceUid,
                                       const std::string& pdf) = 0;

    virtual void SignalVirtualSeriesThumbnailLoaded(const std::string& virtualSeriesId,
                                                    const std::string& jpeg) = 0;

    virtual void SignalDicomSRLoaded(const std::string& studyInstanceUid,
                                     const std::string& seriesInstanceUid,
                                     const std::string& sopInstanceUid) = 0;
  };
  
private:
  OrthancStone::ILoadersContext&                           context_;
  std::unique_ptr<IObserver>                               observer_;
  OrthancStone::DicomSource                                source_;
  size_t                                                   pending_;
  boost::shared_ptr<OrthancStone::LoadedDicomResources>    studies_;
  boost::shared_ptr<OrthancStone::LoadedDicomResources>    series_;
  boost::shared_ptr<OrthancStone::LoadedDicomResources>    instances_;
  boost::shared_ptr<OrthancStone::DicomResourcesLoader>    resourcesLoader_;
  boost::shared_ptr<OrthancStone::SeriesThumbnailsLoader>  thumbnailsLoader_;
  boost::shared_ptr<OrthancStone::SeriesMetadataLoader>    metadataLoader_;
  std::set<std::string>                                    scheduledVirtualSeriesThumbnails_;
  VirtualSeries                                            virtualSeries_;
  std::vector<std::string>                                 skipSeriesFromModalities_;

  typedef std::map<std::string, boost::shared_ptr<OrthancStone::DicomStructuredReport> >  StructuredReports;
  StructuredReports structuredReports_;

  explicit ResourcesLoader(OrthancStone::ILoadersContext& context,
                           const OrthancStone::DicomSource& source) :
    context_(context),
    source_(source),
    pending_(0),
    studies_(new OrthancStone::LoadedDicomResources(Orthanc::DICOM_TAG_STUDY_INSTANCE_UID)),
    series_(new OrthancStone::LoadedDicomResources(Orthanc::DICOM_TAG_SERIES_INSTANCE_UID)),
    instances_(new OrthancStone::LoadedDicomResources(Orthanc::DICOM_TAG_SOP_INSTANCE_UID))
  {
  }

  void Handle(const OrthancStone::DicomResourcesLoader::SuccessMessage& message)
  {
    const Orthanc::SingleValueObject<Orthanc::ResourceType>& payload =
      dynamic_cast<const Orthanc::SingleValueObject<Orthanc::ResourceType>&>(message.GetUserPayload());
    
    OrthancStone::LoadedDicomResources& dicom = *message.GetResources();
    
    LOG(INFO) << "resources loaded: " << dicom.GetSize()
              << ", " << Orthanc::EnumerationToString(payload.GetValue());

    if (payload.GetValue() == Orthanc::ResourceType_Series)
    {
      // the 'dicom' var is actually equivalent to the 'series_' member in this case

      std::vector<std::string> seriesIdsToRemove;

      for (size_t i = 0; i < dicom.GetSize(); i++)
      {
        std::string modality;
        if (!dicom.GetResource(i).LookupStringValue(modality, Orthanc::DICOM_TAG_MODALITY, false))
        {
          modality = ""; // Arbitrary value if "Modality" is missing
        }

        std::string studyInstanceUid, seriesInstanceUid;
        if (dicom.GetResource(i).LookupStringValue(
              studyInstanceUid, Orthanc::DICOM_TAG_STUDY_INSTANCE_UID, false) &&
            dicom.GetResource(i).LookupStringValue(
              seriesInstanceUid, Orthanc::DICOM_TAG_SERIES_INSTANCE_UID, false))
        {
          // skip series that should not be displayed
          if (std::find(skipSeriesFromModalities_.begin(), skipSeriesFromModalities_.end(), modality) == skipSeriesFromModalities_.end())
          {
            thumbnailsLoader_->ScheduleLoadThumbnail(source_, "", studyInstanceUid, seriesInstanceUid);
            metadataLoader_->ScheduleLoadSeries(PRIORITY_LOW + 1, source_, studyInstanceUid, seriesInstanceUid);
          }
          else
          {
            seriesIdsToRemove.push_back(seriesInstanceUid);
          }
        }
      }

      for (size_t i = 0; i < seriesIdsToRemove.size(); i++)
      {
        LOG(INFO) << "series to hide: " << seriesIdsToRemove[i];
        dicom.RemoveResource(seriesIdsToRemove[i]);  
      }
    }
    else if (payload.GetValue() == Orthanc::ResourceType_Instance)
    {
      // This occurs if loading DICOM-SR: Show the DICOM-SR once all its referenced instances are loaded

      for (size_t i = 0; i < dicom.GetSize(); i++)
      {
        std::string studyInstanceUid, seriesInstanceUid, sopInstanceUid;
        if (dicom.GetResource(i).LookupStringValue(studyInstanceUid, Orthanc::DICOM_TAG_STUDY_INSTANCE_UID, false) &&
            dicom.GetResource(i).LookupStringValue(seriesInstanceUid, Orthanc::DICOM_TAG_SERIES_INSTANCE_UID, false) &&
            dicom.GetResource(i).LookupStringValue(sopInstanceUid, Orthanc::DICOM_TAG_SOP_INSTANCE_UID, false))
        {
          for (StructuredReports::const_iterator it = structuredReports_.begin(); it != structuredReports_.end(); ++it)
          {
            if (it->second->IsReferencedInstance(studyInstanceUid, seriesInstanceUid, sopInstanceUid))
            {
              bool complete = true;
              for (size_t j = 0; j < it->second->GetReferencedInstancesCount(); j++)
              {
                std::string referencedStudyInstanceUid, referencedSeriesInstanceUid, referencedInstanceInstanceUid, sopClassUid;
                it->second->GetReferencedInstance(referencedStudyInstanceUid, referencedSeriesInstanceUid, referencedInstanceInstanceUid, sopClassUid, j);
                if (!instances_->HasResource(referencedInstanceInstanceUid))
                {
                  complete = false;
                  break;
                }
              }

              if (complete)
              {
                LOG(INFO) << "Loaded all the instances referred by DICOM-SR instance: " << sopInstanceUid;
                if (observer_ != NULL)
                {
                  observer_->SignalDicomSRLoaded(it->second->GetStudyInstanceUid(),
                                                 it->second->GetSeriesInstanceUid(),
                                                 it->second->GetSopInstanceUid());
                }
              }
            }
          }
        }
      }
    }

    if (payload.GetValue() == Orthanc::ResourceType_Study ||
        payload.GetValue() == Orthanc::ResourceType_Series)
    {
      if (pending_ == 0)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
      else
      {
        pending_ --;
        if (pending_ == 0 &&
            observer_.get() != NULL)
        {
          observer_->SignalResourcesLoaded();
        }
      }
    }
  }

  void Handle(const OrthancStone::SeriesThumbnailsLoader::SuccessMessage& message)
  {
    if (observer_.get() != NULL)
    {
      observer_->SignalSeriesThumbnailLoaded(
        message.GetStudyInstanceUid(), message.GetSeriesInstanceUid());
    }
  }

  void Handle(const OrthancStone::SeriesMetadataLoader::SuccessMessage& message)
  {
    for (size_t i = 0; i < message.GetInstancesCount(); i++)
    {
      std::string sopInstanceUid, sopClassUid;
      if (message.GetInstance(i).LookupStringValue(sopInstanceUid, Orthanc::DICOM_TAG_SOP_INSTANCE_UID, false) &&
          message.GetInstance(i).LookupStringValue(sopClassUid, Orthanc::DICOM_TAG_SOP_CLASS_UID, false) &&
          OrthancStone::IsStructuredReport(OrthancStone::StringToSopClassUid(sopClassUid)))
      {
        std::unique_ptr<OrthancStone::ILoadersContext::ILock> lock(context_.Lock());
        lock->Schedule(
          GetSharedObserver(), PRIORITY_NORMAL, OrthancStone::ParseDicomFromWadoCommand::Create(
            source_, message.GetStudyInstanceUid(), message.GetSeriesInstanceUid(), sopInstanceUid,
            false /* no transcoding */, Orthanc::DicomTransferSyntax_LittleEndianExplicit /* dummy value */,
            new InstanceInfo(message.GetStudyInstanceUid(), message.GetSeriesInstanceUid(), sopInstanceUid, Action_StructuredReport)));
        return;
      }
    }

    if (observer_.get() != NULL)
    {
      observer_->SignalSeriesMetadataLoaded(
        message.GetStudyInstanceUid(), message.GetSeriesInstanceUid());
    }
  }

  void FetchInternal(const std::string& patientId,
                     const std::string& studyInstanceUid,
                     const std::string& seriesInstanceUid)
  {
    // Firstly, load the study
    Orthanc::DicomMap filter;

    if (!patientId.empty())
    {
      filter.SetValue(Orthanc::DICOM_TAG_PATIENT_ID, patientId, false);
    }

    if (!studyInstanceUid.empty())
    {
      filter.SetValue(Orthanc::DICOM_TAG_STUDY_INSTANCE_UID, studyInstanceUid, false);
    }

    std::set<Orthanc::DicomTag> tags;
    tags.insert(Orthanc::DICOM_TAG_STUDY_DESCRIPTION);  // Necessary for Orthanc DICOMweb plugin

    resourcesLoader_->ScheduleQido(
      studies_, PRIORITY_HIGH, source_, Orthanc::ResourceType_Study, filter, tags,
      new Orthanc::SingleValueObject<Orthanc::ResourceType>(Orthanc::ResourceType_Study));

    pending_++;

    // Secondly, load the series
    if (!seriesInstanceUid.empty())
    {
      filter.SetValue(Orthanc::DICOM_TAG_SERIES_INSTANCE_UID, seriesInstanceUid, false);
    }
    
    tags.insert(Orthanc::DICOM_TAG_SERIES_NUMBER);  // Necessary for Google Cloud Platform
    
    resourcesLoader_->ScheduleQido(
      series_, PRIORITY_HIGH, source_, Orthanc::ResourceType_Series, filter, tags,
      new Orthanc::SingleValueObject<Orthanc::ResourceType>(Orthanc::ResourceType_Series));

    pending_++;
  }


  enum Action
  {
    Action_Pdf,
    Action_StructuredReport,
    Action_ReferencedInstance
  };


  class InstanceInfo : public Orthanc::IDynamicObject
  {
  private:
    std::string  studyInstanceUid_;
    std::string  seriesInstanceUid_;
    std::string  sopInstanceUid_;
    Action       action_;

  public:
    InstanceInfo(const std::string& studyInstanceUid,
                 const std::string& seriesInstanceUid,
                 const std::string& sopInstanceUid,
                 Action action) :
      studyInstanceUid_(studyInstanceUid),
      seriesInstanceUid_(seriesInstanceUid),
      sopInstanceUid_(sopInstanceUid),
      action_(action)
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

    Action GetAction() const
    {
      return action_;
    }
  };


  void Handle(const OrthancStone::ParseDicomSuccessMessage& message)
  {
    const InstanceInfo& info = dynamic_cast<const InstanceInfo&>(message.GetOrigin().GetPayload());

    if (observer_.get() != NULL)
    {
      switch (info.GetAction())
      {
        case Action_Pdf:
        {
          std::string pdf;
          if (message.GetDicom().ExtractPdf(pdf))
          {
            observer_->SignalSeriesPdfLoaded(info.GetStudyInstanceUid(), info.GetSeriesInstanceUid(), pdf);
          }
          else
          {
            LOG(ERROR) << "Unable to extract PDF from series: " << info.GetSeriesInstanceUid();
          }

          break;
        }

        case Action_StructuredReport:
        {
          try
          {
            boost::shared_ptr<OrthancStone::DicomStructuredReport> sr(new OrthancStone::DicomStructuredReport(message.GetDicom()));

            for (size_t i = 0; i < sr->GetReferencedInstancesCount(); i++)
            {
              std::string studyInstanceUid;
              std::string seriesInstanceUid;
              std::string sopInstanceUid;
              std::string sopClassUid;
              sr->GetReferencedInstance(studyInstanceUid, seriesInstanceUid, sopInstanceUid, sopClassUid, i);

              Orthanc::DicomMap filter;
              filter.SetValue(Orthanc::DICOM_TAG_STUDY_INSTANCE_UID, studyInstanceUid, false);
              filter.SetValue(Orthanc::DICOM_TAG_SERIES_INSTANCE_UID, seriesInstanceUid, false);
              filter.SetValue(Orthanc::DICOM_TAG_SOP_INSTANCE_UID, sopInstanceUid, false);

              std::set<Orthanc::DicomTag> tags;

              resourcesLoader_->ScheduleQido(
                instances_, PRIORITY_NORMAL, source_, Orthanc::ResourceType_Instance, filter, tags,
                new Orthanc::SingleValueObject<Orthanc::ResourceType>(Orthanc::ResourceType_Instance));
            }

            structuredReports_[info.GetSeriesInstanceUid()] = sr;

            if (observer_.get() != NULL)
            {
              observer_->SignalSeriesMetadataLoaded(
                info.GetStudyInstanceUid(), info.GetSeriesInstanceUid());
            }
          }
          catch (Orthanc::OrthancException& e)
          {
            LOG(ERROR) << "Cannot decode DICOM-SR: " << e.What();
          }
          break;
        }

        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
    }
  }

  void FetchVirtualSeriesThumbnail(const std::string& virtualSeriesId,
                                   const std::string& studyInstanceUid,
                                   const std::string& seriesInstanceUid,
                                   const std::string& sopInstanceUid)
  {
    if (scheduledVirtualSeriesThumbnails_.find(virtualSeriesId) == scheduledVirtualSeriesThumbnails_.end())
    {
      scheduledVirtualSeriesThumbnails_.insert(virtualSeriesId);
      
      std::map<std::string, std::string> arguments;
      std::map<std::string, std::string> headers;
      arguments["viewport"] = (
        boost::lexical_cast<std::string>(thumbnailsLoader_->GetThumbnailWidth()) + "," +
        boost::lexical_cast<std::string>(thumbnailsLoader_->GetThumbnailHeight()));
      headers["Accept"] = Orthanc::MIME_JPEG;

      const std::string uri = ("studies/" + studyInstanceUid + "/series/" + seriesInstanceUid +
                               "/instances/" + sopInstanceUid + "/frames/1/rendered");

      {
        std::unique_ptr<OrthancStone::ILoadersContext::ILock> lock(context_.Lock());
        lock->Schedule(
          GetSharedObserver(), PRIORITY_LOW + 2, source_.CreateDicomWebCommand(
            uri, arguments, headers, new Orthanc::SingleValueObject<std::string>(virtualSeriesId)));
      }
    }
  }

  void HandleInstanceThumbnail(const OrthancStone::HttpCommand::SuccessMessage& message)
  {
    if (observer_.get() != NULL)
    {
      const std::string& virtualSeriesId =
        dynamic_cast<const Orthanc::SingleValueObject<std::string>&>(
          message.GetOrigin().GetPayload()).GetValue();
      observer_->SignalVirtualSeriesThumbnailLoaded(virtualSeriesId, message.GetAnswer());
    }
  }

public:
  void SetSkipSeriesFromModalities(const std::vector<std::string>& skipSeriesFromModalities)
  {
    skipSeriesFromModalities_ = skipSeriesFromModalities;
  }

  static boost::shared_ptr<ResourcesLoader> Create(const OrthancStone::ILoadersContext::ILock& lock,
                                                   const OrthancStone::DicomSource& source)
  {
    boost::shared_ptr<ResourcesLoader> loader(new ResourcesLoader(lock.GetContext(), source));

    loader->resourcesLoader_ = OrthancStone::DicomResourcesLoader::Create(lock);
    loader->thumbnailsLoader_ = OrthancStone::SeriesThumbnailsLoader::Create(lock, PRIORITY_LOW);
    loader->metadataLoader_ = OrthancStone::SeriesMetadataLoader::Create(lock);
    
    loader->Register<OrthancStone::DicomResourcesLoader::SuccessMessage>(
      *loader->resourcesLoader_, &ResourcesLoader::Handle);

    loader->Register<OrthancStone::SeriesThumbnailsLoader::SuccessMessage>(
      *loader->thumbnailsLoader_, &ResourcesLoader::Handle);

    loader->Register<OrthancStone::SeriesMetadataLoader::SuccessMessage>(
      *loader->metadataLoader_, &ResourcesLoader::Handle);

    loader->Register<OrthancStone::ParseDicomSuccessMessage>(
      lock.GetOracleObservable(), &ResourcesLoader::Handle);

    loader->Register<OrthancStone::HttpCommand::SuccessMessage>(
      lock.GetOracleObservable(), &ResourcesLoader::HandleInstanceThumbnail);

    return loader;
  }
  
  void FetchAllStudies()
  {
    FetchInternal("", "", "");
  }
  
  void FetchPatient(const std::string& patientId)
  {
    if (!patientId.empty())
    {
      FetchInternal(patientId, "", "");
    }
  }
  
  void FetchStudy(const std::string& studyInstanceUid)
  {
    if (!studyInstanceUid.empty())
    {
      FetchInternal("", studyInstanceUid, "");
    }
  }
  
  void FetchSeries(const std::string& studyInstanceUid,
                   const std::string& seriesInstanceUid)
  {
    if (!studyInstanceUid.empty() &&
        !seriesInstanceUid.empty())
    {
      FetchInternal("", studyInstanceUid, seriesInstanceUid);
    }
  }

  size_t GetStudiesCount() const
  {
    return studies_->GetSize();
  }

  size_t GetSeriesCount() const
  {
    return series_->GetSize();
  }

  void GetStudy(Orthanc::DicomMap& target,
                size_t i) const
  {
    target.Assign(studies_->GetResource(i));
  }

  void GetSeries(Orthanc::DicomMap& target,
                 size_t i) const
  {
    target.Assign(series_->GetResource(i));

    // Complement with the study-level tags
    std::string studyInstanceUid;
    if (target.LookupStringValue(studyInstanceUid, Orthanc::DICOM_TAG_STUDY_INSTANCE_UID, false) &&
        studies_->HasResource(studyInstanceUid))
    {
      studies_->MergeResource(target, studyInstanceUid);
    }
  }

  OrthancStone::SeriesThumbnailType GetSeriesThumbnail(std::string& image,
                                                       std::string& mime,
                                                       const std::string& seriesInstanceUid) const
  {
    return thumbnailsLoader_->GetSeriesThumbnail(image, mime, seriesInstanceUid);
  }

  void FetchSeriesMetadata(int priority,
                           const std::string& studyInstanceUid,
                           const std::string& seriesInstanceUid) const
  {
    metadataLoader_->ScheduleLoadSeries(priority, source_, studyInstanceUid, seriesInstanceUid);
  }

  bool IsSeriesComplete(const std::string& seriesInstanceUid) const
  {
    OrthancStone::SeriesMetadataLoader::Accessor accessor(*metadataLoader_, seriesInstanceUid);
    return accessor.IsComplete();
  }

  bool LookupVirtualSeries(std::map<std::string, unsigned int>& virtualSeries /* out */,
                           const std::string& seriesInstanceUid)
  {
    OrthancStone::SeriesMetadataLoader::Accessor accessor(*metadataLoader_, seriesInstanceUid);
    if (accessor.IsComplete() &&
        accessor.GetInstancesCount() >= 2)
    {
      bool hasMultiframe = false;
      
      for (size_t i = 0; i < accessor.GetInstancesCount(); i++)
      {
        OrthancStone::DicomInstanceParameters p(accessor.GetInstance(i));

        if (p.GetNumberOfFrames() > 1)
        {
          hasMultiframe = true;
        }
      }

      if (hasMultiframe)
      {
        std::string studyInstanceUid;
        std::list<std::string> instancesWithoutFrameNumber;
        
        for (size_t i = 0; i < accessor.GetInstancesCount(); i++)
        {
          OrthancStone::DicomInstanceParameters p(accessor.GetInstance(i));

          if (p.HasNumberOfFrames())
          {
            const std::string virtualSeriesId = virtualSeries_.AddSingleInstance(seriesInstanceUid, p.GetSopInstanceUid());
            if (virtualSeries.find(virtualSeriesId) != virtualSeries.end())
            {
              throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
            }
            else
            {
              virtualSeries[virtualSeriesId] = p.GetNumberOfFrames();
              FetchVirtualSeriesThumbnail(virtualSeriesId, p.GetStudyInstanceUid(), seriesInstanceUid, p.GetSopInstanceUid());
            }
          }
          else
          {
            studyInstanceUid = p.GetStudyInstanceUid();
            instancesWithoutFrameNumber.push_back(p.GetSopInstanceUid());
          }
        }

        if (!instancesWithoutFrameNumber.empty())
        {
          /**
           * Group together in a single "virtual series" all the DICOM
           * instances that have no value for the tag "NumberOfFrames"
           * (0028,0008). This can happen in US CINE series. New in
           * Stone Web viewer 2.1.
           * https://groups.google.com/g/orthanc-users/c/V-vOnlwj06A/m/2sPNwteYAAAJ
           **/
          const std::string virtualSeriesId = virtualSeries_.AddMultipleInstances(seriesInstanceUid, instancesWithoutFrameNumber);
          virtualSeries[virtualSeriesId] = instancesWithoutFrameNumber.size();
          FetchVirtualSeriesThumbnail(virtualSeriesId, studyInstanceUid, seriesInstanceUid, instancesWithoutFrameNumber.front());
        }

        return true;
      }
      else
      {
        return false;
      }
    }
    else
    {
      return false;
    }
  }

  IFramesCollection* GetSeriesFrames(const std::string& seriesInstanceUid) const
  {
    OrthancStone::SeriesMetadataLoader::Accessor accessor(*metadataLoader_, seriesInstanceUid);
    
    if (accessor.IsComplete())
    {
      StructuredReports::const_iterator sr = structuredReports_.find(seriesInstanceUid);
      if (sr != structuredReports_.end())
      {
        assert(sr->second != NULL);

        try
        {
          return new DicomStructuredReportFrames(*sr->second, *instances_);
        }
        catch (Orthanc::OrthancException&)
        {
          LOG(INFO) << "All the instances referenced by the DICOM-SR series \""
                    << seriesInstanceUid << "\" are not available yet";
          return NULL;
        }
      }

      std::unique_ptr<OrthancStone::SortedFrames> target(new OrthancStone::SortedFrames);
      target->Clear();

      for (size_t i = 0; i < accessor.GetInstancesCount(); i++)
      {
        target->AddInstance(accessor.GetInstance(i));
      }

      target->Sort();
      
      return new SortedFramesCollection(target.release());
    }
    else
    {
      return NULL;
    }
  }

  IFramesCollection* GetVirtualSeriesFrames(const std::string& virtualSeriesId) const
  {
    const std::string& seriesInstanceUid = virtualSeries_.GetSeriesInstanceUid(virtualSeriesId);
    
    OrthancStone::SeriesMetadataLoader::Accessor accessor(*metadataLoader_, seriesInstanceUid);
    
    if (accessor.IsComplete())
    {
      const std::list<std::string>& sopInstanceUids = virtualSeries_.GetSopInstanceUids(virtualSeriesId);

      std::unique_ptr<OrthancStone::SortedFrames> target(new OrthancStone::SortedFrames);
      target->Clear();

      for (std::list<std::string>::const_iterator
             it = sopInstanceUids.begin(); it != sopInstanceUids.end(); ++it)
      {
        Orthanc::DicomMap instance;
        if (accessor.LookupInstance(instance, *it))
        {
          target->AddInstance(instance);
        }
        else
        {
          LOG(ERROR) << "Missing instance: " << *it;
        }
      }
      
      target->Sort();

      return new SortedFramesCollection(target.release());
    }
    else
    {
      return NULL;
    }
  }

  size_t GetSeriesNumberOfFrames(const std::string& seriesInstanceUid) const
  {
    OrthancStone::SeriesMetadataLoader::Accessor accessor(*metadataLoader_, seriesInstanceUid);
    
    if (accessor.IsComplete())
    {
      size_t count = 0;
      
      for (size_t i = 0; i < accessor.GetInstancesCount(); i++)
      {
        uint32_t f;
        if (accessor.GetInstance(i).ParseUnsignedInteger32(f, Orthanc::DICOM_TAG_NUMBER_OF_FRAMES))
        {
          count += f;
        }
        else
        {
          count++;
        }
      }

      return count;
    }
    else
    {
      return 0;
    }
  }

  void AcquireObserver(IObserver* observer)
  {  
    observer_.reset(observer);
  }

  void FetchPdf(const std::string& studyInstanceUid,
                const std::string& seriesInstanceUid)
  {
    OrthancStone::SeriesMetadataLoader::Accessor accessor(*metadataLoader_, seriesInstanceUid);
    
    if (accessor.IsComplete())
    {
      if (accessor.GetInstancesCount() > 1)
      {
        LOG(INFO) << "Series with more than one instance, will show the first PDF: "
                  << seriesInstanceUid;
      }

      for (size_t i = 0; i < accessor.GetInstancesCount(); i++)
      {
        std::string sopClassUid, sopInstanceUid;
        if (accessor.GetInstance(i).LookupStringValue(sopClassUid, Orthanc::DICOM_TAG_SOP_CLASS_UID, false) &&
            accessor.GetInstance(i).LookupStringValue(sopInstanceUid, Orthanc::DICOM_TAG_SOP_INSTANCE_UID, false) &&
            OrthancStone::StringToSopClassUid(sopClassUid) == OrthancStone::SopClassUid_EncapsulatedPdf)
        {
          std::unique_ptr<OrthancStone::ILoadersContext::ILock> lock(context_.Lock());
          lock->Schedule(
            GetSharedObserver(), PRIORITY_NORMAL, OrthancStone::ParseDicomFromWadoCommand::Create(
              source_, studyInstanceUid, seriesInstanceUid, sopInstanceUid,
              false /* no transcoding */, Orthanc::DicomTransferSyntax_LittleEndianExplicit /* dummy value */,
              new InstanceInfo(studyInstanceUid, seriesInstanceUid, sopInstanceUid, Action_Pdf)));
          return;
        }
      }

      LOG(WARNING) << "Series without a PDF: " << seriesInstanceUid;
    }
  }
};



class FramesCache : public boost::noncopyable
{
private:
  class CachedImage : public Orthanc::ICacheable
  {
  private:
    std::unique_ptr<Orthanc::ImageAccessor>  image_;
    unsigned int                             quality_;

  public:
    CachedImage(Orthanc::ImageAccessor* image,
                unsigned int quality) :
      image_(image),
      quality_(quality)
    {
      assert(image != NULL);
    }

    virtual size_t GetMemoryUsage() const ORTHANC_OVERRIDE
    {    
      assert(image_.get() != NULL);
      return (image_->GetBytesPerPixel() * image_->GetPitch() * image_->GetHeight());
    }

    const Orthanc::ImageAccessor& GetImage() const
    {
      assert(image_.get() != NULL);
      return *image_;
    }

    unsigned int GetQuality() const
    {
      return quality_;
    }
  };


  static std::string GetKey(const std::string& sopInstanceUid,
                            unsigned int frameNumber)
  {
    return sopInstanceUid + "|" + boost::lexical_cast<std::string>(frameNumber);
  }
  

  Orthanc::MemoryObjectCache  cache_;
  
public:
  FramesCache()
  {
    SetMaximumSize(100 * 1024 * 1024);  // 100 MB
    //SetMaximumSize(1);  // DISABLE CACHE
  }
  
  size_t GetMaximumSize()
  {
    return cache_.GetMaximumSize();
  }
    
  void SetMaximumSize(size_t size)
  {
    cache_.SetMaximumSize(size);
  }

  /**
   * Returns "true" iff the provided image has better quality than the
   * previously cached one, or if no cache was previously available.
   **/
  bool Acquire(const std::string& sopInstanceUid,
               unsigned int frameNumber,
               Orthanc::ImageAccessor* image /* transfer ownership */,
               unsigned int quality)
  {
    std::unique_ptr<Orthanc::ImageAccessor> protection(image);
    
    if (image == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }
    else if (image->GetFormat() != Orthanc::PixelFormat_Float32 &&
             image->GetFormat() != Orthanc::PixelFormat_RGB24)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageFormat);
    }

    const std::string& key = GetKey(sopInstanceUid, frameNumber);

    bool invalidate = false;
    
    {
      /**
       * Access the previous cached entry, with side effect of tagging
       * it as the most recently accessed frame (update of LRU recycling)
       **/
      Orthanc::MemoryObjectCache::Accessor accessor(cache_, key, false /* unique lock */);

      if (accessor.IsValid())
      {
        const CachedImage& previous = dynamic_cast<const CachedImage&>(accessor.GetValue());
        
        // There is already a cached image for this frame
        if (previous.GetQuality() < quality)
        {
          // The previously stored image has poorer quality
          invalidate = true;
        }
        else
        {
          // No update in the quality, don't change the cache
          return false;   
        }
      }
      else
      {
        invalidate = false;
      }
    }

    if (invalidate)
    {
      cache_.Invalidate(key);
    }
        
    cache_.Acquire(key, new CachedImage(protection.release(), quality));
    return true;
  }

  class Accessor : public boost::noncopyable
  {
  private:
    Orthanc::MemoryObjectCache::Accessor accessor_;

    const CachedImage& GetCachedImage() const
    {
      if (IsValid())
      {
        return dynamic_cast<CachedImage&>(accessor_.GetValue());
      }
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
    }
    
  public:
    Accessor(FramesCache& that,
             const std::string& sopInstanceUid,
             unsigned int frameNumber) :
      accessor_(that.cache_, GetKey(sopInstanceUid, frameNumber), false /* shared lock */)
    {
    }

    bool IsValid() const
    {
      return accessor_.IsValid();
    }

    const Orthanc::ImageAccessor& GetImage() const
    {
      return GetCachedImage().GetImage();
    }

    unsigned int GetQuality() const
    {
      return GetCachedImage().GetQuality();
    }
  };
};



class InstancesCache : public boost::noncopyable
{
private:
  // Maps "SOP Instance UID" to DICOM parameters
  typedef std::map<std::string, OrthancStone::DicomInstanceParameters*>  Content;

  Content  content_;

  void Clear()
  {
    for (Content::iterator it = content_.begin(); it != content_.end(); ++it)
    {
      assert(it->second != NULL);
      delete it->second;
    }

    content_.clear();
  }

public:
  ~InstancesCache()
  {
    Clear();
  }

  void Store(const std::string& sopInstanceUid,
             const OrthancStone::DicomInstanceParameters& parameters)
  {
    Content::iterator found = content_.find(sopInstanceUid);
    if (found == content_.end())
    {
      content_[sopInstanceUid] = parameters.Clone();
    }
  }

  class Accessor : public boost::noncopyable
  {
  private:
    std::unique_ptr<OrthancStone::DicomInstanceParameters>  parameters_;

  public:
    Accessor(InstancesCache& that,
             const std::string& sopInstanceUid)
    {
      Content::iterator found = that.content_.find(sopInstanceUid);
      if (found != that.content_.end())
      {
        assert(found->second != NULL);
        parameters_.reset(found->second->Clone());
      }
    }

    bool IsValid() const
    {
      return parameters_.get() != NULL;
    }

    const OrthancStone::DicomInstanceParameters& GetParameters() const
    {
      if (IsValid())
      {
        return *parameters_;
      }
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
    }
  };
};



class SeriesCursor : public boost::noncopyable
{
public:
  enum Action
  {
    Action_FastPlus,
    Action_Plus,
    Action_None,
    Action_Minus,
    Action_FastMinus
  };
  
private:
  std::vector<size_t>  prefetch_;
  int                  framesCount_;
  int                  currentFrame_;
  bool                 isCircularPrefetch_;
  int                  fastDelta_;
  Action               lastAction_;

  int ComputeNextFrame(int currentFrame,
                       Action action,
                       bool isCircular) const
  {
    if (framesCount_ == 0)
    {
      assert(currentFrame == 0);
      return 0;
    }

    int nextFrame = currentFrame;
    
    switch (action)
    {
      case Action_FastPlus:
        nextFrame += fastDelta_;
        break;

      case Action_Plus:
        nextFrame += 1;
        break;

      case Action_None:
        break;

      case Action_Minus:
        nextFrame -= 1;
        break;

      case Action_FastMinus:
        nextFrame -= fastDelta_;
        break;

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    if (isCircular)
    {
      while (nextFrame < 0)
      {
        nextFrame += framesCount_;
      }

      while (nextFrame >= framesCount_)
      {
        nextFrame -= framesCount_;
      }
    }
    else
    {
      if (nextFrame < 0)
      {
        nextFrame = 0;
      }
      else if (nextFrame >= framesCount_)
      {
        nextFrame = framesCount_ - 1;
      }
    }

    return nextFrame;
  }
  
  void UpdatePrefetch()
  {
    /**
     * This method will order the frames of the series according to
     * the number of "actions" (i.e. mouse wheels) that are necessary
     * to reach them, starting from the current frame. It is assumed
     * that once one action is done, it is more likely that the user
     * will do the same action just afterwards.
     **/
    
    prefetch_.clear();

    if (framesCount_ == 0)
    {
      return;
    }

    prefetch_.reserve(framesCount_);
    
    // Breadth-first search using a FIFO. The queue associates a frame
    // and the action that is the most likely in this frame
    typedef std::list< std::pair<int, Action> >  Queue;

    Queue queue;
    std::set<int>  visited;  // Frames that have already been visited

    queue.push_back(std::make_pair(currentFrame_, lastAction_));

    while (!queue.empty())
    {
      int frame = queue.front().first;
      Action previousAction = queue.front().second;
      queue.pop_front();

      if (visited.find(frame) == visited.end())
      {
        visited.insert(frame);
        prefetch_.push_back(frame);

        switch (previousAction)
        {
          case Action_None:
          case Action_Plus:
            queue.push_back(std::make_pair(ComputeNextFrame(frame, Action_Plus, isCircularPrefetch_), Action_Plus));
            queue.push_back(std::make_pair(ComputeNextFrame(frame, Action_Minus, isCircularPrefetch_), Action_Minus));
            queue.push_back(std::make_pair(ComputeNextFrame(frame, Action_FastPlus, isCircularPrefetch_), Action_FastPlus));
            queue.push_back(std::make_pair(ComputeNextFrame(frame, Action_FastMinus, isCircularPrefetch_), Action_FastMinus));
            break;
          
          case Action_Minus:
            queue.push_back(std::make_pair(ComputeNextFrame(frame, Action_Minus, isCircularPrefetch_), Action_Minus));
            queue.push_back(std::make_pair(ComputeNextFrame(frame, Action_Plus, isCircularPrefetch_), Action_Plus));
            queue.push_back(std::make_pair(ComputeNextFrame(frame, Action_FastMinus, isCircularPrefetch_), Action_FastMinus));
            queue.push_back(std::make_pair(ComputeNextFrame(frame, Action_FastPlus, isCircularPrefetch_), Action_FastPlus));
            break;

          case Action_FastPlus:
            queue.push_back(std::make_pair(ComputeNextFrame(frame, Action_FastPlus, isCircularPrefetch_), Action_FastPlus));
            queue.push_back(std::make_pair(ComputeNextFrame(frame, Action_FastMinus, isCircularPrefetch_), Action_FastMinus));
            queue.push_back(std::make_pair(ComputeNextFrame(frame, Action_Plus, isCircularPrefetch_), Action_Plus));
            queue.push_back(std::make_pair(ComputeNextFrame(frame, Action_Minus, isCircularPrefetch_), Action_Minus));
            break;
              
          case Action_FastMinus:
            queue.push_back(std::make_pair(ComputeNextFrame(frame, Action_FastMinus, isCircularPrefetch_), Action_FastMinus));
            queue.push_back(std::make_pair(ComputeNextFrame(frame, Action_FastPlus, isCircularPrefetch_), Action_FastPlus));
            queue.push_back(std::make_pair(ComputeNextFrame(frame, Action_Minus, isCircularPrefetch_), Action_Minus));
            queue.push_back(std::make_pair(ComputeNextFrame(frame, Action_Plus, isCircularPrefetch_), Action_Plus));
            break;

          default:
            throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
        }
      }
    }

    assert(prefetch_.size() == framesCount_);
  }

  bool CheckFrameIndex(int frame) const
  {
    return ((framesCount_ == 0 && frame == 0) ||
            (framesCount_ > 0 && frame >= 0 && frame < framesCount_));
  }
  
public:
  explicit SeriesCursor(size_t framesCount,
                        bool startAtMiddle /* Whether to start at the middle frame */) :
    framesCount_(framesCount),
    currentFrame_(startAtMiddle ? framesCount / 2 : 0),
    isCircularPrefetch_(false),
    lastAction_(Action_None)
  {
    SetFastDelta(framesCount / 20);
    UpdatePrefetch();
  }

  size_t GetFramesCount() const
  {
    return framesCount_;
  }

  void SetCircularPrefetch(bool isCircularPrefetch)
  {
    isCircularPrefetch_ = isCircularPrefetch;
    UpdatePrefetch();
  }

  void SetFastDelta(int delta)
  {
    fastDelta_ = (delta < 0 ? -delta : delta);

    if (fastDelta_ <= 0)
    {
      fastDelta_ = 1;
    }
  }

  void SetCurrentIndex(size_t frame)
  {
    if (frame >= framesCount_)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      currentFrame_ = frame;
      lastAction_ = Action_None;
      UpdatePrefetch();
    }
  }

  size_t GetCurrentIndex() const
  {
    assert(CheckFrameIndex(currentFrame_));
    return static_cast<size_t>(currentFrame_);
  }

  void Apply(Action action,
             bool isCircular)
  {
    currentFrame_ = ComputeNextFrame(currentFrame_, action, isCircular);
    lastAction_ = action;
    UpdatePrefetch();
  }

  size_t GetPrefetchSize() const
  {
    assert(prefetch_.size() == framesCount_);
    return prefetch_.size();
  }

  size_t GetPrefetchIndex(size_t i) const
  {
    if (i >= prefetch_.size())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      assert(CheckFrameIndex(prefetch_[i]));
      return static_cast<size_t>(prefetch_[i]);
    }
  }
};




/**
 * Returns a clipped line (out: x1, y1, x2, y2), in the coordinate
 * system of "instance1". Note that the frame of reference UID is not
 * checked by this function.
 **/
static bool GetReferenceLineCoordinates(double& x1,
                                        double& y1,
                                        double& x2,
                                        double& y2,
                                        const OrthancStone::DicomInstanceParameters& instance1,
                                        unsigned int frame1,
                                        const OrthancStone::CoordinateSystem3D& plane2)
{
  if (instance1.GetWidth() == 0 &&
      instance1.GetHeight() == 0)
  {
    return false;
  }
  else
  {
    /**
     * Compute the 2D extent of the "instance1", expressed in
     * centimeters, in the 2D plane defined by this DICOM instance.
     *
     * In a multiframe image (cf. "ExtractFrameOffsets()"), the plane of
     * each frame is a translation of the plane of the first frame along
     * its normal. As a consequence, the extent is the same for each
     * frame, so we can ignore the frame number.
     **/
    OrthancStone::Extent2D extent;

    double ox = -instance1.GetPixelSpacingX() / 2.0;
    double oy = -instance1.GetPixelSpacingY() / 2.0;
    extent.AddPoint(ox, oy);
    extent.AddPoint(ox + instance1.GetPixelSpacingX() * static_cast<double>(instance1.GetWidth()),
                    oy + instance1.GetPixelSpacingY() * static_cast<double>(instance1.GetHeight()));

    const OrthancStone::CoordinateSystem3D c1 = instance1.GetFrameGeometry(frame1);
  
    OrthancStone::Vector direction, origin;
  
    if (!extent.IsEmpty() &&
        OrthancStone::GeometryToolbox::IntersectTwoPlanes(origin, direction,
                                                          c1.GetOrigin(), c1.GetNormal(),
                                                          plane2.GetOrigin(), plane2.GetNormal()))
    {
      double ax, ay, bx, by;
      c1.ProjectPoint(ax, ay, origin);
      c1.ProjectPoint(bx, by, origin + 100.0 * direction);
    
      return OrthancStone::GeometryToolbox::ClipLineToRectangle(
        x1, y1, x2, y2,
        ax, ay, bx, by,
        extent.GetX1(), extent.GetY1(), extent.GetX2(), extent.GetY2());
    }
    else
    {
      return false;
    }
  }
}



class StoneAnnotationsRegistry : public boost::noncopyable
{
private:
  class Index
  {
  private:
    std::string  sopInstanceUid_;
    size_t       frame_;

  public:
    Index(const std::string& sopInstanceUid,
          size_t frame) :
      sopInstanceUid_(sopInstanceUid),
      frame_(frame)
    {
    }

    const std::string& GetSopInstanceUid() const
    {
      return sopInstanceUid_;
    }

    size_t GetFrame() const
    {
      return frame_;
    }

    bool operator< (const Index& other) const
    {
      if (sopInstanceUid_ < other.sopInstanceUid_)
      {
        return true;
      }
      else if (sopInstanceUid_ > other.sopInstanceUid_)
      {
        return false;
      }
      else
      {
        return frame_ < other.frame_;
      }
    }
  };
  
  typedef std::map<Index, Json::Value*>  Content;

  Content  content_;

  void Clear()
  {
    for (Content::iterator it = content_.begin(); it != content_.end(); ++it)
    {
      assert(it->second != NULL);
      delete it->second;
    }

    content_.clear();
  }

  StoneAnnotationsRegistry()
  {
  }

public:
  ~StoneAnnotationsRegistry()
  {
    Clear();
  }

  static StoneAnnotationsRegistry& GetInstance()
  {
    static StoneAnnotationsRegistry singleton;
    return singleton;
  }
  
  void Save(const std::string& sopInstanceUid,
            size_t frame,
            const OrthancStone::AnnotationsSceneLayer& layer)
  {
    std::unique_ptr<Json::Value> serialized(new Json::Value);
    layer.Serialize(*serialized);

    const Index index(sopInstanceUid, frame);
    
    Content::iterator found = content_.find(index);
    if (found == content_.end())
    {
      content_[index] = serialized.release();
    }
    else
    {
      assert(found->second != NULL);
      delete found->second;
      found->second = serialized.release();
    }
  }

  void Load(OrthancStone::AnnotationsSceneLayer& layer,
            const std::string& sopInstanceUid,
            size_t frame) const
  {
    const Index index(sopInstanceUid, frame);
    
    Content::const_iterator found = content_.find(index);
    if (found == content_.end())
    {
      layer.Clear();
    }
    else
    {
      assert(found->second != NULL);
      layer.Unserialize(*found->second);
    }
  }
};



class OverlaysRegistry : public boost::noncopyable
{
private:
  typedef std::map<std::string, OrthancStone::LookupTableTextureSceneLayer*>  Content;

  Content  content_;

public:
  ~OverlaysRegistry()
  {
    for (Content::iterator it = content_.begin(); it != content_.end(); ++it)
    {
      assert(it->second != NULL);
      delete it->second;
    }
  }

  static OverlaysRegistry& GetInstance()
  {
    static OverlaysRegistry singleton;
    return singleton;
  }

  void Register(const std::string& sopInstanceUid,
                const OrthancStone::DicomInstanceParameters& parameters,
                int overlayX,
                int overlayY,
                const Orthanc::ImageAccessor& overlay)
  {
    // Don't register twice the same overlay
    Content::iterator found = content_.find(sopInstanceUid);
    if (found == content_.end())
    {
      content_[sopInstanceUid] = parameters.CreateOverlayTexture(overlayX, overlayY, overlay);
    }
  }

  class Accessor : public boost::noncopyable
  {
  private:
    const OrthancStone::LookupTableTextureSceneLayer* texture_;

  public:
    Accessor(const OverlaysRegistry& registry,
             const std::string& sopInstanceUid)
    {
      Content::const_iterator found = registry.content_.find(sopInstanceUid);
      if (found == registry.content_.end())
      {
        texture_ = NULL;
      }
      else
      {
        assert(found->second != NULL);
        texture_ = found->second;
      }
    }

    bool IsValid() const
    {
      return texture_ != NULL;
    }

    OrthancStone::LookupTableTextureSceneLayer* CreateTexture() const
    {
      if (texture_ == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
      else
      {
        return dynamic_cast<OrthancStone::LookupTableTextureSceneLayer*>(texture_->Clone());
      }
    }
  };
};



class LayersHolder : public boost::noncopyable
{
private:
  typedef std::map<int, OrthancStone::ISceneLayer*>  Layers;

  Layers layers_;

public:
  ~LayersHolder()
  {
    for (Layers::iterator it = layers_.begin(); it != layers_.end(); ++it)
    {
      if (it->second != NULL)
      {
        delete it->second;
      }
    }
  }

  void AddLayer(int depth,
                OrthancStone::ISceneLayer* layer)
  {
    std::unique_ptr<OrthancStone::ISceneLayer> protection(layer);

    if (layers_.find(depth) != layers_.end())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls, "Two layer sources are writing to the same depth: " +
                                      boost::lexical_cast<std::string>(depth));
    }
    else if (layer == NULL)
    {
      layers_[depth] = NULL;
    }
    else
    {
      layers_[depth] = protection.release();
    }
  }

  void Commit(OrthancStone::Scene2D& scene)
  {
    for (Layers::iterator it = layers_.begin(); it != layers_.end(); ++it)
    {
      if (it->second == NULL)
      {
        scene.DeleteLayer(it->first);
      }
      else
      {
        std::unique_ptr<OrthancStone::ISceneLayer> layer(it->second);
        it->second = NULL;
        scene.SetLayer(it->first, layer.release());
      }
    }
  }
};



class ViewerViewport : public OrthancStone::ObserverBase<ViewerViewport>
{
public:
  class IObserver : public boost::noncopyable
  {
  public:
    virtual ~IObserver()
    {
    }

    virtual void SignalSeriesDetailsReady(const ViewerViewport& viewport) = 0;

    virtual void SignalFrameUpdated(const ViewerViewport& viewport,
                                    size_t currentFrame,
                                    size_t countFrames,
                                    DisplayedFrameQuality quality,
                                    unsigned int instanceNumber,
                                    const std::string& contentDate,
                                    const std::string& contentTime) = 0;

    // "click" is a 3D vector in world coordinates
    virtual void SignalCrosshair(const ViewerViewport& viewport,
                                 const OrthancStone::Vector& click) = 0;

    virtual void SignalSynchronizedBrowsing(const ViewerViewport& viewport,
                                            const OrthancStone::Vector& click,
                                            const OrthancStone::Vector& normal) = 0;

    virtual void SignalWindowingUpdated(const ViewerViewport& viewport,
                                        const OrthancStone::Windowing& windowing) = 0;

    virtual void SignalStoneAnnotationsChanged(const ViewerViewport& viewport,
                                               const std::string& sopInstanceUid,
                                               size_t frame) = 0;

    virtual void SignalStoneAnnotationAdded(const ViewerViewport& viewport) = 0;

    virtual void SignalStoneAnnotationRemoved(const ViewerViewport& viewport) = 0;

    virtual void SignalStoneTextAnnotationRequired(const ViewerViewport& viewport,
                                                   const OrthancStone::ScenePoint2D& pointedPosition,
                                                   const OrthancStone::ScenePoint2D& labelPosition) = 0;
  };

private:
  class ICommand : public Orthanc::IDynamicObject
  {
  private:
    boost::shared_ptr<ViewerViewport>  viewport_;
    
  public:
    explicit ICommand(boost::shared_ptr<ViewerViewport> viewport) :
      viewport_(viewport)
    {
      if (viewport == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }
    }
    
    virtual ~ICommand()
    {
    }

    ViewerViewport& GetViewport() const
    {
      assert(viewport_ != NULL);
      return *viewport_;
    }
    
    virtual void Handle(const OrthancStone::DicomResourcesLoader::SuccessMessage& message) const
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
    }
    
    virtual void Handle(const OrthancStone::HttpCommand::SuccessMessage& message) const
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
    }

    virtual void Handle(const OrthancStone::ParseDicomSuccessMessage& message) const
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
    }
  };

  class LoadSeriesDetailsFromInstance : public ICommand
  {
  public:
    explicit LoadSeriesDetailsFromInstance(boost::shared_ptr<ViewerViewport> viewport) :
      ICommand(viewport)
    {
    }
    
    virtual void Handle(const OrthancStone::DicomResourcesLoader::SuccessMessage& message) const ORTHANC_OVERRIDE
    {
      if (message.GetResources()->GetSize() != 1)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol);
      }

      const Orthanc::DicomMap& dicom = message.GetResources()->GetResource(0);
      
      {
        OrthancStone::DicomInstanceParameters params(dicom);
        
        params.EnrichUsingDicomWeb(message.GetResources()->GetSourceJson(0));

        if (params.HasPixelSpacing())
        {
          GetViewport().centralPixelSpacingX_ = params.GetPixelSpacingX();
          GetViewport().centralPixelSpacingY_ = params.GetPixelSpacingY();
          GetViewport().stoneAnnotations_->SetUnits(OrthancStone::Units_Millimeters);
        }
        else
        {
          GetViewport().centralPixelSpacingX_ = 1;
          GetViewport().centralPixelSpacingY_ = 1;
          GetViewport().stoneAnnotations_->SetUnits(OrthancStone::Units_Pixels);
        }

        if (!OrthancStone::LinearAlgebra::IsNear(params.GetPixelSpacingX(), 0) &&
            !OrthancStone::LinearAlgebra::IsNear(params.GetPixelSpacingY(), 0) &&
            params.GetWidth() != 0 &&
            params.GetHeight() != 0)
        {
          GetViewport().centralPhysicalWidth_ = (params.GetPixelSpacingX() *
                                                 static_cast<double>(params.GetWidth()));
          GetViewport().centralPhysicalHeight_ = (params.GetPixelSpacingY() *
                                                  static_cast<double>(params.GetHeight()));
        }

        GetViewport().windowingPresets_.resize(params.GetWindowingPresetsCount());

        for (size_t i = 0; i < params.GetWindowingPresetsCount(); i++)
        {
          LOG(INFO) << "Preset windowing " << (i + 1) << "/" << params.GetWindowingPresetsCount()
                    << ": " << params.GetWindowingPreset(i).GetCenter()
                    << "," << params.GetWindowingPreset(i).GetWidth();

          GetViewport().windowingPresets_[i] = params.GetWindowingPreset(i);
        }

        GetViewport().SetDefaultWindowing(params);
      }

      uint32_t cineRate;
      if (dicom.ParseUnsignedInteger32(cineRate, Orthanc::DICOM_TAG_CINE_RATE) &&
          cineRate > 0)
      {
        /**
         * If we detect a cine sequence, start on the first frame
         * instead of on the middle frame.
         **/
        GetViewport().cursor_->SetCurrentIndex(0);
        GetViewport().cineRate_ = cineRate;
      }
      else
      {
        GetViewport().cineRate_ = DEFAULT_CINE_RATE;
      }

      GetViewport().Redraw();

      if (GetViewport().observer_.get() != NULL)
      {
        GetViewport().observer_->SignalSeriesDetailsReady(GetViewport());
      }
    }
  };


  class SetLowQualityFrame : public ICommand
  {
  private:
    std::string   sopInstanceUid_;
    unsigned int  frameNumber_;
    OrthancStone::Windowing  windowing_;
    bool          isMonochrome1_;
    bool          isPrefetch_;
    
  public:
    SetLowQualityFrame(boost::shared_ptr<ViewerViewport> viewport,
                       const std::string& sopInstanceUid,
                       unsigned int frameNumber,
                       const OrthancStone::Windowing& windowing,
                       bool isMonochrome1,
                       bool isPrefetch) :
      ICommand(viewport),
      sopInstanceUid_(sopInstanceUid),
      frameNumber_(frameNumber),
      windowing_(windowing),
      isMonochrome1_(isMonochrome1),
      isPrefetch_(isPrefetch)
    {
    }
    
    virtual void Handle(const OrthancStone::HttpCommand::SuccessMessage& message) const ORTHANC_OVERRIDE
    {
      std::unique_ptr<Orthanc::JpegReader> jpeg(new Orthanc::JpegReader);
      jpeg->ReadFromMemory(message.GetAnswer());

      std::unique_ptr<Orthanc::ImageAccessor> converted;
      
      switch (jpeg->GetFormat())
      {
        case Orthanc::PixelFormat_RGB24:
          converted.reset(jpeg.release());
          break;

        case Orthanc::PixelFormat_Grayscale8:
        {
          if (isMonochrome1_)
          {
            Orthanc::ImageProcessing::Invert(*jpeg);
          }

          converted.reset(new Orthanc::Image(Orthanc::PixelFormat_Float32, jpeg->GetWidth(),
                                             jpeg->GetHeight(), false));

          Orthanc::ImageProcessing::Convert(*converted, *jpeg);

          /**

             Orthanc::ImageProcessing::ShiftScale() computes "(x + offset) * scaling".
             The system to solve is thus:           

             (0 + offset) * scaling = windowingCenter - windowingWidth / 2     [a]
             (255 + offset) * scaling = windowingCenter + windowingWidth / 2   [b]

             Resolution:

             [b - a] => 255 * scaling = windowingWidth
             [a] => offset = (windowingCenter - windowingWidth / 2) / scaling

          **/

          const float center = static_cast<float>(windowing_.GetCenter());
          const float width = static_cast<float>(windowing_.GetWidth());
          const float scaling = width / 255.0f;
          const float offset = (OrthancStone::LinearAlgebra::IsCloseToZero(scaling) ? 0 :
                                (center - width / 2.0f) / scaling);

          Orthanc::ImageProcessing::ShiftScale(*converted, offset, scaling, false);
          break;
        }

        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
      }

      assert(converted.get() != NULL);
      GetViewport().RenderCurrentSceneFromCommand(*converted, sopInstanceUid_, frameNumber_, DisplayedFrameQuality_Low);
      GetViewport().framesCache_->Acquire(sopInstanceUid_, frameNumber_, converted.release(), QUALITY_JPEG);

      if (isPrefetch_)
      {
        GetViewport().ScheduleNextPrefetch();
      }
    }
  };


  class SetFullDicomFrame : public ICommand
  {
  private:
    std::string   studyInstanceUid_;
    std::string   seriesInstanceUid_;
    std::string   sopInstanceUid_;
    unsigned int  frameNumber_;
    int           priority_;
    bool          isPrefetch_;
    bool          serverSideTranscoding_;
    
  public:
    SetFullDicomFrame(boost::shared_ptr<ViewerViewport> viewport,
                      const std::string& studyInstanceUid,
                      const std::string& seriesInstanceUid,
                      const std::string& sopInstanceUid,
                      unsigned int frameNumber,
                      int priority,
                      bool isPrefetch,
                      bool serverSideTranscoding) :
      ICommand(viewport),
      studyInstanceUid_(studyInstanceUid),
      seriesInstanceUid_(seriesInstanceUid),
      sopInstanceUid_(sopInstanceUid),
      frameNumber_(frameNumber),
      priority_(priority),
      isPrefetch_(isPrefetch),
      serverSideTranscoding_(serverSideTranscoding)
    {
    }
    
    virtual void Handle(const OrthancStone::ParseDicomSuccessMessage& message) const ORTHANC_OVERRIDE
    {
      std::unique_ptr<Orthanc::ImageAccessor> frame;
      
      try
      {
        frame.reset(message.GetDicom().DecodeFrame(frameNumber_));
      }
      catch (Orthanc::OrthancException& e)
      {
        if (e.GetErrorCode() == Orthanc::ErrorCode_NotImplemented)
        {
          if (!serverSideTranscoding_)
          {
            // If we haven't tried server-side rendering yet, give it a try
            LOG(INFO) << "Switching to server-side transcoding";
            GetViewport().serverSideTranscoding_ = true;
            GetViewport().ScheduleLoadFullDicomFrame(studyInstanceUid_, seriesInstanceUid_, sopInstanceUid_, frameNumber_, priority_, isPrefetch_);
          }
          return;
        }
        else
        {
          throw;
        }
      }

      if (frame.get() == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
      else
      {
        Apply(GetViewport(), message.GetDicom(), frame.release(), sopInstanceUid_, frameNumber_);

        if (isPrefetch_)
        {
          GetViewport().ScheduleNextPrefetch();
        }
      }
    }

    static void Apply(ViewerViewport& viewport,
                      const Orthanc::ParsedDicomFile& dicom,
                      Orthanc::ImageAccessor* f,
                      const std::string& sopInstanceUid,
                      unsigned int frameNumber)
    {
      std::unique_ptr<Orthanc::ImageAccessor> frameProtection(f);
      
      Orthanc::DicomMap tags;
      dicom.ExtractDicomSummary(tags, ORTHANC_STONE_MAX_TAG_LENGTH);

      OrthancStone::DicomInstanceParameters parameters(tags);
      viewport.instancesCache_->Store(sopInstanceUid, parameters);

      std::unique_ptr<Orthanc::ImageAccessor> converted;
      
      if (frameProtection->GetFormat() == Orthanc::PixelFormat_RGB24)
      {
        converted.reset(frameProtection.release());
      }
      else
      {
        converted.reset(new Orthanc::Image(Orthanc::PixelFormat_Float32, frameProtection->GetWidth(), frameProtection->GetHeight(), false));
        Orthanc::ImageProcessing::Convert(*converted, *frameProtection);
        parameters.ApplyRescaleAndDoseScaling(*converted, false /* don't use double */);
      }

      try
      {
        int x, y;
        std::unique_ptr<Orthanc::ImageAccessor> overlay(dicom.DecodeAllOverlays(x, y));

        if (overlay.get() != NULL &&
            overlay->GetWidth() > 0 &&
            overlay->GetHeight() > 0)
        {
          OverlaysRegistry::GetInstance().Register(sopInstanceUid, parameters, x, y, *overlay);
        }
      }
      catch (Orthanc::OrthancException& e)
      {
        LOG(ERROR) << "Cannot decode overlays from instance " << sopInstanceUid;
      }
      
      assert(converted.get() != NULL);
      viewport.RenderCurrentSceneFromCommand(*converted, sopInstanceUid, frameNumber, DisplayedFrameQuality_High);
      viewport.framesCache_->Acquire(sopInstanceUid, frameNumber, converted.release(), QUALITY_FULL);
    }
  };


  class PrefetchItem
  {
  private:
    size_t   cursorIndex_;
    bool     isFullQuality_;

  public:
    PrefetchItem(size_t cursorIndex,
                 bool isFullQuality) :
      cursorIndex_(cursorIndex),
      isFullQuality_(isFullQuality)
    {
    }

    size_t GetCursorIndex() const
    {
      return cursorIndex_;
    }

    bool IsFullQuality() const
    {
      return isFullQuality_;
    }
  };
  

  std::unique_ptr<IObserver>                   observer_;
  OrthancStone::WebAssemblyLoadersContext&               context_;
  boost::shared_ptr<OrthancStone::WebAssemblyViewport>   viewport_;
  boost::shared_ptr<OrthancStone::DicomResourcesLoader> loader_;
  OrthancStone::DicomSource                    source_;
  boost::shared_ptr<FramesCache>               framesCache_;
  boost::shared_ptr<InstancesCache>            instancesCache_;
  std::unique_ptr<IFramesCollection>           frames_;
  std::unique_ptr<SeriesCursor>                cursor_;
  WindowingTracker                             windowingTracker_;
  std::vector<OrthancStone::Windowing>         windowingPresets_;
  unsigned int                                 cineRate_;
  bool                                         inverted_;
  bool                                         fitNextContent_;
  std::list<PrefetchItem>                      prefetchQueue_;
  bool                                         serverSideTranscoding_;
  OrthancStone::Vector                         synchronizationOffset_;
  bool                                         synchronizationEnabled_;
  double                                       centralPhysicalWidth_;   // LSD-479
  double                                       centralPhysicalHeight_;
  double                                       centralPixelSpacingX_;
  double                                       centralPixelSpacingY_;

  bool         hasFocusOnInstance_;
  std::string  focusSopInstanceUid_;
  unsigned int focusFrameNumber_;

  // The coordinates of Stone annotations are expressed in 2D
  // coordinates of the current texture, with (0,0) corresponding to
  // the center of the top-left pixel
  boost::shared_ptr<OrthancStone::AnnotationsSceneLayer>  stoneAnnotations_;


  bool linearInterpolation_;
  std::string pendingSeriesInstanceUid_;


  // WARNING: The ownership is not transferred
  std::list<ILayerSource*>  layerSources_;


  // TODO - Share this with the compositor?
  OrthancStone::FontRenderer  font_;


  void UpdateWindowing(WindowingState state,
                       const OrthancStone::Windowing& windowing)
  {
    if (windowingTracker_.Update(state, windowing))
    {
      UpdateCurrentTextureParameters();

      if (observer_.get() != NULL)
      {
        observer_->SignalWindowingUpdated(*this, windowingTracker_.GetWindowing());
      }
    }
  }


  void SetDefaultWindowing(const OrthancStone::DicomInstanceParameters& instance)
  {
    windowingTracker_.Reset();

    if (instance.GetWindowingPresetsCount() == 0)
    {
      LOG(INFO) << "No preset windowing";
      UpdateWindowing(WindowingState_Fallback, instance.GetFallbackWindowing());
    }
    else
    {
      UpdateWindowing(WindowingState_GlobalPreset, instance.GetWindowingPreset(0));
    }
  }


  void ScheduleNextPrefetch()
  {
    while (!prefetchQueue_.empty())
    {
      size_t cursorIndex = prefetchQueue_.front().GetCursorIndex();
      bool isFullQuality = prefetchQueue_.front().IsFullQuality();
      prefetchQueue_.pop_front();
      
      const std::string sopInstanceUid = frames_->GetInstanceOfFrame(cursorIndex).GetSopInstanceUid();
      unsigned int frameNumber = frames_->GetFrameNumberInInstance(cursorIndex);

      {
        FramesCache::Accessor accessor(*framesCache_, sopInstanceUid, frameNumber);
        if (!accessor.IsValid() ||
            (isFullQuality && accessor.GetQuality() == 0))
        {
          if (isFullQuality)
          {
            ScheduleLoadFullDicomFrame(cursorIndex, PRIORITY_NORMAL, true);
          }
          else
          {
            ScheduleLoadRenderedFrame(cursorIndex, PRIORITY_NORMAL, true);
          }
          
          return;  // We have found a new frame to cache, stop the lookup loop
        }
      }
    }
  }
  
  
  void ClearViewport()
  {
    {
      std::unique_ptr<OrthancStone::IViewport::ILock> lock(viewport_->Lock());
      lock->GetController().GetScene().DeleteLayer(LAYER_TEXTURE);
      //lock->GetCompositor().Refresh(lock->GetController().GetScene());
      lock->Invalidate();
    }
  }


  /**
   * NB: "frame" is only used to estimate the memory size to store 1
   * frame, in order to avoid prefetching too much data.
   **/
  void SetupPrefetchAfterRendering(const Orthanc::ImageAccessor& frame,
                                   DisplayedFrameQuality quality)
  {
    const size_t cursorIndex = cursor_->GetCurrentIndex();

    // Prepare prefetching
    prefetchQueue_.clear();

    if (1)  // DISABLE PREFETCHING
    {
      const size_t frameSize = frame.GetPitch() * frame.GetHeight();
      size_t prefetchedSize = 0;
    
      for (size_t i = 0; i < cursor_->GetPrefetchSize() && i < 16 &&
             prefetchedSize <= framesCache_->GetMaximumSize() / 2; i++)
      {
        size_t a = cursor_->GetPrefetchIndex(i);
        if (a != cursorIndex)
        {
          prefetchQueue_.push_back(PrefetchItem(a, i < 2));
          prefetchedSize += frameSize;
        }
      }
    }

    ScheduleNextPrefetch();
    
    if (frames_.get() != NULL &&
        cursor_.get() != NULL &&
        observer_.get() != NULL)
    {
      const Orthanc::DicomMap& instance = frames_->GetInstanceOfFrame(cursor_->GetCurrentIndex()).GetTags();

      uint32_t instanceNumber = 0;
      std::string contentDate;
      std::string contentTime;

      instance.ParseUnsignedInteger32(instanceNumber, Orthanc::DICOM_TAG_INSTANCE_NUMBER);
      instance.LookupStringValue(contentDate, Orthanc::DicomTag(0x0008, 0x0023), false);
      instance.LookupStringValue(contentTime, Orthanc::DicomTag(0x0008, 0x0033), false);

      observer_->SignalFrameUpdated(*this, cursorIndex, frames_->GetFramesCount(), quality, instanceNumber, contentDate, contentTime);
    }
  }
  
  
  void RenderCurrentScene(const Orthanc::ImageAccessor& frame,
                          const OrthancStone::DicomInstanceParameters& instance,
                          size_t frameIndex,
                          const OrthancStone::CoordinateSystem3D& plane)
  {
    /**
     * IMPORTANT - DO NOT use "instance.GetWidth()" and
     * "instance.GetHeight()" in this method. Use the information from
     * "frame" instead. Indeed, the "instance" information is taken
     * from DICOMweb "/studies/.../series/.../metadata". But,
     * "SeriesMetadataExtrapolatedTags" includes the "Columns" and
     * "Rows" DICOM tags for performance, which make this information
     * unreliable if the series includes instances with varying sizes
     * (cf. LSD-479).
     **/
    
    bool isMonochrome1 = false;
    if (!OrthancStone::IsStructuredReport(instance.GetSopClassUid()))
    {
      isMonochrome1 = (instance.GetImageInformation().GetPhotometricInterpretation() ==
                       Orthanc::PhotometricInterpretation_Monochrome1);
    }
      
    std::unique_ptr<OrthancStone::TextureBaseSceneLayer> layer;

    switch (frame.GetFormat())
    {
      case Orthanc::PixelFormat_RGB24:
        layer.reset(new OrthancStone::ColorTextureSceneLayer(frame));
        break;

      case Orthanc::PixelFormat_Float32:
      {
        {
          // New in Stone Web viewer 2.2: Deal with Philips multiframe
          // (cf. mail from Tomas Kenda on 2021-08-17)
          InstancesCache::Accessor accessor(*instancesCache_, instance.GetSopInstanceUid());
          OrthancStone::Windowing windowing;
          if (accessor.IsValid())
          {
            if (accessor.GetParameters().LookupPerFrameWindowing(windowing, frameIndex))
            {
              UpdateWindowing(WindowingState_FramePreset, windowing);
            }
            else if (accessor.GetParameters().GetWindowingPresetsCount() > 0)
            {
              UpdateWindowing(WindowingState_FramePreset, accessor.GetParameters().GetWindowingPreset(0));
            }
          }
        }

        std::unique_ptr<OrthancStone::FloatTextureSceneLayer> tmp(
          new OrthancStone::FloatTextureSceneLayer(frame));

        if (windowingTracker_.GetState() == WindowingState_None ||
            windowingTracker_.GetState() == WindowingState_Fallback)
        {
          const Orthanc::ImageAccessor& texture = tmp->GetTexture();
          if (texture.GetFormat() != Orthanc::PixelFormat_Float32)
          {
            throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
          }
          else
          {
            float minValue, maxValue;
            Orthanc::ImageProcessing::GetMinMaxFloatValue(minValue, maxValue, texture);

            const float center = (minValue + maxValue) / 2.0f;
            const float width = maxValue - minValue;
            UpdateWindowing(WindowingState_Fallback, OrthancStone::Windowing(center, width));
          }
        }

        tmp->SetCustomWindowing(windowingTracker_.GetWindowing().GetCenter(), windowingTracker_.GetWindowing().GetWidth());
        tmp->SetInverted(inverted_ ^ isMonochrome1);
        layer.reset(tmp.release());
        break;
      }

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageFormat);
    }

    assert(layer.get() != NULL);

    layer->SetLinearInterpolation(linearInterpolation_);

    double pixelSpacingX, pixelSpacingY;

    if (instance.HasPixelSpacing())
    {
      pixelSpacingX = instance.GetPixelSpacingX();
      pixelSpacingY = instance.GetPixelSpacingY();
    }
    else
    {
      pixelSpacingX = centralPixelSpacingX_;
      pixelSpacingY = centralPixelSpacingY_;
    }

    // This hack has been introduced to fix an issue where the first US image of a series had negative PhysicalDeltaX value 
    // while the other images from the series had different geometries.  layer->SetOrigin was failing later.
    // At the end, it appears that the measures are corrects although we have set the pixelSpacing to 1m so the values are likely
    // overwritten later on...
    if (pixelSpacingX < 0 || pixelSpacingY < 0)
    {
      LOG(ERROR) << "Pixel spacings are invalid: " << pixelSpacingX << " " << pixelSpacingY << " setting dummy values to 1000, 1000 -> measures might be invalid !";
      pixelSpacingX = 1000;
      pixelSpacingY = 1000;
    }

    if (FIX_LSD_479)
    {
      /**
       * Some series contain a first instance (secondary capture) that
       * is completely different from others wrt. to resolution and
       * pixel spacing. We make sure to rescale each frame to fit in a
       * square that corresponds to the extent of the frame in the
       * middle of the series.
       **/
      double physicalWidth = pixelSpacingX * static_cast<double>(frame.GetWidth()); 
      double physicalHeight = pixelSpacingY * static_cast<double>(frame.GetHeight());

      if (OrthancStone::LinearAlgebra::IsCloseToZero(physicalWidth) ||
          OrthancStone::LinearAlgebra::IsCloseToZero(physicalHeight))
      {
        // Numerical instability, don't try further processing
        layer->SetPixelSpacing(pixelSpacingX, pixelSpacingY);
      }
      else
      {
        double scale = std::max(centralPhysicalWidth_ / physicalWidth,
                                centralPhysicalHeight_ / physicalHeight);
        layer->SetPixelSpacing(pixelSpacingX * scale, pixelSpacingY * scale);
        layer->SetOrigin((centralPhysicalWidth_ - physicalWidth * scale) / 2.0,
                         (centralPhysicalHeight_ - physicalHeight * scale) / 2.0);
      }
    }
    else
    {
      layer->SetPixelSpacing(pixelSpacingX, pixelSpacingY);
    }

    StoneAnnotationsRegistry::GetInstance().Load(*stoneAnnotations_, instance.GetSopInstanceUid(), frameIndex);

    LayersHolder holder;

    for (std::list<ILayerSource*>::const_iterator it = layerSources_.begin(); it != layerSources_.end(); ++it)
    {
      assert(*it != NULL);
      holder.AddLayer((*it)->GetDepth(), (*it)->Create(frame, instance, frameIndex,
                                                       layer->GetPixelSpacingX(), layer->GetPixelSpacingY(), plane));
    }

    std::unique_ptr<OrthancStone::ISceneLayer> structuredReportAnnotations;

    if (frames_.get() != NULL &&
        frames_->GetType() == FramesCollectionType_DicomSR)
    {
      const DicomStructuredReportFrames& sr = dynamic_cast<const DicomStructuredReportFrames&>(*frames_);
      holder.AddLayer(LAYER_STRUCTURED_REPORT, sr.ExtractAnnotations(instance.GetSopInstanceUid(), frameIndex,
                                                                     layer->GetOriginX(), layer->GetOriginY(),
                                                                     layer->GetPixelSpacingX(), layer->GetPixelSpacingY()));
    }
    else
    {
      holder.AddLayer(LAYER_STRUCTURED_REPORT, NULL);
    }

    holder.AddLayer(LAYER_TEXTURE, layer.release());

    {
      std::unique_ptr<OrthancStone::IViewport::ILock> lock(viewport_->Lock());

      OrthancStone::Scene2D& scene = lock->GetController().GetScene();

      holder.Commit(scene);

      stoneAnnotations_->Render(scene);  // Necessary for "FitContent()" to work

      if (fitNextContent_)
      {
        lock->RefreshCanvasSize();

        if (OrthancStone::IsStructuredReport(instance.GetSopClassUid()))
        {
          // Fit to the width of the textual report
          scene.FitTopContent(lock->GetCompositor().GetCanvasWidth(), lock->GetCompositor().GetCanvasHeight(), 0.1);
        }
        else
        {
          lock->GetCompositor().FitContent(scene);
        }

        stoneAnnotations_->Render(scene);
        fitNextContent_ = false;
      }
        
      //lock->GetCompositor().Refresh(scene);
      lock->Invalidate();
    }
  }


  void RenderCurrentSceneFromCommand(const Orthanc::ImageAccessor& frame,
                                     const std::string& loadedSopInstanceUid,
                                     unsigned int loadedFrameNumber,
                                     DisplayedFrameQuality quality)
  {
    if (cursor_.get() != NULL &&
        frames_.get() != NULL)
    {
      const size_t cursorIndex = cursor_->GetCurrentIndex();
      const OrthancStone::DicomInstanceParameters& instance = frames_->GetInstanceOfFrame(cursorIndex);
      const unsigned int frameNumber = frames_->GetFrameNumberInInstance(cursorIndex);

      // Only change the scene if the loaded frame still corresponds to the current cursor
      if (instance.GetSopInstanceUid() == loadedSopInstanceUid &&
          frameNumber == loadedFrameNumber)
      {
        const OrthancStone::CoordinateSystem3D plane = IFramesCollection::GetFrameGeometry(*frames_, cursorIndex);
        
        if (quality == DisplayedFrameQuality_Low)
        {
          FramesCache::Accessor accessor(*framesCache_, instance.GetSopInstanceUid(), frameNumber);
          if (accessor.IsValid() &&
              accessor.GetQuality() == QUALITY_FULL)
          {
            // A high-res image was downloaded in between: Use this cached image instead of the low-res
            RenderCurrentScene(accessor.GetImage(), instance, frameNumber, plane);
            SetupPrefetchAfterRendering(frame, DisplayedFrameQuality_High);
          }
          else
          {
            // This frame is only available in low-res: Download the full DICOM
            RenderCurrentScene(frame, instance, frameNumber, plane);
            SetupPrefetchAfterRendering(frame, quality);

            /**
             * The command "SetupPrefetchAfterRendering()" must be
             * after "SetupPrefetchAfterRendering(quality)", as the
             * DICOM instance might already be cached by the oracle,
             * which makes a call to "observer_->SignalFrameUpdated()"
             * with a low quality, whereas the high quality is
             * available.
             **/
            ScheduleLoadFullDicomFrame(cursorIndex, PRIORITY_HIGH, false /* not a prefetch */);
          }
        }
        else
        {
          assert(quality == DisplayedFrameQuality_High);
          SetupPrefetchAfterRendering(frame, quality);
          RenderCurrentScene(frame, instance, frameNumber, plane);
        }
      }
    }
  }

  void ScheduleLoadFullDicomFrame(const std::string& studyInstanceUid,
                                  const std::string& seriesInstanceUid,
                                  const std::string& sopInstanceUid,
                                  unsigned int frameNumber,
                                  int priority,
                                  bool isPrefetch)
  {
    if (frames_.get() != NULL)
    {
      std::unique_ptr<OrthancStone::ILoadersContext::ILock> lock(context_.Lock());
      lock->Schedule(
        GetSharedObserver(), priority, OrthancStone::ParseDicomFromWadoCommand::Create(
          source_, studyInstanceUid, seriesInstanceUid, sopInstanceUid, serverSideTranscoding_,
          Orthanc::DicomTransferSyntax_LittleEndianExplicit,
          new SetFullDicomFrame(GetSharedObserver(), studyInstanceUid, seriesInstanceUid,
                                sopInstanceUid, frameNumber, priority, isPrefetch, serverSideTranscoding_)));
    }
  }

  void ScheduleLoadFullDicomFrame(size_t cursorIndex,
                                  int priority,
                                  bool isPrefetch)
  {
    if (frames_.get() != NULL)
    {
      std::string studyInstanceUid = frames_->GetInstanceOfFrame(cursorIndex).GetStudyInstanceUid();
      std::string seriesInstanceUid = frames_->GetInstanceOfFrame(cursorIndex).GetSeriesInstanceUid();
      std::string sopInstanceUid = frames_->GetInstanceOfFrame(cursorIndex).GetSopInstanceUid();
      unsigned int frameNumber = frames_->GetFrameNumberInInstance(cursorIndex);
      ScheduleLoadFullDicomFrame(studyInstanceUid, seriesInstanceUid, sopInstanceUid, frameNumber, priority, isPrefetch);
    }
  }

  void ScheduleLoadRenderedFrame(size_t cursorIndex,
                                 int priority,
                                 bool isPrefetch)
  {
    if (!source_.HasDicomWebRendered())
    {
      ScheduleLoadFullDicomFrame(cursorIndex, priority, isPrefetch);
    }
    else if (frames_.get() != NULL)
    {
      const OrthancStone::DicomInstanceParameters& instance = frames_->GetInstanceOfFrame(cursorIndex);
      unsigned int frameNumber = frames_->GetFrameNumberInInstance(cursorIndex);

      /**
       * If the full-resolution DICOM file is already available in the
       * cache of the oracle, bypass the loading of the "rendered" and
       * use the cached DICOM file.
       **/
      std::unique_ptr<OrthancStone::WebAssemblyOracle::CachedInstanceAccessor> accessor(
        context_.AccessCachedInstance(instance.GetSopInstanceUid()));

      if (accessor.get() != NULL &&
          accessor->IsValid())
      {
        try
        {
          std::unique_ptr<Orthanc::ImageAccessor> frame;
          if (OrthancStone::IsStructuredReport(instance.GetSopClassUid()))
          {
            OrthancStone::DicomStructuredReport report(const_cast<Orthanc::ParsedDicomFile&>(accessor->GetDicom()));
            frame.reset(report.Render(font_, GetHighlightedColorInternal(), GetAnnotationsColorInternal()));
          }
          else
          {
            frame.reset(accessor->GetDicom().DecodeFrame(frameNumber));
          }

          SetFullDicomFrame::Apply(*this, accessor->GetDicom(), frame.release(), instance.GetSopInstanceUid(), frameNumber);
          return;  // Success
        }
        catch (Orthanc::OrthancException&)
        {
          /**
           * This happens if the cached DICOM file uses a transfer
           * syntax that is not supported by DCMTK (such as
           * JPEG2k). Fallback to "/rendered" in order to re-download
           * the DICOM file using server-side transcoding. This
           * happens on WRIX dataset.
           **/
        }
      }

      bool isMonochrome1 = (instance.GetImageInformation().GetPhotometricInterpretation() ==
                            Orthanc::PhotometricInterpretation_Monochrome1);

      const std::string uri = ("studies/" + instance.GetStudyInstanceUid() +
                               "/series/" + instance.GetSeriesInstanceUid() +
                               "/instances/" + instance.GetSopInstanceUid() +
                               "/frames/" + boost::lexical_cast<std::string>(frameNumber + 1) + "/rendered");

      std::map<std::string, std::string> headers, arguments;
      // arguments["quality"] = "10";   // Low-level quality for test purpose
      arguments["window"] = (
        boost::lexical_cast<std::string>(windowingTracker_.GetWindowing().GetCenter()) + ","  +
        boost::lexical_cast<std::string>(windowingTracker_.GetWindowing().GetWidth()) + ",linear");

      std::unique_ptr<OrthancStone::IOracleCommand> command(
        source_.CreateDicomWebCommand(
          uri, arguments, headers, new SetLowQualityFrame(
            GetSharedObserver(), instance.GetSopInstanceUid(), frameNumber,
            windowingTracker_.GetWindowing(), isMonochrome1, isPrefetch)));

      {
        std::unique_ptr<OrthancStone::ILoadersContext::ILock> lock(context_.Lock());
        lock->Schedule(GetSharedObserver(), priority, command.release());
      }
    }
  }

  void UpdateCurrentTextureParameters()
  {
    std::unique_ptr<OrthancStone::IViewport::ILock> lock(viewport_->Lock());

    if (lock->GetController().GetScene().HasLayer(LAYER_TEXTURE))
    {
      if (lock->GetController().GetScene().GetLayer(LAYER_TEXTURE).GetType() ==
          OrthancStone::ISceneLayer::Type_FloatTexture)
      {
        dynamic_cast<OrthancStone::FloatTextureSceneLayer&>(
          lock->GetController().GetScene().GetLayer(LAYER_TEXTURE)).
          SetCustomWindowing(windowingTracker_.GetWindowing().GetCenter(), windowingTracker_.GetWindowing().GetWidth());
      }
        
      lock->Invalidate();
    }
  }
  
  ViewerViewport(OrthancStone::WebAssemblyLoadersContext& context,
                 const OrthancStone::DicomSource& source,
                 const std::string& canvas,
                 boost::shared_ptr<FramesCache> cache,
                 boost::shared_ptr<InstancesCache> instancesCache,
                 bool softwareRendering,
                 bool linearInterpolation) :
    context_(context),
    source_(source),
    framesCache_(cache),
    instancesCache_(instancesCache),
    fitNextContent_(true),
    hasFocusOnInstance_(false),
    focusFrameNumber_(0),
    synchronizationOffset_(OrthancStone::LinearAlgebra::CreateVector(0, 0, 0)),
    synchronizationEnabled_(false),
    centralPhysicalWidth_(1),
    centralPhysicalHeight_(1),
    centralPixelSpacingX_(1),
    centralPixelSpacingY_(1),
    linearInterpolation_(linearInterpolation)
  {
    if (!framesCache_)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }

    if (softwareRendering)
    {
      LOG(INFO) << "Creating Cairo viewport in canvas: " << canvas;
      viewport_ = OrthancStone::WebAssemblyCairoViewport::Create(canvas);
    }
    else
    {
      LOG(INFO) << "Creating WebGL viewport in canvas: " << canvas;
      viewport_ = OrthancStone::WebGLViewport::Create(canvas);
    }

    {
      std::unique_ptr<OrthancStone::IViewport::ILock> lock(viewport_->Lock());
      std::string ttf;
      Orthanc::EmbeddedResources::GetFileResource(ttf, Orthanc::EmbeddedResources::UBUNTU_FONT);
      lock->GetCompositor().SetFont(0, ttf, 16 /* font size */, Orthanc::Encoding_Latin1);
      font_.LoadFont(ttf, 16 /* font size */);
    }
    
    emscripten_set_wheel_callback(viewport_->GetCanvasCssSelector().c_str(), this, true, OnWheel);

    stoneAnnotations_.reset(new OrthancStone::AnnotationsSceneLayer(LAYER_ANNOTATIONS_STONE));
    stoneAnnotations_->SetProbedLayer(LAYER_TEXTURE);
    stoneAnnotations_->SetColor(GetAnnotationsColorInternal());
    stoneAnnotations_->SetHoverColor(GetHighlightedColorInternal());
  }


  void Handle(const OrthancStone::ViewportController::GrayscaleWindowingChanged& message)
  {
    // This event is triggered by the windowing mouse action, from class "GrayscaleWindowingSceneTracker"
    UpdateWindowing(WindowingState_User, message.GetWindowing());
  }

  
  static EM_BOOL OnWheel(int eventType,
                         const EmscriptenWheelEvent *wheelEvent,
                         void *userData)
  {
    ViewerViewport& that = *reinterpret_cast<ViewerViewport*>(userData);

    if (that.frames_.get() != NULL &&
        that.cursor_.get() != NULL)
    {
      const bool isCtrl = wheelEvent->mouse.ctrlKey;
      const bool isShift = wheelEvent->mouse.shiftKey;
      
      const size_t previousCursorIndex = that.cursor_->GetCurrentIndex();
      
      if (wheelEvent->deltaY < 0)
      {
        that.ChangeFrame(isCtrl ? SeriesCursor::Action_FastMinus :
                         SeriesCursor::Action_Minus, false /* not circular */);
      }
      else if (wheelEvent->deltaY > 0)
      {
        that.ChangeFrame(isCtrl ? SeriesCursor::Action_FastPlus :
                         SeriesCursor::Action_Plus, false /* not circular */);
      }

      if (that.synchronizationEnabled_)
      {
        const size_t currentCursorIndex = that.cursor_->GetCurrentIndex();

        const OrthancStone::CoordinateSystem3D current = IFramesCollection::GetFrameGeometry(*that.frames_, currentCursorIndex);
      
        if (isShift &&
            previousCursorIndex != currentCursorIndex)
        {
          const OrthancStone::CoordinateSystem3D previous = IFramesCollection::GetFrameGeometry(*that.frames_, previousCursorIndex);
          that.synchronizationOffset_ += previous.GetOrigin() - current.GetOrigin();
        }

        that.observer_->SignalSynchronizedBrowsing(
          that, current.GetOrigin() + that.synchronizationOffset_, current.GetNormal());
      }
    }
    
    return true;
  }

  void Handle(const OrthancStone::DicomResourcesLoader::SuccessMessage& message)
  {
    dynamic_cast<const ICommand&>(message.GetUserPayload()).Handle(message);
  }

  void Handle(const OrthancStone::HttpCommand::SuccessMessage& message)
  {
    dynamic_cast<const ICommand&>(message.GetOrigin().GetPayload()).Handle(message);
  }

  void Handle(const OrthancStone::ParseDicomSuccessMessage& message)
  {
    dynamic_cast<const ICommand&>(message.GetOrigin().GetPayload()).Handle(message);
  }


  void RefreshAnnotations(bool save)
  {
    {
      std::unique_ptr<OrthancStone::IViewport::ILock> lock(viewport_->Lock());
      stoneAnnotations_->Render(lock->GetController().GetScene());
      lock->Invalidate();
    }

    if (save)
    {
      if (cursor_.get() != NULL &&
          frames_.get() != NULL)
      {
        const size_t cursorIndex = cursor_->GetCurrentIndex();
        const OrthancStone::DicomInstanceParameters& instance = frames_->GetInstanceOfFrame(cursorIndex);
        const unsigned int frameNumber = frames_->GetFrameNumberInInstance(cursorIndex);

        StoneAnnotationsRegistry::GetInstance().Save(instance.GetSopInstanceUid(), frameNumber, *stoneAnnotations_);

        if (observer_.get() != NULL)
        {
          observer_->SignalStoneAnnotationsChanged(*this, instance.GetSopInstanceUid(), frameNumber);
        }
      }
    }
  }
  
  void Handle(const OrthancStone::ViewportController::SceneTransformChanged& message)
  {
    RefreshAnnotations(false /* don't save */);
  }

  void Handle(const OrthancStone::AnnotationsSceneLayer::AnnotationChangedMessage& message)
  {
    RefreshAnnotations(true /* save */);
  }

  void Handle(const OrthancStone::AnnotationsSceneLayer::AnnotationAddedMessage& message)
  {
    RefreshAnnotations(true /* save */);

    if (observer_.get() != NULL)
    {
      observer_->SignalStoneAnnotationAdded(*this);
    }
  }

  void Handle(const OrthancStone::AnnotationsSceneLayer::AnnotationRemovedMessage& message)
  {
    RefreshAnnotations(true /* save */);

    if (observer_.get() != NULL)
    {
      observer_->SignalStoneAnnotationRemoved(*this);
    }
  }

  void Handle(const OrthancStone::AnnotationsSceneLayer::TextAnnotationRequiredMessage& message)
  {
    if (observer_.get() != NULL)
    {
      observer_->SignalStoneTextAnnotationRequired(*this, message.GetPointedPosition(), message.GetLabelPosition());
    }
  }

  static OrthancStone::Color& GetAnnotationsColorInternal()
  {
    static OrthancStone::Color color_;
    return color_;
  }

  static OrthancStone::Color& GetHighlightedColorInternal()
  {
    static OrthancStone::Color color_;
    return color_;
  }

public:
  virtual ~ViewerViewport()
  {
    // Unregister the callbacks to avoid any call with a "void*" that
    // has been destroyed. "WebAssemblyViewport::CreateObjectCookie()"
    // provides a more advanced alternative.
    emscripten_set_wheel_callback(viewport_->GetCanvasCssSelector().c_str(), this, true, NULL);
  }

  static boost::shared_ptr<ViewerViewport> Create(OrthancStone::WebAssemblyLoadersContext& context,
                                                  const OrthancStone::DicomSource& source,
                                                  const std::string& canvas,
                                                  boost::shared_ptr<FramesCache> framesCache,
                                                  boost::shared_ptr<InstancesCache> instancesCache,
                                                  bool softwareRendering,
                                                  bool linearInterpolation)
  {
    boost::shared_ptr<ViewerViewport> viewport(
      new ViewerViewport(context, source, canvas, framesCache, instancesCache, softwareRendering, linearInterpolation));

    {
      std::unique_ptr<OrthancStone::ILoadersContext::ILock> lock(context.Lock());
    
      viewport->loader_ = OrthancStone::DicomResourcesLoader::Create(*lock);
      viewport->Register<OrthancStone::DicomResourcesLoader::SuccessMessage>(
        *viewport->loader_, &ViewerViewport::Handle);

      viewport->Register<OrthancStone::HttpCommand::SuccessMessage>(
        lock->GetOracleObservable(), &ViewerViewport::Handle);

      viewport->Register<OrthancStone::ParseDicomSuccessMessage>(
        lock->GetOracleObservable(), &ViewerViewport::Handle);

      viewport->Register<OrthancStone::AnnotationsSceneLayer::AnnotationChangedMessage>(
        *viewport->stoneAnnotations_, &ViewerViewport::Handle);

      viewport->Register<OrthancStone::AnnotationsSceneLayer::AnnotationAddedMessage>(
        *viewport->stoneAnnotations_, &ViewerViewport::Handle);

      viewport->Register<OrthancStone::AnnotationsSceneLayer::AnnotationRemovedMessage>(
        *viewport->stoneAnnotations_, &ViewerViewport::Handle);

      viewport->Register<OrthancStone::AnnotationsSceneLayer::TextAnnotationRequiredMessage>(
        *viewport->stoneAnnotations_, &ViewerViewport::Handle);
    }

    {
      std::unique_ptr<OrthancStone::IViewport::ILock> lock(viewport->viewport_->Lock());
      viewport->Register<OrthancStone::ViewportController::GrayscaleWindowingChanged>(lock->GetController(), &ViewerViewport::Handle);
      viewport->Register<OrthancStone::ViewportController::SceneTransformChanged>(lock->GetController(), &ViewerViewport::Handle);
    }

    return viewport;    
  }


  void AddLayerSource(ILayerSource& source)  // Ownership is not transfered
  {
    layerSources_.push_back(&source);
  }


  void SetFrames(IFramesCollection* frames)
  {
    if (frames == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }

    fitNextContent_ = true;
    cineRate_ = DEFAULT_CINE_RATE;
    inverted_ = false;
    serverSideTranscoding_ = false;
    OrthancStone::LinearAlgebra::AssignVector(synchronizationOffset_, 0, 0, 0);

    frames_.reset(frames);
    cursor_.reset(new SeriesCursor(frames_->GetFramesCount(), false));

    if (frames_->GetFramesCount() != 0)
    {
      const OrthancStone::DicomInstanceParameters& firstInstance = frames_->GetInstanceOfFrame(0);
      std::string modality;
      if (firstInstance.GetTags().LookupStringValue(modality, Orthanc::DICOM_TAG_MODALITY, false))
      {
        if (modality == "MR" ||
            modality == "CT" ||
            modality == "NM" ||
            modality == "OPT" ||
            modality == "PT" ||
            modality == "RTDOSE" ||
            modality == "XA")
        {
          // For series that might correspond to 3D images, use their
          // central frame as the first frame to be displayed
          cursor_.reset(new SeriesCursor(frames_->GetFramesCount(), true));
        }
      }

      SetDefaultWindowing(firstInstance);
    }
    else
    {
      windowingTracker_.Reset();
    }

    LOG(INFO) << "Number of frames in series: " << frames_->GetFramesCount();

    ClearViewport();
    prefetchQueue_.clear();

    if (observer_.get() != NULL)
    {
      observer_->SignalFrameUpdated(*this, cursor_->GetCurrentIndex(),
                                    frames_->GetFramesCount(), DisplayedFrameQuality_None, 0, "", "");
    }

    centralPhysicalWidth_ = 1;
    centralPhysicalHeight_ = 1;

    if (frames_->GetFramesCount() != 0)
    {
      const OrthancStone::DicomInstanceParameters& centralInstance = frames_->GetInstanceOfFrame(cursor_->GetCurrentIndex());

      /**
       * Avoid loading metadata if we know that this cannot be a
       * "true" image with pixel data. Retrieving instance metadata on
       * RTSTRUCT can lead to very large JSON whose parsing will
       * freeze the browser for several seconds.
       **/
      const OrthancStone::SopClassUid uid = centralInstance.GetSopClassUid();
      if (uid != OrthancStone::SopClassUid_EncapsulatedPdf &&
          uid != OrthancStone::SopClassUid_RTDose &&
          uid != OrthancStone::SopClassUid_RTPlan &&
          uid != OrthancStone::SopClassUid_RTStruct &&
          GetSeriesThumbnailType(uid) != OrthancStone::SeriesThumbnailType_Video)
      {
        // Fetch the details of the series from the central instance
        const std::string uri = ("studies/" + centralInstance.GetStudyInstanceUid() +
                                 "/series/" + centralInstance.GetSeriesInstanceUid() +
                                 "/instances/" + centralInstance.GetSopInstanceUid() + "/metadata");
        
        loader_->ScheduleGetDicomWeb(
          boost::make_shared<OrthancStone::LoadedDicomResources>(Orthanc::DICOM_TAG_SOP_INSTANCE_UID),
          0, source_, uri, new LoadSeriesDetailsFromInstance(GetSharedObserver()));
      }
    }

    ApplyScheduledFocus();
  }


  // This method is used when the layout of the HTML page changes,
  // which does not trigger the "emscripten_set_resize_callback()"
  void UpdateSize(bool fitContent)
  {
    std::unique_ptr<OrthancStone::IViewport::ILock> lock(viewport_->Lock());
    lock->RefreshCanvasSize();

    if (fitContent)
    {
      lock->GetCompositor().FitContent(lock->GetController().GetScene());
    }

    stoneAnnotations_->ClearHover();
    stoneAnnotations_->Render(lock->GetController().GetScene());
    
    lock->Invalidate();
  }

  void AcquireObserver(IObserver* observer)
  {  
    observer_.reset(observer);
  }

  const std::string& GetCanvasId() const
  {
    assert(viewport_);
    return viewport_->GetCanvasId();
  }


  void Redraw()
  {
    if (cursor_.get() != NULL &&
        frames_.get() != NULL)
    {
      const size_t cursorIndex = cursor_->GetCurrentIndex();

      const OrthancStone::DicomInstanceParameters& instance = frames_->GetInstanceOfFrame(cursorIndex);
      const unsigned int frameNumber = frames_->GetFrameNumberInInstance(cursorIndex);

      FramesCache::Accessor accessor(*framesCache_, instance.GetSopInstanceUid(), frameNumber);
      if (accessor.IsValid())
      {
        RenderCurrentScene(accessor.GetImage(), instance, frameNumber, IFramesCollection::GetFrameGeometry(*frames_, cursorIndex));

        DisplayedFrameQuality quality;
        
        if (accessor.GetQuality() < QUALITY_FULL)
        {
          // This frame is only available in low-res: Download the full DICOM
          ScheduleLoadFullDicomFrame(cursorIndex, PRIORITY_HIGH, false /* not a prefetch */);
          quality = DisplayedFrameQuality_Low;
        }
        else
        {
          quality = DisplayedFrameQuality_High;
        }

        SetupPrefetchAfterRendering(accessor.GetImage(), quality);
      }
      else
      {
        // This frame is not cached yet: Load it
        if (source_.HasDicomWebRendered())
        {
          ScheduleLoadRenderedFrame(cursorIndex, PRIORITY_HIGH, false /* not a prefetch */);
        }
        else
        {
          ScheduleLoadFullDicomFrame(cursorIndex, PRIORITY_HIGH, false /* not a prefetch */);
        }
      }
    }
  }


  // Returns "true" iff the frame has indeed changed
  bool ChangeFrame(SeriesCursor::Action action,
                   bool isCircular)
  {
    if (cursor_.get() != NULL)
    {
      size_t previous = cursor_->GetCurrentIndex();
      
      cursor_->Apply(action, isCircular);
      
      size_t current = cursor_->GetCurrentIndex();
      if (previous != current)
      {
        Redraw();
        return true;
      }
    }

    return false;
  }


  void SetFrame(unsigned int frameNumber)
  {
    if (cursor_.get() != NULL &&
        frameNumber < cursor_->GetFramesCount() &&
        frameNumber != cursor_->GetCurrentIndex())
    {
      cursor_->SetCurrentIndex(frameNumber);
      Redraw();
    }
  }


  void GoToFirstFrame()
  {
    if (cursor_.get() != NULL &&
        cursor_->GetCurrentIndex() != 0)
    {
      cursor_->SetCurrentIndex(0);
      Redraw();
    }
  }


  void GoToLastFrame()
  {
    if (cursor_.get() != NULL)
    {
      size_t last = cursor_->GetFramesCount() - 1;
      if (cursor_->GetCurrentIndex() != last)
      {
        cursor_->SetCurrentIndex(last);
        Redraw();
      }
    }
  }
  

  bool GetCurrentFrameOfReferenceUid(std::string& frameOfReferenceUid) const
  {
    if (cursor_.get() != NULL &&
        frames_.get() != NULL)
    {
      frameOfReferenceUid = frames_->GetInstanceOfFrame(cursor_->GetCurrentIndex()).GetFrameOfReferenceUid();
      return true;
    }
    else
    {
      return false;
    }
  }

  bool GetCurrentPlane(OrthancStone::CoordinateSystem3D& plane) const
  {
    if (cursor_.get() != NULL &&
        frames_.get() != NULL)
    {
      plane = IFramesCollection::GetFrameGeometry(*frames_, cursor_->GetCurrentIndex());
      return true;
    }
    else
    {
      return false;
    }
  }

  void UpdateReferenceLines(const std::list<const ViewerViewport*>& viewports)
  {
    std::unique_ptr<OrthancStone::PolylineSceneLayer> layer(new OrthancStone::PolylineSceneLayer);
    
    if (cursor_.get() != NULL &&
        frames_.get() != NULL)
    {
      const size_t cursorIndex = cursor_->GetCurrentIndex();
      const OrthancStone::DicomInstanceParameters& instance = frames_->GetInstanceOfFrame(cursorIndex);
      const unsigned int frame = frames_->GetFrameNumberInInstance(cursorIndex);

      for (std::list<const ViewerViewport*>::const_iterator
             it = viewports.begin(); it != viewports.end(); ++it)
      {
        assert(*it != NULL);

        OrthancStone::CoordinateSystem3D otherPlane;
        std::string otherFrameOfReferenceUid;
        if ((*it)->GetCurrentPlane(otherPlane) &&
            (*it)->GetCurrentFrameOfReferenceUid(otherFrameOfReferenceUid) &&
            otherFrameOfReferenceUid == instance.GetFrameOfReferenceUid())
        {
          double x1, y1, x2, y2;
          if (GetReferenceLineCoordinates(x1, y1, x2, y2, instance, frame, otherPlane))
          {
            OrthancStone::PolylineSceneLayer::Chain chain;
            chain.push_back(OrthancStone::ScenePoint2D(x1, y1));
            chain.push_back(OrthancStone::ScenePoint2D(x2, y2));
            layer->AddChain(chain, false, 0, 255, 0);
          }
        }
      }
    }

    {
      std::unique_ptr<OrthancStone::IViewport::ILock> lock(viewport_->Lock());

      if (layer->GetChainsCount() == 0)
      {
        lock->GetController().GetScene().DeleteLayer(LAYER_REFERENCE_LINES);
      }
      else
      {
        lock->GetController().GetScene().SetLayer(LAYER_REFERENCE_LINES, layer.release());
      }
      
      //lock->GetCompositor().Refresh(lock->GetController().GetScene());
      lock->Invalidate();
    }
  }


  void ClearReferenceLines()
  {
    {
      std::unique_ptr<OrthancStone::IViewport::ILock> lock(viewport_->Lock());
      lock->GetController().GetScene().DeleteLayer(LAYER_REFERENCE_LINES);
      lock->Invalidate();
    }
  }


  void StretchWindowing()
  {
    float minValue, maxValue;
    
    {
      std::unique_ptr<OrthancStone::IViewport::ILock> lock(viewport_->Lock());

      if (!lock->GetController().GetScene().HasLayer(LAYER_TEXTURE) ||
          lock->GetController().GetScene().GetLayer(LAYER_TEXTURE).GetType() !=
          OrthancStone::ISceneLayer::Type_FloatTexture)
      {
        return;
      }

      const Orthanc::ImageAccessor& texture =
        dynamic_cast<OrthancStone::FloatTextureSceneLayer&>(
          lock->GetController().GetScene().GetLayer(LAYER_TEXTURE)).GetTexture();
      if (texture.GetFormat() != Orthanc::PixelFormat_Float32)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }

      Orthanc::ImageProcessing::GetMinMaxFloatValue(minValue, maxValue, texture);
    }

    const float center = (minValue + maxValue) / 2.0f;
    const float width = maxValue - minValue;
    UpdateWindowing(WindowingState_User, OrthancStone::Windowing(center, width));
  }

  void FlipX()
  {
    {
      std::unique_ptr<OrthancStone::IViewport::ILock> lock(viewport_->Lock());
      lock->GetController().GetScene().FlipViewportX(
        lock->GetCompositor().GetCanvasWidth(), lock->GetCompositor().GetCanvasHeight());
      lock->Invalidate();
    }    
  }

  void FlipY()
  {
    {
      std::unique_ptr<OrthancStone::IViewport::ILock> lock(viewport_->Lock());
      lock->GetController().GetScene().FlipViewportY(
        lock->GetCompositor().GetCanvasWidth(), lock->GetCompositor().GetCanvasHeight());
      lock->Invalidate();
    }
  }

  void RotateLeft()
  {
    {
      std::unique_ptr<OrthancStone::IViewport::ILock> lock(viewport_->Lock());
      lock->GetController().GetScene().RotateViewport(
        -PI / 2.0, lock->GetCompositor().GetCanvasWidth(), lock->GetCompositor().GetCanvasHeight());
      lock->Invalidate();
    }    
  }

  void RotateRight()
  {
    {
      std::unique_ptr<OrthancStone::IViewport::ILock> lock(viewport_->Lock());
      lock->GetController().GetScene().RotateViewport(
        PI / 2.0, lock->GetCompositor().GetCanvasWidth(), lock->GetCompositor().GetCanvasHeight());
      lock->Invalidate();
    }
  }

  void Invert()
  {
    inverted_ = !inverted_;
    
    {
      std::unique_ptr<OrthancStone::IViewport::ILock> lock(viewport_->Lock());

      if (lock->GetController().GetScene().HasLayer(LAYER_TEXTURE) &&
          lock->GetController().GetScene().GetLayer(LAYER_TEXTURE).GetType() ==
          OrthancStone::ISceneLayer::Type_FloatTexture)
      {
        OrthancStone::FloatTextureSceneLayer& layer = 
          dynamic_cast<OrthancStone::FloatTextureSceneLayer&>(
            lock->GetController().GetScene().GetLayer(LAYER_TEXTURE));

        // NB: Using "IsInverted()" instead of "inverted_" is for
        // compatibility with MONOCHROME1 images
        layer.SetInverted(!layer.IsInverted());
        lock->Invalidate();
      }
    }
  }



  class Interactor : public OrthancStone::DefaultViewportInteractor
  {
  private:
    ViewerViewport&  viewer_;
    WebViewerAction  leftAction_;
    WebViewerAction  middleAction_;
    WebViewerAction  rightAction_;

    bool IsAction(const OrthancStone::PointerEvent& event,
                  WebViewerAction action)
    {
      switch (event.GetMouseButton())
      {
        case OrthancStone::MouseButton_Left:
          return (leftAction_ == action);

        case OrthancStone::MouseButton_Middle:
          return (middleAction_ == action);
      
        case OrthancStone::MouseButton_Right:
          return (rightAction_ == action);

        default:
          return false;
      }
    }
    
  public:
    Interactor(ViewerViewport& viewer,
               WebViewerAction leftAction,
               WebViewerAction middleAction,
               WebViewerAction rightAction) :
      viewer_(viewer),
      leftAction_(leftAction),
      middleAction_(middleAction),
      rightAction_(rightAction)
    {
      SetLeftButtonAction(ConvertWebViewerAction(leftAction));
      SetMiddleButtonAction(ConvertWebViewerAction(middleAction));
      SetRightButtonAction(ConvertWebViewerAction(rightAction));
    }

    virtual OrthancStone::IFlexiblePointerTracker* CreateTracker(
      boost::weak_ptr<OrthancStone::IViewport> viewport,
      const OrthancStone::PointerEvent& event,
      unsigned int viewportWidth,
      unsigned int viewportHeight) ORTHANC_OVERRIDE
    {
      boost::shared_ptr<OrthancStone::IViewport> lock1(viewport.lock());
      
      if (lock1 &&
          IsAction(event, WebViewerAction_Crosshair))
      {
        OrthancStone::CoordinateSystem3D plane;
        if (viewer_.GetCurrentPlane(plane))
        {
          std::unique_ptr<OrthancStone::IViewport::ILock> lock2(lock1->Lock());

          const OrthancStone::ScenePoint2D p = event.GetMainPosition();
          double x = p.GetX();
          double y = p.GetY();
          lock2->GetController().GetCanvasToSceneTransform().Apply(x, y);
          
          OrthancStone::Vector click = plane.MapSliceToWorldCoordinates(x, y);
          if (viewer_.observer_.get() != NULL)
          {
            viewer_.observer_->SignalCrosshair(viewer_, click);
          }
        }
        
        return NULL;  // No need for a tracker, this is just a click
      }
      else
      {
        // Only the left mouse button can be used to edit/create/remove annotations
        if (event.GetMouseButton() == OrthancStone::MouseButton_Left)
        {
          switch (leftAction_)
          {
            case WebViewerAction_CreateAngle:
              viewer_.stoneAnnotations_->SetActiveTool(OrthancStone::AnnotationsSceneLayer::Tool_Angle);
              break;
              
            case WebViewerAction_CreateCircle:
              viewer_.stoneAnnotations_->SetActiveTool(OrthancStone::AnnotationsSceneLayer::Tool_Circle);
              break;
              
            case WebViewerAction_CreateLength:
              viewer_.stoneAnnotations_->SetActiveTool(OrthancStone::AnnotationsSceneLayer::Tool_Length);
              break;

            case WebViewerAction_RemoveMeasure:
              viewer_.stoneAnnotations_->SetActiveTool(OrthancStone::AnnotationsSceneLayer::Tool_Remove);
              break;

            case WebViewerAction_CreatePixelProbe:
              viewer_.stoneAnnotations_->SetActiveTool(OrthancStone::AnnotationsSceneLayer::Tool_PixelProbe);
              break;

            case WebViewerAction_CreateEllipseProbe:
              viewer_.stoneAnnotations_->SetActiveTool(OrthancStone::AnnotationsSceneLayer::Tool_EllipseProbe);
              break;

            case WebViewerAction_CreateRectangleProbe:
              viewer_.stoneAnnotations_->SetActiveTool(OrthancStone::AnnotationsSceneLayer::Tool_RectangleProbe);
              break;

            case WebViewerAction_CreateTextAnnotation:
              viewer_.stoneAnnotations_->SetActiveTool(OrthancStone::AnnotationsSceneLayer::Tool_TextAnnotation);
              break;

            default:
              viewer_.stoneAnnotations_->SetActiveTool(OrthancStone::AnnotationsSceneLayer::Tool_Edit);
              break;
          }

          {
            std::unique_ptr<OrthancStone::IViewport::ILock> lock2(lock1->Lock());

            std::unique_ptr<OrthancStone::IFlexiblePointerTracker> t;
            t.reset(viewer_.stoneAnnotations_->CreateTracker(event.GetMainPosition(), lock2->GetController().GetScene()));

            if (t.get() != NULL)
            {
              return t.release();
            }
          }
        }

        return DefaultViewportInteractor::CreateTracker(
          viewport, event, viewportWidth, viewportHeight);
      }
    }

    virtual bool HasMouseHover() const ORTHANC_OVERRIDE
    {
      return true;
    }

    virtual void HandleMouseHover(OrthancStone::IViewport& viewport,
                                  const OrthancStone::PointerEvent& event) ORTHANC_OVERRIDE
    {
      std::unique_ptr<OrthancStone::IViewport::ILock> lock(viewport.Lock());

      if (viewer_.stoneAnnotations_->SetMouseHover(event.GetMainPosition(), lock->GetController().GetScene()))
      {
        viewer_.stoneAnnotations_->Render(lock->GetController().GetScene());
        lock->Invalidate();
      }
    }
  };
  

  void SetMouseButtonActions(WebViewerAction leftAction,
                             WebViewerAction middleAction,
                             WebViewerAction rightAction)
  {
    assert(viewport_ != NULL);
    viewport_->AcquireInteractor(new Interactor(*this, leftAction, middleAction, rightAction));
  }

  void FitForPrint()
  {
    viewport_->FitForPrint();
  }

  void ScheduleFrameFocus(const std::string& sopInstanceUid,
                          unsigned int frameNumber)
  {
    hasFocusOnInstance_ = true;
    focusSopInstanceUid_ = sopInstanceUid;
    focusFrameNumber_ = frameNumber;
    
    ApplyScheduledFocus();
  }

  void ApplyScheduledFocus()
  {
    size_t cursorIndex;
    
    if (hasFocusOnInstance_ &&
        cursor_.get() != NULL &&
        frames_.get() != NULL &&
        frames_->LookupFrame(cursorIndex, focusSopInstanceUid_, focusFrameNumber_))
    {
      size_t current = cursor_->GetCurrentIndex();

      if (current != cursorIndex)
      {
        cursor_->SetCurrentIndex(cursorIndex);
        Redraw();
      }
      
      hasFocusOnInstance_ = false;
    }
  }

  void FocusOnPoint(const OrthancStone::Vector& p)
  {
    //static const double MAX_DISTANCE = 0.5;   // 0.5 cm => TODO parameter?
    static const double MAX_DISTANCE = std::numeric_limits<double>::infinity();

    size_t cursorIndex;
    if (cursor_.get() != NULL &&
        frames_.get() != NULL &&
        frames_->FindClosestFrame(cursorIndex, p, MAX_DISTANCE))
    {
      cursor_->SetCurrentIndex(cursorIndex);
      Redraw();
    }
  }

  unsigned int GetCineRate() const
  {
    return cineRate_;
  }

  void FormatWindowingPresets(Json::Value& target) const
  {
    target = Json::arrayValue;

    for (size_t i = 0; i < windowingPresets_.size(); i++)
    {
      const double c = windowingPresets_[i].GetCenter();
      const double w = windowingPresets_[i].GetWidth();
      
      std::string name = "Preset";
      if (windowingPresets_.size() > 1)
      {
        name += " " + boost::lexical_cast<std::string>(i + 1);
      }

      Json::Value preset = Json::objectValue;
      preset["name"] = name;
      preset["center"] = c;
      preset["width"] = w;
      preset["info"] =
        ("C " + boost::lexical_cast<std::string>(static_cast<int>(boost::math::iround<double>(c))) +
         ", W " + boost::lexical_cast<std::string>(static_cast<int>(boost::math::iround<double>(w))));
      
      target.append(preset);
    }
  }


  void SetSynchronizedBrowsingEnabled(int enabled)
  {
    OrthancStone::LinearAlgebra::AssignVector(synchronizationOffset_, 0, 0, 0);
    synchronizationEnabled_ = enabled;
  }


  void SignalStoneAnnotationsChanged(const std::string& sopInstanceUid,
                                     size_t frame)
  {
    if (cursor_.get() != NULL &&
        frames_.get() != NULL)
    {
      const size_t cursorIndex = cursor_->GetCurrentIndex();
      const OrthancStone::DicomInstanceParameters& instance = frames_->GetInstanceOfFrame(cursorIndex);
      const unsigned int frameNumber = frames_->GetFrameNumberInInstance(cursorIndex);

      if (instance.GetSopInstanceUid() == sopInstanceUid &&
          frameNumber == frame)
      {
        StoneAnnotationsRegistry::GetInstance().Load(*stoneAnnotations_, instance.GetSopInstanceUid(), frame);

        {
          std::unique_ptr<OrthancStone::IViewport::ILock> lock(viewport_->Lock());
          stoneAnnotations_->Render(lock->GetController().GetScene());
          lock->Invalidate();
        }
      }
    }    
  }

  void SetLinearInterpolation(bool linearInterpolation)
  {
    if (linearInterpolation_ != linearInterpolation)
    {
      linearInterpolation_ = linearInterpolation;
      Redraw();
    }
  }

  void AddTextAnnotation(const std::string& label,
                         const OrthancStone::ScenePoint2D& pointedPosition,
                         const OrthancStone::ScenePoint2D& labelPosition)
  {
    stoneAnnotations_->AddTextAnnotation(label, pointedPosition, labelPosition);
    Redraw();
  }


  bool GetCurrentFrame(std::string& sopInstanceUid /* out */,
                       unsigned int& frameNumber /* out */) const
  {
    if (cursor_.get() != NULL &&
        frames_.get() != NULL)
    {
      const size_t cursorIndex = cursor_->GetCurrentIndex();
      const OrthancStone::DicomInstanceParameters& instance = frames_->GetInstanceOfFrame(cursorIndex);
      sopInstanceUid = instance.GetSopInstanceUid();
      frameNumber = frames_->GetFrameNumberInInstance(cursorIndex);
      return true;
    }
    else
    {
      return false;
    }
  }


  void SignalSynchronizedBrowsing()
  {
    if (synchronizationEnabled_ &&
        frames_.get() != NULL &&
        cursor_.get() != NULL)
    {
      const size_t currentCursorIndex = cursor_->GetCurrentIndex();

      const OrthancStone::CoordinateSystem3D current = IFramesCollection::GetFrameGeometry(*frames_, currentCursorIndex);
      
      observer_->SignalSynchronizedBrowsing(
        *this, current.GetOrigin() + synchronizationOffset_, current.GetNormal());
    }
  }


  void SetUserWindowing(const OrthancStone::Windowing& windowing)
  {
    UpdateWindowing(WindowingState_User, windowing);
  }


  void SetPendingSeriesInstanceUid(const std::string& seriesInstanceUid)
  {
    pendingSeriesInstanceUid_ = seriesInstanceUid;
  }


  void ClearPendingSeriesInstanceUid()
  {
    pendingSeriesInstanceUid_.clear();
  }


  const std::string& GetPendingSeriesInstanceUid() const
  {
    return pendingSeriesInstanceUid_;
  }


  static void SetAnnotationsColor(const OrthancStone::Color& color)
  {
    GetAnnotationsColorInternal() = color;
  }


  static void SetHighlightedColor(const OrthancStone::Color& color)
  {
    GetHighlightedColorInternal() = color;
  }
};





class OrientationMarkersSource : public ILayerSource
{
public:
  virtual int GetDepth() const ORTHANC_OVERRIDE
  {
    return LAYER_ORIENTATION_MARKERS;
  }

  virtual OrthancStone::ISceneLayer* Create(const Orthanc::ImageAccessor& frame,
                                            const OrthancStone::DicomInstanceParameters& instance,
                                            unsigned int frameNumber,
                                            double pixelSpacingX,
                                            double pixelSpacingY,
                                            const OrthancStone::CoordinateSystem3D& plane) ORTHANC_OVERRIDE
  {
    if (instance.GetGeometry().IsValid())
    {
      std::unique_ptr<OrthancStone::MacroSceneLayer> layer(new OrthancStone::MacroSceneLayer);

      std::string top, bottom, left, right;
      instance.GetGeometry().GetOrientationMarkers(top, bottom, left, right);

      std::unique_ptr<OrthancStone::TextSceneLayer> text;

      text.reset(new OrthancStone::TextSceneLayer);
      text->SetText(top);
      text->SetPosition(pixelSpacingX * static_cast<double>(frame.GetWidth()) / 2.0, 0);
      text->SetAnchor(OrthancStone::BitmapAnchor_TopCenter);
      layer->AddLayer(text.release());

      text.reset(new OrthancStone::TextSceneLayer);
      text->SetText(bottom);
      text->SetPosition(pixelSpacingX * static_cast<double>(frame.GetWidth()) / 2.0,
                        pixelSpacingY * static_cast<double>(frame.GetHeight()));
      text->SetAnchor(OrthancStone::BitmapAnchor_BottomCenter);
      layer->AddLayer(text.release());

      text.reset(new OrthancStone::TextSceneLayer);
      text->SetText(left);
      text->SetPosition(0, pixelSpacingY * static_cast<double>(frame.GetHeight()) / 2.0);
      text->SetAnchor(OrthancStone::BitmapAnchor_CenterLeft);
      layer->AddLayer(text.release());

      text.reset(new OrthancStone::TextSceneLayer);
      text->SetText(right);
      text->SetPosition(pixelSpacingX * static_cast<double>(frame.GetWidth()),
                        pixelSpacingY * static_cast<double>(frame.GetHeight()) / 2.0);
      text->SetAnchor(OrthancStone::BitmapAnchor_CenterRight);
      layer->AddLayer(text.release());

      return layer.release();
    }
    else
    {
      return NULL;
    }
  }
};


class OsiriXLayerSource : public ILayerSource
{
private:
  // The coordinates of OsiriX annotations are expressed in 3D world coordinates
  OrthancStone::OsiriX::CollectionOfAnnotations  annotations_;
  OrthancStone::Color                            color_;

public:
  OsiriXLayerSource() :
    color_(0, 255, 0)
  {
  }

  OrthancStone::OsiriX::CollectionOfAnnotations& GetAnnotations()
  {
    return annotations_;
  }

  void SetColor(const OrthancStone::Color& color)
  {
    color_ = color;
  }

  virtual int GetDepth() const ORTHANC_OVERRIDE
  {
    return LAYER_ANNOTATIONS_OSIRIX;
  }

  virtual OrthancStone::ISceneLayer* Create(const Orthanc::ImageAccessor& frame,
                                            const OrthancStone::DicomInstanceParameters& instance,
                                            unsigned int frameNumber,
                                            double pixelSpacingX,
                                            double pixelSpacingY,
                                            const OrthancStone::CoordinateSystem3D& plane) ORTHANC_OVERRIDE
  {
    std::set<size_t> a;
    annotations_.LookupSopInstanceUid(a, instance.GetSopInstanceUid());
    if (plane.IsValid() &&
        !a.empty())
    {
      std::unique_ptr<OrthancStone::MacroSceneLayer> layer(new OrthancStone::MacroSceneLayer);
      // layer->Reserve(a.size());

      OrthancStone::OsiriXLayerFactory factory;
      factory.SetColor(color_);

      for (std::set<size_t>::const_iterator it = a.begin(); it != a.end(); ++it)
      {
        const OrthancStone::OsiriX::Annotation& annotation = annotations_.GetAnnotation(*it);
        layer->AddLayer(factory.Create(annotation, plane));
      }

      return layer.release();
    }

    return NULL;
  }
};



class OverlayLayerSource : public ILayerSource
{
public:
  virtual int GetDepth() const ORTHANC_OVERRIDE
  {
    return LAYER_OVERLAY;
  }

  virtual OrthancStone::ISceneLayer* Create(const Orthanc::ImageAccessor& frame,
                                            const OrthancStone::DicomInstanceParameters& instance,
                                            unsigned int frameNumber,
                                            double pixelSpacingX,
                                            double pixelSpacingY,
                                            const OrthancStone::CoordinateSystem3D& plane) ORTHANC_OVERRIDE
  {
    OverlaysRegistry::Accessor accessor(OverlaysRegistry::GetInstance(), instance.GetSopInstanceUid());
    if (accessor.IsValid())
    {
      std::unique_ptr<OrthancStone::LookupTableTextureSceneLayer> layer(accessor.CreateTexture());
      layer->SetLinearInterpolation(false);
      return layer.release();
    }
    else
    {
      return NULL;
    }
  }
};



typedef std::map<std::string, boost::shared_ptr<ViewerViewport> >  Viewports;

static Viewports allViewports_;
static bool showReferenceLines_ = true;
static std::unique_ptr<OverlayLayerSource>  overlayLayerSource_;
static std::unique_ptr<OsiriXLayerSource>   osiriXLayerSource_;

// Orientation markers, new in Stone Web viewer 2.4
static std::unique_ptr<OrientationMarkersSource>  orientationMarkersSource_;


static void UpdateReferenceLines()
{
  if (showReferenceLines_)
  {
    std::list<const ViewerViewport*> viewports;
    
    for (Viewports::const_iterator it = allViewports_.begin(); it != allViewports_.end(); ++it)
    {
      assert(it->second != NULL);
      viewports.push_back(it->second.get());
    }

    for (Viewports::iterator it = allViewports_.begin(); it != allViewports_.end(); ++it)
    {
      assert(it->second != NULL);
      it->second->UpdateReferenceLines(viewports);
    }
  }
  else
  {
    for (Viewports::iterator it = allViewports_.begin(); it != allViewports_.end(); ++it)
    {
      assert(it->second != NULL);
      it->second->ClearReferenceLines();
    }
  }
}


class WebAssemblyObserver : public ResourcesLoader::IObserver,
                            public ViewerViewport::IObserver
{
public:
  virtual void SignalResourcesLoaded() ORTHANC_OVERRIDE
  {
    DISPATCH_JAVASCRIPT_EVENT("ResourcesLoaded");
  }

  virtual void SignalSeriesThumbnailLoaded(const std::string& studyInstanceUid,
                                           const std::string& seriesInstanceUid) ORTHANC_OVERRIDE
  {
    EM_ASM({
        const customEvent = document.createEvent("CustomEvent");
        customEvent.initCustomEvent("ThumbnailLoaded", false, false,
                                    { "studyInstanceUid" : UTF8ToString($0),
                                        "seriesInstanceUid" : UTF8ToString($1) });
        window.dispatchEvent(customEvent);
      },
      studyInstanceUid.c_str(),
      seriesInstanceUid.c_str());
  }

  virtual void SignalSeriesMetadataLoaded(const std::string& studyInstanceUid,
                                          const std::string& seriesInstanceUid) ORTHANC_OVERRIDE
  {
    EM_ASM({
        const customEvent = document.createEvent("CustomEvent");
        customEvent.initCustomEvent("MetadataLoaded", false, false,
                                    { "studyInstanceUid" : UTF8ToString($0),
                                        "seriesInstanceUid" : UTF8ToString($1) });
        window.dispatchEvent(customEvent);
      },
      studyInstanceUid.c_str(),
      seriesInstanceUid.c_str());

    for (Viewports::const_iterator it = allViewports_.begin(); it != allViewports_.end(); ++it)
    {
      assert(it->second != NULL);
      it->second->ApplyScheduledFocus();
    }
  }

  virtual void SignalSeriesDetailsReady(const ViewerViewport& viewport) ORTHANC_OVERRIDE
  {
    EM_ASM({
        const customEvent = document.createEvent("CustomEvent");
        customEvent.initCustomEvent("SeriesDetailsReady", false, false,
                                    { "canvasId" : UTF8ToString($0) });
        window.dispatchEvent(customEvent);
      },
      viewport.GetCanvasId().c_str()
      );
  }

  virtual void SignalFrameUpdated(const ViewerViewport& viewport,
                                  size_t currentFrame,
                                  size_t countFrames,
                                  DisplayedFrameQuality quality,
                                  unsigned int instanceNumber,
                                  const std::string& contentDate,
                                  const std::string& contentTime) ORTHANC_OVERRIDE
  {
    EM_ASM({
        const customEvent = document.createEvent("CustomEvent");
        customEvent.initCustomEvent("FrameUpdated", false, false,
                                    { "canvasId" : UTF8ToString($0),
                                        "currentFrame" : $1,
                                        "numberOfFrames" : $2,
                                        "quality" : $3,
                                        "instanceNumber" : $4,
                                        "contentDate" : UTF8ToString($5),
                                        "contentTime" : UTF8ToString($6),
                                         });
        window.dispatchEvent(customEvent);
      },
      viewport.GetCanvasId().c_str(),
      static_cast<int>(currentFrame),
      static_cast<int>(countFrames),
      quality, 
      instanceNumber, 
      contentDate.c_str(),
      contentTime.c_str());

    UpdateReferenceLines();
  }

  virtual void SignalCrosshair(const ViewerViewport& viewport,
                               const OrthancStone::Vector& click) ORTHANC_OVERRIDE
  {
    if (click.size() != 3u)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    
    for (Viewports::const_iterator it = allViewports_.begin(); it != allViewports_.end(); ++it)
    {
      // TODO - One could check the "Frame Of Reference UID" here
      assert(it->second.get() != NULL);
      if (it->second.get() != &viewport)
      {
        it->second->FocusOnPoint(click);
      }
    }
  }

  virtual void SignalSynchronizedBrowsing(const ViewerViewport& viewport,
                                          const OrthancStone::Vector& click,
                                          const OrthancStone::Vector& normal) ORTHANC_OVERRIDE
  {
    for (Viewports::const_iterator it = allViewports_.begin(); it != allViewports_.end(); ++it)
    {
      assert(it->second.get() != NULL);

      OrthancStone::CoordinateSystem3D plane;
      bool isOpposite;
      
      if (it->second.get() != &viewport &&
          it->second->GetCurrentPlane(plane) &&
          OrthancStone::GeometryToolbox::IsParallelOrOpposite(isOpposite, plane.GetNormal(), normal))
      {
        it->second->FocusOnPoint(click);
      }
    }
  }

  virtual void SignalSeriesPdfLoaded(const std::string& studyInstanceUid,
                                     const std::string& seriesInstanceUid,
                                     const std::string& pdf) ORTHANC_OVERRIDE
  {
    EM_ASM({
        const customEvent = document.createEvent("CustomEvent");
        customEvent.initCustomEvent("PdfLoaded", false, false,
                                    { "studyInstanceUid" : UTF8ToString($0),
                                        "seriesInstanceUid" : UTF8ToString($1),
                                        "pdfPointer" : $2,
                                        "pdfSize": $3});
        window.dispatchEvent(customEvent);
      },
      studyInstanceUid.c_str(),
      seriesInstanceUid.c_str(),
      pdf.empty() ? 0 : reinterpret_cast<intptr_t>(pdf.c_str()),  // Explicit conversion to an integer
      pdf.size());
  }


  virtual void SignalVirtualSeriesThumbnailLoaded(const std::string& virtualSeriesId,
                                                  const std::string& jpeg) ORTHANC_OVERRIDE
  {
    std::string dataUriScheme;
    Orthanc::Toolbox::EncodeDataUriScheme(dataUriScheme, "image/jpeg", jpeg);    
    
    EM_ASM({
        const customEvent = document.createEvent("CustomEvent");
        customEvent.initCustomEvent("VirtualSeriesThumbnailLoaded", false, false,
                                    { "virtualSeriesId" : UTF8ToString($0),
                                        "thumbnail" : UTF8ToString($1) });
        window.dispatchEvent(customEvent);
      },
      virtualSeriesId.c_str(),
      dataUriScheme.c_str());
  }

  virtual void SignalWindowingUpdated(const ViewerViewport& viewport,
                                      const OrthancStone::Windowing& windowing) ORTHANC_OVERRIDE
  {
    EM_ASM({
        const customEvent = document.createEvent("CustomEvent");
        customEvent.initCustomEvent("WindowingUpdated", false, false,
                                    { "canvasId" : UTF8ToString($0),
                                        "windowingCenter" : $1,
                                        "windowingWidth" : $2 });
        window.dispatchEvent(customEvent);
      },
      viewport.GetCanvasId().c_str(),
      static_cast<int>(boost::math::iround<double>(windowing.GetCenter())),
      static_cast<int>(boost::math::iround<double>(windowing.GetWidth())));

    UpdateReferenceLines();
  }

  virtual void SignalStoneAnnotationsChanged(const ViewerViewport& viewport,
                                             const std::string& sopInstanceUid,
                                             size_t frame) ORTHANC_OVERRIDE
  {
    for (Viewports::const_iterator it = allViewports_.begin(); it != allViewports_.end(); ++it)
    {
      assert(it->second.get() != NULL);

      if (it->second.get() != &viewport)
      {
        it->second->SignalStoneAnnotationsChanged(sopInstanceUid, frame);
      }
    }
  }
  
  virtual void SignalStoneAnnotationAdded(const ViewerViewport& viewport) ORTHANC_OVERRIDE
  {
    EM_ASM({
        const customEvent = document.createEvent("CustomEvent");
        customEvent.initCustomEvent("StoneAnnotationAdded", false, false,
                                    { "canvasId" : UTF8ToString($0) });
        window.dispatchEvent(customEvent);
      },
      viewport.GetCanvasId().c_str());
  }

  virtual void SignalStoneAnnotationRemoved(const ViewerViewport& viewport) ORTHANC_OVERRIDE
  {
    EM_ASM({
        const customEvent = document.createEvent("CustomEvent");
        customEvent.initCustomEvent("StoneAnnotationRemoved", false, false,
                                    { "canvasId" : UTF8ToString($0) });
        window.dispatchEvent(customEvent);
      },
      viewport.GetCanvasId().c_str());
  }

  virtual void SignalStoneTextAnnotationRequired(const ViewerViewport& viewport,
                                                 const OrthancStone::ScenePoint2D& pointedPosition,
                                                 const OrthancStone::ScenePoint2D& labelPosition) ORTHANC_OVERRIDE
  {
    EM_ASM({
        const customEvent = document.createEvent("CustomEvent");
        customEvent.initCustomEvent("TextAnnotationRequired", false, false,
                                    { "canvasId" : UTF8ToString($0),
                                      "pointedX" : $1,
                                      "pointedY" : $2,
                                      "labelX" : $3,
                                      "labelY" : $4 });
        window.dispatchEvent(customEvent);
      },
      viewport.GetCanvasId().c_str(),
      pointedPosition.GetX(),
      pointedPosition.GetY(),
      labelPosition.GetX(),
      labelPosition.GetY() );
  }

  virtual void SignalDicomSRLoaded(const std::string& studyInstanceUid,
                                   const std::string& seriesInstanceUid,
                                   const std::string& sopInstanceUid) ORTHANC_OVERRIDE;
};



static OrthancStone::DicomSource source_;
static boost::shared_ptr<FramesCache> framesCache_;
static boost::shared_ptr<InstancesCache> instancesCache_;
static boost::shared_ptr<OrthancStone::WebAssemblyLoadersContext> context_;
static std::string stringBuffer_;
static bool softwareRendering_ = false;
static bool linearInterpolation_ = true;
static WebViewerAction leftButtonAction_ = WebViewerAction_Windowing;
static WebViewerAction middleButtonAction_ = WebViewerAction_Pan;
static WebViewerAction rightButtonAction_ = WebViewerAction_Zoom;


static void FormatTags(std::string& target,
                       const Orthanc::DicomMap& tags)
{
  Orthanc::DicomArray arr(tags);
  Json::Value v = Json::objectValue;

  for (size_t i = 0; i < arr.GetSize(); i++)
  {
    const Orthanc::DicomElement& element = arr.GetElement(i);
    if (!element.GetValue().IsBinary() &&
        !element.GetValue().IsNull())
    {
      v[element.GetTag().Format()] = element.GetValue().GetContent();
    }
  }

  target = v.toStyledString();
}


static ResourcesLoader& GetResourcesLoader()
{
  static boost::shared_ptr<ResourcesLoader>  resourcesLoader_;

  if (!resourcesLoader_)
  {
    std::unique_ptr<OrthancStone::ILoadersContext::ILock> lock(context_->Lock());
    resourcesLoader_ = ResourcesLoader::Create(*lock, source_);
    resourcesLoader_->AcquireObserver(new WebAssemblyObserver);
  }

  return *resourcesLoader_;
}


typedef IStoneWebViewerPlugin* (*PluginInitializer) (IStoneWebViewerContext&);

static const PluginInitializer pluginsInitializers_[] = {
  NULL
};

static std::list< boost::shared_ptr<IStoneWebViewerPlugin> >  plugins_;


static boost::shared_ptr<ViewerViewport> GetViewport(const std::string& canvas)
{
  Viewports::iterator found = allViewports_.find(canvas);
  if (found == allViewports_.end())
  {
    boost::shared_ptr<ViewerViewport> viewport(
      ViewerViewport::Create(*context_, source_, canvas, framesCache_, instancesCache_, softwareRendering_, linearInterpolation_));
    viewport->SetMouseButtonActions(leftButtonAction_, middleButtonAction_, rightButtonAction_);
    viewport->AcquireObserver(new WebAssemblyObserver);
    viewport->AddLayerSource(*overlayLayerSource_);
    viewport->AddLayerSource(*osiriXLayerSource_);
    viewport->AddLayerSource(*orientationMarkersSource_);

    for (std::list< boost::shared_ptr<IStoneWebViewerPlugin> >::iterator it = plugins_.begin(); it != plugins_.end(); ++it)
    {
      viewport->AddLayerSource((*it)->GetLayerSource());
    }

    allViewports_[canvas] = viewport;
    return viewport;
  }
  else
  {
    return found->second;
  }
}


void WebAssemblyObserver::SignalDicomSRLoaded(const std::string& studyInstanceUid,
                                              const std::string& seriesInstanceUid,
                                              const std::string& sopInstanceUid)
{
  for (Viewports::iterator it = allViewports_.begin(); it != allViewports_.end(); ++it)
  {
    if (it->second->GetPendingSeriesInstanceUid() == seriesInstanceUid)
    {
      it->second->ClearPendingSeriesInstanceUid();

      std::unique_ptr<IFramesCollection> frames(GetResourcesLoader().GetSeriesFrames(seriesInstanceUid));

      if (frames.get() != NULL)
      {
        it->second->SetFrames(frames.release());
      }
    }
  }
}


class StoneWebViewerContext : public IStoneWebViewerContext
{
public:
  static StoneWebViewerContext& GetInstance()
  {
    static StoneWebViewerContext instance;
    return instance;
  }

  virtual void RedrawAllViewports() ORTHANC_OVERRIDE
  {
    for (Viewports::iterator it = allViewports_.begin(); it != allViewports_.end(); ++it)
    {
      assert(it->second != NULL);
      it->second->Redraw();
    }
  }

  virtual bool GetSelectedFrame(Orthanc::ImageAccessor& target /* out */,
                                std::string& sopInstanceUid /* out */,
                                unsigned int& frameNumber /* out */,
                                const std::string& canvas /* in */) ORTHANC_OVERRIDE
  {
    boost::shared_ptr<ViewerViewport> viewport = GetViewport(canvas);

    if (viewport->GetCurrentFrame(sopInstanceUid, frameNumber))
    {
      FramesCache::Accessor accessor(*framesCache_, sopInstanceUid, frameNumber);
      if (accessor.IsValid())
      {
        accessor.GetImage().GetReadOnlyAccessor(target);
        return true;
      }
      else
      {
        return false;
      }
    }
    else
    {
      LOG(WARNING) << "No active frame";
      return false;
    }
  }
};



static void SetAnnotationsColor(const OrthancStone::Color& color)
{
  ViewerViewport::SetAnnotationsColor(color);
  DicomStructuredReportFrames::SetAnnotationsColor(color);
  osiriXLayerSource_->SetColor(color);
}


static void SetHighlightedColor(const OrthancStone::Color& color)
{
  ViewerViewport::SetHighlightedColor(color);
}


extern "C"
{
  int main(int argc, char const *argv[]) 
  {
    printf("Initializing Stone\n");
    Orthanc::InitializeFramework("", true);
    Orthanc::Logging::EnableInfoLevel(true);
    //Orthanc::Logging::EnableTraceLevel(true);

    LOG(INFO) << "Using DCMTK version: " << DCMTK_VERSION_NUMBER;

    context_.reset(new OrthancStone::WebAssemblyLoadersContext(1, 4, 1));
    context_->SetDicomCacheSize(128 * 1024 * 1024);  // 128MB
    
    framesCache_.reset(new FramesCache);
    instancesCache_.reset(new InstancesCache);
    overlayLayerSource_.reset(new OverlayLayerSource);
    osiriXLayerSource_.reset(new OsiriXLayerSource);
    orientationMarkersSource_.reset(new OrientationMarkersSource);

    SetAnnotationsColor(OrthancStone::Color(0x40, 0x82, 0xad));  // This was COLOR_PRIMITIVES until 2.6
    SetHighlightedColor(OrthancStone::Color(0x40, 0xad, 0x79));  // This was COLOR_HOVER until 2.6

    for (size_t i = 0; pluginsInitializers_[i] != NULL; i++)
    {
      std::unique_ptr<IStoneWebViewerPlugin> plugin(pluginsInitializers_[i] (StoneWebViewerContext::GetInstance()));
      if (plugin.get() != NULL)
      {
        plugins_.push_back(boost::shared_ptr<IStoneWebViewerPlugin>(plugin.release()));
      }
    }

    DISPATCH_JAVASCRIPT_EVENT("StoneInitialized");
  }


  EMSCRIPTEN_KEEPALIVE
  void SetDicomWebRoot(const char* uri,
                       int useRendered)
  {
    try
    {
      source_.SetDicomWebSource(uri);
      source_.SetDicomWebRendered(useRendered != 0);
    }
    EXTERN_CATCH_EXCEPTIONS;
  }
  

  EMSCRIPTEN_KEEPALIVE
  void SetDicomWebThroughOrthanc(const char* orthancRoot,
                                 const char* serverName,
                                 int hasRendered)
  {
    try
    {
      context_->SetLocalOrthanc(orthancRoot);
      source_.SetDicomWebThroughOrthancSource(serverName);
      source_.SetDicomWebRendered(hasRendered != 0);
    }
    EXTERN_CATCH_EXCEPTIONS;
  }


  EMSCRIPTEN_KEEPALIVE
  void AddHttpHeader(const char* header,
                     const char* value)
  {
    try
    {
      source_.AddHttpHeader(header, value);
    }
    EXTERN_CATCH_EXCEPTIONS;
  }
  

  EMSCRIPTEN_KEEPALIVE
  void SetDicomCacheSize(int sizeMB)
  {
    try
    {
      if (sizeMB == 0)
      {
        LOG(WARNING) << "The DICOM cache is disabled";
      }
      else
      {
        LOG(INFO) << "The DICOM cache size is set to " << sizeMB << "MB";
      }

      if (sizeMB >= 0)
      {
        context_->SetDicomCacheSize(sizeMB * 1024 * 1024);
      }
    }
    EXTERN_CATCH_EXCEPTIONS;
  }
  

  EMSCRIPTEN_KEEPALIVE
  void SetSkipSeriesFromModalities(const char* value)
  {
    try
    {
      LOG(WARNING) << "SetSkipSeriesFromModalities " << value;
      
      Json::Value modalities;
      Orthanc::Toolbox::ReadJson(modalities, value);
      std::vector<std::string> skipSeriesFromModalities;

      for (Json::Value::ArrayIndex i = 0; i < modalities.size(); i++)
      {
        skipSeriesFromModalities.push_back(modalities[i].asString());
      }
      GetResourcesLoader().SetSkipSeriesFromModalities(skipSeriesFromModalities);
    }
    EXTERN_CATCH_EXCEPTIONS;
  }


  EMSCRIPTEN_KEEPALIVE
  void FetchAllStudies()
  {
    try
    {
      GetResourcesLoader().FetchAllStudies();
    }
    EXTERN_CATCH_EXCEPTIONS;
  }

  EMSCRIPTEN_KEEPALIVE
  void FetchPatient(const char* patientId)
  {
    try
    {
      GetResourcesLoader().FetchPatient(patientId);
    }
    EXTERN_CATCH_EXCEPTIONS;
  }

  EMSCRIPTEN_KEEPALIVE
  void FetchStudy(const char* studyInstanceUid)
  {
    try
    {
      GetResourcesLoader().FetchStudy(studyInstanceUid);
    }
    EXTERN_CATCH_EXCEPTIONS;
  }

  EMSCRIPTEN_KEEPALIVE
  void FetchSeries(const char* studyInstanceUid,
                   const char* seriesInstanceUid)
  {
    try
    {
      GetResourcesLoader().FetchSeries(studyInstanceUid, seriesInstanceUid);
    }
    EXTERN_CATCH_EXCEPTIONS;
  }
  
  EMSCRIPTEN_KEEPALIVE
  int GetStudiesCount()
  {
    try
    {
      return GetResourcesLoader().GetStudiesCount();
    }
    EXTERN_CATCH_EXCEPTIONS;
    return 0;  // on exception
  }
  
  EMSCRIPTEN_KEEPALIVE
  int GetSeriesCount()
  {
    try
    {
      return GetResourcesLoader().GetSeriesCount();
    }
    EXTERN_CATCH_EXCEPTIONS;
    return 0;  // on exception
  }


  EMSCRIPTEN_KEEPALIVE
  const char* GetStringBuffer()
  {
    return stringBuffer_.c_str();
  }
  

  EMSCRIPTEN_KEEPALIVE
  const char* GetStoneWebViewerVersion()
  {
    return STONE_WEB_VIEWER_VERSION;
  }
  

  EMSCRIPTEN_KEEPALIVE
  const char* GetEmscriptenVersion()
  {
    // WARNING - "static" is important, otherwise the string would be
    // freed when returning to JavaScript
    static const std::string EMSCRIPTEN_VERSION = (
      boost::lexical_cast<std::string>(__EMSCRIPTEN_major__) + "." +
      boost::lexical_cast<std::string>(__EMSCRIPTEN_minor__) + "." +
      boost::lexical_cast<std::string>(__EMSCRIPTEN_tiny__));
    return EMSCRIPTEN_VERSION.c_str();
  }
  

  EMSCRIPTEN_KEEPALIVE
  void LoadStudyTags(int i)
  {
    try
    {
      if (i < 0)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
      
      Orthanc::DicomMap dicom;
      GetResourcesLoader().GetStudy(dicom, i);
      FormatTags(stringBuffer_, dicom);
    }
    EXTERN_CATCH_EXCEPTIONS;
  }
  

  EMSCRIPTEN_KEEPALIVE
  void LoadSeriesTags(int i)
  {
    try
    {
      if (i < 0)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
      
      Orthanc::DicomMap dicom;
      GetResourcesLoader().GetSeries(dicom, i);
      FormatTags(stringBuffer_, dicom);
    }
    EXTERN_CATCH_EXCEPTIONS;
  }
  

  EMSCRIPTEN_KEEPALIVE
  int LoadSeriesThumbnail(const char* seriesInstanceUid)
  {
    try
    {
      std::string image, mime;
      switch (GetResourcesLoader().GetSeriesThumbnail(image, mime, seriesInstanceUid))
      {
        case OrthancStone::SeriesThumbnailType_Image:
          Orthanc::Toolbox::EncodeDataUriScheme(stringBuffer_, mime, image);
          return ThumbnailType_Image;
          
        case OrthancStone::SeriesThumbnailType_Pdf:
          return ThumbnailType_Pdf;
          
        case OrthancStone::SeriesThumbnailType_Video:
          return ThumbnailType_Video;
          
        case OrthancStone::SeriesThumbnailType_StructuredReport:
          return ThumbnailType_StructuredReport;

        case OrthancStone::SeriesThumbnailType_NotLoaded:
          return ThumbnailType_Loading;
          
        case OrthancStone::SeriesThumbnailType_Unsupported:
          return ThumbnailType_NoPreview;

        default:
          return ThumbnailType_Unknown;
      }
    }
    EXTERN_CATCH_EXCEPTIONS;
    return ThumbnailType_Unknown;
  }


  EMSCRIPTEN_KEEPALIVE
  void SpeedUpFetchSeriesMetadata(const char* studyInstanceUid,
                                  const char* seriesInstanceUid)
  {
    try
    {
      GetResourcesLoader().FetchSeriesMetadata(PRIORITY_HIGH, studyInstanceUid, seriesInstanceUid);
    }
    EXTERN_CATCH_EXCEPTIONS;
  }


  EMSCRIPTEN_KEEPALIVE
  int IsSeriesComplete(const char* seriesInstanceUid)
  {
    try
    {
      return GetResourcesLoader().IsSeriesComplete(seriesInstanceUid) ? 1 : 0;
    }
    EXTERN_CATCH_EXCEPTIONS;
    return 0;
  }

  EMSCRIPTEN_KEEPALIVE
  int LoadSeriesInViewport(const char* canvas,
                           const char* seriesInstanceUid)
  {
    try
    {
      std::unique_ptr<IFramesCollection> frames(GetResourcesLoader().GetSeriesFrames(seriesInstanceUid));

      if (frames.get() != NULL)
      {
        switch (frames->GetType())
        {
          case FramesCollectionType_None:
            GetViewport(canvas)->ClearPendingSeriesInstanceUid();
            break;

          case FramesCollectionType_DicomSR:
            GetViewport(canvas)->SetPendingSeriesInstanceUid(seriesInstanceUid);
            break;

          default:
            throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
        }

        GetViewport(canvas)->SetFrames(frames.release());
        return 1;
      }
      else
      {
        GetViewport(canvas)->ClearPendingSeriesInstanceUid();
        return 0;
      }
    }
    EXTERN_CATCH_EXCEPTIONS;
    return 0;
  }


  EMSCRIPTEN_KEEPALIVE
  int LoadVirtualSeriesInViewport(const char* canvas,
                                  const char* virtualSeriesId)
  {
    try
    {
      std::unique_ptr<IFramesCollection> frames(GetResourcesLoader().GetVirtualSeriesFrames(virtualSeriesId));

      if (frames.get() != NULL)
      {
        GetViewport(canvas)->SetFrames(frames.release());
        return 1;
      }
      else
      {
        return 0;
      }
    }
    EXTERN_CATCH_EXCEPTIONS;
    return 0;
  }


  EMSCRIPTEN_KEEPALIVE
  void AllViewportsUpdateSize(int fitContent)
  {
    try
    {
      for (Viewports::iterator it = allViewports_.begin(); it != allViewports_.end(); ++it)
      {
        assert(it->second != NULL);
        it->second->UpdateSize(fitContent != 0);
      }
    }
    EXTERN_CATCH_EXCEPTIONS;
  }


  EMSCRIPTEN_KEEPALIVE
  int DecrementFrame(const char* canvas,
                     int isCircular)
  {
    try
    {
      bool changed = GetViewport(canvas)->ChangeFrame(SeriesCursor::Action_Minus, isCircular);
      GetViewport(canvas)->SignalSynchronizedBrowsing();
      return changed ? 1 : 0;
    }
    EXTERN_CATCH_EXCEPTIONS;
    return 0;
  }


  EMSCRIPTEN_KEEPALIVE
  int IncrementFrame(const char* canvas,
                     int isCircular)
  {
    try
    {
      bool changed = GetViewport(canvas)->ChangeFrame(SeriesCursor::Action_Plus, isCircular);
      GetViewport(canvas)->SignalSynchronizedBrowsing();
      return changed ? 1 : 0;
    }
    EXTERN_CATCH_EXCEPTIONS;
    return 0;
  }  


  EMSCRIPTEN_KEEPALIVE
  void SetFrame(const char* canvas,
                int frameNumber)
  {
    try
    {
      if (frameNumber >= 0)
      {
        GetViewport(canvas)->SetFrame(static_cast<unsigned int>(frameNumber));
        GetViewport(canvas)->SignalSynchronizedBrowsing();
      }
    }
    EXTERN_CATCH_EXCEPTIONS;
  }


  EMSCRIPTEN_KEEPALIVE
  void GoToFirstFrame(const char* canvas)
  {
    try
    {
      GetViewport(canvas)->GoToFirstFrame();
      GetViewport(canvas)->SignalSynchronizedBrowsing();
    }
    EXTERN_CATCH_EXCEPTIONS;
  }


  EMSCRIPTEN_KEEPALIVE
  void GoToLastFrame(const char* canvas)
  {
    try
    {
      GetViewport(canvas)->GoToLastFrame();
      GetViewport(canvas)->SignalSynchronizedBrowsing();
    }
    EXTERN_CATCH_EXCEPTIONS;
  }


  EMSCRIPTEN_KEEPALIVE
  void ShowReferenceLines(int show)
  {
    try
    {
      showReferenceLines_ = (show != 0);
      UpdateReferenceLines();
    }
    EXTERN_CATCH_EXCEPTIONS;
  }  


  EMSCRIPTEN_KEEPALIVE
  void SetWindowing(const char* canvas,
                    int center,
                    int width)
  {
    try
    {
      GetViewport(canvas)->SetUserWindowing(OrthancStone::Windowing(center, width));
    }
    EXTERN_CATCH_EXCEPTIONS;
  }  


  EMSCRIPTEN_KEEPALIVE
  void StretchWindowing(const char* canvas)
  {
    try
    {
      GetViewport(canvas)->StretchWindowing();
    }
    EXTERN_CATCH_EXCEPTIONS;
  }


  EMSCRIPTEN_KEEPALIVE
  void InvertContrast(const char* canvas)
  {
    try
    {
      GetViewport(canvas)->Invert();
    }
    EXTERN_CATCH_EXCEPTIONS;
  }  


  EMSCRIPTEN_KEEPALIVE
  void FlipX(const char* canvas)
  {
    try
    {
      GetViewport(canvas)->FlipX();
    }
    EXTERN_CATCH_EXCEPTIONS;
  }  


  EMSCRIPTEN_KEEPALIVE
  void FlipY(const char* canvas)
  {
    try
    {
      GetViewport(canvas)->FlipY();
    }
    EXTERN_CATCH_EXCEPTIONS;
  }  
  

  EMSCRIPTEN_KEEPALIVE
  void RotateLeft(const char* canvas)
  {
    try
    {
      GetViewport(canvas)->RotateLeft();
    }
    EXTERN_CATCH_EXCEPTIONS;
  }  


  EMSCRIPTEN_KEEPALIVE
  void RotateRight(const char* canvas)
  {
    try
    {
      GetViewport(canvas)->RotateRight();
    }
    EXTERN_CATCH_EXCEPTIONS;
  }  
  

  EMSCRIPTEN_KEEPALIVE
  void SetSoftwareRendering(int softwareRendering)
  {
    softwareRendering_ = softwareRendering;
  }  


  EMSCRIPTEN_KEEPALIVE
  int IsSoftwareRendering()
  {
    return softwareRendering_;
  }  


  EMSCRIPTEN_KEEPALIVE
  void SetLinearInterpolation(int linearInterpolation)
  {
    linearInterpolation_ = linearInterpolation;

    try
    {
      for (Viewports::iterator it = allViewports_.begin(); it != allViewports_.end(); ++it)
      {
        assert(it->second != NULL);
        it->second->SetLinearInterpolation(linearInterpolation);
      }
    }
    EXTERN_CATCH_EXCEPTIONS;
  }  


  EMSCRIPTEN_KEEPALIVE
  void SetMouseButtonActions(int leftAction,
                             int middleAction,
                             int rightAction)
  {
    try
    {
      leftButtonAction_ = static_cast<WebViewerAction>(leftAction);
      middleButtonAction_ = static_cast<WebViewerAction>(middleAction);
      rightButtonAction_ = static_cast<WebViewerAction>(rightAction);
      
      for (Viewports::iterator it = allViewports_.begin(); it != allViewports_.end(); ++it)
      {
        assert(it->second != NULL);
        it->second->SetMouseButtonActions(leftButtonAction_, middleButtonAction_, rightButtonAction_);
      }
    }
    EXTERN_CATCH_EXCEPTIONS;
  }


  EMSCRIPTEN_KEEPALIVE
  int GetLeftMouseButtonAction()
  {
    return static_cast<int>(leftButtonAction_);
  }
  

  EMSCRIPTEN_KEEPALIVE
  int GetMiddleMouseButtonAction()
  {
    return static_cast<int>(middleButtonAction_);
  }
  

  EMSCRIPTEN_KEEPALIVE
  int GetRightMouseButtonAction()
  {
    return static_cast<int>(rightButtonAction_);
  }
  

  EMSCRIPTEN_KEEPALIVE
  void FitForPrint()
  {
    try
    {
      for (Viewports::iterator it = allViewports_.begin(); it != allViewports_.end(); ++it)
      {
        assert(it->second != NULL);
        it->second->FitForPrint();
      }
    }
    EXTERN_CATCH_EXCEPTIONS;
  }


  // Side-effect: "GetStringBuffer()" is filled with the "Series
  // Instance UID" of the first loaded annotation
  EMSCRIPTEN_KEEPALIVE
  int LoadOsiriXAnnotations(const void* xmlPointer,
                            int xmlSize,
                            int clearPreviousAnnotations)
  {
    try
    {
      if (clearPreviousAnnotations)
      {
        osiriXLayerSource_->GetAnnotations().Clear();
      }

      osiriXLayerSource_->GetAnnotations().LoadXml(reinterpret_cast<const char*>(xmlPointer), xmlSize);
      
      // Force redraw, as the annotations might have changed
      for (Viewports::iterator it = allViewports_.begin(); it != allViewports_.end(); ++it)
      {
        assert(it->second != NULL);
        it->second->Redraw();
      }

      if (osiriXLayerSource_->GetAnnotations().GetSize() == 0)
      {
        stringBuffer_.clear();
      }
      else
      {
        stringBuffer_ = osiriXLayerSource_->GetAnnotations().GetAnnotation(0).GetSeriesInstanceUid();
      }

      LOG(WARNING) << "Loaded " << osiriXLayerSource_->GetAnnotations().GetSize() << " annotations from OsiriX";
      return 1;
    }
    EXTERN_CATCH_EXCEPTIONS;
    return 0;
  }


  EMSCRIPTEN_KEEPALIVE
  void FocusFirstOsiriXAnnotation(const char* canvas)
  {
    try
    {
      if (osiriXLayerSource_->GetAnnotations().GetSize() != 0)
      {
        const OrthancStone::OsiriX::Annotation& annotation = osiriXLayerSource_->GetAnnotations().GetAnnotation(0);
        
        boost::shared_ptr<ViewerViewport> viewport = GetViewport(canvas);
        viewport->ScheduleFrameFocus(annotation.GetSopInstanceUid(), 0 /* focus on first frame */);

        // Force redraw, as the annotations might already have changed
        viewport->Redraw();
      }
    }
    EXTERN_CATCH_EXCEPTIONS;
  }


  EMSCRIPTEN_KEEPALIVE
  void FetchPdf(const char* studyInstanceUid,
                const char* seriesInstanceUid)
  {
    try
    {
      LOG(INFO) << "Fetching PDF series: " << seriesInstanceUid;
      GetResourcesLoader().FetchPdf(studyInstanceUid, seriesInstanceUid);
    }
    EXTERN_CATCH_EXCEPTIONS;
  }


  EMSCRIPTEN_KEEPALIVE
  unsigned int GetCineRate(const char* canvas)
  {
    try
    {
      return GetViewport(canvas)->GetCineRate();
    }
    EXTERN_CATCH_EXCEPTIONS;
    return 0;
  }


  EMSCRIPTEN_KEEPALIVE
  unsigned int GetSeriesNumberOfFrames(const char* seriesInstanceUid)
  {
    try
    {
      return GetResourcesLoader().GetSeriesNumberOfFrames(seriesInstanceUid);
    }
    EXTERN_CATCH_EXCEPTIONS;
    return 0;
  }


  EMSCRIPTEN_KEEPALIVE
  void LoadWindowingPresets(const char* canvas)
  {
    try
    {
      Json::Value v;
      GetViewport(canvas)->FormatWindowingPresets(v);
      stringBuffer_ = v.toStyledString();
    }
    EXTERN_CATCH_EXCEPTIONS;
  }


  EMSCRIPTEN_KEEPALIVE
  void SetSynchronizedBrowsingEnabled(int enabled)
  {
    try
    {
      for (Viewports::iterator it = allViewports_.begin(); it != allViewports_.end(); ++it)
      {
        assert(it->second != NULL);
        it->second->SetSynchronizedBrowsingEnabled(enabled);
      }
    }
    EXTERN_CATCH_EXCEPTIONS;
  }


  EMSCRIPTEN_KEEPALIVE
  int LookupVirtualSeries(const char* seriesInstanceUid)
  {
    try
    {
      typedef std::map<std::string, unsigned int>  VirtualSeries;

      VirtualSeries virtualSeries;
      if (GetResourcesLoader().LookupVirtualSeries(virtualSeries, seriesInstanceUid))
      {
        Json::Value json = Json::arrayValue;
        for (VirtualSeries::const_iterator it = virtualSeries.begin(); it != virtualSeries.end(); ++it)
        {
          Json::Value item = Json::objectValue;
          item["ID"] = it->first;
          item["NumberOfFrames"] = it->second;
          json.append(item);
        }

        stringBuffer_ = json.toStyledString();
        return true;
      }
      else
      {
        return false;
      }
    }
    EXTERN_CATCH_EXCEPTIONS;
    return false;
  }


  EMSCRIPTEN_KEEPALIVE
  void AddTextAnnotation(const char* canvas,
                         const char* label,
                         double pointedX,
                         double pointedY,
                         double labelX,
                         double labelY)
  {
    try
    {
      GetViewport(canvas)->AddTextAnnotation(label, OrthancStone::ScenePoint2D(pointedX, pointedY),
                                             OrthancStone::ScenePoint2D(labelX, labelY));
    }
    EXTERN_CATCH_EXCEPTIONS;
  }


  EMSCRIPTEN_KEEPALIVE
  void SetAnnotationsColor(int red,
                           int green,
                           int blue)
  {
    try
    {
      OrthancStone::Color color(red, green, blue);
      SetAnnotationsColor(color);
    }
    EXTERN_CATCH_EXCEPTIONS;
  }


  EMSCRIPTEN_KEEPALIVE
  void SetHighlightedAnnotationsColor(int red,
                                      int green,
                                      int blue)
  {
    try
    {
      OrthancStone::Color color(red, green, blue);
      SetHighlightedColor(color);
    }
    EXTERN_CATCH_EXCEPTIONS;
  }


  EMSCRIPTEN_KEEPALIVE
  void *Allocate(size_t size)
  {
    return malloc(size);
  }


  EMSCRIPTEN_KEEPALIVE
  void Deallocate(void* ptr)
  {
    free(ptr);
  }
}
