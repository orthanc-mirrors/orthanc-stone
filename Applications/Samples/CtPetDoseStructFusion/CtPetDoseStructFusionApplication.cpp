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


#include "CtPetDoseStructFusionApplication.h"

#if ORTHANC_ENABLE_QT == 1
#  include "Qt/CtPetDoseStructFusionMainWindow.h"
#endif

#if ORTHANC_ENABLE_WASM == 1
#  include <Platforms/Wasm/WasmViewport.h>
#endif

namespace CtPetDoseStructFusion
{

  void CtPetDoseStructFusionApplication::Initialize(StoneApplicationContext* context,
                                           IStatusBar& statusBar,
                                           const boost::program_options::variables_map& parameters)
  {
    using namespace OrthancStone;

    context_ = context;
    statusBar_ = &statusBar;

    {// initialize viewports and layout
      mainLayout_ = new LayoutWidget("main-layout");
      mainLayout_->SetPadding(10);
      mainLayout_->SetBackgroundCleared(true);
      mainLayout_->SetBackgroundColor(0, 0, 0);
      mainLayout_->SetHorizontal();

      thumbnailsLayout_ = new LayoutWidget("thumbnail-layout");
      thumbnailsLayout_->SetPadding(10);
      thumbnailsLayout_->SetBackgroundCleared(true);
      thumbnailsLayout_->SetBackgroundColor(50, 50, 50);
      thumbnailsLayout_->SetVertical();

      mainWidget_ = new SliceViewerWidget(IObserver::GetBroker(), "main-viewport");
      //mainWidget_->RegisterObserver(*this);

      // hierarchy
      mainLayout_->AddWidget(thumbnailsLayout_);
      mainLayout_->AddWidget(mainWidget_);

      // sources
      smartLoader_.reset(new SmartLoader(IObserver::GetBroker(), context->GetOrthancApiClient()));
      smartLoader_->SetImageQuality(SliceImageQuality_FullPam);

      mainLayout_->SetTransmitMouseOver(true);
      mainWidgetInteractor_.reset(new MainWidgetInteractor(*this));
      mainWidget_->SetInteractor(*mainWidgetInteractor_);
      thumbnailInteractor_.reset(new ThumbnailInteractor(*this));
    }

    statusBar.SetMessage("Use the key \"s\" to reinitialize the layout");
    statusBar.SetMessage("Use the key \"n\" to go to next image in the main viewport");

#if TODO_BGO_CDSF
    if (parameters.count("studyId") < 1)
    {
      LOG(WARNING) << "The study ID is missing, will take the first studyId found in Orthanc";
      context->GetOrthancApiClient().GetJsonAsync("/studies", new Callable<CtPetDoseStructFusionApplication, OrthancApiClient::JsonResponseReadyMessage>(*this, &CtPetDoseStructFusionApplication::OnStudyListReceived));
    }
    else
    {
      SelectStudy(parameters["studyId"].as<std::string>());
    }
#endif
// TODO_BGO_CDSF
  }


  void CtPetDoseStructFusionApplication::DeclareStartupOptions(boost::program_options::options_description& options)
  {
    boost::program_options::options_description generic("Sample options");
#if TODO_BGO_CDSF
    generic.add_options()
        ("studyId", boost::program_options::value<std::string>(),
         "Orthanc ID of the study")
        ;

    options.add(generic);
#endif
  }

  void CtPetDoseStructFusionApplication::OnStudyListReceived(const OrthancApiClient::JsonResponseReadyMessage& message)
  {
    const Json::Value& response = message.GetJson();

    if (response.isArray() &&
        response.size() >= 1)
    {
      SelectStudy(response[0].asString());
    }
  }
  void CtPetDoseStructFusionApplication::OnStudyReceived(const OrthancApiClient::JsonResponseReadyMessage& message)
  {
    const Json::Value& response = message.GetJson();

    if (response.isObject() && response["Series"].isArray())
    {
      for (size_t i=0; i < response["Series"].size(); i++)
      {
        context_->GetOrthancApiClient().GetJsonAsync(
          "/series/" + response["Series"][(int)i].asString(), 
          new Callable<
              CtPetDoseStructFusionApplication
            , OrthancApiClient::JsonResponseReadyMessage>(
              *this, 
              &CtPetDoseStructFusionApplication::OnSeriesReceived
          )
        );
      }
    }
  }

  void CtPetDoseStructFusionApplication::OnSeriesReceived(const OrthancApiClient::JsonResponseReadyMessage& message)
  {
    const Json::Value& response = message.GetJson();

    if (response.isObject() &&
        response["Instances"].isArray() &&
        response["Instances"].size() > 0)
    {
      // keep track of all instances IDs
      const std::string& seriesId = response["ID"].asString();
      seriesTags_[seriesId] = response;
      instancesIdsPerSeriesId_[seriesId] = std::vector<std::string>();
      for (size_t i = 0; i < response["Instances"].size(); i++)
      {
        const std::string& instanceId = response["Instances"][static_cast<int>(i)].asString();
        instancesIdsPerSeriesId_[seriesId].push_back(instanceId);
      }

      // load the first instance in the thumbnail
      LoadThumbnailForSeries(seriesId, instancesIdsPerSeriesId_[seriesId][0]);

      // if this is the first thumbnail loaded, load the first instance in the mainWidget
      if (mainWidget_->GetLayerCount() == 0)
      {
        smartLoader_->SetFrameInWidget(*mainWidget_, 0, instancesIdsPerSeriesId_[seriesId][0], 0);
      }
    }
  }

  void CtPetDoseStructFusionApplication::LoadThumbnailForSeries(const std::string& seriesId, const std::string& instanceId)
  {
    LOG(INFO) << "Loading thumbnail for series " << seriesId;
    
    SliceViewerWidget* thumbnailWidget = 
      new SliceViewerWidget(IObserver::GetBroker(), "thumbnail-series-" + seriesId);
    thumbnails_.push_back(thumbnailWidget);
    thumbnailsLayout_->AddWidget(thumbnailWidget);
    
    thumbnailWidget->RegisterObserverCallback(
      new Callable<CtPetDoseStructFusionApplication, SliceViewerWidget::GeometryChangedMessage>
      (*this, &CtPetDoseStructFusionApplication::OnWidgetGeometryChanged));
    
    smartLoader_->SetFrameInWidget(*thumbnailWidget, 0, instanceId, 0);
    thumbnailWidget->SetInteractor(*thumbnailInteractor_);
  }

  void CtPetDoseStructFusionApplication::SelectStudy(const std::string& studyId)
  {
    context_->GetOrthancApiClient().GetJsonAsync("/studies/" + studyId, new Callable<CtPetDoseStructFusionApplication, OrthancApiClient::JsonResponseReadyMessage>(*this, &CtPetDoseStructFusionApplication::OnStudyReceived));
  }

  void CtPetDoseStructFusionApplication::OnWidgetGeometryChanged(const SliceViewerWidget::GeometryChangedMessage& message)
  {
    // TODO: The "const_cast" could probably be replaced by "mainWidget_"
    const_cast<SliceViewerWidget&>(message.GetOrigin()).FitContent();
  }

  void CtPetDoseStructFusionApplication::SelectSeriesInMainViewport(const std::string& seriesId)
  {
    smartLoader_->SetFrameInWidget(*mainWidget_, 0, instancesIdsPerSeriesId_[seriesId][0], 0);
  }

  bool CtPetDoseStructFusionApplication::Handle(const StoneSampleCommands::SelectTool& value) ORTHANC_OVERRIDE
  {
    currentTool_ = value.tool;
    return true;
  }

  bool CtPetDoseStructFusionApplication::Handle(const StoneSampleCommands::Action& value) ORTHANC_OVERRIDE
  {
    switch (value.type)
    {
    case ActionType_Invert:
      // TODO
      break;
    case ActionType_UndoCrop:
      // TODO
      break;
    case ActionType_Rotate:
      // TODO
      break;
    default:
      throw std::runtime_error("Action type not supported");
    }
    return true;
  }

#if ORTHANC_ENABLE_QT==1
  QStoneMainWindow* CtPetDoseStructFusionApplication::CreateQtMainWindow()
  {
    return new CtPetDoseStructFusionMainWindow(dynamic_cast<OrthancStone::NativeStoneApplicationContext&>(*context_), *this);
  }
#endif

#if ORTHANC_ENABLE_WASM==1
  void CtPetDoseStructFusionApplication::InitializeWasm() {

    AttachWidgetToWasmViewport("canvasThumbnails", thumbnailsLayout_);
    AttachWidgetToWasmViewport("canvasMain", mainWidget_);
  }
#endif


}
