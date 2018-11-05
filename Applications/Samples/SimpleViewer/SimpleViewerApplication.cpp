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


#include "SimpleViewerApplication.h"

#if ORTHANC_ENABLE_QT == 1
#  include "Qt/SimpleViewerMainWindow.h"
#endif

#if ORTHANC_ENABLE_WASM == 1
#  include <Platforms/Wasm/WasmViewport.h>
#endif

namespace SimpleViewer
{

  void SimpleViewerApplication::Initialize(StoneApplicationContext* context,
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

      mainWidget_ = new LayerWidget(IObserver::broker_, "main-viewport");
      //mainWidget_->RegisterObserver(*this);

      // hierarchy
      mainLayout_->AddWidget(thumbnailsLayout_);
      mainLayout_->AddWidget(mainWidget_);

      orthancApiClient_.reset(new OrthancApiClient(IObserver::broker_, context_->GetWebService()));

      // sources
      smartLoader_.reset(new SmartLoader(IObserver::broker_, *orthancApiClient_));
      smartLoader_->SetImageQuality(SliceImageQuality_FullPam);

      mainLayout_->SetTransmitMouseOver(true);
      mainWidgetInteractor_.reset(new MainWidgetInteractor(*this));
      mainWidget_->SetInteractor(*mainWidgetInteractor_);
      thumbnailInteractor_.reset(new ThumbnailInteractor(*this));
    }

    statusBar.SetMessage("Use the key \"s\" to reinitialize the layout");
    statusBar.SetMessage("Use the key \"n\" to go to next image in the main viewport");


    if (parameters.count("studyId") < 1)
    {
      LOG(WARNING) << "The study ID is missing, will take the first studyId found in Orthanc";
      orthancApiClient_->GetJsonAsync("/studies", new Callable<SimpleViewerApplication, OrthancApiClient::JsonResponseReadyMessage>(*this, &SimpleViewerApplication::OnStudyListReceived));
    }
    else
    {
      SelectStudy(parameters["studyId"].as<std::string>());
    }
  }


  void SimpleViewerApplication::DeclareStartupOptions(boost::program_options::options_description& options)
  {
    boost::program_options::options_description generic("Sample options");
    generic.add_options()
        ("studyId", boost::program_options::value<std::string>(),
         "Orthanc ID of the study")
        ;

    options.add(generic);
  }

  void SimpleViewerApplication::OnStudyListReceived(const OrthancApiClient::JsonResponseReadyMessage& message)
  {
    const Json::Value& response = message.GetJson();

    if (response.isArray() && response.size() > 1)
    {
      SelectStudy(response[0].asString());
    }
  }
  void SimpleViewerApplication::OnStudyReceived(const OrthancApiClient::JsonResponseReadyMessage& message)
  {
    const Json::Value& response = message.GetJson();

    if (response.isObject() && response["Series"].isArray())
    {
      for (size_t i=0; i < response["Series"].size(); i++)
      {
        orthancApiClient_->GetJsonAsync("/series/" + response["Series"][(int)i].asString(), new Callable<SimpleViewerApplication, OrthancApiClient::JsonResponseReadyMessage>(*this, &SimpleViewerApplication::OnSeriesReceived));
      }
    }
  }

  void SimpleViewerApplication::OnSeriesReceived(const OrthancApiClient::JsonResponseReadyMessage& message)
  {
    const Json::Value& response = message.GetJson();

    if (response.isObject() && response["Instances"].isArray() && response["Instances"].size() > 0)
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

  void SimpleViewerApplication::LoadThumbnailForSeries(const std::string& seriesId, const std::string& instanceId)
  {
    LOG(INFO) << "Loading thumbnail for series " << seriesId;
    LayerWidget* thumbnailWidget = new LayerWidget(IObserver::broker_, "thumbnail-series-" + seriesId);
    thumbnails_.push_back(thumbnailWidget);
    thumbnailsLayout_->AddWidget(thumbnailWidget);
    thumbnailWidget->RegisterObserverCallback(new Callable<SimpleViewerApplication, LayerWidget::GeometryChangedMessage>(*this, &SimpleViewerApplication::OnWidgetGeometryChanged));
    smartLoader_->SetFrameInWidget(*thumbnailWidget, 0, instanceId, 0);
    thumbnailWidget->SetInteractor(*thumbnailInteractor_);
  }

  void SimpleViewerApplication::SelectStudy(const std::string& studyId)
  {
    orthancApiClient_->GetJsonAsync("/studies/" + studyId, new Callable<SimpleViewerApplication, OrthancApiClient::JsonResponseReadyMessage>(*this, &SimpleViewerApplication::OnStudyReceived));
  }

  void SimpleViewerApplication::OnWidgetGeometryChanged(const LayerWidget::GeometryChangedMessage& message)
  {
    message.GetOrigin().FitContent();
  }

  void SimpleViewerApplication::SelectSeriesInMainViewport(const std::string& seriesId)
  {
    smartLoader_->SetFrameInWidget(*mainWidget_, 0, instancesIdsPerSeriesId_[seriesId][0], 0);
  }



  void SimpleViewerApplication::ExecuteCommand(ICommand& command)
  {
    statusBar_->SetMessage("received command: " + std::string(command.GetName()));
    if (command.GetName() == "selectTool:circle-measure")
    {
      SelectTool(Tools_CircleMeasure);
    }
    else if (command.GetName() == "selectTool:line-measure")
    {
      SelectTool(Tools_LineMeasure);
    }
    else if (command.GetName() == "selectTool:crop")
    {
      SelectTool(Tools_Crop);
    }
    else if (command.GetName() == "selectTool:windowing")
    {
      SelectTool(Tools_Windowing);
    }
    else if (command.GetName() == "action:rotate")
    {
      ExecuteAction(Actions_Rotate);
    }
    else if (command.GetName() == "action:undo-crop")
    {
      ExecuteAction(Actions_UndoCrop);
    }
    else if (command.GetName() == "action:invert")
    {
      ExecuteAction(Actions_Invert);
    }
    else
    {
      command.Execute();
    }
  }

  void SimpleViewerApplication::ExecuteAction(SimpleViewerApplication::Actions action)
  {
    // TODO
  }

  void SimpleViewerApplication::SelectTool(SimpleViewerApplication::Tools tool)
  {
    currentTool_ = tool;
  }

#if ORTHANC_ENABLE_QT==1
  QStoneMainWindow* SimpleViewerApplication::CreateQtMainWindow()
  {
    return new SimpleViewerMainWindow(dynamic_cast<OrthancStone::NativeStoneApplicationContext&>(*context_), *this);
  }
#endif

#if ORTHANC_ENABLE_WASM==1
  void SimpleViewerApplication::InitializeWasm() {

    AttachWidgetToWasmViewport("canvasThumbnails", thumbnailsLayout_);
    AttachWidgetToWasmViewport("canvasMain", mainWidget_);
  }
#endif


}
