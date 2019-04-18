/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2019 Osimis S.A., Belgium
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
#include "Layers/LineLayerRenderer.h"
#include "Layers/SliceOutlineRenderer.h"
#include "Toolbox/DownloadStack.h"
#include "Toolbox/GeometryToolbox.h"
#include "Toolbox/OrthancSlicesLoader.h"
#include "Volumes/ImageBuffer3D.h"
#include "Volumes/ISlicedVolume.h"
#include "Widgets/SliceViewerWidget.h"

#include <Core/Logging.h>
#include <Core/Images/ImageProcessing.h>
#include <Core/OrthancException.h>

#include <boost/math/special_functions/round.hpp>


namespace OrthancStone
{
  // TODO: Handle errors while loading
  class OrthancVolumeImage :
    public ISlicedVolume,
    public IObserver
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
        LOG(ERROR) << "A slice in the volume image is not parallel to the others.";
        return false;
      }

      if (a.GetConverter().GetExpectedPixelFormat() != b.GetConverter().GetExpectedPixelFormat())
      {
        LOG(ERROR) << "The pixel format changes across the slices of the volume image.";
        return false;
      }

      if (a.GetWidth() != b.GetWidth() ||
          a.GetHeight() != b.GetHeight())
      {
        LOG(ERROR) << "The slices dimensions (width/height) are varying throughout the volume image";
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


    void OnSliceGeometryReady(const OrthancSlicesLoader::SliceGeometryReadyMessage& message)
    {
      assert(&message.GetOrigin() == &loader_);

      if (loader_.GetSliceCount() == 0)
      {
        LOG(ERROR) << "Empty volume image";
        EmitMessage(ISlicedVolume::GeometryErrorMessage(*this));
        return;
      }

      for (size_t i = 1; i < loader_.GetSliceCount(); i++)
      {
        if (!IsCompatible(loader_.GetSlice(0), loader_.GetSlice(i)))
        {
          EmitMessage(ISlicedVolume::GeometryErrorMessage(*this));
          return;
        }
      }

      double spacingZ;

      if (loader_.GetSliceCount() > 1)
      {
        spacingZ = GetDistance(loader_.GetSlice(0), loader_.GetSlice(1));
      }
      else
      {
        // This is a volume with one single slice: Choose a dummy
        // z-dimension for voxels
        spacingZ = 1;
      }

      for (size_t i = 1; i < loader_.GetSliceCount(); i++)
      {
        if (!LinearAlgebra::IsNear(spacingZ, GetDistance(loader_.GetSlice(i - 1), loader_.GetSlice(i)),
                                   0.001 /* this is expressed in mm */))
        {
          LOG(ERROR) << "The distance between successive slices is not constant in a volume image";
          EmitMessage(ISlicedVolume::GeometryErrorMessage(*this));
          return;
        }
      }

      unsigned int width = loader_.GetSlice(0).GetWidth();
      unsigned int height = loader_.GetSlice(0).GetHeight();
      Orthanc::PixelFormat format = loader_.GetSlice(0).GetConverter().GetExpectedPixelFormat();
      LOG(INFO) << "Creating a volume image of size " << width << "x" << height
                << "x" << loader_.GetSliceCount() << " in " << Orthanc::EnumerationToString(format);

      image_.reset(new ImageBuffer3D(format, width, height, static_cast<unsigned int>(loader_.GetSliceCount()), computeRange_));
      image_->SetAxialGeometry(loader_.GetSlice(0).GetGeometry());
      image_->SetVoxelDimensions(loader_.GetSlice(0).GetPixelSpacingX(),
                                 loader_.GetSlice(0).GetPixelSpacingY(), spacingZ);
      image_->Clear();

      downloadStack_.reset(new DownloadStack(static_cast<unsigned int>(loader_.GetSliceCount())));
      pendingSlices_ = loader_.GetSliceCount();

      for (unsigned int i = 0; i < 4; i++)  // Limit to 4 simultaneous downloads
      {
        ScheduleSliceDownload();
      }

      // TODO Check the DicomFrameConverter are constant

      EmitMessage(ISlicedVolume::GeometryReadyMessage(*this));
    }


    void OnSliceGeometryError(const OrthancSlicesLoader::SliceGeometryErrorMessage& message)
    {
      assert(&message.GetOrigin() == &loader_);

      LOG(ERROR) << "Unable to download a volume image";
      EmitMessage(ISlicedVolume::GeometryErrorMessage(*this));
    }


    void OnSliceImageReady(const OrthancSlicesLoader::SliceImageReadyMessage& message)
    {
      assert(&message.GetOrigin() == &loader_);

      {
        ImageBuffer3D::SliceWriter writer(*image_, VolumeProjection_Axial, message.GetSliceIndex());
        Orthanc::ImageProcessing::Copy(writer.GetAccessor(), message.GetImage());
      }

      EmitMessage(ISlicedVolume::SliceContentChangedMessage
                  (*this, message.GetSliceIndex(), message.GetSlice()));

      if (pendingSlices_ == 1)
      {
        EmitMessage(ISlicedVolume::VolumeReadyMessage(*this));
        pendingSlices_ = 0;
      }
      else if (pendingSlices_ > 1)
      {
        pendingSlices_ -= 1;
      }

      ScheduleSliceDownload();
    }


    void OnSliceImageError(const OrthancSlicesLoader::SliceImageErrorMessage& message)
    {
      assert(&message.GetOrigin() == &loader_);

      LOG(ERROR) << "Cannot download slice " << message.GetSliceIndex() << " in a volume image";
      ScheduleSliceDownload();
    }


  public:
    OrthancVolumeImage(MessageBroker& broker,
                       OrthancApiClient& orthanc,
                       bool computeRange) :
      ISlicedVolume(broker),
      IObserver(broker),
      loader_(broker, orthanc),
      computeRange_(computeRange),
      pendingSlices_(0)
    {
      loader_.RegisterObserverCallback(
        new Callable<OrthancVolumeImage, OrthancSlicesLoader::SliceGeometryReadyMessage>
        (*this, &OrthancVolumeImage::OnSliceGeometryReady));

      loader_.RegisterObserverCallback(
        new Callable<OrthancVolumeImage, OrthancSlicesLoader::SliceGeometryErrorMessage>
        (*this, &OrthancVolumeImage::OnSliceGeometryError));

      loader_.RegisterObserverCallback(
        new Callable<OrthancVolumeImage, OrthancSlicesLoader::SliceImageReadyMessage>
        (*this, &OrthancVolumeImage::OnSliceImageReady));

      loader_.RegisterObserverCallback(
        new Callable<OrthancVolumeImage, OrthancSlicesLoader::SliceImageErrorMessage>
        (*this, &OrthancVolumeImage::OnSliceImageError));
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
      height_ = static_cast<unsigned int>(volume.GetSliceCount());
      depth_ = axial.GetHeight();

      pixelSpacingX_ = axial.GetPixelSpacingX();
      pixelSpacingY_ = axialThickness;
      sliceThickness_ = axial.GetPixelSpacingY();

      Vector origin = axial.GetGeometry().GetOrigin();
      origin += (static_cast<double>(volume.GetSliceCount() - 1) *
                axialThickness * axial.GetGeometry().GetNormal());

      reference_ = CoordinateSystem3D(origin,
                                      axial.GetGeometry().GetAxisX(),
                                      - axial.GetGeometry().GetNormal());
    }

    void SetupSagittal(const OrthancVolumeImage& volume)
    {
      const Slice& axial = volume.GetSlice(0);
      double axialThickness = ComputeAxialThickness(volume);

      width_ = axial.GetHeight();
      height_ = static_cast<unsigned int>(volume.GetSliceCount());
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



  class VolumeImageMPRSlicer :
    public IVolumeSlicer,
    public IObserver
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

    void OnGeometryReady(const ISlicedVolume::GeometryReadyMessage& message)
    {
      assert(&message.GetOrigin() == &volume_);

      // These 3 values are only used to speed up the IVolumeSlicer
      axialGeometry_.reset(new VolumeImageGeometry(volume_, VolumeProjection_Axial));
      coronalGeometry_.reset(new VolumeImageGeometry(volume_, VolumeProjection_Coronal));
      sagittalGeometry_.reset(new VolumeImageGeometry(volume_, VolumeProjection_Sagittal));

      EmitMessage(IVolumeSlicer::GeometryReadyMessage(*this));
    }

    void OnGeometryError(const ISlicedVolume::GeometryErrorMessage& message)
    {
      assert(&message.GetOrigin() == &volume_);

      EmitMessage(IVolumeSlicer::GeometryErrorMessage(*this));
    }

    void OnContentChanged(const ISlicedVolume::ContentChangedMessage& message)
    {
      assert(&message.GetOrigin() == &volume_);

      EmitMessage(IVolumeSlicer::ContentChangedMessage(*this));
    }

    void OnSliceContentChanged(const ISlicedVolume::SliceContentChangedMessage& message)
    {
      assert(&message.GetOrigin() == &volume_);

      //IVolumeSlicer::OnSliceContentChange(slice);

      // TODO Improve this?
      EmitMessage(IVolumeSlicer::ContentChangedMessage(*this));
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
    VolumeImageMPRSlicer(MessageBroker& broker,
                         OrthancVolumeImage&  volume) :
                         IVolumeSlicer(broker),
                         IObserver(broker),
                         volume_(volume)
    {
      volume_.RegisterObserverCallback(
        new Callable<VolumeImageMPRSlicer, ISlicedVolume::GeometryReadyMessage>
        (*this, &VolumeImageMPRSlicer::OnGeometryReady));

      volume_.RegisterObserverCallback(
        new Callable<VolumeImageMPRSlicer, ISlicedVolume::GeometryErrorMessage>
        (*this, &VolumeImageMPRSlicer::OnGeometryError));

      volume_.RegisterObserverCallback(
        new Callable<VolumeImageMPRSlicer, ISlicedVolume::ContentChangedMessage>
        (*this, &VolumeImageMPRSlicer::OnContentChanged));

      volume_.RegisterObserverCallback(
        new Callable<VolumeImageMPRSlicer, ISlicedVolume::SliceContentChangedMessage>
        (*this, &VolumeImageMPRSlicer::OnSliceContentChanged));
    }

    virtual bool GetExtent(std::vector<Vector>& points,
                           const CoordinateSystem3D& viewportSlice) ORTHANC_OVERRIDE
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

    virtual void ScheduleLayerCreation(const CoordinateSystem3D& viewportSlice) ORTHANC_OVERRIDE
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
            ImageBuffer3D::SliceReader reader(volume_.GetImage(), projection, static_cast<unsigned int>(closest));

            // TODO Transfer ownership if non-axial, to avoid memcpy
            frame.reset(Orthanc::Image::Clone(reader.GetAccessor()));
          }

          std::auto_ptr<Slice> slice(geometry.GetSlice(closest));

          RendererFactory factory(*frame, *slice, isFullQuality);

          EmitMessage(IVolumeSlicer::LayerReadyMessage(*this, factory, slice->GetGeometry()));
          return;
        }
      }

      // Error
      CoordinateSystem3D slice;
      EmitMessage(IVolumeSlicer::LayerErrorMessage(*this, slice));
    }
  };


  class VolumeImageInteractor :
    public IWorldSceneInteractor,
    public IObserver
  {
  private:
    SliceViewerWidget&                  widget_;
    VolumeProjection                    projection_;
    std::auto_ptr<VolumeImageGeometry>  slices_;
    size_t                              slice_;

  protected:
    void OnGeometryReady(const ISlicedVolume::GeometryReadyMessage& message)
    {
      if (slices_.get() == NULL)
      {
        const OrthancVolumeImage& image =
          dynamic_cast<const OrthancVolumeImage&>(message.GetOrigin());

        slices_.reset(new VolumeImageGeometry(image, projection_));
        SetSlice(slices_->GetSliceCount() / 2);

        widget_.FitContent();
      }
    }

    virtual IWorldSceneMouseTracker* CreateMouseTracker(WorldSceneWidget& widget,
                                                        const ViewportGeometry& view,
                                                        MouseButton button,
                                                        KeyboardModifiers modifiers,
                                                        int viewportX,
                                                        int viewportY,
                                                        double x,
                                                        double y,
                                                        IStatusBar* statusBar,
                                                        const std::vector<Touch>& touches) ORTHANC_OVERRIDE
    {
      return  NULL;
    }

    virtual void MouseOver(CairoContext& context,
                           WorldSceneWidget& widget,
                           const ViewportGeometry& view,
                           double x,
                           double y,
                           IStatusBar* statusBar) ORTHANC_OVERRIDE
    {
    }

    virtual void MouseWheel(WorldSceneWidget& widget,
                            MouseWheelDirection direction,
                            KeyboardModifiers modifiers,
                            IStatusBar* statusBar) ORTHANC_OVERRIDE
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
                            IStatusBar* statusBar) ORTHANC_OVERRIDE
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
    VolumeImageInteractor(MessageBroker& broker,
                          OrthancVolumeImage& volume,
                          SliceViewerWidget& widget,
                          VolumeProjection projection) :
                          IObserver(broker),
                          widget_(widget),
                          projection_(projection)
    {
      widget.SetInteractor(*this);

      volume.RegisterObserverCallback(
        new Callable<VolumeImageInteractor, ISlicedVolume::GeometryReadyMessage>
        (*this, &VolumeImageInteractor::OnGeometryReady));
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
          slice = static_cast<unsigned int>(slices_->GetSliceCount()) - 1;
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



  class ReferenceLineSource : public IVolumeSlicer
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
      IVolumeSlicer(broker),
      otherPlane_(otherPlane)
    {
      EmitMessage(IVolumeSlicer::GeometryReadyMessage(*this));
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
        EmitMessage(IVolumeSlicer::LayerErrorMessage(*this, reference.GetGeometry()));
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
          EmitMessage(IVolumeSlicer::LayerReadyMessage(*this, factory, reference.GetGeometry()));
        }
        else
        {
          // Error: Parallel slices
          EmitMessage(IVolumeSlicer::LayerErrorMessage(*this, reference.GetGeometry()));
        }
      }
    }
  };
}
