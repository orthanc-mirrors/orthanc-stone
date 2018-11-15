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

#include "SampleApplicationBase.h"

#include "../../Framework/Layers/CircleMeasureTracker.h"
#include "../../Framework/Layers/LineMeasureTracker.h"
#include "../../Framework/Widgets/SliceViewerWidget.h"
#include "../../Framework/Widgets/LayoutWidget.h"
#include "../../Framework/Messages/IObserver.h"
#include "../../Framework/SmartLoader.h"

#if ORTHANC_ENABLE_WASM==1
#include "../../Platforms/Wasm/WasmPlatformApplicationAdapter.h"
#include "../../Platforms/Wasm/Defaults.h"
#endif

#include <Core/Images/Font.h>
#include <Core/Logging.h>

namespace OrthancStone
{
  namespace Samples
  {
    class SimpleViewerApplication :
      public SampleSingleCanvasWithButtonsApplicationBase,
      public IObserver
    {
    private:
      class ThumbnailInteractor : public IWorldSceneInteractor
      {
      private:
        SimpleViewerApplication&  application_;

      public:
        ThumbnailInteractor(SimpleViewerApplication&  application) :
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
                                                            IStatusBar* statusBar)
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
        }

        virtual void KeyPressed(WorldSceneWidget& widget,
                                KeyboardKeys key,
                                char keyChar,
                                KeyboardModifiers modifiers,
                                IStatusBar* statusBar)
        {
        }
      };

      class MainWidgetInteractor : public IWorldSceneInteractor
      {
      private:
        SimpleViewerApplication&  application_;
        
      public:
        MainWidgetInteractor(SimpleViewerApplication&  application) :
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
                                                            IStatusBar* statusBar)
        {
          if (button == MouseButton_Left)
          {
            if (application_.currentTool_ == Tools_LineMeasure)
            {
              return new LineMeasureTracker(statusBar, dynamic_cast<SliceViewerWidget&>(widget).GetSlice(),
                                            x, y, 255, 0, 0, application_.GetFont());
            }
            else if (application_.currentTool_ == Tools_CircleMeasure)
            {
              return new CircleMeasureTracker(statusBar, dynamic_cast<SliceViewerWidget&>(widget).GetSlice(),
                                              x, y, 255, 0, 0, application_.GetFont());
            }
          }
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

            case 'l':
              application_.currentTool_ = Tools_LineMeasure;
              break;

            case 'c':
              application_.currentTool_ = Tools_CircleMeasure;
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
        SimpleViewerApplicationAdapter(MessageBroker& broker, SimpleViewerApplication& application)
          : WasmPlatformApplicationAdapter(broker, application),
            viewerApplication_(application)
        {
        }

        virtual void HandleMessageFromWeb(std::string& output, const std::string& input) 
        {
          if (input == "select-tool:line-measure")
          {
            viewerApplication_.currentTool_ = Tools_LineMeasure;
            NotifyStatusUpdateFromCppToWeb("currentTool=line-measure");
          }
          else if (input == "select-tool:circle-measure")
          {
            viewerApplication_.currentTool_ = Tools_CircleMeasure;
            NotifyStatusUpdateFromCppToWeb("currentTool=circle-measure");
          }

          output = "ok";
        }

        virtual void NotifyStatusUpdateFromCppToWeb(const std::string& statusUpdateMessage) 
        {
          UpdateStoneApplicationStatusFromCpp(statusUpdateMessage.c_str());
        }

      };
#endif
      enum Tools {
        Tools_LineMeasure,
        Tools_CircleMeasure
      };

      Tools                                currentTool_;
      std::auto_ptr<MainWidgetInteractor>  mainWidgetInteractor_;
      std::auto_ptr<ThumbnailInteractor>   thumbnailInteractor_;
      LayoutWidget*                        mainLayout_;
      LayoutWidget*                        thumbnailsLayout_;
      std::vector<SliceViewerWidget*>      thumbnails_;

      std::map<std::string, std::vector<std::string> > instancesIdsPerSeriesId_;
      std::map<std::string, Json::Value> seriesTags_;

      unsigned int                         currentInstanceIndex_;
      OrthancStone::WidgetViewport*        wasmViewport1_;
      OrthancStone::WidgetViewport*        wasmViewport2_;

      IStatusBar*                          statusBar_;
      std::auto_ptr<SmartLoader>           smartLoader_;

      Orthanc::Font                        font_;

    public:
      SimpleViewerApplication(MessageBroker& broker) :
        IObserver(broker),
        currentTool_(Tools_LineMeasure),
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

          mainWidget_ = new SliceViewerWidget(GetBroker(), "main-viewport");
          //mainWidget_->RegisterObserver(*this);

          // hierarchy
          mainLayout_->AddWidget(thumbnailsLayout_);
          mainLayout_->AddWidget(mainWidget_);

          // sources
          smartLoader_.reset(new SmartLoader(GetBroker(), context->GetOrthancApiClient()));
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
          context->GetOrthancApiClient().GetJsonAsync(
            "/studies",
            new Callable<SimpleViewerApplication, OrthancApiClient::JsonResponseReadyMessage>
            (*this, &SimpleViewerApplication::OnStudyListReceived));
        }
        else
        {
          SelectStudy(parameters["studyId"].as<std::string>());
        }
      }

      void OnStudyListReceived(const OrthancApiClient::JsonResponseReadyMessage& message)
      {
        const Json::Value& response = message.GetJson();

        if (response.isArray() &&
            response.size() >= 1)
        {
          SelectStudy(response[0].asString());
        }
      }
      
      void OnStudyReceived(const OrthancApiClient::JsonResponseReadyMessage& message)
      {
        const Json::Value& response = message.GetJson();

        if (response.isObject() && response["Series"].isArray())
        {
          for (size_t i=0; i < response["Series"].size(); i++)
          {
            context_->GetOrthancApiClient().GetJsonAsync(
              "/series/" + response["Series"][(int)i].asString(),
              new Callable<SimpleViewerApplication, OrthancApiClient::JsonResponseReadyMessage>
              (*this, &SimpleViewerApplication::OnSeriesReceived));
          }
        }
      }

      void OnSeriesReceived(const OrthancApiClient::JsonResponseReadyMessage& message)
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
          SliceViewerWidget& widget = *dynamic_cast<SliceViewerWidget*>(mainWidget_);
          if (widget.GetLayerCount() == 0)
          {
            smartLoader_->SetFrameInWidget(widget, 0, instancesIdsPerSeriesId_[seriesId][0], 0);
          }
        }
      }

      void LoadThumbnailForSeries(const std::string& seriesId, const std::string& instanceId)
      {
        LOG(INFO) << "Loading thumbnail for series " << seriesId;
        SliceViewerWidget* thumbnailWidget = new SliceViewerWidget(GetBroker(), "thumbnail-series-" + seriesId);
        thumbnails_.push_back(thumbnailWidget);
        thumbnailsLayout_->AddWidget(thumbnailWidget);
        thumbnailWidget->RegisterObserverCallback(new Callable<SimpleViewerApplication, SliceViewerWidget::GeometryChangedMessage>(*this, &SimpleViewerApplication::OnWidgetGeometryChanged));
        smartLoader_->SetFrameInWidget(*thumbnailWidget, 0, instanceId, 0);
        thumbnailWidget->SetInteractor(*thumbnailInteractor_);
      }

      void SelectStudy(const std::string& studyId)
      {
        LOG(INFO) << "Selecting study: " << studyId;
        context_->GetOrthancApiClient().GetJsonAsync(
          "/studies/" + studyId, new Callable<SimpleViewerApplication, OrthancApiClient::JsonResponseReadyMessage>
          (*this, &SimpleViewerApplication::OnStudyReceived));
      }

      void OnWidgetGeometryChanged(const SliceViewerWidget::GeometryChangedMessage& message)
      {
        // TODO: The "const_cast" could probably be replaced by "mainWidget"
        const_cast<SliceViewerWidget&>(message.GetOrigin()).FitContent();
      }

      void SelectSeriesInMainViewport(const std::string& seriesId)
      {
        SliceViewerWidget& widget = *dynamic_cast<SliceViewerWidget*>(mainWidget_);
        smartLoader_->SetFrameInWidget(widget, 0, instancesIdsPerSeriesId_[seriesId][0], 0);
      }

      const Orthanc::Font& GetFont() const
      {
        return font_;
      }
      
      virtual void OnPushButton1Clicked() {}
      virtual void OnPushButton2Clicked() {}
      virtual void OnTool1Clicked() { currentTool_ = Tools_LineMeasure;}
      virtual void OnTool2Clicked() { currentTool_ = Tools_CircleMeasure;}

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
        AttachWidgetToWasmViewport("canvas2", mainWidget_);
      }
#endif

    };
  }
}
