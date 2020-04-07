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

#include "Applications/IStoneApplication.h"
#include "Framework/Widgets/WorldSceneWidget.h"
#include "Framework/Widgets/LayoutWidget.h"

#if ORTHANC_ENABLE_WASM==1
  #include "Platforms/Wasm/WasmPlatformApplicationAdapter.h"
  #include "Platforms/Wasm/Defaults.h"
  #include "Platforms/Wasm/WasmViewport.h"
#endif

#if ORTHANC_ENABLE_QT==1
  #include "Qt/SampleMainWindow.h"
  #include "Qt/SampleMainWindowWithButtons.h"
#endif

#include "Framework/Layers/DicomSeriesVolumeSlicer.h"
#include "Framework/Widgets/SliceViewerWidget.h"
#include "Framework/Volumes/StructureSetLoader.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>
#include <Core/Images/ImageTraits.h>

#include <boost/math/constants/constants.hpp>
#include "Framework/dev.h"
#include "Framework/Widgets/LayoutWidget.h"
#include "Framework/Layers/DicomStructureSetSlicer.h"

namespace OrthancStone
{
  namespace Samples
  {
    class RtViewerDemoBaseApplication : public IStoneApplication
    {
    protected:
      // ownership is transferred to the application context
#ifndef RESTORE_NON_RTVIEWERDEMO_BEHAVIOR
      LayoutWidget*          mainWidget_;
#else
      WorldSceneWidget*  mainWidget_;
#endif

    public:
      virtual void Initialize(StoneApplicationContext* context,
                              IStatusBar& statusBar,
                              const boost::program_options::variables_map& parameters) ORTHANC_OVERRIDE
      {
      }

      virtual std::string GetTitle() const ORTHANC_OVERRIDE
      {
        return "Stone of Orthanc - Sample";
      }

      /**
       * In the basic samples, the commands are handled by the platform adapter and NOT
       * by the application handler
      */
      virtual void HandleSerializedMessage(const char* data) ORTHANC_OVERRIDE {};


      virtual void Finalize() ORTHANC_OVERRIDE {}
      virtual IWidget* GetCentralWidget() ORTHANC_OVERRIDE {return mainWidget_;}

#if ORTHANC_ENABLE_WASM==1
      // default implementations for a single canvas named "canvas" in the HTML and an empty WasmApplicationAdapter

      virtual void InitializeWasm() ORTHANC_OVERRIDE
      {
        AttachWidgetToWasmViewport("canvas", mainWidget_);
      }

      virtual WasmPlatformApplicationAdapter* CreateWasmApplicationAdapter(MessageBroker& broker)
      {
        return new WasmPlatformApplicationAdapter(broker, *this);
      }
#endif

    };

    // this application actually works in Qt and WASM
    class RtViewerDemoBaseSingleCanvasWithButtonsApplication : public RtViewerDemoBaseApplication
    {
public:
      virtual void OnPushButton1Clicked() {}
      virtual void OnPushButton2Clicked() {}
      virtual void OnTool1Clicked() {}
      virtual void OnTool2Clicked() {}

      virtual void GetButtonNames(std::string& pushButton1,
                                  std::string& pushButton2,
                                  std::string& tool1,
                                  std::string& tool2
                                  ) {
        pushButton1 = "action1";
        pushButton2 = "action2";
        tool1 = "tool1";
        tool2 = "tool2";
      }

#if ORTHANC_ENABLE_QT==1
      virtual QStoneMainWindow* CreateQtMainWindow() {
        return new SampleMainWindowWithButtons(dynamic_cast<OrthancStone::NativeStoneApplicationContext&>(*context_), *this);
      }
#endif

    };

    // this application actually works in SDL and WASM
    class RtViewerDemoBaseApplicationSingleCanvas : public RtViewerDemoBaseApplication
    {
public:

#if ORTHANC_ENABLE_QT==1
      virtual QStoneMainWindow* CreateQtMainWindow() {
        return new SampleMainWindow(dynamic_cast<OrthancStone::NativeStoneApplicationContext&>(*context_), *this);
      }
#endif
    };
  }
}



namespace OrthancStone
{
  namespace Samples
  {
    template <Orthanc::PixelFormat T>
    void ReadDistributionInternal(std::vector<float>& distribution,
                                  const Orthanc::ImageAccessor& image)
    {
      const unsigned int width = image.GetWidth();
      const unsigned int height = image.GetHeight();
      
      distribution.resize(width * height);
      size_t pos = 0;

      for (unsigned int y = 0; y < height; y++)
      {
        for (unsigned int x = 0; x < width; x++, pos++)
        {
          distribution[pos] = Orthanc::ImageTraits<T>::GetFloatPixel(image, x, y);
        }
      }
    }

    void ReadDistribution(std::vector<float>& distribution,
                          const Orthanc::ImageAccessor& image)
    {
      switch (image.GetFormat())
      {
        case Orthanc::PixelFormat_Grayscale8:
          ReadDistributionInternal<Orthanc::PixelFormat_Grayscale8>(distribution, image);
          break;

        case Orthanc::PixelFormat_Grayscale16:
          ReadDistributionInternal<Orthanc::PixelFormat_Grayscale16>(distribution, image);
          break;

        case Orthanc::PixelFormat_SignedGrayscale16:
          ReadDistributionInternal<Orthanc::PixelFormat_SignedGrayscale16>(distribution, image);
          break;

        case Orthanc::PixelFormat_Grayscale32:
          ReadDistributionInternal<Orthanc::PixelFormat_Grayscale32>(distribution, image);
          break;

        case Orthanc::PixelFormat_Grayscale64:
          ReadDistributionInternal<Orthanc::PixelFormat_Grayscale64>(distribution, image);
          break;

        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
      }
    }


    class DoseInteractor : public VolumeImageInteractor
    {
    private:
      SliceViewerWidget&         widget_;
      size_t               layer_;
      DicomFrameConverter  converter_;


      
    protected:
      virtual void NotifySliceChange(const ISlicedVolume& slicedVolume,
                                    const size_t& sliceIndex,
                                    const Slice& slice)
      {
        converter_ = slice.GetConverter();
        
  #if 0
        const OrthancVolumeImage& volume = dynamic_cast<const OrthancVolumeImage&>(slicedVolume);

        RenderStyle s = widget_.GetLayerStyle(layer_);

        if (volume.FitWindowingToRange(s, slice.GetConverter()))
        {
          printf("Windowing: %f => %f\n", s.customWindowCenter_, s.customWindowWidth_);
          widget_.SetLayerStyle(layer_, s);
        }
  #endif
      }

      virtual void NotifyVolumeReady(const ISlicedVolume& slicedVolume)
      {
        const float percentile = 0.01f;
        const OrthancVolumeImage& volume = dynamic_cast<const OrthancVolumeImage&>(slicedVolume);

        std::vector<float> distribution;
        ReadDistribution(distribution, volume.GetImage().GetInternalImage());
        std::sort(distribution.begin(), distribution.end());

        int start = static_cast<int>(std::ceil(distribution.size() * percentile));
        int end = static_cast<int>(std::floor(distribution.size() * (1.0f - percentile)));

        float a = 0;
        float b = 0;
        
        if (start < end &&
            start >= 0 &&
            end < static_cast<int>(distribution.size()))
        {
          a = distribution[start];
          b = distribution[end];
        }
        else if (!distribution.empty())
        {
          // Too small distribution: Use full range
          a = distribution.front();
          b = distribution.back();
        }

        //printf("%f %f\n", a, b);

        RenderStyle s = widget_.GetLayerStyle(layer_);
        s.windowing_ = ImageWindowing_Custom;
        s.customWindowCenter_ = static_cast<float>(converter_.Apply((a + b) / 2.0f));
        s.customWindowWidth_ = static_cast<float>(converter_.Apply(b - a));
        
        // 96.210556 => 192.421112
        widget_.SetLayerStyle(layer_, s);
        printf("Windowing: %f => %f\n", s.customWindowCenter_, s.customWindowWidth_);      
      }

    public:
      DoseInteractor(MessageBroker& broker, OrthancVolumeImage& volume,
                    SliceViewerWidget& widget,
                    VolumeProjection projection,
                    size_t layer) :
        VolumeImageInteractor(broker, volume, widget, projection),
        widget_(widget),
        layer_(layer)
      {
      }
    };

    class RtViewerDemoApplication :
      public RtViewerDemoBaseApplicationSingleCanvas,
      public IObserver
    {
    public:
      std::vector<std::pair<SliceViewerWidget*, size_t> > doseCtWidgetLayerPairs_;
      std::list<OrthancStone::IWorldSceneInteractor*>    interactors_;

      class Interactor : public IWorldSceneInteractor
      {
      private:
        RtViewerDemoApplication&  application_;

      public:
        Interactor(RtViewerDemoApplication&  application) :
          application_(application)
        {
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
          const std::vector<Touch>& displayTouches)
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
          if (statusBar != NULL)
          {
            Vector p = dynamic_cast<SliceViewerWidget&>(widget).GetSlice().MapSliceToWorldCoordinates(x, y);

            char buf[64];
            sprintf(buf, "X = %.02f Y = %.02f Z = %.02f (in cm)",
              p[0] / 10.0, p[1] / 10.0, p[2] / 10.0);
            statusBar->SetMessage(buf);
          }
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
            application_.OffsetSlice(-scale);
            break;

          case MouseWheelDirection_Down:
            application_.OffsetSlice(scale);
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
            // TODO: recursively traverse children
            widget.FitContent();
            break;

          default:
            break;
          }
        }
      };

      void OffsetSlice(int offset)
      {
        if (source_ != NULL)
        {
          int slice = static_cast<int>(slice_) + offset;

          if (slice < 0)
          {
            slice = 0;
          }

          if (slice >= static_cast<int>(source_->GetSliceCount()))
          {
            slice = static_cast<int>(source_->GetSliceCount()) - 1;
          }

          if (slice != static_cast<int>(slice_))
          {
            SetSlice(slice);
          }
        }
      }


      SliceViewerWidget& GetMainWidget()
      {
        return *dynamic_cast<SliceViewerWidget*>(mainWidget_);
      }


      void SetSlice(size_t index)
      {
        if (source_ != NULL &&
          index < source_->GetSliceCount())
        {
          slice_ = static_cast<unsigned int>(index);

#if 1
          GetMainWidget().SetSlice(source_->GetSlice(slice_).GetGeometry());
#else
          // TEST for scene extents - Rotate the axes
          double a = 15.0 / 180.0 * boost::math::constants::pi<double>();

#if 1
          Vector x; GeometryToolbox::AssignVector(x, cos(a), sin(a), 0);
          Vector y; GeometryToolbox::AssignVector(y, -sin(a), cos(a), 0);
#else
          // Flip the normal
          Vector x; GeometryToolbox::AssignVector(x, cos(a), sin(a), 0);
          Vector y; GeometryToolbox::AssignVector(y, sin(a), -cos(a), 0);
#endif

          SliceGeometry s(source_->GetSlice(slice_).GetGeometry().GetOrigin(), x, y);
          widget_->SetSlice(s);
#endif
        }
      }


      void OnMainWidgetGeometryReady(const IVolumeSlicer::GeometryReadyMessage& message)
      {
        // Once the geometry of the series is downloaded from Orthanc,
        // display its middle slice, and adapt the viewport to fit this
        // slice
        if (source_ == &message.GetOrigin())
        {
          SetSlice(source_->GetSliceCount() / 2);
        }

        GetMainWidget().FitContent();
      }

    DicomFrameConverter                                converter_;

    void OnSliceContentChangedMessage(const ISlicedVolume::SliceContentChangedMessage&   message)
    {
      converter_ = message.GetSlice().GetConverter();
    }

    void OnVolumeReadyMessage(const ISlicedVolume::VolumeReadyMessage& message)
    {
      const float percentile = 0.01f;

      auto& slicedVolume = message.GetOrigin();
      const OrthancVolumeImage& volume = dynamic_cast<const OrthancVolumeImage&>(slicedVolume);

      std::vector<float> distribution;
      ReadDistribution(distribution, volume.GetImage().GetInternalImage());
      std::sort(distribution.begin(), distribution.end());

      int start = static_cast<int>(std::ceil(distribution.size() * percentile));
      int end = static_cast<int>(std::floor(distribution.size() * (1.0f - percentile)));

      float a = 0;
      float b = 0;

      if (start < end &&
        start >= 0 &&
        end < static_cast<int>(distribution.size()))
      {
        a = distribution[start];
        b = distribution[end];
      }
      else if (!distribution.empty())
      {
        // Too small distribution: Use full range
        a = distribution.front();
        b = distribution.back();
      }

      //printf("WINDOWING %f %f\n", a, b);

      for (const auto& pair : doseCtWidgetLayerPairs_)
      {
        auto widget = pair.first;
        auto layer = pair.second;
        RenderStyle s = widget->GetLayerStyle(layer);
        s.windowing_ = ImageWindowing_Custom;
        s.customWindowCenter_ = static_cast<float>(converter_.Apply((a + b) / 2.0f));
        s.customWindowWidth_ = static_cast<float>(converter_.Apply(b - a));

        // 96.210556 => 192.421112
        widget->SetLayerStyle(layer, s);
        printf("Windowing: %f => %f\n", s.customWindowCenter_, s.customWindowWidth_);
      }
    }
      


      size_t AddDoseLayer(SliceViewerWidget& widget,
        OrthancVolumeImage& volume, VolumeProjection projection);

      void AddStructLayer(
        SliceViewerWidget& widget, StructureSetLoader& loader);

      SliceViewerWidget* CreateDoseCtWidget(
        std::unique_ptr<OrthancVolumeImage>& ct,
        std::unique_ptr<OrthancVolumeImage>& dose,
        std::unique_ptr<StructureSetLoader>& structLoader,
        VolumeProjection projection);

      void AddCtLayer(SliceViewerWidget& widget, OrthancVolumeImage& volume);

      std::unique_ptr<Interactor>         mainWidgetInteractor_;
      const DicomSeriesVolumeSlicer*    source_;
      unsigned int                      slice_;

      std::string                                        ctSeries_;
      std::string                                        doseInstance_;
      std::string                                        doseSeries_;
      std::string                                        structInstance_;
      std::unique_ptr<OrthancStone::OrthancVolumeImage>    dose_;
      std::unique_ptr<OrthancStone::OrthancVolumeImage>    ct_;
      std::unique_ptr<OrthancStone::StructureSetLoader>    struct_;

    public:
      RtViewerDemoApplication(MessageBroker& broker) :
        IObserver(broker),
        source_(NULL),
        slice_(0)
      {
      }

      /*
      dev options on bgo xps15

      COMMAND LINE
      --ct-series=a04ecf01-79b2fc33-58239f7e-ad9db983-28e81afa --dose-instance=830a69ff-8e4b5ee3-b7f966c8-bccc20fb-d322dceb --struct-instance=54460695-ba3885ee-ddf61ac0-f028e31d-a6e474d9

      URL PARAMETERS
      ?ct-series=a04ecf01-79b2fc33-58239f7e-ad9db983-28e81afa&dose-instance=830a69ff-8e4b5ee3-b7f966c8-bccc20fb-d322dceb&struct-instance=54460695-ba3885ee-ddf61ac0-f028e31d-a6e474d9

      */

      void ParseParameters(const boost::program_options::variables_map&  parameters)
      {
        // Generic
        {
          if (parameters.count("verbose"))
          {
            Orthanc::Logging::EnableInfoLevel(true);
            LOG(INFO) << "Verbose logs (info) are enabled";
          }
        }

        {
          if (parameters.count("trace"))
          {
            LOG(INFO) << "parameters.count(\"trace\") != 0";
            Orthanc::Logging::EnableTraceLevel(true);
            VLOG(1) << "Trace logs (debug) are enabled";
          }
        }

        // CT series
        {

          if (parameters.count("ct-series") != 1)
          {
            LOG(ERROR) << "There must be exactly one CT series specified";
            throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
          }
          ctSeries_ = parameters["ct-series"].as<std::string>();
        }

        // RTDOSE 
        {
          if (parameters.count("dose-instance") == 1)
          {
            doseInstance_ = parameters["dose-instance"].as<std::string>();
          }
          else
          {
#ifdef BGO_NOT_IMPLEMENTED_YET
            // Dose series
            if (parameters.count("dose-series") != 1)
            {
              LOG(ERROR) << "the RTDOSE series is missing";
              throw Orthanc::OrthancException(
                Orthanc::ErrorCode_ParameterOutOfRange);
            }
            doseSeries_ = parameters["ct"].as<std::string>();
#endif
            LOG(ERROR) << "the RTSTRUCT instance is missing";
            throw Orthanc::OrthancException(
              Orthanc::ErrorCode_ParameterOutOfRange);
          }
        }
        
        // RTSTRUCT 
        {
          if (parameters.count("struct-instance") == 1)
          {
            structInstance_ = parameters["struct-instance"].as<std::string>();
          }
          else
          {
#ifdef BGO_NOT_IMPLEMENTED_YET
            // Struct series
            if (parameters.count("struct-series") != 1)
            {
              LOG(ERROR) << "the RTSTRUCT series is missing";
              throw Orthanc::OrthancException(
                Orthanc::ErrorCode_ParameterOutOfRange);
            }
            structSeries_ = parameters["struct-series"].as<std::string>();
#endif
            LOG(ERROR) << "the RTSTRUCT instance is missing";
            throw Orthanc::OrthancException(
              Orthanc::ErrorCode_ParameterOutOfRange);
          }
        }
      }

      virtual void DeclareStartupOptions(
        boost::program_options::options_description& options)
      {
        boost::program_options::options_description generic(
          "RtViewerDemo options. Please note that some of these options "
          "are mutually exclusive");
        generic.add_options()
          ("ct-series", boost::program_options::value<std::string>(),
            "Orthanc ID of the CT series")
          ("dose-instance", boost::program_options::value<std::string>(), 
            "Orthanc ID of the RTDOSE instance (incompatible with dose-series)")
          ("dose-series", boost::program_options::value<std::string>(), 
            "NOT IMPLEMENTED YET. Orthanc ID of the RTDOSE series (incompatible"
            " with dose-instance)")
          ("struct-instance", boost::program_options::value<std::string>(), 
            "Orthanc ID of the RTSTRUCT instance (incompatible with struct-"
            "series)")
          ("struct-series", boost::program_options::value<std::string>(), 
            "NOT IMPLEMENTED YET. Orthanc ID of the RTSTRUCT (incompatible with"
            " struct-instance)")
          ("smooth", boost::program_options::value<bool>()->default_value(true),
            "Enable bilinear image smoothing")
          ;

        options.add(generic);
      }

      virtual void Initialize(
        StoneApplicationContext*                      context,
        IStatusBar&                                   statusBar,
        const boost::program_options::variables_map&  parameters)
      {
        using namespace OrthancStone;

        ParseParameters(parameters);

        context_ = context;

        statusBar.SetMessage("Use the key \"s\" to reinitialize the layout");

        if (!ctSeries_.empty())
        {
          printf("CT = [%s]\n", ctSeries_.c_str());

          ct_.reset(new OrthancStone::OrthancVolumeImage(
            IObserver::GetBroker(), context->GetOrthancApiClient(), false));
          ct_->ScheduleLoadSeries(ctSeries_);
          //ct_->ScheduleLoadSeries(
          //  "a04ecf01-79b2fc33-58239f7e-ad9db983-28e81afa"); // IBA
          //ct_->ScheduleLoadSeries(
          //  "03677739-1d8bca40-db1daf59-d74ff548-7f6fc9c0"); // 0522c0001 TCIA
        }

        if (!doseSeries_.empty() ||
          !doseInstance_.empty())
        {
          dose_.reset(new OrthancStone::OrthancVolumeImage(
            IObserver::GetBroker(), context->GetOrthancApiClient(), true));


          dose_->RegisterObserverCallback(
            new Callable<RtViewerDemoApplication, ISlicedVolume::VolumeReadyMessage>
            (*this, &RtViewerDemoApplication::OnVolumeReadyMessage));

          dose_->RegisterObserverCallback(
            new Callable<RtViewerDemoApplication, ISlicedVolume::SliceContentChangedMessage>
            (*this, &RtViewerDemoApplication::OnSliceContentChangedMessage));

          if (doseInstance_.empty())
          {
            dose_->ScheduleLoadSeries(doseSeries_);
          }
          else
          {
            dose_->ScheduleLoadInstance(doseInstance_);
          }

          //dose_->ScheduleLoadInstance(
            //"830a69ff-8e4b5ee3-b7f966c8-bccc20fb-d322dceb");  // IBA 1
          //dose_->ScheduleLoadInstance(
            //"269f26f4-0c83eeeb-2e67abbd-5467a40f-f1bec90c"); //0522c0001 TCIA
        }

        if (!structInstance_.empty())
        {
          struct_.reset(new OrthancStone::StructureSetLoader(
            IObserver::GetBroker(), context->GetOrthancApiClient()));

          struct_->ScheduleLoadInstance(structInstance_);

          //struct_->ScheduleLoadInstance(
            //"54460695-ba3885ee-ddf61ac0-f028e31d-a6e474d9"); // IBA
          //struct_->ScheduleLoadInstance(
            //"17cd032b-ad92a438-ca05f06a-f9e96668-7e3e9e20"); // 0522c0001 TCIA
        }

        mainWidget_ = new LayoutWidget("main-layout");
        mainWidget_->SetBackgroundColor(0, 0, 0);
        mainWidget_->SetBackgroundCleared(true);
        mainWidget_->SetPadding(0);

        auto axialWidget = CreateDoseCtWidget
        (ct_, dose_, struct_, OrthancStone::VolumeProjection_Axial);
        mainWidget_->AddWidget(axialWidget);
               
        std::unique_ptr<OrthancStone::LayoutWidget> subLayout(
          new OrthancStone::LayoutWidget("main-layout"));
        subLayout->SetVertical();
        subLayout->SetPadding(5);

        auto coronalWidget = CreateDoseCtWidget
        (ct_, dose_, struct_, OrthancStone::VolumeProjection_Coronal);
        subLayout->AddWidget(coronalWidget);

        auto sagittalWidget = CreateDoseCtWidget
        (ct_, dose_, struct_, OrthancStone::VolumeProjection_Sagittal);
        subLayout->AddWidget(sagittalWidget);
        
        mainWidget_->AddWidget(subLayout.release());
      }
    };


    size_t RtViewerDemoApplication::AddDoseLayer(
      SliceViewerWidget& widget,
      OrthancVolumeImage& volume, VolumeProjection projection)
    {
      size_t layer = widget.AddLayer(
        new VolumeImageMPRSlicer(IObserver::GetBroker(), volume));

      RenderStyle s;
      //s.drawGrid_ = true;
      s.SetColor(255, 0, 0);  // Draw missing PET layer in red
      s.alpha_ = 0.3f;
      s.applyLut_ = true;
      s.lut_ = Orthanc::EmbeddedResources::COLORMAP_JET;
      s.interpolation_ = ImageInterpolation_Bilinear;
      widget.SetLayerStyle(layer, s);

      return layer;
    }

    void RtViewerDemoApplication::AddStructLayer(
      SliceViewerWidget& widget, StructureSetLoader& loader)
    {
      widget.AddLayer(new DicomStructureSetSlicer(
        IObserver::GetBroker(), loader));
    }

    SliceViewerWidget* RtViewerDemoApplication::CreateDoseCtWidget(
      std::unique_ptr<OrthancVolumeImage>& ct,
      std::unique_ptr<OrthancVolumeImage>& dose,
      std::unique_ptr<StructureSetLoader>& structLoader,
      VolumeProjection projection)
    {
      std::unique_ptr<OrthancStone::SliceViewerWidget> widget(
        new OrthancStone::SliceViewerWidget(IObserver::GetBroker(),
          "ct-dose-widget"));

      if (ct.get() != NULL)
      {
        AddCtLayer(*widget, *ct);
      }

      if (dose.get() != NULL)
      {
        size_t layer = AddDoseLayer(*widget, *dose, projection);

        // we need to store the dose rendering widget because we'll update them
        // according to various asynchronous events
        doseCtWidgetLayerPairs_.push_back(std::make_pair(widget.get(), layer));
#if 0
        interactors_.push_back(new VolumeImageInteractor(
          IObserver::GetBroker(), *dose, *widget, projection));
#else
        interactors_.push_back(new DoseInteractor(
          IObserver::GetBroker(), *dose, *widget, projection, layer));
#endif
      }
      else if (ct.get() != NULL)
      {
        interactors_.push_back(
          new VolumeImageInteractor(
            IObserver::GetBroker(), *ct, *widget, projection));
      }

      if (structLoader.get() != NULL)
      {
        AddStructLayer(*widget, *structLoader);
      }

      return widget.release();
    }

    void RtViewerDemoApplication::AddCtLayer(
      SliceViewerWidget& widget,
      OrthancVolumeImage& volume)
    {
      size_t layer = widget.AddLayer(
        new VolumeImageMPRSlicer(IObserver::GetBroker(), volume));

      RenderStyle s;
      //s.drawGrid_ = true;
      s.alpha_ = 1;
      s.windowing_ = ImageWindowing_Bone;
      widget.SetLayerStyle(layer, s);
    }
  }
}



#if ORTHANC_ENABLE_WASM==1

#include "Platforms/Wasm/WasmWebService.h"
#include "Platforms/Wasm/WasmViewport.h"

#include <emscripten/emscripten.h>

//#include "SampleList.h"


OrthancStone::IStoneApplication* CreateUserApplication(OrthancStone::MessageBroker& broker)
{
  return new OrthancStone::Samples::RtViewerDemoApplication(broker);
}

OrthancStone::WasmPlatformApplicationAdapter* CreateWasmApplicationAdapter(OrthancStone::MessageBroker& broker, OrthancStone::IStoneApplication* application)
{
  return dynamic_cast<OrthancStone::Samples::RtViewerDemoApplication*>(application)->CreateWasmApplicationAdapter(broker);
}

#else

//#include "SampleList.h"
#if ORTHANC_ENABLE_SDL==1
#include "Applications/Sdl/SdlStoneApplicationRunner.h"
#endif
#if ORTHANC_ENABLE_QT==1
#include "Applications/Qt/SampleQtApplicationRunner.h"
#endif
#include "Framework/Messages/MessageBroker.h"

int main(int argc, char* argv[])
{
  OrthancStone::MessageBroker broker;
  OrthancStone::Samples::RtViewerDemoApplication sampleStoneApplication(broker);

#if ORTHANC_ENABLE_SDL==1
  OrthancStone::SdlStoneApplicationRunner sdlApplicationRunner(broker, sampleStoneApplication);
  return sdlApplicationRunner.Execute(argc, argv);
#endif
#if ORTHANC_ENABLE_QT==1
  OrthancStone::Samples::SampleQtApplicationRunner qtAppRunner(broker, sampleStoneApplication);
  return qtAppRunner.Execute(argc, argv);
#endif
}


#endif







