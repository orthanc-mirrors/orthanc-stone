/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2018 Osimis S.A., Belgium
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

#include "Layers/FrameRenderer.h"
#include "Layers/VolumeSlicerBase.h"
#include "Layers/SliceOutlineRenderer.h"
#include "Layers/LineLayerRenderer.h"
#include "Widgets/SliceViewerWidget.h"
#include "Toolbox/DownloadStack.h"
#include "Toolbox/GeometryToolbox.h"
#include "Toolbox/OrthancSlicesLoader.h"
#include "Volumes/ImageBuffer3D.h"
#include "Volumes/SlicedVolumeBase.h"

#include <Core/Logging.h>
#include <Core/Images/ImageProcessing.h>

#include <boost/math/special_functions/round.hpp>


namespace OrthancStone
{
  // TODO: Handle errors while loading
  class OrthancVolumeImage :
      public SlicedVolumeBase,
      public OrthancStone::IObserver
  {
  private:
    OrthancSlicesLoader           loader_;
    std::auto_ptr<ImageBuffer3D>  image_;
    std::auto_ptr<DownloadStack>  downloadStack_;
    bool                          computeRange_;
    size_t                        pendingSlices_;
    
    void ScheduleSliceDownload()
    {
      assert(downloadStack_.get() != NULL);

      unsigned int slice;
      if (downloadStack_->Pop(slice))
      {
        loader_.ScheduleLoadSliceImage(slice, SliceImageQuality_Jpeg90);
      }
    }


    static bool IsCompatible(const Slice& a,
                             const Slice& b)
    {
      if (!GeometryToolbox::IsParallel(a.GetGeometry().GetNormal(),
                                       b.GetGeometry().GetNormal()))
      {
        LOG(ERROR) << "Some slice in the volume image is not parallel to the others";
        return false;
      }

      if (a.GetConverter().GetExpectedPixelFormat() != b.GetConverter().GetExpectedPixelFormat())
      {
        LOG(ERROR) << "The pixel format changes across the slices of the volume image";
        return false;
      }

      if (a.GetWidth() != b.GetWidth() ||
          a.GetHeight() != b.GetHeight())
      {
        LOG(ERROR) << "The width/height of the slices change across the volume image";
        return false;
      }

      if (!LinearAlgebra::IsNear(a.GetPixelSpacingX(), b.GetPixelSpacingX()) ||
          !LinearAlgebra::IsNear(a.GetPixelSpacingY(), b.GetPixelSpacingY()))
      {
        LOG(ERROR) << "The pixel spacing of the slices change across the volume image";
        return false;
      }

      return true;
    }


    static double GetDistance(const Slice& a,
                              const Slice& b)
    {
      return fabs(a.GetGeometry().ProjectAlongNormal(a.GetGeometry().GetOrigin()) -
                  a.GetGeometry().ProjectAlongNormal(b.GetGeometry().GetOrigin()));
    }


    void OnSliceGeometryReady(const OrthancSlicesLoader& loader)
    {
      if (loader.GetSliceCount() == 0)
      {
        LOG(ERROR) << "Empty volume image";
        SlicedVolumeBase::NotifyGeometryError();
        return;
      }

      for (size_t i = 1; i < loader.GetSliceCount(); i++)
      {
        if (!IsCompatible(loader.GetSlice(0), loader.GetSlice(i)))
        {
          SlicedVolumeBase::NotifyGeometryError();
          return;
        }
      }

      double spacingZ;

      if (loader.GetSliceCount() > 1)
      {
        spacingZ = GetDistance(loader.GetSlice(0), loader.GetSlice(1));
      }
      else
      {
        // This is a volume with one single slice: Choose a dummy
        // z-dimension for voxels
        spacingZ = 1;
      }

      for (size_t i = 1; i < loader.GetSliceCount(); i++)
      {
        if (!LinearAlgebra::IsNear(spacingZ, GetDistance(loader.GetSlice(i - 1), loader.GetSlice(i)),
                                   0.001 /* this is expressed in mm */))
        {
          LOG(ERROR) << "The distance between successive slices is not constant in a volume image";
          SlicedVolumeBase::NotifyGeometryError();
          return;
        }
      }

      unsigned int width = loader.GetSlice(0).GetWidth();
      unsigned int height = loader.GetSlice(0).GetHeight();
      Orthanc::PixelFormat format = loader.GetSlice(0).GetConverter().GetExpectedPixelFormat();
      LOG(INFO) << "Creating a volume image of size " << width << "x" << height
                << "x" << loader.GetSliceCount() << " in " << Orthanc::EnumerationToString(format);

      image_.reset(new ImageBuffer3D(format, width, height, loader.GetSliceCount(), computeRange_));
      image_->SetAxialGeometry(loader.GetSlice(0).GetGeometry());
      image_->SetVoxelDimensions(loader.GetSlice(0).GetPixelSpacingX(),
                                 loader.GetSlice(0).GetPixelSpacingY(), spacingZ);
      image_->Clear();

      downloadStack_.reset(new DownloadStack(loader.GetSliceCount()));
      pendingSlices_ = loader.GetSliceCount();

      for (unsigned int i = 0; i < 4; i++)  // Limit to 4 simultaneous downloads
      {
        ScheduleSliceDownload();
      }

      // TODO Check the DicomFrameConverter are constant

      SlicedVolumeBase::NotifyGeometryReady();
    }

    virtual void OnSliceImageReady(const OrthancSlicesLoader& loader,
                                   unsigned int sliceIndex,
                                   const Slice& slice,
                                   const Orthanc::ImageAccessor& image,
                                   SliceImageQuality quality)
    {
      {
        ImageBuffer3D::SliceWriter writer(*image_, VolumeProjection_Axial, sliceIndex);
        Orthanc::ImageProcessing::Copy(writer.GetAccessor(), image);
      }

      SlicedVolumeBase::NotifySliceChange(sliceIndex, slice);

      if (pendingSlices_ == 1)
      {
        SlicedVolumeBase::NotifyVolumeReady();
        pendingSlices_ = 0;
      }
      else if (pendingSlices_ > 1)
      {
        pendingSlices_ -= 1;
      }

      ScheduleSliceDownload();
    }

    virtual void HandleMessage(const IObservable& from, const IMessage& message)
    {
      switch (message.GetType())
      {
        case MessageType_SliceLoader_GeometryReady:
          OnSliceGeometryReady(dynamic_cast<const OrthancSlicesLoader&>(from));
          break;
        
        case MessageType_SliceLoader_GeometryError:
          LOG(ERROR) << "Unable to download a volume image";
          SlicedVolumeBase::NotifyGeometryError();
          break;
        
        case MessageType_SliceLoader_ImageReady:
        {
          const OrthancSlicesLoader::SliceImageReadyMessage& msg =
            dynamic_cast<const OrthancSlicesLoader::SliceImageReadyMessage&>(message);
          OnSliceImageReady(dynamic_cast<const OrthancSlicesLoader&>(from),
                            msg.GetSliceIndex(),
                            msg.GetSlice(),
                            msg.GetImage(),
                            msg.GetEffectiveQuality());
          break;
        }
      
        case MessageType_SliceLoader_ImageError:
        {
          const OrthancSlicesLoader::SliceImageErrorMessage& msg =
            dynamic_cast<const OrthancSlicesLoader::SliceImageErrorMessage&>(message);
          LOG(ERROR) << "Cannot download slice " << msg.GetSliceIndex() << " in a volume image";
          ScheduleSliceDownload();
          break;
        }
      
        default:
          VLOG("unhandled message type" << message.GetType());
      }
    }

  public:
    OrthancVolumeImage(MessageBroker& broker,
                       OrthancApiClient& orthanc,
                       bool computeRange) :
      OrthancStone::IObserver(broker),
      loader_(broker, orthanc),
      computeRange_(computeRange),
      pendingSlices_(0)
    {
      // TODO: replace with new callables loader_.RegisterObserver(*this);
    }

    void ScheduleLoadSeries(const std::string& seriesId)
    {
      loader_.ScheduleLoadSeries(seriesId);
    }

    void ScheduleLoadInstance(const std::string& instanceId)
    {
      loader_.ScheduleLoadInstance(instanceId);
    }

    void ScheduleLoadFrame(const std::string& instanceId,
                           unsigned int frame)
    {
      loader_.ScheduleLoadFrame(instanceId, frame);
    }

    virtual size_t GetSliceCount() const
    {
      return loader_.GetSliceCount();
    }

    virtual const Slice& GetSlice(size_t index) const
    {
      return loader_.GetSlice(index);
    }

    ImageBuffer3D& GetImage() const
    {
      if (image_.get() == NULL)
      {
        // The geometry is not ready yet
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
      else
      {
        return *image_;
      }
    }

    bool FitWindowingToRange(RenderStyle& style,
                             const DicomFrameConverter& converter) const
    {
      if (image_.get() == NULL)
      {
        return false;
      }
      else
      {
        return image_->FitWindowingToRange(style, converter);
      }
    }
  };


  class VolumeImageGeometry
  {
  private:
    unsigned int         width_;
    unsigned int         height_;
    size_t               depth_;
    double               pixelSpacingX_;
    double               pixelSpacingY_;
    double               sliceThickness_;
    CoordinateSystem3D   reference_;
    DicomFrameConverter  converter_;
    
    double ComputeAxialThickness(const OrthancVolumeImage& volume) const
    {
      double thickness;
      
      size_t n = volume.GetSliceCount();
      if (n > 1)
      {
        const Slice& a = volume.GetSlice(0);
        const Slice& b = volume.GetSlice(n - 1);
        thickness = ((reference_.ProjectAlongNormal(b.GetGeometry().GetOrigin()) -
                      reference_.ProjectAlongNormal(a.GetGeometry().GetOrigin())) /
                     (static_cast<double>(n) - 1.0));
      }
      else
      {
        thickness = volume.GetSlice(0).GetThickness();
      }

      if (thickness <= 0)
      {
        // The slices should have been sorted with increasing Z
        // (along the normal) by the OrthancSlicesLoader
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
      }
      else
      {
        return thickness;
      }
    }
    
    void SetupAxial(const OrthancVolumeImage& volume)
    {
      const Slice& axial = volume.GetSlice(0);
      
      width_ = axial.GetWidth();
      height_ = axial.GetHeight();
      depth_ = volume.GetSliceCount();

      pixelSpacingX_ = axial.GetPixelSpacingX();
      pixelSpacingY_ = axial.GetPixelSpacingY();
      sliceThickness_ = ComputeAxialThickness(volume);

      reference_ = axial.GetGeometry();
    }

    void SetupCoronal(const OrthancVolumeImage& volume)
    {
      const Slice& axial = volume.GetSlice(0);
      double axialThickness = ComputeAxialThickness(volume);

      width_ = axial.GetWidth();
      height_ = volume.GetSliceCount();
      depth_ = axial.GetHeight();

      pixelSpacingX_ = axial.GetPixelSpacingX();
      pixelSpacingY_ = axialThickness;
      sliceThickness_ = axial.GetPixelSpacingY();

      Vector origin = axial.GetGeometry().GetOrigin();
      origin += (static_cast<double>(volume.GetSliceCount() - 1) *
                 axialThickness * axial.GetGeometry().GetNormal());
      
      reference_ = CoordinateSystem3D(origin,
                                      axial.GetGeometry().GetAxisX(),
                                      -axial.GetGeometry().GetNormal());
    }

    void SetupSagittal(const OrthancVolumeImage& volume)
    {
      const Slice& axial = volume.GetSlice(0);
      double axialThickness = ComputeAxialThickness(volume);

      width_ = axial.GetHeight();
      height_ = volume.GetSliceCount();
      depth_ = axial.GetWidth();

      pixelSpacingX_ = axial.GetPixelSpacingY();
      pixelSpacingY_ = axialThickness;
      sliceThickness_ = axial.GetPixelSpacingX();

      Vector origin = axial.GetGeometry().GetOrigin();
      origin += (static_cast<double>(volume.GetSliceCount() - 1) *
                 axialThickness * axial.GetGeometry().GetNormal());
      
      reference_ = CoordinateSystem3D(origin,
                                      axial.GetGeometry().GetAxisY(),
                                      axial.GetGeometry().GetNormal());
    }

  public:
    VolumeImageGeometry(const OrthancVolumeImage& volume,
                        VolumeProjection projection)
    {
      if (volume.GetSliceCount() == 0)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }

      converter_ = volume.GetSlice(0).GetConverter();

      switch (projection)
      {
      case VolumeProjection_Axial:
        SetupAxial(volume);
        break;

      case VolumeProjection_Coronal:
        SetupCoronal(volume);
        break;

      case VolumeProjection_Sagittal:
        SetupSagittal(volume);
        break;

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
    }

    size_t GetSliceCount() const
    {
      return depth_;
    }

    const Vector& GetNormal() const
    {
      return reference_.GetNormal();
    }
    
    bool LookupSlice(size_t& index,
                     const CoordinateSystem3D& slice) const
    {
      bool opposite;
      if (!GeometryToolbox::IsParallelOrOpposite(opposite,
                                                 reference_.GetNormal(),
                                                 slice.GetNormal()))
      {
        return false;
      }
      
      double z = (reference_.ProjectAlongNormal(slice.GetOrigin()) -
                  reference_.ProjectAlongNormal(reference_.GetOrigin())) / sliceThickness_;

      int s = static_cast<int>(boost::math::iround(z));

      if (s < 0 ||
          s >= static_cast<int>(depth_))
      {
        return false;
      }
      else
      {
        index = static_cast<size_t>(s);
        return true;
      }
    }

    Slice* GetSlice(size_t slice) const
    {
      if (slice >= depth_)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
      else
      {
        CoordinateSystem3D origin(reference_.GetOrigin() +
                                  static_cast<double>(slice) * sliceThickness_ * reference_.GetNormal(),
                                  reference_.GetAxisX(),
                                  reference_.GetAxisY());

        return new Slice(origin, pixelSpacingX_, pixelSpacingY_, sliceThickness_,
                         width_, height_, converter_);
      }
    }
  };



  class VolumeImageSource :
      public VolumeSlicerBase,
      private ISlicedVolume::IObserver
  {
  private:
    class RendererFactory : public LayerReadyMessage::IRendererFactory
    {
    private:
      const Orthanc::ImageAccessor&  frame_;
      const Slice&                   slice_;
      bool                           isFullQuality_;

    public:
      RendererFactory(const Orthanc::ImageAccessor& frame,
                      const Slice& slice,
                      bool isFullQuality) :
        frame_(frame),
        slice_(slice),
        isFullQuality_(isFullQuality)
      {
      }

      virtual ILayerRenderer* CreateRenderer() const
      {
        return FrameRenderer::CreateRenderer(frame_, slice_, isFullQuality_);
      }
    };


    OrthancVolumeImage&                 volume_;
    std::auto_ptr<VolumeImageGeometry>  axialGeometry_;
    std::auto_ptr<VolumeImageGeometry>  coronalGeometry_;
    std::auto_ptr<VolumeImageGeometry>  sagittalGeometry_;

    
    bool IsGeometryReady() const
    {
      return axialGeometry_.get() != NULL;
    }

    
    virtual void NotifyGeometryReady(const ISlicedVolume& volume)
    {
      // These 3 values are only used to speed up the IVolumeSlicer
      axialGeometry_.reset(new VolumeImageGeometry(volume_, VolumeProjection_Axial));
      coronalGeometry_.reset(new VolumeImageGeometry(volume_, VolumeProjection_Coronal));
      sagittalGeometry_.reset(new VolumeImageGeometry(volume_, VolumeProjection_Sagittal));
      
      VolumeSlicerBase::NotifyGeometryReady();
    }

    virtual void NotifyGeometryError(const ISlicedVolume& volume)
    {
      VolumeSlicerBase::NotifyGeometryError();
    }

    virtual void NotifyContentChange(const ISlicedVolume& volume)
    {
      VolumeSlicerBase::NotifyContentChange();
    }

    virtual void NotifySliceChange(const ISlicedVolume& volume,
                                   const size_t& sliceIndex,
                                   const Slice& slice)
    {
      //VolumeSlicerBase::NotifySliceChange(slice);

      // TODO Improve this?
      VolumeSlicerBase::NotifyContentChange();
    }

    virtual void NotifyVolumeReady(const ISlicedVolume& volume)
    {
    }

    const VolumeImageGeometry& GetProjectionGeometry(VolumeProjection projection)
    {
      if (!IsGeometryReady())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }

      switch (projection)
      {
      case VolumeProjection_Axial:
        return *axialGeometry_;

      case VolumeProjection_Sagittal:
        return *sagittalGeometry_;

      case VolumeProjection_Coronal:
        return *coronalGeometry_;

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
    }


    bool DetectProjection(VolumeProjection& projection,
                          const CoordinateSystem3D& viewportSlice)
    {
      bool isOpposite;  // Ignored

      if (GeometryToolbox::IsParallelOrOpposite(isOpposite,
                                                viewportSlice.GetNormal(),
                                                axialGeometry_->GetNormal()))
      {
        projection = VolumeProjection_Axial;
        return true;
      }
      else if (GeometryToolbox::IsParallelOrOpposite(isOpposite,
                                                     viewportSlice.GetNormal(),
                                                     sagittalGeometry_->GetNormal()))
      {
        projection = VolumeProjection_Sagittal;
        return true;
      }
      else if (GeometryToolbox::IsParallelOrOpposite(isOpposite,
                                                     viewportSlice.GetNormal(),
                                                     coronalGeometry_->GetNormal()))
      {
        projection = VolumeProjection_Coronal;
        return true;
      }
      else
      {
        return false;
      }
    }


  public:
    VolumeImageSource(MessageBroker& broker, OrthancVolumeImage&  volume) :
      VolumeSlicerBase(broker),
      volume_(volume)
    {
      volume_.Register(*this);
    }

    virtual bool GetExtent(std::vector<Vector>& points,
                           const CoordinateSystem3D& viewportSlice)
    {
      VolumeProjection projection;
      
      if (!IsGeometryReady() ||
          !DetectProjection(projection, viewportSlice))
      {
        return false;
      }
      else
      {
        // As the slices of the volumic image are arranged in a box,
        // we only consider one single reference slice (the one with index 0).
        std::auto_ptr<Slice> slice(GetProjectionGeometry(projection).GetSlice(0));
        slice->GetExtent(points);
        
        return true;
      }
    }
    

    virtual void ScheduleLayerCreation(const CoordinateSystem3D& viewportSlice)
    {
      VolumeProjection projection;
      
      if (IsGeometryReady() &&
          DetectProjection(projection, viewportSlice))
      {
        const VolumeImageGeometry& geometry = GetProjectionGeometry(projection);

        size_t closest;

        if (geometry.LookupSlice(closest, viewportSlice))
        {
          bool isFullQuality = true;  // TODO

          std::auto_ptr<Orthanc::Image> frame;

          {
            ImageBuffer3D::SliceReader reader(volume_.GetImage(), projection, closest);

            // TODO Transfer ownership if non-axial, to avoid memcpy
            frame.reset(Orthanc::Image::Clone(reader.GetAccessor()));
          }

          std::auto_ptr<Slice> slice(geometry.GetSlice(closest));

          RendererFactory factory(*frame, *slice, isFullQuality);
          VolumeSlicerBase::NotifyLayerReady(factory,
            //new SliceOutlineRenderer(slice),
            slice->GetGeometry());
          return;
        }
      }

      // Error
      CoordinateSystem3D slice;
      VolumeSlicerBase::NotifyLayerError(slice);
    }
  };


  class VolumeImageInteractor :
      public IWorldSceneInteractor,
      protected ISlicedVolume::IObserver
  {
  private:
    SliceViewerWidget&                        widget_;
    VolumeProjection                    projection_;
    std::auto_ptr<VolumeImageGeometry>  slices_;
    size_t                              slice_;

  protected:
    virtual void NotifyGeometryReady(const ISlicedVolume& volume)
    {
      if (slices_.get() == NULL)
      {
        const OrthancVolumeImage& image = dynamic_cast<const OrthancVolumeImage&>(volume);

        slices_.reset(new VolumeImageGeometry(image, projection_));
        SetSlice(slices_->GetSliceCount() / 2);

        widget_.FitContent();
      }
    }

    virtual void NotifyGeometryError(const ISlicedVolume& volume)
    {
    }

    virtual void NotifyContentChange(const ISlicedVolume& volume)
    {
    }

    virtual void NotifySliceChange(const ISlicedVolume& volume,
                                   const size_t& sliceIndex,
                                   const Slice& slice)
    {
    }

    virtual void NotifyVolumeReady(const ISlicedVolume& volume)
    {
    }

    virtual IWorldSceneMouseTracker* CreateMouseTracker(WorldSceneWidget& widget,
                                                        const ViewportGeometry& view,
                                                        MouseButton button,
                                                        double x,
                                                        double y,
                                                        IStatusBar* statusBar)
    {
      return NULL;
    }

    virtual void MouseOver(CairoContext& context,
                           WorldSceneWidget& widget,
                           const ViewportGeometry& view,
                           double x,
                           double y,
                           IStatusBar* statusBar)
    {
    }

    virtual void MouseWheel(WorldSceneWidget& widget,
                            MouseWheelDirection direction,
                            KeyboardModifiers modifiers,
                            IStatusBar* statusBar)
    {
      int scale = (modifiers & KeyboardModifiers_Control ? 10 : 1);

      switch (direction)
      {
      case MouseWheelDirection_Up:
        OffsetSlice(-scale);
        break;

      case MouseWheelDirection_Down:
        OffsetSlice(scale);
        break;

      default:
        break;
      }
    }

    virtual void KeyPressed(WorldSceneWidget& widget,
                            KeyboardKeys key,
                            char keyChar,
                            KeyboardModifiers modifiers,
                            IStatusBar* statusBar)
    {
      switch (keyChar)
      {
      case 's':
        widget.FitContent();
        break;

      default:
        break;
      }
    }

  public:
    VolumeImageInteractor(OrthancVolumeImage& volume,
                          SliceViewerWidget& widget,
                          VolumeProjection projection) :
      widget_(widget),
      projection_(projection)
    {
      volume.Register(*this);
      widget.SetInteractor(*this);
    }

    bool IsGeometryReady() const
    {
      return slices_.get() != NULL;
    }

    size_t GetSliceCount() const
    {
      if (slices_.get() == NULL)
      {
        return 0;
      }
      else
      {
        return slices_->GetSliceCount();
      }
    }

    void OffsetSlice(int offset)
    {
      if (slices_.get() != NULL)
      {
        int slice = static_cast<int>(slice_) + offset;

        if (slice < 0)
        {
          slice = 0;
        }

        if (slice >= static_cast<int>(slices_->GetSliceCount()))
        {
          slice = slices_->GetSliceCount() - 1;
        }

        if (slice != static_cast<int>(slice_))
        {
          SetSlice(slice);
        }
      }
    }

    void SetSlice(size_t slice)
    {
      if (slices_.get() != NULL)
      {
        slice_ = slice;

        std::auto_ptr<Slice> tmp(slices_->GetSlice(slice_));
        widget_.SetSlice(tmp->GetGeometry());
      }
    }
  };



  class ReferenceLineSource : public VolumeSlicerBase
  {
  private:
    class RendererFactory : public LayerReadyMessage::IRendererFactory
    {
    private:
      double                     x1_;
      double                     y1_;
      double                     x2_;
      double                     y2_;
      const CoordinateSystem3D&  slice_;

    public:
      RendererFactory(double x1,
                      double y1,
                      double x2,
                      double y2,
                      const CoordinateSystem3D& slice) :
        x1_(x1),
        y1_(y1),
        x2_(x2),
        y2_(y2),
        slice_(slice)
      {
      }

      virtual ILayerRenderer* CreateRenderer() const
      {
        return new LineLayerRenderer(x1_, y1_, x2_, y2_, slice_);
      }
    };

    SliceViewerWidget&  otherPlane_;

  public:
    ReferenceLineSource(MessageBroker& broker, 
                        SliceViewerWidget&  otherPlane) :
      VolumeSlicerBase(broker),
      otherPlane_(otherPlane)
    {
      NotifyGeometryReady();
    }

    virtual bool GetExtent(std::vector<Vector>& points,
                           const CoordinateSystem3D& viewportSlice)
    {
      return false;
    }

    virtual void ScheduleLayerCreation(const CoordinateSystem3D& viewportSlice)
    {
      Slice reference(viewportSlice, 0.001);
      
      Vector p, d;

      const CoordinateSystem3D& slice = otherPlane_.GetSlice();

      // Compute the line of intersection between the two slices
      if (!GeometryToolbox::IntersectTwoPlanes(p, d,
                                               slice.GetOrigin(), slice.GetNormal(),
                                               viewportSlice.GetOrigin(), viewportSlice.GetNormal()))
      {
        // The two slice are parallel, don't try and display the intersection
        NotifyLayerError(reference.GetGeometry());
      }
      else
      {
        double x1, y1, x2, y2;
        viewportSlice.ProjectPoint(x1, y1, p);
        viewportSlice.ProjectPoint(x2, y2, p + 1000.0 * d);

        const Extent2D extent = otherPlane_.GetSceneExtent();
        
        if (GeometryToolbox::ClipLineToRectangle(x1, y1, x2, y2,
                                                 x1, y1, x2, y2,
                                                 extent.GetX1(), extent.GetY1(),
                                                 extent.GetX2(), extent.GetY2()))
        {
          RendererFactory factory(x1, y1, x2, y2, slice);
          NotifyLayerReady(factory, reference.GetGeometry());
        }
        else
        {
          // Error: Parallel slices
          NotifyLayerError(reference.GetGeometry());
        }
      }
    }
  };
}