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

#include "../../Framework/Layers/OrthancFrameLayerSource.h"
#include "../../Framework/Widgets/LayerWidget.h"
#include "../../Framework/Widgets/LayoutWidget.h"
#include "../../Framework/Messages/IObserver.h"
#include "../../Framework/SmartLoader.h"

#include <Core/Logging.h>

namespace OrthancStone
{
  namespace Samples
  {
    class SimpleViewerApplication :
        public SampleApplicationBase,
        public IObserver
    {
    private:
      class Interactor : public IWorldSceneInteractor
      {
      private:
        SimpleViewerApplication&  application_;
        
      public:
        Interactor(SimpleViewerApplication&  application) :
          application_(application)
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
          if (statusBar != NULL)
          {
            Vector p = dynamic_cast<LayerWidget&>(widget).GetSlice().MapSliceToWorldCoordinates(x, y);
            
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
          //          int scale = (modifiers & KeyboardModifiers_Control ? 10 : 1);
          
          //          switch (direction)
          //          {
          //            case MouseWheelDirection_Up:
          //              application_.OffsetSlice(-scale);
          //              break;

          //            case MouseWheelDirection_Down:
          //              application_.OffsetSlice(scale);
          //              break;

          //            default:
          //              break;
          //          }
        }

        virtual void KeyPressed(WorldSceneWidget& widget,
                                char key,
                                KeyboardModifiers modifiers,
                                IStatusBar* statusBar)
        {
          switch (key)
          {
          case 's':
            widget.SetDefaultView();
            break;
          case 'n':
            application_.NextImage(widget);
            break;

          default:
            break;
          }
        }
      };


      //      void OffsetSlice(int offset)
      //      {
      //        if (source_ != NULL)
      //        {
      //          int slice = static_cast<int>(slice_) + offset;

      //          if (slice < 0)
      //          {
      //            slice = 0;
      //          }

      //          if (slice >= static_cast<int>(source_->GetSliceCount()))
      //          {
      //            slice = source_->GetSliceCount() - 1;
      //          }

      //          if (slice != static_cast<int>(slice_))
      //          {
      //            SetSlice(slice);
      //          }
      //        }
      //      }
      

      //      void SetSlice(size_t index)
      //      {
      //        if (source_ != NULL &&
      //            index < source_->GetSliceCount())
      //        {
      //          slice_ = index;

      //#if 1
      //          widget_->SetSlice(source_->GetSlice(slice_).GetGeometry());
      //#else
      //          // TEST for scene extents - Rotate the axes
      //          double a = 15.0 / 180.0 * M_PI;

      //#if 1
      //          Vector x; GeometryToolbox::AssignVector(x, cos(a), sin(a), 0);
      //          Vector y; GeometryToolbox::AssignVector(y, -sin(a), cos(a), 0);
      //#else
      //          // Flip the normal
      //          Vector x; GeometryToolbox::AssignVector(x, cos(a), sin(a), 0);
      //          Vector y; GeometryToolbox::AssignVector(y, sin(a), -cos(a), 0);
      //#endif

      //          SliceGeometry s(source_->GetSlice(slice_).GetGeometry().GetOrigin(), x, y);
      //          widget_->SetSlice(s);
      //#endif
      //        }
      //      }


      virtual void HandleMessage(IObservable& from, const IMessage& message) {
        switch (message.GetType()) {
        case MessageType_Widget_GeometryChanged:
          dynamic_cast<LayerWidget&>(from).SetDefaultView();
          break;
        default:
          VLOG("unhandled message type" << message.GetType());
        }
      }

      std::unique_ptr<Interactor>     interactor_;
      LayoutWidget*                   mainLayout_;
      LayoutWidget*                   thumbnailsLayout_;
      LayerWidget*                    mainViewport_;
      std::vector<LayerWidget*>       thumbnails_;
      std::vector<std::string>        instances_;
      unsigned int                    currentInstanceIndex_;
      OrthancStone::WidgetViewport*                wasmViewport1_;
      OrthancStone::WidgetViewport*                wasmViewport2_;

      IStatusBar*                     statusBar_;
      unsigned int                    slice_;
      std::unique_ptr<SmartLoader>    smartLoader_;

    public:
      SimpleViewerApplication(MessageBroker& broker) :
        IObserver(broker),
        mainLayout_(NULL),
        currentInstanceIndex_(0),
        wasmViewport1_(NULL),
        wasmViewport2_(NULL),
        slice_(0)
      {
        DeclareIgnoredMessage(MessageType_Widget_ContentChanged);
        DeclareHandledMessage(MessageType_Widget_GeometryChanged);
      }

      virtual void Finalize() {}
      virtual IWidget* GetCentralWidget() {return mainLayout_;}

      virtual void DeclareStartupOptions(boost::program_options::options_description& options)
      {
        boost::program_options::options_description generic("Sample options");
        generic.add_options()
            //          ("study", boost::program_options::value<std::string>(),
            //           "Orthanc ID of the study")
            ("instance1", boost::program_options::value<std::string>(),
             "Orthanc ID of the instances")
            ("instance2", boost::program_options::value<std::string>(),
             "Orthanc ID of the instances")
            ;

        options.add(generic);
      }

      virtual void Initialize(BasicApplicationContext* context,
                              IStatusBar& statusBar,
                              const boost::program_options::variables_map& parameters)
      {
        using namespace OrthancStone;

        context_ = context;
        statusBar_ = &statusBar;
        statusBar.SetMessage("Use the key \"s\" to reinitialize the layout");
        statusBar.SetMessage("Use the key \"n\" to go to next image in the main viewport");

        if (parameters.count("instance1") < 1)
        {
          LOG(ERROR) << "The instance ID is missing";
          throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
        }
        if (parameters.count("instance2") < 1)
        {
          LOG(ERROR) << "The instance ID is missing";
          throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
        }
        instances_.push_back(parameters["instance1"].as<std::string>());
        instances_.push_back(parameters["instance2"].as<std::string>());

        mainLayout_ = new LayoutWidget();
        mainLayout_->SetPadding(10);
        mainLayout_->SetBackgroundCleared(true);
        mainLayout_->SetBackgroundColor(0, 0, 0);
        mainLayout_->SetHorizontal();

        thumbnailsLayout_ = new LayoutWidget();
        thumbnailsLayout_->SetPadding(10);
        thumbnailsLayout_->SetBackgroundCleared(true);
        thumbnailsLayout_->SetBackgroundColor(50, 50, 50);
        thumbnailsLayout_->SetVertical();

        mainViewport_ = new LayerWidget(broker_);
        thumbnails_.push_back(new LayerWidget(broker_));
        thumbnails_.push_back(new LayerWidget(broker_));
        mainViewport_->RegisterObserver(*this);
        thumbnails_[0]->RegisterObserver(*this);
        thumbnails_[1]->RegisterObserver(*this);

        // hierarchy
        mainLayout_->AddWidget(thumbnailsLayout_);
        mainLayout_->AddWidget(mainViewport_);
        thumbnailsLayout_->AddWidget(thumbnails_[0]);
        thumbnailsLayout_->AddWidget(thumbnails_[1]);

        // sources
        smartLoader_.reset(new SmartLoader(broker_, context_->GetWebService()));
        smartLoader_->SetImageQuality(SliceImageQuality_FullPam);

        mainViewport_->AddLayer(smartLoader_->GetFrame(instances_[currentInstanceIndex_], 0));
        thumbnails_[0]->AddLayer(smartLoader_->GetFrame(instances_[0], 0));
        thumbnails_[1]->AddLayer(smartLoader_->GetFrame(instances_[1], 0));

        mainLayout_->SetTransmitMouseOver(true);
        interactor_.reset(new Interactor(*this));
        mainViewport_->SetInteractor(*interactor_);
      }

#if ORTHANC_ENABLE_SDL==0
      virtual void InitializeWasm() {

        AttachWidgetToWasmViewport("canvas", thumbnailsLayout_);
        AttachWidgetToWasmViewport("canvas2", mainViewport_);
      }
#endif

      void NextImage(WorldSceneWidget& widget) {
        assert(context_);
        statusBar_->SetMessage("displaying next image");

        currentInstanceIndex_ = (currentInstanceIndex_ + 1) % instances_.size();

        mainViewport_->ReplaceLayer(0, smartLoader_->GetFrame(instances_[currentInstanceIndex_], 0));

      }
    };


  }
}
