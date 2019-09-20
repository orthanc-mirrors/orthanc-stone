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

#include "DicomStructureSetLoader2.h"

#include "../Messages/IObservable.h"
#include "../Oracle/IOracle.h"
#include "../Oracle/OracleCommandExceptionMessage.h"

namespace OrthancStone
{

  DicomStructureSetLoader2::DicomStructureSetLoader2(
    DicomStructureSet2& structureSet
    , IOracle& oracle
    , IObservable& oracleObservable)
    : IObserver(oracleObservable.GetBroker())
    , IObservable(oracleObservable.GetBroker())
    , structureSet_(structureSet)
    , oracle_(oracle)
    , oracleObservable_(oracleObservable)
    , structuresReady_(false)
  {
    LOG(TRACE) << "DicomStructureSetLoader2(" << std::hex << this << std::dec << ")::DicomStructureSetLoader2()";

    oracleObservable.RegisterObserverCallback(
      new Callable<DicomStructureSetLoader2, OrthancRestApiCommand::SuccessMessage>
      (*this, &DicomStructureSetLoader2::HandleSuccessMessage));

    oracleObservable.RegisterObserverCallback(
      new Callable<DicomStructureSetLoader2, OracleCommandExceptionMessage>
      (*this, &DicomStructureSetLoader2::HandleExceptionMessage));
  }

  DicomStructureSetLoader2::~DicomStructureSetLoader2()
  {
    LOG(TRACE) << "DicomStructureSetLoader2(" << std::hex << this << std::dec << ")::~DicomStructureSetLoader2()";
    oracleObservable_.Unregister(this);
  }

  void DicomStructureSetLoader2::LoadInstanceFromString(const std::string& body)
  {
    OrthancPlugins::FullOrthancDataset dicom(body);
    //loader.content_.reset(new DicomStructureSet(dicom));
    structureSet_.Clear();
    structureSet_.SetContents(dicom);
    SetStructuresReady();
  }

  void DicomStructureSetLoader2::HandleSuccessMessage(const OrthancRestApiCommand::SuccessMessage& message)
  {
    const std::string& body = message.GetAnswer();
    LoadInstanceFromString(body);
  }

  void DicomStructureSetLoader2::HandleExceptionMessage(const OracleCommandExceptionMessage& message)
  {
    LOG(ERROR) << "DicomStructureSetLoader2::HandleExceptionMessage: error when trying to load data. "
      << "Error: " << message.GetException().What() << " Details: "
      << message.GetException().GetDetails();
  }

  void DicomStructureSetLoader2::LoadInstance(const std::string& instanceId)
  {
    std::auto_ptr<OrthancRestApiCommand> command(new OrthancRestApiCommand);
    command->SetHttpHeader("Accept-Encoding", "gzip");

    std::string uri = "/instances/" + instanceId + "/tags?ignore-length=3006-0050";

    command->SetUri(uri);
    oracle_.Schedule(*this, command.release());
  }

  void DicomStructureSetLoader2::SetStructuresReady()
  {
    structuresReady_ = true;
  }

  bool DicomStructureSetLoader2::AreStructuresReady() const
  {
    return structuresReady_;
  }

  /*

    void LoaderStateMachine::HandleExceptionMessage(const OracleCommandExceptionMessage& message)
    {
      LOG(ERROR) << "LoaderStateMachine::HandleExceptionMessage: error in the state machine, stopping all processing";
      LOG(ERROR) << "Error: " << message.GetException().What() << " Details: " <<
        message.GetException().GetDetails();
        Clear();
    }

    LoaderStateMachine::~LoaderStateMachine()
    {
      Clear();
    }


  */

}
