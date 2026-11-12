/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2023 Osimis S.A., Belgium
 * Copyright (C) 2021-2026 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
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


#include "../../../OrthancStone/Sources/Loaders/OrthancMultiframeVolumeLoader.h"
#include "../../../OrthancStone/Sources/Loaders/OrthancSeriesVolumeProgressiveLoader.h"
#include "../../../OrthancStone/Sources/Scene2D/GrayscaleStyleConfigurator.h"
#include "../../../OrthancStone/Sources/StoneException.h"
#include "../../../OrthancStone/Sources/StoneInitialization.h"
#include "../../../OrthancStone/Sources/Viewport/DefaultViewportInteractor.h"
#include "../../../OrthancStone/Sources/Volumes/DicomVolumeImageMPRSlicer.h"
#include "../../../OrthancStone/Sources/Volumes/VolumeSceneLayerSource.h"
#include "SingleViewportSdlApplication.h"

#include <boost/program_options.hpp>



namespace OrthancStone
{
  class VolumeViewerCore :
    public ISingleViewportApplicationCore,
    public Old::ObserverBase<VolumeViewerCore>
  {
  private:
    boost::shared_ptr<StoneApplication::Context>  context_;
    boost::shared_ptr<ViewportController>         controller_;

    VolumeProjection                 projection_;
    size_t                           currentPlane_;
    std::vector<CoordinateSystem3D>  planes_;

    boost::shared_ptr<DicomVolumeImage>                      volume_;
    boost::shared_ptr<OrthancSeriesVolumeProgressiveLoader>  seriesLoader_;
    boost::shared_ptr<OrthancMultiframeVolumeLoader>         instanceLoader_;
    boost::shared_ptr<IVolumeSlicer>                         slicer_;
    std::unique_ptr<VolumeSceneLayerSource>                  source_;

    void Invalidate()
    {
      if (currentPlane_ < planes_.size())
      {
        source_->Update(controller_->GetScene(), planes_[currentPlane_]);
      }

      controller_->InvalidateViewport();
    }

    void UpdateProjection()
    {
      if (volume_->HasGeometry())
      {
        VolumeImageGeometry geometry = volume_->GetGeometry();

        const unsigned int depth = geometry.GetProjectionDepth(projection_);
        currentPlane_ = depth / 2;

        LOG(WARNING) << "Number of cutting planes along the projection: " << depth;

        planes_.resize(depth);

        for (unsigned int z = 0; z < depth; z++)
        {
          planes_[z] = geometry.GetProjectionSlice(projection_, z);
        }
      }
      else
      {
        planes_.clear();
        currentPlane_ = 0;
      }

      Invalidate();
    }

    void OnGeometryReady(const DicomVolumeImage::GeometryReadyMessage& message)
    {
      LOG(WARNING) << "Dimension of the volume: "
                   << volume_->GetGeometry().GetWidth() << "x"
                   << volume_->GetGeometry().GetHeight() << "x"
                   << volume_->GetGeometry().GetDepth();
      UpdateProjection();
    }

    void OnContentUpdated(const DicomVolumeImage::ContentUpdatedMessage& message)
    {
      Invalidate();
    }

  protected:
    virtual void CreateComponents(const boost::shared_ptr<StoneApplication::Context>& context,
                                  const boost::shared_ptr<ViewportController>& controller) ORTHANC_OVERRIDE
    {
      context_ = context;
      controller_ = controller;

      volume_.reset(new DicomVolumeImage);
      slicer_.reset(new DicomVolumeImageMPRSlicer(volume_));
      source_.reset(new VolumeSceneLayerSource(0, slicer_));

      std::unique_ptr<GrayscaleStyleConfigurator> style(new GrayscaleStyleConfigurator);
      style->SetLinearInterpolation(true);
      source_->SetConfigurator(style.release());

      seriesLoader_ = OrthancSeriesVolumeProgressiveLoader::Create(*context, volume_, true /* progressive */);
      instanceLoader_ = OrthancMultiframeVolumeLoader::Create(*context, volume_);

      Register<DicomVolumeImage::GeometryReadyMessage>(*seriesLoader_, &VolumeViewerCore::OnGeometryReady);
      Register<DicomVolumeImage::ContentUpdatedMessage>(*seriesLoader_, &VolumeViewerCore::OnContentUpdated);

      Register<DicomVolumeImage::GeometryReadyMessage>(*instanceLoader_, &VolumeViewerCore::OnGeometryReady);
      Register<DicomVolumeImage::ContentUpdatedMessage>(*instanceLoader_, &VolumeViewerCore::OnContentUpdated);
    }

    virtual IViewportInteractor* CreateMouseInteractor() ORTHANC_OVERRIDE
    {
      return new DefaultViewportInteractor;
    }

    virtual void HandleKeyDown(const IEnvironment::ILock& environmentLock,
                               char key) ORTHANC_OVERRIDE
    {
      switch (key)
      {
        case 'a':
          projection_ = VolumeProjection_Axial;
          UpdateProjection();
          break;

        case 'c':
          projection_ = VolumeProjection_Coronal;
          UpdateProjection();
          break;

        case 's':
          projection_ = VolumeProjection_Sagittal;
          UpdateProjection();
          break;
      }
    }

    virtual void HandleMouseWheel(const IEnvironment::ILock& environmentLock,
                                  MouseWheelDirection direction,
                                  bool fast) ORTHANC_OVERRIDE
    {
      int delta;

      switch (direction)
      {
        case MouseWheelDirection_Down:
          delta = 1;
          break;

        case MouseWheelDirection_Up:
          delta = -1;
          break;

        default:
          break;
      }

      if (fast)
      {
        delta *= static_cast<int>(planes_.size()) / 10;
      }

      int tmp = static_cast<int>(currentPlane_) + delta;

      size_t nextPlane;

      if (tmp < 0)
      {
        nextPlane = 0;
      }
      else if (tmp >= static_cast<int>(planes_.size()))
      {
        nextPlane = planes_.size() - 1;
      }
      else
      {
        nextPlane = tmp;
      }

      if (nextPlane != currentPlane_)
      {
        currentPlane_ = nextPlane;
        Invalidate();
      }
    }

  public:
    VolumeViewerCore() :
      // Dummy initialization
      projection_(VolumeProjection::VolumeProjection_Axial),
      currentPlane_(0)
    {
    }

    void LoadSeriesFromOrthanc(VolumeProjection projection,
                               const std::string& seriesId)
    {
      projection_ = projection;
      seriesLoader_->LoadSeries(seriesId);
    }

    void LoadInstanceFromOrthanc(VolumeProjection projection,
                                 const std::string& instanceId)
    {
      projection_ = projection;
      instanceLoader_->LoadInstance(instanceId);
    }
  };


  class Startup : public SingleViewportSdlApplication::ICoreStartup
  {
  private:
    std::string            orthancUrl_;
    Orthanc::ResourceType  level_;
    std::string            resourceId_;
    VolumeProjection       projection_;

  public:
    Startup(int argc, char* argv[]) :
      orthancUrl_("http://orthanc:orthanc@localhost:8043"),
      level_(Orthanc::ResourceType_Series),
      resourceId_("a7e40137-e4b37f56-14ce875b-9a81702d-eac823bf"),
      projection_(VolumeProjection_Axial)
    {
      namespace po = boost::program_options;
      po::options_description desc("Usage");

      desc.add_options()
        ("orthanc", po::value<std::string>()->default_value(orthancUrl_),
         "Base URL of the Orthanc instance")

        ("series", po::value<std::string>(),
         "Orthanc ID of the series to display")

        ("instance", po::value<std::string>(),
         "Orthanc ID of the instance to display")

        ("projection", po::value<std::string>()->default_value("axial"),
         "Projection to use: \"axial\", \"sagittal\", or \"coronal\"")
        ;

      std::cout << desc << std::endl;

      po::variables_map vm;
      try
      {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
      }
      catch (std::exception& e)
      {
        std::cerr << "Please check your command line options! (\"" << e.what() << "\")" << std::endl;
      }

      if (vm.count("orthanc") > 0)
      {
        // maybe check URL validity here
        orthancUrl_ = vm["orthanc"].as<std::string>();
      }

      if (vm.count("series") > 0 &&
          vm.count("instance") > 0)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadParameterType,
                                        "Command-line options \"series\" and \"instance\" are mutually exclusive");
      }
      else if (vm.count("series") > 0)
      {
        level_ = Orthanc::ResourceType_Series;
        resourceId_ = vm["series"].as<std::string>();
      }
      else if (vm.count("instance") > 0)
      {
        level_ = Orthanc::ResourceType_Instance;
        resourceId_ = vm["instance"].as<std::string>();
      }

      if (vm.count("projection") > 0)
      {
        const std::string projection = vm["projection"].as<std::string>();

        if (projection == "axial")
        {
          projection_ = VolumeProjection_Axial;
        }
        else if (projection == "sagittal")
        {
          projection_ = VolumeProjection_Sagittal;
        }
        else if (projection == "coronal")
        {
          projection_ = VolumeProjection_Coronal;
        }
        else
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange, "Bad projection: " + projection);
        }
      }
    }

    const std::string& GetOrthancUrl() const
    {
      return orthancUrl_;
    }

    virtual void Start(ISingleViewportApplicationCore& core) ORTHANC_OVERRIDE
    {
      switch (level_)
      {
        case Orthanc::ResourceType_Series:
          dynamic_cast<VolumeViewerCore&>(core).LoadSeriesFromOrthanc(projection_, resourceId_);
          break;

        case Orthanc::ResourceType_Instance:
          dynamic_cast<VolumeViewerCore&>(core).LoadInstanceFromOrthanc(projection_, resourceId_);
          break;

        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
      }
    }
  };
}


/**
 * IMPORTANT: The full arguments to "main()" are needed for SDL on
 * Windows. Otherwise, one gets the linking error "undefined reference
 * to `SDL_main'". https://wiki.libsdl.org/FAQWindows
 **/
int main(int argc, char* argv[])
{
  int status = 0;

  OrthancStone::StoneInitialize();
  OrthancStone::SdlWindow::GlobalInitialize();

  try
  {
    std::unique_ptr<OrthancStone::Startup> startup(new OrthancStone::Startup(argc, argv));

    Orthanc::WebServiceParameters orthancWebService;
    orthancWebService.SetUrl(startup->GetOrthancUrl());

    OrthancStone::StoneApplication::Configuration configuration;
    configuration.SetRemoteOrthancParameters(orthancWebService);
    configuration.SetDicomCacheSize(128 * 1024 * 1024);  // TODO Refactoring - Remove this

    boost::shared_ptr<OrthancStone::ISingleViewportApplicationCore> core(new OrthancStone::VolumeViewerCore);

    OrthancStone::SingleViewportSdlApplication app(
      configuration, core, "Stone of Orthanc", 800, 600, true /* use OpenGL */);

    app.SetCoreStartup(startup.release());
    app.Run();
  }
  catch (Orthanc::OrthancException& e)
  {
    LOG(ERROR) << "OrthancException: " << e.What();
    status = -1;
  }
  catch (OrthancStone::StoneException& e)
  {
    LOG(ERROR) << "StoneException: " << e.What();
    status = -1;
  }
  catch (std::runtime_error& e)
  {
    LOG(ERROR) << "Runtime error: " << e.what();
    status = -1;
  }
  catch (...)
  {
    LOG(ERROR) << "Native exception";
    status = -1;
  }

  OrthancStone::SdlWindow::GlobalFinalize();
  OrthancStone::StoneFinalize();

  return status;
}
