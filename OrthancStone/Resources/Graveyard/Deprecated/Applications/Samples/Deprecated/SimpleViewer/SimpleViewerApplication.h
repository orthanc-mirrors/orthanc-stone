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


#pragma once

 /*
 This header contains the command definitions for the sample applications
 */
#include "Applications/Samples/StoneSampleCommands_generated.hpp"
using namespace StoneSampleCommands;

#include "Applications/IStoneApplication.h"

#include "../../../../Framework/Deprecated/Layers/CircleMeasureTracker.h"
#include "../../../../Framework/Deprecated/Layers/LineMeasureTracker.h"
#include "../../../../Framework/Deprecated/SmartLoader.h"
#include "../../../../Framework/Deprecated/Widgets/LayoutWidget.h"
#include "../../../../Framework/Deprecated/Widgets/SliceViewerWidget.h"
#include "../../../../Framework/Messages/IObserver.h"

#if ORTHANC_ENABLE_WASM==1
#include "Platforms/Wasm/WasmPlatformApplicationAdapter.h"
#include "Platforms/Wasm/Defaults.h"
#endif

#if ORTHANC_ENABLE_QT==1
#include "Qt/SimpleViewerMainWindow.h"
#endif

#include <Core/Images/Font.h>
#include <Core/Logging.h>

#include "ThumbnailInteractor.h"
#include "MainWidgetInteractor.h"
#include "AppStatus.h"

using namespace OrthancStone;


namespace SimpleViewer
{

  class SimpleViewerApplication
    : public IStoneApplication
    , public IObserver
    , public IObservable
    , public StoneSampleCommands::IHandler
  {
  public:

    struct StatusUpdatedMessage : public IMessage
    {
      ORTHANC_STONE_MESSAGE(__FILE__, __LINE__);

      const AppStatus& status_;

      StatusUpdatedMessage(const AppStatus& status)
        : status_(status)
      {
      }
    };

  private:
    Tool                                currentTool_;

    std::unique_ptr<MainWidgetInteractor> mainWidgetInteractor_;
    std::unique_ptr<ThumbnailInteractor>  thumbnailInteractor_;
    Deprecated::LayoutWidget*                       mainLayout_;
    Deprecated::LayoutWidget*                       thumbnailsLayout_;
    Deprecated::SliceViewerWidget*                  mainWidget_;
    std::vector<Deprecated::SliceViewerWidget*>     thumbnails_;
    std::map<std::string, std::vector<std::string> > instancesIdsPerSeriesId_;
    std::map<std::string, Json::Value>  seriesTags_;
    unsigned int                        currentInstanceIndex_;
    Deprecated::WidgetViewport*       wasmViewport1_;
    Deprecated::WidgetViewport*       wasmViewport2_;

    Deprecated::IStatusBar*                         statusBar_;
    std::unique_ptr<Deprecated::SmartLoader>          smartLoader_;

    Orthanc::Font                       font_;

  public:
    SimpleViewerApplication(MessageBroker& broker) :
      IObserver(broker),
      IObservable(broker),
      currentTool_(StoneSampleCommands::Tool_LineMeasure),
      mainLayout_(NULL),
      currentInstanceIndex_(0),
      wasmViewport1_(NULL),
      wasmViewport2_(NULL)
    {
      font_.LoadFromResource(Orthanc::EmbeddedResources::FONT_UBUNTU_MONO_BOLD_16);
    }

    virtual void Finalize() ORTHANC_OVERRIDE {}
    virtual Deprecated::IWidget* GetCentralWidget() ORTHANC_OVERRIDE {return mainLayout_;}

    virtual void DeclareStartupOptions(boost::program_options::options_description& options) ORTHANC_OVERRIDE;
    virtual void Initialize(StoneApplicationContext* context,
                            Deprecated::IStatusBar& statusBar,
                            const boost::program_options::variables_map& parameters) ORTHANC_OVERRIDE;

    void OnStudyListReceived(const Deprecated::OrthancApiClient::JsonResponseReadyMessage& message);

    void OnStudyReceived(const Deprecated::OrthancApiClient::JsonResponseReadyMessage& message);

    void OnSeriesReceived(const Deprecated::OrthancApiClient::JsonResponseReadyMessage& message);

    void LoadThumbnailForSeries(const std::string& seriesId, const std::string& instanceId);

    void SelectStudy(const std::string& studyId);

    void OnWidgetGeometryChanged(const Deprecated::SliceViewerWidget::GeometryChangedMessage& message);

    void SelectSeriesInMainViewport(const std::string& seriesId);


    Tool GetCurrentTool() const
    {
      return currentTool_;
    }

    const Orthanc::Font& GetFont() const
    {
      return font_;
    }

    // ExecuteAction method was empty (its body was a single "TODO" comment)
    virtual bool Handle(const SelectTool& value) ORTHANC_OVERRIDE;
    virtual bool Handle(const Action& value) ORTHANC_OVERRIDE;

    template<typename T>
    bool ExecuteCommand(const T& cmd)
    {
      std::string cmdStr = StoneSampleCommands::StoneSerialize(cmd);
      return StoneSampleCommands::StoneDispatchToHandler(cmdStr, this);
    }

    virtual void HandleSerializedMessage(const char* data) ORTHANC_OVERRIDE
    {
      StoneSampleCommands::StoneDispatchToHandler(data, this);
    }

    virtual std::string GetTitle() const ORTHANC_OVERRIDE {return "SimpleViewer";}

#if ORTHANC_ENABLE_WASM==1
    virtual void InitializeWasm() ORTHANC_OVERRIDE;
#endif

#if ORTHANC_ENABLE_QT==1
    virtual QStoneMainWindow* CreateQtMainWindow();
#endif
  };


}
