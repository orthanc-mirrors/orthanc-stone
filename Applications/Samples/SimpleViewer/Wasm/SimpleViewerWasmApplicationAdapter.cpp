/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2019 Osimis S.A., Belgium
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

#include "SimpleViewerWasmApplicationAdapter.h"

namespace SimpleViewer
{

  SimpleViewerWasmApplicationAdapter::SimpleViewerWasmApplicationAdapter(MessageBroker &broker, SimpleViewerApplication &application)
      : WasmPlatformApplicationAdapter(broker, application),
        viewerApplication_(application)
  {
    application.RegisterObserverCallback(new Callable<SimpleViewerWasmApplicationAdapter, SimpleViewerApplication::StatusUpdatedMessage>(*this, &SimpleViewerWasmApplicationAdapter::OnStatusUpdated));
  }

  void SimpleViewerWasmApplicationAdapter::OnStatusUpdated(const SimpleViewerApplication::StatusUpdatedMessage &message)
  {
    Json::Value statusJson;
    message.status_.ToJson(statusJson);

    Json::Value event;
    event["event"] = "appStatusUpdated";
    event["data"] = statusJson;

    Json::StreamWriterBuilder builder;
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    std::ostringstream outputStr;

    writer->write(event, &outputStr);

    NotifyStatusUpdateFromCppToWeb(outputStr.str());
  }

} // namespace SimpleViewer