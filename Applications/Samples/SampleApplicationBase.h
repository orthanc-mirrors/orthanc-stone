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

#include "../../Applications/IStoneApplication.h"
#include "../../Framework/Widgets/WorldSceneWidget.h"

#if ORTHANC_ENABLE_WASM==1
#include "../../Platforms/Wasm/WasmPlatformApplicationAdapter.h"
#include "../../Platforms/Wasm/Defaults.h"
#endif

#if ORTHANC_ENABLE_QT==1
#include "Qt/SampleMainWindow.h"
#include "Qt/SampleMainWindowWithButtons.h"
#endif

namespace OrthancStone
{
  namespace Samples
  {
    class SampleApplicationBase : public IStoneApplication
    {
    protected:
      BaseCommandBuilder commandBuilder_;
      WorldSceneWidget*  mainWidget_;   // ownership is transfered to the application context

    public:
      virtual void Initialize(StoneApplicationContext* context,
                              IStatusBar& statusBar,
                              const boost::program_options::variables_map& parameters)
      {
      }

      virtual std::string GetTitle() const
      {
        return "Stone of Orthanc - Sample";
      }

      virtual BaseCommandBuilder& GetCommandBuilder() {return commandBuilder_;}

      virtual void Finalize() {}
      virtual IWidget* GetCentralWidget() {return mainWidget_;}

#if ORTHANC_ENABLE_WASM==1
      // default implementations for a single canvas named "canvas" in the HTML and an emtpy WasmApplicationAdapter

      virtual void InitializeWasm()
      {
        AttachWidgetToWasmViewport("canvas", mainWidget_);
      }

      virtual WasmPlatformApplicationAdapter* CreateWasmApplicationAdapter(MessageBroker& broker)
      {
        return new WasmPlatformApplicationAdapter(broker, *this);
      }
#endif

    };

    // this application actually works in Qt and WASM
    class SampleSingleCanvasWithButtonsApplicationBase : public SampleApplicationBase
    {
public:
      virtual void OnPushButton1Clicked() {}
      virtual void OnPushButton2Clicked() {}
      virtual void OnTool1Clicked() {}
      virtual void OnTool2Clicked() {}

      virtual void GetButtonNames(std::string& pushButton1,
                                  std::string& pushButton2,
                                  std::string& tool1,
                                  std::string& tool2
                                  ) {
        pushButton1 = "action1";
        pushButton2 = "action2";
        tool1 = "tool1";
        tool2 = "tool2";
      }

#if ORTHANC_ENABLE_QT==1
      virtual QStoneMainWindow* CreateQtMainWindow() {
        return new SampleMainWindowWithButtons(dynamic_cast<OrthancStone::NativeStoneApplicationContext&>(*context_), *this);
      }
#endif

    };

    // this application actually works in SDL and WASM
    class SampleSingleCanvasApplicationBase : public SampleApplicationBase
    {
public:

#if ORTHANC_ENABLE_QT==1
      virtual QStoneMainWindow* CreateQtMainWindow() {
        return new SampleMainWindow(dynamic_cast<OrthancStone::NativeStoneApplicationContext&>(*context_), *this);
      }
#endif
    };
  }
}