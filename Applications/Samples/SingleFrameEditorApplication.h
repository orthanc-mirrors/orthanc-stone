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

#include <Core/Logging.h>

namespace OrthancStone
{

  class GrayscaleBitmapStack :
    public WorldSceneWidget,
    public IObservable
  {
  public:
    typedef OriginMessage<MessageType_Widget_GeometryChanged, GrayscaleBitmapStack> GeometryChangedMessage;
    typedef OriginMessage<MessageType_Widget_ContentChanged, GrayscaleBitmapStack> ContentChangedMessage;

  protected:
    virtual Extent2D GetSceneExtent()
    {
    }

    virtual bool RenderScene(CairoContext& context,
                             const ViewportGeometry& view)
    {
    }

  public:
    GrayscaleBitmapStack(MessageBroker& broker,
                         const std::string& name) :
      WorldSceneWidget(name),
      IObservable(broker)
    {
    }
  
  };

  
  namespace Samples
  {
    class SingleFrameEditorApplication :
        public SampleSingleCanvasApplicationBase,
        public IObserver
    {
      enum Tools
      {
        Tools_Crop,
        Tools_Windowing,
        Tools_Zoom,
        Tools_Pan
      };

      enum Actions
      {
        Actions_Invert,
        Actions_RotateLeft,
        Actions_RotateRight
      };

    private:
      class Interactor : public IWorldSceneInteractor
      {
      private:
        SingleFrameEditorApplication&  application_;
        
      public:
        Interactor(SingleFrameEditorApplication&  application) :
          application_(application)
        {
        }
        
        virtual IWorldSceneMouseTracker* CreateMouseTracker(WorldSceneWidget& widget,
                                                            const ViewportGeometry& view,
                                                            MouseButton button,
                                                            KeyboardModifiers modifiers,
                                                            double x,
                                                            double y,
                                                            IStatusBar* statusBar)
        {
          switch (application_.currentTool_) {
          case Tools_Zoom:
            printf("ZOOM\n");

            case Tools_Crop:
          case Tools_Windowing:
          case Tools_Pan:
            // TODO return the right mouse tracker
            return NULL;
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
            char buf[64];
            sprintf(buf, "X = %.02f Y = %.02f (in cm)", x / 10.0, y / 10.0);
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
          case 'p':
            application_.currentTool_ = Tools_Pan;
            break;
          case 'z':
            application_.currentTool_ = Tools_Zoom;
            break;
          case 'c':
            application_.currentTool_ = Tools_Crop;
            break;
          case 'w':
            application_.currentTool_ = Tools_Windowing;
            break;
          case 'i':
            application_.Invert();
            break;
          case 'r':
            if (modifiers == KeyboardModifiers_None)
              application_.Rotate(90);
            else
              application_.Rotate(-90);
            break;
          case 'e':
            application_.Export();
            break;
          default:
            break;
          }
        }
      };

      void OnGeometryChanged(const GrayscaleBitmapStack::GeometryChangedMessage& message)
      {
        mainWidget_->FitContent();
      }
      
      std::auto_ptr<Interactor>        mainWidgetInteractor_;
      std::auto_ptr<OrthancApiClient>  orthancApiClient_;
      Tools                            currentTool_;
      const OrthancFrameLayerSource*   source_;
      unsigned int                     slice_;

    public:
      SingleFrameEditorApplication(MessageBroker& broker) :
        IObserver(broker),
        currentTool_(Tools_Zoom),
        source_(NULL),
        slice_(0)
      {
      }
      
      virtual void DeclareStartupOptions(boost::program_options::options_description& options)
      {
        boost::program_options::options_description generic("Sample options");
        generic.add_options()
            ("instance", boost::program_options::value<std::string>(),
             "Orthanc ID of the instance")
            ("frame", boost::program_options::value<unsigned int>()->default_value(0),
             "Number of the frame, for multi-frame DICOM instances")
            ;

        options.add(generic);
      }

      virtual void Initialize(StoneApplicationContext* context,
                              IStatusBar& statusBar,
                              const boost::program_options::variables_map& parameters)
      {
        using namespace OrthancStone;

        context_ = context;

        statusBar.SetMessage("Use the key \"s\" to reinitialize the layout, \"p\" to pan, \"z\" to zoom, \"c\" to crop, \"i\" to invert, \"w\" to change windowing, \"r\" to rotate cw,  \"shift+r\" to rotate ccw");

        if (parameters.count("instance") != 1)
        {
          LOG(ERROR) << "The instance ID is missing";
          throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
        }

        std::string instance = parameters["instance"].as<std::string>();
        int frame = parameters["frame"].as<unsigned int>();

        orthancApiClient_.reset(new OrthancApiClient(IObserver::broker_, context_->GetWebService()));
        mainWidget_ = new GrayscaleBitmapStack(broker_, "main-widget");

        dynamic_cast<GrayscaleBitmapStack*>(mainWidget_)->RegisterObserverCallback(
          new Callable<SingleFrameEditorApplication,
          GrayscaleBitmapStack::GeometryChangedMessage>
          (*this, &SingleFrameEditorApplication::OnGeometryChanged));

        mainWidget_->SetTransmitMouseOver(true);

        mainWidgetInteractor_.reset(new Interactor(*this));
        mainWidget_->SetInteractor(*mainWidgetInteractor_);
      }


      void Invert()
      {
        // TODO
      }

      void Rotate(int degrees)
      {
        // TODO
      }

      void Export()
      {
        // TODO: export dicom file to a temporary file
      }
    };


  }
}
