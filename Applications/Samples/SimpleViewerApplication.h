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

#include <Core/Logging.h>

namespace OrthancStone
{
  namespace Samples
  {
    class SimpleViewerApplication :
      public SampleApplicationBase,
      private ILayerSource::IObserver
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
        
      
      virtual void NotifyGeometryReady(const ILayerSource& source)
      {
        // Once the geometry of the series is downloaded from Orthanc,
        // display its first slice, and adapt the viewport to fit this
        // slice
        if (source_ == &source)
        {
          //SetSlice(source_->GetSliceCount() / 2);
        }

        mainLayout_->SetDefaultView();
      }
      
      virtual void NotifyGeometryError(const ILayerSource& source)
      {
      }
      
      virtual void NotifyContentChange(const ILayerSource& source)
      {
      }

      virtual void NotifySliceChange(const ILayerSource& source,
                                     const Slice& slice)
      {
      }
 
      virtual void NotifyLayerReady(std::auto_ptr<ILayerRenderer>& layer,
                                    const ILayerSource& source,
                                    const CoordinateSystem3D& slice,
                                    bool isError)
      {
      }

      LayoutWidget*                   mainLayout_;
      LayoutWidget*                   thumbnailsLayout_;
      LayerWidget*                    mainViewport_;
      std::vector<LayerWidget*>       thumbnails_;
      std::vector<std::string>        instances_;
      unsigned int                    currentInstanceIndex_;

      OrthancFrameLayerSource*        source_;
      unsigned int                    slice_;
      
    public:
      SimpleViewerApplication() :
        mainLayout_(NULL),
        currentInstanceIndex_(0),
        source_(NULL),
        slice_(0)
      {
      }
      
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

      virtual void Initialize(IStatusBar& statusBar,
                              const boost::program_options::variables_map& parameters)
      {
        using namespace OrthancStone;

        statusBar.SetMessage("Use the key \"s\" to reinitialize the layout");

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

        mainViewport_ = new LayerWidget();
        thumbnails_.push_back(new LayerWidget());
        thumbnails_.push_back(new LayerWidget());

        // hierarchy
        mainLayout_->AddWidget(thumbnailsLayout_);
        mainLayout_->AddWidget(mainViewport_);
        thumbnailsLayout_->AddWidget(thumbnails_[0]);
        thumbnailsLayout_->AddWidget(thumbnails_[1]);

        // sources
        source_ = new OrthancFrameLayerSource(context_->GetWebService());
        source_->LoadFrame(instances_[currentInstanceIndex_], 0);

        mainViewport_->AddLayer(source_);

        OrthancFrameLayerSource* thumb0 = new OrthancFrameLayerSource(context_->GetWebService());
        thumb0->LoadFrame(instances_[0], 0);
        OrthancFrameLayerSource* thumb1 = new OrthancFrameLayerSource(context_->GetWebService());
        thumb1->LoadFrame(instances_[1], 0);

        thumbnails_[0]->AddLayer(thumb0);
        thumbnails_[1]->AddLayer(thumb1);

        mainLayout_->SetTransmitMouseOver(true);
        mainViewport_->SetInteractor(context_->AddInteractor(new Interactor(*this)));
        context_->SetCentralWidget(mainLayout_);
      }
    };
  }
}
