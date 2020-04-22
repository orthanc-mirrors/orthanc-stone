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

#include "SampleApplicationBase.h"

#include "../../../Framework/Deprecated/Layers/CircleMeasureTracker.h"
#include "../../../Framework/Deprecated/Layers/LineMeasureTracker.h"
#include "../../../Framework/Deprecated/SmartLoader.h"
#include "../../../Framework/Deprecated/Widgets/LayoutWidget.h"
#include "../../../Framework/Deprecated/Widgets/SliceViewerWidget.h"
#include "../../../Framework/Messages/IObserver.h"

#if ORTHANC_ENABLE_WASM==1
#include "../../../Platforms/Wasm/WasmPlatformApplicationAdapter.h"
#include "../../../Platforms/Wasm/Defaults.h"
#endif

#include <Core/Images/Font.h>
#include <Core/Logging.h>

namespace OrthancStone
{
  namespace Samples
  {
    class SimpleViewerApplication :
      public SampleSingleCanvasWithButtonsApplicationBase,
      public ObserverBase<SimpleViewerApplication>
    {
    private:
      class ThumbnailInteractor : public Deprecated::IWorldSceneInteractor
      {
      private:
        SimpleViewerApplication&  application_;

      public:
        ThumbnailInteractor(SimpleViewerApplication&  application) :
          application_(application)
        {
        }

        virtual Deprecated::IWorldSceneMouseTracker* CreateMouseTracker(Deprecated::WorldSceneWidget& widget,
                                                            const Deprecated::ViewportGeometry& view,
                                                            MouseButton button,
                                                            KeyboardModifiers modifiers,
                                                            int viewportX,
                                                            int viewportY,
                                                            double x,
                                                            double y,
                                                            Deprecated::IStatusBar* statusBar,
                                                            const std::vector<Deprecated::Touch>& displayTouches)
        {
          if (button == MouseButton_Left)
          {
            statusBar->SetMessage("selected thumbnail " + widget.GetName());
            std::string seriesId = widget.GetName().substr(strlen("thumbnail-series-"));
            application_.SelectSeriesInMainViewport(seriesId);
          }
          return NULL;
        }

        virtual void MouseOver(CairoContext& context,
                               Deprecated::WorldSceneWidget& widget,
                               const Deprecated::ViewportGeometry& view,
                               double x,
                               double y,
                               Deprecated::IStatusBar* statusBar)
        {
        }

        virtual void MouseWheel(Deprecated::WorldSceneWidget& widget,
                                MouseWheelDirection direction,
                                KeyboardModifiers modifiers,
                                Deprecated::IStatusBar* statusBar)
        {
        }

        virtual void KeyPressed(Deprecated::WorldSceneWidget& widget,
                                KeyboardKeys key,
                                char keyChar,
                                KeyboardModifiers modifiers,
                                Deprecated::IStatusBar* statusBar)
        {
        }
      };

      class MainWidgetInteractor : public Deprecated::IWorldSceneInteractor
      {
      private:
        SimpleViewerApplication&  application_;
        
      public:
        MainWidgetInteractor(SimpleViewerApplication&  application) :
          application_(application)
        {
        }
        
        virtual Deprecated::IWorldSceneMouseTracker* CreateMouseTracker(Deprecated::WorldSceneWidget& widget,
                                                            const Deprecated::ViewportGeometry& view,
                                                            MouseButton button,
                                                            KeyboardModifiers modifiers,
                                                            int viewportX,
                                                            int viewportY,
                                                            double x,
                                                            double y,
                                                            Deprecated::IStatusBar* statusBar,
                                                            const std::vector<Deprecated::Touch>& displayTouches)
        {
          if (button == MouseButton_Left)
          {
            if (application_.currentTool_ == Tool_LineMeasure)
            {
              return new Deprecated::LineMeasureTracker(statusBar, dynamic_cast<Deprecated::SliceViewerWidget&>(widget).GetSlice(),
                                            x, y, 255, 0, 0, application_.GetFont());
            }
            else if (application_.currentTool_ == Tool_CircleMeasure)
            {
              return new Deprecated::CircleMeasureTracker(statusBar, dynamic_cast<Deprecated::SliceViewerWidget&>(widget).GetSlice(),
                                              x, y, 255, 0, 0, application_.GetFont());
            }
          }
          return NULL;
        }

        virtual void MouseOver(CairoContext& context,
                               Deprecated::WorldSceneWidget& widget,
                               const Deprecated::ViewportGeometry& view,
                               double x,
                               double y,
                               Deprecated::IStatusBar* statusBar)
        {
          if (statusBar != NULL)
          {
            Vector p = dynamic_cast<Deprecated::SliceViewerWidget&>(widget).GetSlice().MapSliceToWorldCoordinates(x, y);
            
            char buf[64];
            sprintf(buf, "X = %.02f Y = %.02f Z = %.02f (in cm)",
                    p[0] / 10.0, p[1] / 10.0, p[2] / 10.0);
            statusBar->SetMessage(buf);
          }
        }

        virtual void MouseWheel(Deprecated::WorldSceneWidget& widget,
                                MouseWheelDirection direction,
                                KeyboardModifiers modifiers,
                                Deprecated::IStatusBar* statusBar)
        {
        }

        virtual void KeyPressed(Deprecated::WorldSceneWidget& widget,
                                KeyboardKeys key,
                                char keyChar,
                                KeyboardModifiers modifiers,
                                Deprecated::IStatusBar* statusBar)
        {
          switch (keyChar)
          {
            case 's':
              widget.FitContent();
              break;

            case 'l':
              application_.currentTool_ = Tool_LineMeasure;
              break;

            case 'c':
              application_.currentTool_ = Tool_CircleMeasure;
              break;

            default:
              break;
          }
        }
      };


#if ORTHANC_ENABLE_WASM==1
      class SimpleViewerApplicationAdapter : public WasmPlatformApplicationAdapter
      {
        SimpleViewerApplication&  viewerApplication_;

      public:
        SimpleViewerApplicationAdapter(SimpleViewerApplication& application)
          : WasmPlatformApplicationAdapter(application),
            viewerApplication_(application)
        {
        }

        virtual void HandleSerializedMessageFromWeb(std::string& output, const std::string& input) 
        {
          if (input == "select-tool:line-measure")
          {
            viewerApplication_.currentTool_ = Tool_LineMeasure;
            NotifyStatusUpdateFromCppToWebWithString("currentTool=line-measure");
          }
          else if (input == "select-tool:circle-measure")
          {
            viewerApplication_.currentTool_ = Tool_CircleMeasure;
            NotifyStatusUpdateFromCppToWebWithString("currentTool=circle-measure");
          }

          output = "ok";
        }

        virtual void NotifySerializedMessageFromCppToWeb(const std::string& statusUpdateMessage) 
        {
          UpdateStoneApplicationStatusFromCppWithSerializedMessage(statusUpdateMessage.c_str());
        }

        virtual void NotifyStatusUpdateFromCppToWebWithString(const std::string& statusUpdateMessage) 
        {
          UpdateStoneApplicationStatusFromCppWithString(statusUpdateMessage.c_str());
        }

      };
#endif
      enum Tool {
        Tool_LineMeasure,
        Tool_CircleMeasure
      };

      Tool                                 currentTool_;
      std::unique_ptr<MainWidgetInteractor>  mainWidgetInteractor_;
      std::unique_ptr<ThumbnailInteractor>   thumbnailInteractor_;
      Deprecated::LayoutWidget*                        mainLayout_;
      Deprecated::LayoutWidget*                        thumbnailsLayout_;
      std::vector<boost::shared_ptr<Deprecated::SliceViewerWidget> >      thumbnails_;

      std::map<std::string, std::vector<std::string> > instancesIdsPerSeriesId_;
      std::map<std::string, Json::Value> seriesTags_;

      unsigned int                         currentInstanceIndex_;
      Deprecated::WidgetViewport*        wasmViewport1_;
      Deprecated::WidgetViewport*        wasmViewport2_;

      Deprecated::IStatusBar*                          statusBar_;
      std::unique_ptr<Deprecated::SmartLoader>           smartLoader_;

      Orthanc::Font                        font_;

    public:
      SimpleViewerApplication() :
        currentTool_(Tool_LineMeasure),
        mainLayout_(NULL),
        currentInstanceIndex_(0),
        wasmViewport1_(NULL),
        wasmViewport2_(NULL)
      {
        font_.LoadFromResource(Orthanc::EmbeddedResources::FONT_UBUNTU_MONO_BOLD_16);
//        DeclareIgnoredMessage(MessageType_Widget_ContentChanged);
      }

      virtual void DeclareStartupOptions(boost::program_options::options_description& options)
      {
        boost::program_options::options_description generic("Sample options");
        generic.add_options()
          ("studyId", boost::program_options::value<std::string>(),
           "Orthanc ID of the study")
          ;

        options.add(generic);
      }

      virtual void Initialize(StoneApplicationContext* context,
                              Deprecated::IStatusBar& statusBar,
                              const boost::program_options::variables_map& parameters)
      {
        using namespace OrthancStone;

        context_ = context;
        statusBar_ = &statusBar;

        {// initialize viewports and layout
          mainLayout_ = new Deprecated::LayoutWidget("main-layout");
          mainLayout_->SetPadding(10);
          mainLayout_->SetBackgroundCleared(true);
          mainLayout_->SetBackgroundColor(0, 0, 0);
          mainLayout_->SetHorizontal();

          boost::shared_ptr<Deprecated::LayoutWidget> thumbnailsLayout_(new Deprecated::LayoutWidget("thumbnail-layout"));
          thumbnailsLayout_->SetPadding(10);
          thumbnailsLayout_->SetBackgroundCleared(true);
          thumbnailsLayout_->SetBackgroundColor(50, 50, 50);
          thumbnailsLayout_->SetVertical();

          boost::shared_ptr<Deprecated::SliceViewerWidget> widget
            (new Deprecated::SliceViewerWidget("main-viewport"));
          SetCentralWidget(widget);
          //mainWidget_->RegisterObserver(*this);

          // hierarchy
          mainLayout_->AddWidget(thumbnailsLayout_);
          mainLayout_->AddWidget(widget);

          // sources
          smartLoader_.reset(new Deprecated::SmartLoader(context->GetOrthancApiClient()));
          smartLoader_->SetImageQuality(Deprecated::SliceImageQuality_FullPam);

          mainLayout_->SetTransmitMouseOver(true);
          mainWidgetInteractor_.reset(new MainWidgetInteractor(*this));
          widget->SetInteractor(*mainWidgetInteractor_);
          thumbnailInteractor_.reset(new ThumbnailInteractor(*this));
        }

        statusBar.SetMessage("Use the key \"s\" to reinitialize the layout");
        statusBar.SetMessage("Use the key \"n\" to go to next image in the main viewport");


        if (parameters.count("studyId") < 1)
        {
          LOG(WARNING) << "The study ID is missing, will take the first studyId found in Orthanc";
          context->GetOrthancApiClient()->GetJsonAsync(
            "/studies",
            new Deprecated::DeprecatedCallable<SimpleViewerApplication, Deprecated::OrthancApiClient::JsonResponseReadyMessage>
            (GetSharedObserver(), &SimpleViewerApplication::OnStudyListReceived));
        }
        else
        {
          SelectStudy(parameters["studyId"].as<std::string>());
        }
      }

      void OnStudyListReceived(const Deprecated::OrthancApiClient::JsonResponseReadyMessage& message)
      {
        const Json::Value& response = message.GetJson();

        if (response.isArray() &&
            response.size() >= 1)
        {
          SelectStudy(response[0].asString());
        }
      }
      
      void OnStudyReceived(const Deprecated::OrthancApiClient::JsonResponseReadyMessage& message)
      {
        const Json::Value& response = message.GetJson();

        if (response.isObject() && response["Series"].isArray())
        {
          for (size_t i=0; i < response["Series"].size(); i++)
          {
            context_->GetOrthancApiClient()->GetJsonAsync(
              "/series/" + response["Series"][(int)i].asString(),
              new Deprecated::DeprecatedCallable<SimpleViewerApplication, Deprecated::OrthancApiClient::JsonResponseReadyMessage>
              (GetSharedObserver(), &SimpleViewerApplication::OnSeriesReceived));
          }
        }
      }

      void OnSeriesReceived(const Deprecated::OrthancApiClient::JsonResponseReadyMessage& message)
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
          Deprecated::SliceViewerWidget& widget = dynamic_cast<Deprecated::SliceViewerWidget&>(*GetCentralWidget());
          if (widget.GetLayerCount() == 0)
          {
            smartLoader_->SetFrameInWidget(widget, 0, instancesIdsPerSeriesId_[seriesId][0], 0);
          }
        }
      }

      void LoadThumbnailForSeries(const std::string& seriesId, const std::string& instanceId)
      {
        LOG(INFO) << "Loading thumbnail for series " << seriesId;
        boost::shared_ptr<Deprecated::SliceViewerWidget> thumbnailWidget(new Deprecated::SliceViewerWidget("thumbnail-series-" + seriesId));
        thumbnails_.push_back(thumbnailWidget);
        thumbnailsLayout_->AddWidget(thumbnailWidget);
        Register<Deprecated::SliceViewerWidget::GeometryChangedMessage>(*thumbnailWidget, &SimpleViewerApplication::OnWidgetGeometryChanged);
        smartLoader_->SetFrameInWidget(*thumbnailWidget, 0, instanceId, 0);
        thumbnailWidget->SetInteractor(*thumbnailInteractor_);
      }

      void SelectStudy(const std::string& studyId)
      {
        LOG(INFO) << "Selecting study: " << studyId;
        context_->GetOrthancApiClient()->GetJsonAsync(
          "/studies/" + studyId, new Deprecated::DeprecatedCallable<SimpleViewerApplication, Deprecated::OrthancApiClient::JsonResponseReadyMessage>
          (GetSharedObserver(), &SimpleViewerApplication::OnStudyReceived));
      }

      void OnWidgetGeometryChanged(const Deprecated::SliceViewerWidget::GeometryChangedMessage& message)
      {
        // TODO: The "const_cast" could probably be replaced by "mainWidget"
        const_cast<Deprecated::SliceViewerWidget&>(message.GetOrigin()).FitContent();
      }

      void SelectSeriesInMainViewport(const std::string& seriesId)
      {
        Deprecated::SliceViewerWidget& widget = dynamic_cast<Deprecated::SliceViewerWidget&>(*GetCentralWidget());
        smartLoader_->SetFrameInWidget(widget, 0, instancesIdsPerSeriesId_[seriesId][0], 0);
      }

      const Orthanc::Font& GetFont() const
      {
        return font_;
      }
      
      virtual void OnPushButton1Clicked() {}
      virtual void OnPushButton2Clicked() {}
      virtual void OnTool1Clicked() { currentTool_ = Tool_LineMeasure;}
      virtual void OnTool2Clicked() { currentTool_ = Tool_CircleMeasure;}

      virtual void GetButtonNames(std::string& pushButton1,
                                  std::string& pushButton2,
                                  std::string& tool1,
                                  std::string& tool2)
      {
        tool1 = "line";
        tool2 = "circle";
        pushButton1 = "action1";
        pushButton2 = "action2";
      }

#if ORTHANC_ENABLE_WASM==1
      virtual void InitializeWasm()
      {
        AttachWidgetToWasmViewport("canvas", thumbnailsLayout_);
        AttachWidgetToWasmViewport("canvas2", widget);
      }
#endif

    };
  }
}
