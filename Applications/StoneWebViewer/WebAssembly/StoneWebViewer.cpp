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


#include <emscripten.h>


#define DISPATCH_JAVASCRIPT_EVENT(name)                         \
  EM_ASM(                                                       \
    const customEvent = document.createEvent("CustomEvent");    \
    customEvent.initCustomEvent(name, false, false, undefined); \
    window.dispatchEvent(customEvent);                          \
    );


#define EXTERN_CATCH_EXCEPTIONS                         \
  catch (Orthanc::OrthancException& e)                  \
  {                                                     \
    LOG(ERROR) << "OrthancException: " << e.What();     \
    DISPATCH_JAVASCRIPT_EVENT("StoneException");        \
  }                                                     \
  catch (OrthancStone::StoneException& e)               \
  {                                                     \
    LOG(ERROR) << "StoneException: " << e.What();       \
    DISPATCH_JAVASCRIPT_EVENT("StoneException");        \
  }                                                     \
  catch (std::exception& e)                             \
  {                                                     \
    LOG(ERROR) << "Runtime error: " << e.what();        \
    DISPATCH_JAVASCRIPT_EVENT("StoneException");        \
  }                                                     \
  catch (...)                                           \
  {                                                     \
    LOG(ERROR) << "Native exception";                   \
    DISPATCH_JAVASCRIPT_EVENT("StoneException");        \
  }


// Orthanc framework includes
#include <Cache/MemoryObjectCache.h>
#include <DicomFormat/DicomArray.h>
#include <DicomParsing/ParsedDicomFile.h>
#include <Images/Image.h>
#include <Images/ImageProcessing.h>
#include <Images/JpegReader.h>
#include <Logging.h>

// Stone includes
#include <Loaders/DicomResourcesLoader.h>
#include <Loaders/SeriesMetadataLoader.h>
#include <Loaders/SeriesThumbnailsLoader.h>
#include <Loaders/WebAssemblyLoadersContext.h>
#include <Messages/ObserverBase.h>
#include <Oracle/ParseDicomFromWadoCommand.h>
#include <Oracle/ParseDicomSuccessMessage.h>
#include <Scene2D/ColorTextureSceneLayer.h>
#include <Scene2D/FloatTextureSceneLayer.h>
#include <Scene2D/PolylineSceneLayer.h>
#include <Scene2DViewport/ViewportController.h>
#include <StoneException.h>
#include <Toolbox/DicomInstanceParameters.h>
#include <Toolbox/GeometryToolbox.h>
#include <Toolbox/SortedFrames.h>
#include <Viewport/DefaultViewportInteractor.h>
#include <Viewport/WebAssemblyCairoViewport.h>
#include <Viewport/WebGLViewport.h>

#include <boost/make_shared.hpp>
#include <stdio.h>


enum EMSCRIPTEN_KEEPALIVE ThumbnailType
{
  ThumbnailType_Image,
    ThumbnailType_NoPreview,
    ThumbnailType_Pdf,
    ThumbnailType_Video,
    ThumbnailType_Loading,
    ThumbnailType_Unknown
};


enum EMSCRIPTEN_KEEPALIVE DisplayedFrameQuality
{
  DisplayedFrameQuality_None,
    DisplayedFrameQuality_Low,
    DisplayedFrameQuality_High
    };


enum EMSCRIPTEN_KEEPALIVE MouseAction
{
  MouseAction_GrayscaleWindowing,
    MouseAction_Zoom,
    MouseAction_Pan,
    MouseAction_Rotate
    };
  


static const int PRIORITY_HIGH = -100;
static const int PRIORITY_LOW = 100;
static const int PRIORITY_NORMAL = 0;

static const unsigned int QUALITY_JPEG = 0;
static const unsigned int QUALITY_FULL = 1;

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
  };
  
private:
  std::unique_ptr<IObserver>                               observer_;
  OrthancStone::DicomSource                                source_;
  size_t                                                   pending_;
  boost::shared_ptr<OrthancStone::LoadedDicomResources>    studies_;
  boost::shared_ptr<OrthancStone::LoadedDicomResources>    series_;
  boost::shared_ptr<OrthancStone::DicomResourcesLoader>    resourcesLoader_;
  boost::shared_ptr<OrthancStone::SeriesThumbnailsLoader>  thumbnailsLoader_;
  boost::shared_ptr<OrthancStone::SeriesMetadataLoader>    metadataLoader_;

  explicit ResourcesLoader(const OrthancStone::DicomSource& source) :
    source_(source),
    pending_(0),
    studies_(new OrthancStone::LoadedDicomResources(Orthanc::DICOM_TAG_STUDY_INSTANCE_UID)),
    series_(new OrthancStone::LoadedDicomResources(Orthanc::DICOM_TAG_SERIES_INSTANCE_UID))
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
      for (size_t i = 0; i < dicom.GetSize(); i++)
      {
        std::string studyInstanceUid, seriesInstanceUid;
        if (dicom.GetResource(i).LookupStringValue(
              studyInstanceUid, Orthanc::DICOM_TAG_STUDY_INSTANCE_UID, false) &&
            dicom.GetResource(i).LookupStringValue(
              seriesInstanceUid, Orthanc::DICOM_TAG_SERIES_INSTANCE_UID, false))
        {
          thumbnailsLoader_->ScheduleLoadThumbnail(source_, "", studyInstanceUid, seriesInstanceUid);
          metadataLoader_->ScheduleLoadSeries(PRIORITY_LOW + 1, source_, studyInstanceUid, seriesInstanceUid);
        }
      }
    }

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
    if (observer_.get() != NULL)
    {
      observer_->SignalSeriesMetadataLoaded(
        message.GetStudyInstanceUid(), message.GetSeriesInstanceUid());
    }
  }

  void FetchInternal(const std::string& studyInstanceUid,
                     const std::string& seriesInstanceUid)
  {
    // Firstly, load the study
    Orthanc::DicomMap filter;
    filter.SetValue(Orthanc::DICOM_TAG_STUDY_INSTANCE_UID, studyInstanceUid, false);

    std::set<Orthanc::DicomTag> tags;
    tags.insert(Orthanc::DICOM_TAG_STUDY_DESCRIPTION);  // Necessary for Orthanc DICOMweb plugin

    resourcesLoader_->ScheduleQido(
      studies_, PRIORITY_HIGH, source_, Orthanc::ResourceType_Study, filter, tags,
      new Orthanc::SingleValueObject<Orthanc::ResourceType>(Orthanc::ResourceType_Study));

    // Secondly, load the series
    if (!seriesInstanceUid.empty())
    {
      filter.SetValue(Orthanc::DICOM_TAG_SERIES_INSTANCE_UID, seriesInstanceUid, false);
    }
    
    tags.insert(Orthanc::DICOM_TAG_SERIES_NUMBER);  // Necessary for Google Cloud Platform
    
    resourcesLoader_->ScheduleQido(
      series_, PRIORITY_HIGH, source_, Orthanc::ResourceType_Series, filter, tags,
      new Orthanc::SingleValueObject<Orthanc::ResourceType>(Orthanc::ResourceType_Series));

    pending_ += 2;
  }

public:
  static boost::shared_ptr<ResourcesLoader> Create(OrthancStone::ILoadersContext::ILock& lock,
                                                   const OrthancStone::DicomSource& source)
  {
    boost::shared_ptr<ResourcesLoader> loader(new ResourcesLoader(source));

    loader->resourcesLoader_ = OrthancStone::DicomResourcesLoader::Create(lock);
    loader->thumbnailsLoader_ = OrthancStone::SeriesThumbnailsLoader::Create(lock, PRIORITY_LOW);
    loader->metadataLoader_ = OrthancStone::SeriesMetadataLoader::Create(lock);
    
    loader->Register<OrthancStone::DicomResourcesLoader::SuccessMessage>(
      *loader->resourcesLoader_, &ResourcesLoader::Handle);

    loader->Register<OrthancStone::SeriesThumbnailsLoader::SuccessMessage>(
      *loader->thumbnailsLoader_, &ResourcesLoader::Handle);

    loader->Register<OrthancStone::SeriesMetadataLoader::SuccessMessage>(
      *loader->metadataLoader_, &ResourcesLoader::Handle);
    
    return loader;
  }
  
  void FetchAllStudies()
  {
    FetchInternal("", "");
  }
  
  void FetchStudy(const std::string& studyInstanceUid)
  {
    FetchInternal(studyInstanceUid, "");
  }
  
  void FetchSeries(const std::string& studyInstanceUid,
                   const std::string& seriesInstanceUid)
  {
    FetchInternal(studyInstanceUid, seriesInstanceUid);
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
                size_t i)
  {
    target.Assign(studies_->GetResource(i));
  }

  void GetSeries(Orthanc::DicomMap& target,
                 size_t i)
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
                                                       const std::string& seriesInstanceUid)
  {
    return thumbnailsLoader_->GetSeriesThumbnail(image, mime, seriesInstanceUid);
  }

  void FetchSeriesMetadata(int priority,
                           const std::string& studyInstanceUid,
                           const std::string& seriesInstanceUid)
  {
    metadataLoader_->ScheduleLoadSeries(priority, source_, studyInstanceUid, seriesInstanceUid);
  }

  bool IsSeriesComplete(const std::string& seriesInstanceUid)
  {
    OrthancStone::SeriesMetadataLoader::Accessor accessor(*metadataLoader_, seriesInstanceUid);
    return accessor.IsComplete();
  }

  bool SortSeriesFrames(OrthancStone::SortedFrames& target,
                        const std::string& seriesInstanceUid)
  {
    OrthancStone::SeriesMetadataLoader::Accessor accessor(*metadataLoader_, seriesInstanceUid);
    
    if (accessor.IsComplete())
    {
      target.Clear();

      for (size_t i = 0; i < accessor.GetInstancesCount(); i++)
      {
        target.AddInstance(accessor.GetInstance(i));
      }

      target.Sort();
      
      return true;
    }
    else
    {
      return false;
    }
  }

  void AcquireObserver(IObserver* observer)
  {  
    observer_.reset(observer);
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
                            size_t frameIndex)
  {
    return sopInstanceUid + "|" + boost::lexical_cast<std::string>(frameIndex);
  }
  

  Orthanc::MemoryObjectCache  cache_;
  
public:
  FramesCache()
  {
    SetMaximumSize(100 * 1024 * 1024);  // 100 MB
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
               size_t frameIndex,
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

    const std::string& key = GetKey(sopInstanceUid, frameIndex);

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
             size_t frameIndex) :
      accessor_(that.cache_, GetKey(sopInstanceUid, frameIndex), false /* shared lock */)
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
  bool                 isCircular_;
  int                  fastDelta_;
  Action               lastAction_;

  int ComputeNextFrame(int currentFrame,
                       Action action) const
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

    if (isCircular_)
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
            queue.push_back(std::make_pair(ComputeNextFrame(frame, Action_Plus), Action_Plus));
            queue.push_back(std::make_pair(ComputeNextFrame(frame, Action_Minus), Action_Minus));
            queue.push_back(std::make_pair(ComputeNextFrame(frame, Action_FastPlus), Action_FastPlus));
            queue.push_back(std::make_pair(ComputeNextFrame(frame, Action_FastMinus), Action_FastMinus));
            break;
          
          case Action_Minus:
            queue.push_back(std::make_pair(ComputeNextFrame(frame, Action_Minus), Action_Minus));
            queue.push_back(std::make_pair(ComputeNextFrame(frame, Action_Plus), Action_Plus));
            queue.push_back(std::make_pair(ComputeNextFrame(frame, Action_FastMinus), Action_FastMinus));
            queue.push_back(std::make_pair(ComputeNextFrame(frame, Action_FastPlus), Action_FastPlus));
            break;

          case Action_FastPlus:
            queue.push_back(std::make_pair(ComputeNextFrame(frame, Action_FastPlus), Action_FastPlus));
            queue.push_back(std::make_pair(ComputeNextFrame(frame, Action_FastMinus), Action_FastMinus));
            queue.push_back(std::make_pair(ComputeNextFrame(frame, Action_Plus), Action_Plus));
            queue.push_back(std::make_pair(ComputeNextFrame(frame, Action_Minus), Action_Minus));
            break;
              
          case Action_FastMinus:
            queue.push_back(std::make_pair(ComputeNextFrame(frame, Action_FastMinus), Action_FastMinus));
            queue.push_back(std::make_pair(ComputeNextFrame(frame, Action_FastPlus), Action_FastPlus));
            queue.push_back(std::make_pair(ComputeNextFrame(frame, Action_Minus), Action_Minus));
            queue.push_back(std::make_pair(ComputeNextFrame(frame, Action_Plus), Action_Plus));
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
  explicit SeriesCursor(size_t framesCount) :
    framesCount_(framesCount),
    currentFrame_(framesCount / 2),  // Start at the middle frame    
    isCircular_(false),
    lastAction_(Action_None)
  {
    SetFastDelta(framesCount / 20);
    UpdatePrefetch();
  }

  void SetCircular(bool isCircular)
  {
    isCircular_ = isCircular;
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

  void Apply(Action action)
  {
    currentFrame_ = ComputeNextFrame(currentFrame_, action);
    lastAction_ = action;
    UpdatePrefetch();
  }

  size_t GetPrefetchSize() const
  {
    assert(prefetch_.size() == framesCount_);
    return prefetch_.size();
  }

  size_t GetPrefetchFrameIndex(size_t i) const
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




class FrameGeometry
{
private:
  bool                              isValid_;
  std::string                       frameOfReferenceUid_;
  OrthancStone::CoordinateSystem3D  coordinates_;
  double                            pixelSpacingX_;
  double                            pixelSpacingY_;
  OrthancStone::Extent2D            extent_;

public:
  explicit FrameGeometry() :
    isValid_(false),
    pixelSpacingX_(1),
    pixelSpacingY_(1)
  {
  }
    
  explicit FrameGeometry(const Orthanc::DicomMap& tags) :
    isValid_(false),
    coordinates_(tags)
  {
    if (!tags.LookupStringValue(
          frameOfReferenceUid_, Orthanc::DICOM_TAG_FRAME_OF_REFERENCE_UID, false))
    {
      frameOfReferenceUid_.clear();
    }

    OrthancStone::GeometryToolbox::GetPixelSpacing(pixelSpacingX_, pixelSpacingY_, tags);

    unsigned int rows, columns;
    if (tags.HasTag(Orthanc::DICOM_TAG_IMAGE_POSITION_PATIENT) &&
        tags.HasTag(Orthanc::DICOM_TAG_IMAGE_ORIENTATION_PATIENT) &&
        tags.ParseUnsignedInteger32(rows, Orthanc::DICOM_TAG_ROWS) &&
        tags.ParseUnsignedInteger32(columns, Orthanc::DICOM_TAG_COLUMNS))
    {
      double ox = -pixelSpacingX_ / 2.0;
      double oy = -pixelSpacingY_ / 2.0;
      extent_.AddPoint(ox, oy);
      extent_.AddPoint(ox + pixelSpacingX_ * static_cast<double>(columns),
                       oy + pixelSpacingY_ * static_cast<double>(rows));

      isValid_ = true;
    }
  }

  bool IsValid() const
  {
    return isValid_;
  }

  const std::string& GetFrameOfReferenceUid() const
  {
    if (isValid_)
    {
      return frameOfReferenceUid_;
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }

  const OrthancStone::CoordinateSystem3D& GetCoordinates() const
  {
    if (isValid_)
    {
      return coordinates_;
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }

  double GetPixelSpacingX() const
  {
    if (isValid_)
    {
      return pixelSpacingX_;
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }

  double GetPixelSpacingY() const
  {
    if (isValid_)
    {
      return pixelSpacingY_;
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }

  bool Intersect(double& x1,  // Coordinates of the clipped line (out)
                 double& y1,
                 double& x2,
                 double& y2,
                 const FrameGeometry& other) const
  {
    if (this == &other)
    {
      return false;
    }
    
    OrthancStone::Vector direction, origin;
        
    if (IsValid() &&
        other.IsValid() &&
        !extent_.IsEmpty() &&
        frameOfReferenceUid_ == other.frameOfReferenceUid_ &&
        OrthancStone::GeometryToolbox::IntersectTwoPlanes(
          origin, direction,
          coordinates_.GetOrigin(), coordinates_.GetNormal(),
          other.coordinates_.GetOrigin(), other.coordinates_.GetNormal()))
    {
      double ax, ay, bx, by;
      coordinates_.ProjectPoint(ax, ay, origin);
      coordinates_.ProjectPoint(bx, by, origin + 100.0 * direction);
      
      return OrthancStone::GeometryToolbox::ClipLineToRectangle(
        x1, y1, x2, y2,
        ax, ay, bx, by,
        extent_.GetX1(), extent_.GetY1(), extent_.GetX2(), extent_.GetY2());
    }
    else
    {
      return false;
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

    virtual void SignalFrameUpdated(const ViewerViewport& viewport,
                                    size_t currentFrame,
                                    size_t countFrames,
                                    DisplayedFrameQuality quality) = 0;
  };

private:
  static const int LAYER_TEXTURE = 0;
  static const int LAYER_REFERENCE_LINES = 1;
  
  
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

  class SetDefaultWindowingCommand : public ICommand
  {
  public:
    explicit SetDefaultWindowingCommand(boost::shared_ptr<ViewerViewport> viewport) :
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

        if (params.HasDefaultWindowing())
        {
          GetViewport().defaultWindowingCenter_ = params.GetDefaultWindowingCenter();
          GetViewport().defaultWindowingWidth_ = params.GetDefaultWindowingWidth();
          LOG(INFO) << "Default windowing: " << params.GetDefaultWindowingCenter()
                    << "," << params.GetDefaultWindowingWidth();

          GetViewport().windowingCenter_ = params.GetDefaultWindowingCenter();
          GetViewport().windowingWidth_ = params.GetDefaultWindowingWidth();
        }
        else
        {
          LOG(INFO) << "No default windowing";
          GetViewport().ResetDefaultWindowing();
        }
      }

      GetViewport().DisplayCurrentFrame();
    }
  };

  class SetLowQualityFrame : public ICommand
  {
  private:
    std::string   sopInstanceUid_;
    unsigned int  frameIndex_;
    float         windowCenter_;
    float         windowWidth_;
    bool          isMonochrome1_;
    bool          isPrefetch_;
    
  public:
    SetLowQualityFrame(boost::shared_ptr<ViewerViewport> viewport,
                       const std::string& sopInstanceUid,
                       unsigned int frameIndex,
                       float windowCenter,
                       float windowWidth,
                       bool isMonochrome1,
                       bool isPrefetch) :
      ICommand(viewport),
      sopInstanceUid_(sopInstanceUid),
      frameIndex_(frameIndex),
      windowCenter_(windowCenter),
      windowWidth_(windowWidth),
      isMonochrome1_(isMonochrome1),
      isPrefetch_(isPrefetch)
    {
    }
    
    virtual void Handle(const OrthancStone::HttpCommand::SuccessMessage& message) const ORTHANC_OVERRIDE
    {
      std::unique_ptr<Orthanc::JpegReader> jpeg(new Orthanc::JpegReader);
      jpeg->ReadFromMemory(message.GetAnswer());

      bool updatedCache;
      
      switch (jpeg->GetFormat())
      {
        case Orthanc::PixelFormat_RGB24:
          updatedCache = GetViewport().cache_->Acquire(
            sopInstanceUid_, frameIndex_, jpeg.release(), QUALITY_JPEG);
          break;

        case Orthanc::PixelFormat_Grayscale8:
        {
          if (isMonochrome1_)
          {
            Orthanc::ImageProcessing::Invert(*jpeg);
          }

          std::unique_ptr<Orthanc::Image> converted(
            new Orthanc::Image(Orthanc::PixelFormat_Float32, jpeg->GetWidth(),
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

          const float scaling = windowWidth_ / 255.0f;
          const float offset = (OrthancStone::LinearAlgebra::IsCloseToZero(scaling) ? 0 :
                                (windowCenter_ - windowWidth_ / 2.0f) / scaling);

          Orthanc::ImageProcessing::ShiftScale(*converted, offset, scaling, false);
          updatedCache = GetViewport().cache_->Acquire(
            sopInstanceUid_, frameIndex_, converted.release(), QUALITY_JPEG);
          break;
        }

        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
      }

      if (updatedCache)
      {
        GetViewport().SignalUpdatedFrame(sopInstanceUid_, frameIndex_);
      }

      if (isPrefetch_)
      {
        GetViewport().ScheduleNextPrefetch();
      }
    }
  };


  class SetFullDicomFrame : public ICommand
  {
  private:
    std::string   sopInstanceUid_;
    unsigned int  frameIndex_;
    bool          isPrefetch_;
    
  public:
    SetFullDicomFrame(boost::shared_ptr<ViewerViewport> viewport,
                      const std::string& sopInstanceUid,
                      unsigned int frameIndex,
                      bool isPrefetch) :
      ICommand(viewport),
      sopInstanceUid_(sopInstanceUid),
      frameIndex_(frameIndex),
      isPrefetch_(isPrefetch)
    {
    }
    
    virtual void Handle(const OrthancStone::ParseDicomSuccessMessage& message) const ORTHANC_OVERRIDE
    {
      Orthanc::DicomMap tags;
      message.GetDicom().ExtractDicomSummary(tags, ORTHANC_STONE_MAX_TAG_LENGTH);

      std::string s;
      if (!tags.LookupStringValue(s, Orthanc::DICOM_TAG_SOP_INSTANCE_UID, false))
      {
        // Safety check
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }      
      
      std::unique_ptr<Orthanc::ImageAccessor> frame(message.GetDicom().DecodeFrame(frameIndex_));

      if (frame.get() == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }

      bool updatedCache;

      if (frame->GetFormat() == Orthanc::PixelFormat_RGB24)
      {
        updatedCache = GetViewport().cache_->Acquire(
          sopInstanceUid_, frameIndex_, frame.release(), QUALITY_FULL);
      }
      else
      {
        double a = 1;
        double b = 0;

        double doseScaling;
        if (tags.ParseDouble(doseScaling, Orthanc::DICOM_TAG_DOSE_GRID_SCALING))
        {
          a = doseScaling;
        }
      
        double rescaleIntercept, rescaleSlope;
        if (tags.ParseDouble(rescaleIntercept, Orthanc::DICOM_TAG_RESCALE_INTERCEPT) &&
            tags.ParseDouble(rescaleSlope, Orthanc::DICOM_TAG_RESCALE_SLOPE))
        {
          a *= rescaleSlope;
          b = rescaleIntercept;
        }

        std::unique_ptr<Orthanc::ImageAccessor> converted(
          new Orthanc::Image(Orthanc::PixelFormat_Float32, frame->GetWidth(), frame->GetHeight(), false));
        Orthanc::ImageProcessing::Convert(*converted, *frame);
        Orthanc::ImageProcessing::ShiftScale2(*converted, b, a, false);

        updatedCache = GetViewport().cache_->Acquire(
          sopInstanceUid_, frameIndex_, converted.release(), QUALITY_FULL);
      }
      
      if (updatedCache)
      {
        GetViewport().SignalUpdatedFrame(sopInstanceUid_, frameIndex_);
      }

      if (isPrefetch_)
      {
        GetViewport().ScheduleNextPrefetch();
      }
    }
  };


  class PrefetchItem
  {
  private:
    size_t   frameIndex_;
    bool     isFull_;

  public:
    PrefetchItem(size_t frameIndex,
                 bool isFull) :
      frameIndex_(frameIndex),
      isFull_(isFull)
    {
    }

    size_t GetFrameIndex() const
    {
      return frameIndex_;
    }

    bool IsFull() const
    {
      return isFull_;
    }
  };
  

  std::unique_ptr<IObserver>                    observer_;
  OrthancStone::ILoadersContext&               context_;
  boost::shared_ptr<OrthancStone::WebAssemblyViewport>   viewport_;
  boost::shared_ptr<OrthancStone::DicomResourcesLoader> loader_;
  OrthancStone::DicomSource                    source_;
  boost::shared_ptr<FramesCache>               cache_;  
  std::unique_ptr<OrthancStone::SortedFrames>  frames_;
  std::unique_ptr<SeriesCursor>                cursor_;
  float                                        windowingCenter_;
  float                                        windowingWidth_;
  float                                        defaultWindowingCenter_;
  float                                        defaultWindowingWidth_;
  bool                                         inverted_;
  bool                                         flipX_;
  bool                                         flipY_;
  bool                                         fitNextContent_;
  bool                                         isCtrlDown_;
  FrameGeometry                                currentFrameGeometry_;
  std::list<PrefetchItem>                      prefetchQueue_;

  void ScheduleNextPrefetch()
  {
    while (!prefetchQueue_.empty())
    {
      size_t index = prefetchQueue_.front().GetFrameIndex();
      bool isFull = prefetchQueue_.front().IsFull();
      prefetchQueue_.pop_front();
      
      const std::string sopInstanceUid = frames_->GetFrameSopInstanceUid(index);
      unsigned int frame = frames_->GetFrameIndex(index);

      {
        FramesCache::Accessor accessor(*cache_, sopInstanceUid, frame);
        if (!accessor.IsValid() ||
            (isFull && accessor.GetQuality() == 0))
        {
          if (isFull)
          {
            ScheduleLoadFullDicomFrame(index, PRIORITY_NORMAL, true);
          }
          else
          {
            ScheduleLoadRenderedFrame(index, PRIORITY_NORMAL, true);
          }
          return;
        }
      }
    }
  }
  
  
  void ResetDefaultWindowing()
  {
    defaultWindowingCenter_ = 128;
    defaultWindowingWidth_ = 256;

    windowingCenter_ = defaultWindowingCenter_;
    windowingWidth_ = defaultWindowingWidth_;

    inverted_ = false;
  }

  void SignalUpdatedFrame(const std::string& sopInstanceUid,
                          unsigned int frameIndex)
  {
    if (cursor_.get() != NULL &&
        frames_.get() != NULL)
    {
      size_t index = cursor_->GetCurrentIndex();

      if (frames_->GetFrameSopInstanceUid(index) == sopInstanceUid &&
          frames_->GetFrameIndex(index) == frameIndex)
      {
        DisplayCurrentFrame();
      }
    }
  }

  void DisplayCurrentFrame()
  {
    DisplayedFrameQuality quality = DisplayedFrameQuality_None;
    
    if (cursor_.get() != NULL &&
        frames_.get() != NULL)
    {
      const size_t index = cursor_->GetCurrentIndex();
      
      unsigned int cachedQuality;
      if (!DisplayFrame(cachedQuality, index))
      {
        // This frame is not cached yet: Load it
        if (source_.HasDicomWebRendered())
        {
          ScheduleLoadRenderedFrame(index, PRIORITY_HIGH, false /* not a prefetch */);
        }
        else
        {
          ScheduleLoadFullDicomFrame(index, PRIORITY_HIGH, false /* not a prefetch */);
        }
      }
      else if (cachedQuality < QUALITY_FULL)
      {
        // This frame is only available in low-res: Download the full DICOM
        ScheduleLoadFullDicomFrame(index, PRIORITY_HIGH, false /* not a prefetch */);
        quality = DisplayedFrameQuality_Low;
      }
      else
      {
        quality = DisplayedFrameQuality_High;
      }

      currentFrameGeometry_ = FrameGeometry(frames_->GetFrameTags(index));

      {
        // Prepare prefetching
        prefetchQueue_.clear();
        for (size_t i = 0; i < cursor_->GetPrefetchSize() && i < 16; i++)
        {
          size_t a = cursor_->GetPrefetchFrameIndex(i);
          if (a != index)
          {
            prefetchQueue_.push_back(PrefetchItem(a, i < 2));
          }
        }

        ScheduleNextPrefetch();
      }      

      if (observer_.get() != NULL)
      {
        observer_->SignalFrameUpdated(*this, cursor_->GetCurrentIndex(),
                                      frames_->GetFramesCount(), quality);
      }
    }
    else
    {
      currentFrameGeometry_ = FrameGeometry();
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


  void SaveCurrentWindowing()
  {
    // Save the current windowing (that could have been altered by
    // GrayscaleWindowingSceneTracker), so that it can be reused
    // by the next frames

    std::unique_ptr<OrthancStone::IViewport::ILock> lock(viewport_->Lock());

    OrthancStone::Scene2D& scene = lock->GetController().GetScene();

    if (scene.HasLayer(LAYER_TEXTURE) &&
        scene.GetLayer(LAYER_TEXTURE).GetType() == OrthancStone::ISceneLayer::Type_FloatTexture)
    {
      OrthancStone::FloatTextureSceneLayer& layer =
        dynamic_cast<OrthancStone::FloatTextureSceneLayer&>(scene.GetLayer(LAYER_TEXTURE));
      layer.GetWindowing(windowingCenter_, windowingWidth_);
    }
  }
  

  bool DisplayFrame(unsigned int& quality,
                    size_t index)
  {
    if (frames_.get() == NULL)
    {
      return false;
    }

    const std::string sopInstanceUid = frames_->GetFrameSopInstanceUid(index);
    unsigned int frame = frames_->GetFrameIndex(index);

    FramesCache::Accessor accessor(*cache_, sopInstanceUid, frame);
    if (accessor.IsValid())
    {
      SaveCurrentWindowing();
      
      quality = accessor.GetQuality();

      std::unique_ptr<OrthancStone::TextureBaseSceneLayer> layer;

      switch (accessor.GetImage().GetFormat())
      {
        case Orthanc::PixelFormat_RGB24:
          layer.reset(new OrthancStone::ColorTextureSceneLayer(accessor.GetImage()));
          break;

        case Orthanc::PixelFormat_Float32:
        {
          std::unique_ptr<OrthancStone::FloatTextureSceneLayer> tmp(
            new OrthancStone::FloatTextureSceneLayer(accessor.GetImage()));
          tmp->SetCustomWindowing(windowingCenter_, windowingWidth_);
          tmp->SetInverted(inverted_ ^ frames_->IsFrameMonochrome1(index));
          layer.reset(tmp.release());
          break;
        }

        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageFormat);
      }

      layer->SetLinearInterpolation(true);
      layer->SetFlipX(flipX_);
      layer->SetFlipY(flipY_);

      double pixelSpacingX, pixelSpacingY;
      OrthancStone::GeometryToolbox::GetPixelSpacing(
        pixelSpacingX, pixelSpacingY, frames_->GetFrameTags(index));
      layer->SetPixelSpacing(pixelSpacingX, pixelSpacingY);

      if (layer.get() == NULL)
      {
        return false;
      }
      else
      {
        std::unique_ptr<OrthancStone::IViewport::ILock> lock(viewport_->Lock());

        OrthancStone::Scene2D& scene = lock->GetController().GetScene();

        scene.SetLayer(LAYER_TEXTURE, layer.release());

        if (fitNextContent_)
        {
          lock->GetCompositor().RefreshCanvasSize();
          lock->GetCompositor().FitContent(scene);
          fitNextContent_ = false;
        }
        
        //lock->GetCompositor().Refresh(scene);
        lock->Invalidate();
        return true;
      }
    }
    else
    {
      return false;
    }
  }

  void ScheduleLoadFullDicomFrame(size_t index,
                                  int priority,
                                  bool isPrefetch)
  {
    if (frames_.get() != NULL)
    {
      std::string sopInstanceUid = frames_->GetFrameSopInstanceUid(index);
      unsigned int frame = frames_->GetFrameIndex(index);
      
      {
        std::unique_ptr<OrthancStone::ILoadersContext::ILock> lock(context_.Lock());
        lock->Schedule(
          GetSharedObserver(), priority, OrthancStone::ParseDicomFromWadoCommand::Create(
            source_, frames_->GetStudyInstanceUid(), frames_->GetSeriesInstanceUid(),
            sopInstanceUid, false /* transcoding (TODO) */,
            Orthanc::DicomTransferSyntax_LittleEndianExplicit /* TODO */,
            new SetFullDicomFrame(GetSharedObserver(), sopInstanceUid, frame, isPrefetch)));
      }
    }
  }

  void ScheduleLoadRenderedFrame(size_t index,
                                 int priority,
                                 bool isPrefetch)
  {
    if (!source_.HasDicomWebRendered())
    {
      ScheduleLoadFullDicomFrame(index, priority, isPrefetch);
    }
    else if (frames_.get() != NULL)
    {
      std::string sopInstanceUid = frames_->GetFrameSopInstanceUid(index);
      unsigned int frame = frames_->GetFrameIndex(index);
      bool isMonochrome1 = frames_->IsFrameMonochrome1(index);

      const std::string uri = ("studies/" + frames_->GetStudyInstanceUid() +
                               "/series/" + frames_->GetSeriesInstanceUid() +
                               "/instances/" + sopInstanceUid +
                               "/frames/" + boost::lexical_cast<std::string>(frame + 1) + "/rendered");

      std::map<std::string, std::string> headers, arguments;
      arguments["window"] = (
        boost::lexical_cast<std::string>(windowingCenter_) + ","  +
        boost::lexical_cast<std::string>(windowingWidth_) + ",linear");

      std::unique_ptr<OrthancStone::IOracleCommand> command(
        source_.CreateDicomWebCommand(
          uri, arguments, headers, new SetLowQualityFrame(
            GetSharedObserver(), sopInstanceUid, frame,
            windowingCenter_, windowingWidth_, isMonochrome1, isPrefetch)));

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
          SetCustomWindowing(windowingCenter_, windowingWidth_);
      }

      {
        OrthancStone::TextureBaseSceneLayer& layer = 
          dynamic_cast<OrthancStone::TextureBaseSceneLayer&>(
            lock->GetController().GetScene().GetLayer(LAYER_TEXTURE));

        layer.SetFlipX(flipX_);
        layer.SetFlipY(flipY_);
      }
        
      lock->Invalidate();
    }
  }
  

  ViewerViewport(OrthancStone::ILoadersContext& context,
                 const OrthancStone::DicomSource& source,
                 const std::string& canvas,
                 boost::shared_ptr<FramesCache> cache,
                 bool softwareRendering) :
    context_(context),
    source_(source),
    cache_(cache),
    fitNextContent_(true),
    isCtrlDown_(false),
    flipX_(false),
    flipY_(false)
  {
    if (!cache_)
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
    
    emscripten_set_wheel_callback(viewport_->GetCanvasCssSelector().c_str(), this, true, OnWheel);
    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, false, OnKey);
    emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, false, OnKey);

    ResetDefaultWindowing();

    /*{
      std::unique_ptr<OrthancStone::IViewport::ILock> lock(viewport_->Lock());
      std::unique_ptr<OrthancStone::PolylineSceneLayer> layer(new OrthancStone::PolylineSceneLayer);
      OrthancStone::PolylineSceneLayer::Chain chain;
      chain.push_back(OrthancStone::ScenePoint2D(-10, 0));
      chain.push_back(OrthancStone::ScenePoint2D(10, 0));
      layer->AddChain(chain, false, 255, 0, 0);
      chain.clear();
      chain.push_back(OrthancStone::ScenePoint2D(0, -10));
      chain.push_back(OrthancStone::ScenePoint2D(0, 10));
      layer->AddChain(chain, false, 255, 0, 0);
      chain.clear();
      chain.push_back(OrthancStone::ScenePoint2D(40, 30));
      chain.push_back(OrthancStone::ScenePoint2D(40, 50));
      layer->AddChain(chain, false, 255, 0, 0);
      chain.clear();
      chain.push_back(OrthancStone::ScenePoint2D(30, 40));
      chain.push_back(OrthancStone::ScenePoint2D(50, 40));
      layer->AddChain(chain, false, 255, 0, 0);
      lock->GetController().GetScene().SetLayer(1000, layer.release());
      lock->Invalidate();
      }*/
  }

  static EM_BOOL OnKey(int eventType,
                       const EmscriptenKeyboardEvent *event,
                       void *userData)
  {
    /**
     * WARNING: There is a problem with Firefox 71 that seems to mess
     * the "ctrlKey" value.
     **/
    
    ViewerViewport& that = *reinterpret_cast<ViewerViewport*>(userData);
    that.isCtrlDown_ = event->ctrlKey;
    return false;
  }

  
  static EM_BOOL OnWheel(int eventType,
                         const EmscriptenWheelEvent *wheelEvent,
                         void *userData)
  {
    ViewerViewport& that = *reinterpret_cast<ViewerViewport*>(userData);

    if (that.cursor_.get() != NULL)
    {
      if (wheelEvent->deltaY < 0)
      {
        that.ChangeFrame(that.isCtrlDown_ ? SeriesCursor::Action_FastMinus : SeriesCursor::Action_Minus);
      }
      else if (wheelEvent->deltaY > 0)
      {
        that.ChangeFrame(that.isCtrlDown_ ? SeriesCursor::Action_FastPlus : SeriesCursor::Action_Plus);
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
  
public:
  static boost::shared_ptr<ViewerViewport> Create(OrthancStone::ILoadersContext::ILock& lock,
                                                  const OrthancStone::DicomSource& source,
                                                  const std::string& canvas,
                                                  boost::shared_ptr<FramesCache> cache,
                                                  bool softwareRendering)
  {
    boost::shared_ptr<ViewerViewport> viewport(
      new ViewerViewport(lock.GetContext(), source, canvas, cache, softwareRendering));

    viewport->loader_ = OrthancStone::DicomResourcesLoader::Create(lock);
    viewport->Register<OrthancStone::DicomResourcesLoader::SuccessMessage>(
      *viewport->loader_, &ViewerViewport::Handle);

    viewport->Register<OrthancStone::HttpCommand::SuccessMessage>(
      lock.GetOracleObservable(), &ViewerViewport::Handle);

    viewport->Register<OrthancStone::ParseDicomSuccessMessage>(
      lock.GetOracleObservable(), &ViewerViewport::Handle);

    return viewport;    
  }

  void SetFrames(OrthancStone::SortedFrames* frames)
  {
    if (frames == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }

    flipX_ = false;
    flipY_ = false;
    fitNextContent_ = true;

    frames_.reset(frames);
    cursor_.reset(new SeriesCursor(frames_->GetFramesCount()));

    LOG(INFO) << "Number of frames in series: " << frames_->GetFramesCount();

    ResetDefaultWindowing();
    ClearViewport();
    prefetchQueue_.clear();
    currentFrameGeometry_ = FrameGeometry();

    if (observer_.get() != NULL)
    {
      observer_->SignalFrameUpdated(*this, cursor_->GetCurrentIndex(),
                                    frames_->GetFramesCount(), DisplayedFrameQuality_None);
    }
    
    if (frames_->GetFramesCount() != 0)
    {
      const std::string& sopInstanceUid = frames_->GetFrameSopInstanceUid(cursor_->GetCurrentIndex());

      {
        // Fetch the default windowing for the central instance
        const std::string uri = ("studies/" + frames_->GetStudyInstanceUid() +
                                 "/series/" + frames_->GetSeriesInstanceUid() +
                                 "/instances/" + sopInstanceUid + "/metadata");
        
        loader_->ScheduleGetDicomWeb(
          boost::make_shared<OrthancStone::LoadedDicomResources>(Orthanc::DICOM_TAG_SOP_INSTANCE_UID),
          0, source_, uri, new SetDefaultWindowingCommand(GetSharedObserver()));
      }
    }
  }

  // This method is used when the layout of the HTML page changes,
  // which does not trigger the "emscripten_set_resize_callback()"
  void UpdateSize(bool fitContent)
  {
    std::unique_ptr<OrthancStone::IViewport::ILock> lock(viewport_->Lock());
    lock->GetCompositor().RefreshCanvasSize();

    if (fitContent)
    {
      lock->GetCompositor().FitContent(lock->GetController().GetScene());
    }

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

  void ChangeFrame(SeriesCursor::Action action)
  {
    if (cursor_.get() != NULL)
    {
      size_t previous = cursor_->GetCurrentIndex();
      
      cursor_->Apply(action);
      
      size_t current = cursor_->GetCurrentIndex();
      if (previous != current)
      {
        DisplayCurrentFrame();
      }
    }
  }

  const FrameGeometry& GetCurrentFrameGeometry() const
  {
    return currentFrameGeometry_;
  }

  void UpdateReferenceLines(const std::list<const FrameGeometry*>& planes)
  {
    std::unique_ptr<OrthancStone::PolylineSceneLayer> layer(new OrthancStone::PolylineSceneLayer);
    
    if (GetCurrentFrameGeometry().IsValid())
    {
      for (std::list<const FrameGeometry*>::const_iterator
             it = planes.begin(); it != planes.end(); ++it)
      {
        assert(*it != NULL);
        
        double x1, y1, x2, y2;
        if (GetCurrentFrameGeometry().Intersect(x1, y1, x2, y2, **it))
        {
          OrthancStone::PolylineSceneLayer::Chain chain;
          chain.push_back(OrthancStone::ScenePoint2D(x1, y1));
          chain.push_back(OrthancStone::ScenePoint2D(x2, y2));
          layer->AddChain(chain, false, 0, 255, 0);
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


  void SetDefaultWindowing()
  {
    SetWindowing(defaultWindowingCenter_, defaultWindowingWidth_);
  }

  void SetWindowing(float windowingCenter,
                    float windowingWidth)
  {
    windowingCenter_ = windowingCenter;
    windowingWidth_ = windowingWidth;
    UpdateCurrentTextureParameters();
  }

  void FlipX()
  {
    flipX_ = !flipX_;
    SaveCurrentWindowing();
    UpdateCurrentTextureParameters();
  }

  void FlipY()
  {
    flipY_ = !flipY_;
    SaveCurrentWindowing();
    UpdateCurrentTextureParameters();
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

  void SetMouseButtonActions(OrthancStone::MouseAction leftAction,
                             OrthancStone::MouseAction middleAction,
                             OrthancStone::MouseAction rightAction)
  {
    std::unique_ptr<OrthancStone::DefaultViewportInteractor> interactor(
      new OrthancStone::DefaultViewportInteractor);
    
    interactor->SetLeftButtonAction(leftAction);
    interactor->SetMiddleButtonAction(middleAction);
    interactor->SetRightButtonAction(rightAction);

    assert(viewport_ != NULL);
    viewport_->AcquireInteractor(interactor.release());
  }



  void FitForPrint()  // TODO - REMOVE
  {
    viewport_->FitForPrint();
  }
};





typedef std::map<std::string, boost::shared_ptr<ViewerViewport> >  Viewports;
static Viewports allViewports_;
static bool showReferenceLines_ = true;


static void UpdateReferenceLines()
{
  if (showReferenceLines_)
  {
    std::list<const FrameGeometry*> planes;
    
    for (Viewports::const_iterator it = allViewports_.begin(); it != allViewports_.end(); ++it)
    {
      assert(it->second != NULL);
      planes.push_back(&it->second->GetCurrentFrameGeometry());
    }

    for (Viewports::iterator it = allViewports_.begin(); it != allViewports_.end(); ++it)
    {
      assert(it->second != NULL);
      it->second->UpdateReferenceLines(planes);
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
  }

  virtual void SignalFrameUpdated(const ViewerViewport& viewport,
                                  size_t currentFrame,
                                  size_t countFrames,
                                  DisplayedFrameQuality quality) ORTHANC_OVERRIDE
  {
    EM_ASM({
        const customEvent = document.createEvent("CustomEvent");
        customEvent.initCustomEvent("FrameUpdated", false, false,
                                    { "canvasId" : UTF8ToString($0),
                                        "currentFrame" : $1,
                                        "framesCount" : $2,
                                        "quality" : $3 });
        window.dispatchEvent(customEvent);
      },
      viewport.GetCanvasId().c_str(),
      static_cast<int>(currentFrame),
      static_cast<int>(countFrames),
      quality);


    UpdateReferenceLines();
  };
};



static OrthancStone::DicomSource source_;
static boost::shared_ptr<FramesCache> cache_;
static boost::shared_ptr<OrthancStone::WebAssemblyLoadersContext> context_;
static std::string stringBuffer_;
static bool softwareRendering_ = false;
static OrthancStone::MouseAction leftButtonAction_ = OrthancStone::MouseAction_GrayscaleWindowing;
static OrthancStone::MouseAction middleButtonAction_ = OrthancStone::MouseAction_Pan;
static OrthancStone::MouseAction rightButtonAction_ = OrthancStone::MouseAction_Zoom;


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


static boost::shared_ptr<ViewerViewport> GetViewport(const std::string& canvas)
{
  Viewports::iterator found = allViewports_.find(canvas);
  if (found == allViewports_.end())
  {
    std::unique_ptr<OrthancStone::ILoadersContext::ILock> lock(context_->Lock());
    boost::shared_ptr<ViewerViewport> viewport(
      ViewerViewport::Create(*lock, source_, canvas, cache_, softwareRendering_));
    viewport->SetMouseButtonActions(leftButtonAction_, middleButtonAction_, rightButtonAction_);
    viewport->AcquireObserver(new WebAssemblyObserver);
    allViewports_[canvas] = viewport;
    return viewport;
  }
  else
  {
    return found->second;
  }
}



static OrthancStone::MouseAction ConvertMouseAction(int action)
{
  switch (action)
  {
    case MouseAction_GrayscaleWindowing:
      return OrthancStone::MouseAction_GrayscaleWindowing;
      
    case MouseAction_Zoom:
      return OrthancStone::MouseAction_Zoom;
      
    case MouseAction_Pan:
      return OrthancStone::MouseAction_Pan;
      
    case MouseAction_Rotate:
      return OrthancStone::MouseAction_Rotate;

    default:
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
  }
}



extern "C"
{
  int main(int argc, char const *argv[]) 
  {
    printf("OK\n");
    Orthanc::InitializeFramework("", true);
    Orthanc::Logging::EnableInfoLevel(true);
    //Orthanc::Logging::EnableTraceLevel(true);

    context_.reset(new OrthancStone::WebAssemblyLoadersContext(1, 4, 1));
    cache_.reset(new FramesCache);
    
    DISPATCH_JAVASCRIPT_EVENT("StoneInitialized");
  }


  EMSCRIPTEN_KEEPALIVE
  void SetOrthancRoot(const char* uri,
                      int useRendered)
  {
    try
    {
      context_->SetLocalOrthanc(uri);  // For "source_.SetDicomWebThroughOrthancSource()"
      source_.SetDicomWebSource(std::string(uri) + "/dicom-web");
      source_.SetDicomWebRendered(useRendered != 0);
    }
    EXTERN_CATCH_EXCEPTIONS;
  }
  

  EMSCRIPTEN_KEEPALIVE
  void SetDicomWebServer(const char* serverName,
                         int hasRendered)
  {
    try
    {
      source_.SetDicomWebThroughOrthancSource(serverName);
      source_.SetDicomWebRendered(hasRendered != 0);
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
      std::unique_ptr<OrthancStone::SortedFrames> frames(new OrthancStone::SortedFrames);
      
      if (GetResourcesLoader().SortSeriesFrames(*frames, seriesInstanceUid))
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
  void DecrementFrame(const char* canvas,
                      int fitContent)
  {
    try
    {
      GetViewport(canvas)->ChangeFrame(SeriesCursor::Action_Minus);
    }
    EXTERN_CATCH_EXCEPTIONS;
  }


  EMSCRIPTEN_KEEPALIVE
  void IncrementFrame(const char* canvas,
                      int fitContent)
  {
    try
    {
      GetViewport(canvas)->ChangeFrame(SeriesCursor::Action_Plus);
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
  void SetDefaultWindowing(const char* canvas)
  {
    try
    {
      GetViewport(canvas)->SetDefaultWindowing();
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
      GetViewport(canvas)->SetWindowing(center, width);
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
  void SetMouseButtonActions(int leftAction,
                             int middleAction,
                             int rightAction)
  {
    try
    {
      leftButtonAction_ = ConvertMouseAction(leftAction);
      middleButtonAction_ = ConvertMouseAction(middleAction);
      rightButtonAction_ = ConvertMouseAction(rightAction);
      
      for (Viewports::iterator it = allViewports_.begin(); it != allViewports_.end(); ++it)
      {
        assert(it->second != NULL);
        it->second->SetMouseButtonActions(leftButtonAction_, middleButtonAction_, rightButtonAction_);
      }
    }
    EXTERN_CATCH_EXCEPTIONS;
  }


  EMSCRIPTEN_KEEPALIVE
  void FitForPrint()  // TODO - REMOVE
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
}
