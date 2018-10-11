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

#include "Applications/IStoneApplication.h"

#include "Framework/Layers/OrthancFrameLayerSource.h"
#include "Framework/Layers/CircleMeasureTracker.h"
#include "Framework/Layers/LineMeasureTracker.h"
#include "Framework/Widgets/LayerWidget.h"
#include "Framework/Widgets/LayoutWidget.h"
#include "Framework/Messages/IObserver.h"
#include "Framework/SmartLoader.h"

#if ORTHANC_ENABLE_WASM==1
#include "Platforms/Wasm/WasmPlatformApplicationAdapter.h"
#include "Platforms/Wasm/Defaults.h"
#endif

#if ORTHANC_ENABLE_QT==1
#include "Qt/SimpleViewerMainWindow.h"
#endif

#include <Core/Logging.h>

#include "ThumbnailInteractor.h"
#include "MainWidgetInteractor.h"
#include "AppStatus.h"
#include "Messages.h"

using namespace OrthancStone;


namespace SimpleViewer
{

  class SimpleViewerApplication :
      public IStoneApplication,
      public IObserver,
      public IObservable
  {
  public:

    struct StatusUpdatedMessage : public BaseMessage<SimpleViewerMessageType_AppStatusUpdated>
    {
      const AppStatus& status_;

      StatusUpdatedMessage(const AppStatus& status)
        : BaseMessage(),
          status_(status)
      {
      }
    };

    enum Tools {
      Tools_LineMeasure,
      Tools_CircleMeasure,
      Tools_Crop,
      Tools_Windowing,
      Tools_Zoom,
      Tools_Pan
    };

    enum Actions {
      Actions_Rotate,
      Actions_Invert
    };

  private:
    Tools                           currentTool_;
    std::unique_ptr<MainWidgetInteractor> mainWidgetInteractor_;
    std::unique_ptr<ThumbnailInteractor>  thumbnailInteractor_;
    LayoutWidget*                   mainLayout_;
    LayoutWidget*                   thumbnailsLayout_;
    LayerWidget*                    mainWidget_;
    std::vector<LayerWidget*>       thumbnails_;
    std::map<std::string, std::vector<std::string>> instancesIdsPerSeriesId_;
    std::map<std::string, Json::Value> seriesTags_;
    BaseCommandBuilder              commandBuilder_;

    unsigned int                    currentInstanceIndex_;
    OrthancStone::WidgetViewport*   wasmViewport1_;
    OrthancStone::WidgetViewport*   wasmViewport2_;

    IStatusBar*                     statusBar_;
    std::unique_ptr<SmartLoader>    smartLoader_;
    std::unique_ptr<OrthancApiClient>      orthancApiClient_;

  public:
    SimpleViewerApplication(MessageBroker& broker) :
      IObserver(broker),
      IObservable(broker),
      currentTool_(Tools_LineMeasure),
      mainLayout_(NULL),
      currentInstanceIndex_(0),
      wasmViewport1_(NULL),
      wasmViewport2_(NULL)
    {
    }

    virtual void Finalize() {}
    virtual IWidget* GetCentralWidget() {return mainLayout_;}

    virtual void DeclareStartupOptions(boost::program_options::options_description& options);
    virtual void Initialize(StoneApplicationContext* context,
                            IStatusBar& statusBar,
                            const boost::program_options::variables_map& parameters);

    void OnStudyListReceived(const OrthancApiClient::JsonResponseReadyMessage& message);

    void OnStudyReceived(const OrthancApiClient::JsonResponseReadyMessage& message);

    void OnSeriesReceived(const OrthancApiClient::JsonResponseReadyMessage& message);

    void LoadThumbnailForSeries(const std::string& seriesId, const std::string& instanceId);

    void SelectStudy(const std::string& studyId);

    void OnWidgetGeometryChanged(const LayerWidget::GeometryChangedMessage& message);

    void SelectSeriesInMainViewport(const std::string& seriesId);

    void SelectTool(Tools tool);
    Tools GetCurrentTool() const {return currentTool_;}

    void ExecuteAction(Actions action);

    virtual std::string GetTitle() const {return "SimpleViewer";}
    virtual void ExecuteCommand(ICommand& command);
    virtual BaseCommandBuilder& GetCommandBuilder() {return commandBuilder_;}


#if ORTHANC_ENABLE_WASM==1
    virtual void InitializeWasm();
#endif

#if ORTHANC_ENABLE_QT==1
    virtual QStoneMainWindow* CreateQtMainWindow();
#endif
  };


}
