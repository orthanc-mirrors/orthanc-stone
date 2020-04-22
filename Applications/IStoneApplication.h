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

#include "StoneApplicationContext.h"
#include "../Framework/Deprecated/Viewport/WidgetViewport.h"

#include <boost/program_options.hpp>
#include <json/json.h>

namespace OrthancStone
{
#if ORTHANC_ENABLE_QT==1
  class QStoneMainWindow;
#endif

  // a StoneApplication is an application that can actually be executed
  // in multiple environments.  i.e: it can run natively integrated in a QtApplication
  // or it can be executed as part of a WebPage when compiled into WebAssembly.
  class IStoneApplication : public boost::noncopyable
  {
  protected:
    StoneApplicationContext* context_;

  public:
    virtual ~IStoneApplication()
    {
    }

    virtual void DeclareStartupOptions(boost::program_options::options_description& options) = 0;
    virtual void Initialize(StoneApplicationContext* context,
                            Deprecated::IStatusBar& statusBar,
                            const boost::program_options::variables_map& parameters) = 0;

    /**
      This method is meant to process messages received from the outside world (i.e. GUI)
    */
    virtual void HandleSerializedMessage(const char* data) = 0;

#if ORTHANC_ENABLE_WASM==1
    virtual void InitializeWasm() {}  // specific initialization when the app is running in WebAssembly.  This is called after the other Initialize()
#endif
#if ORTHANC_ENABLE_QT==1
      virtual QStoneMainWindow* CreateQtMainWindow() = 0;
#endif

    virtual std::string GetTitle() const = 0;
    
    virtual void SetCentralWidget(boost::shared_ptr<Deprecated::IWidget> widget) = 0;
    
    virtual boost::shared_ptr<Deprecated::IWidget> GetCentralWidget() = 0;
    
    virtual void Finalize() = 0;
  };
}
