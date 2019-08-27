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


#include "LoaderStateMachine.h"

#include <Core/OrthancException.h>

#if 0
extern bool logbgo233;
extern bool logbgo115;
#endif

namespace OrthancStone
{
  void LoaderStateMachine::State::Handle(const OrthancRestApiCommand::SuccessMessage& message)
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
  }
      

  void LoaderStateMachine::State::Handle(const GetOrthancImageCommand::SuccessMessage& message)
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
  }

      
  void LoaderStateMachine::State::Handle(const GetOrthancWebViewerJpegCommand::SuccessMessage& message)
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
  }


  void LoaderStateMachine::Schedule(OracleCommandWithPayload* command)
  {
#if 0
    if (logbgo233) {
      if (logbgo115)
        LOG(TRACE) << "  LoaderStateMachine::Schedule()";
    }
#endif

    std::auto_ptr<OracleCommandWithPayload> protection(command);

    if (command == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }
      
    if (!command->HasPayload())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange,
                                      "The payload must contain the next state");
    }

#if 0
    if (logbgo233) {
      if (logbgo115)
        LOG(TRACE) << "  * LoaderStateMachine::Schedule(): adding command with addr: " << std::hex << protection.get() << std::dec << " pendingCommands_.size() is now : " << pendingCommands_.size()+1;
    }
#endif
    pendingCommands_.push_back(protection.release());

    Step();
  }


  void LoaderStateMachine::Start()
  {
    if (active_)
    {
      LOG(TRACE) << "LoaderStateMachine::Start() called while active_ is true";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    active_ = true;

    for (size_t i = 0; i < simultaneousDownloads_; i++)
    {
      Step();
    }
  }


  void LoaderStateMachine::Step()
  {
#if 0
    if (logbgo115)
      LOG(TRACE) << "    LoaderStateMachine::Step(): pendingCommands_.size() =  " << pendingCommands_.size();
#endif
    if (!pendingCommands_.empty() &&
        activeCommands_ < simultaneousDownloads_)
    {

      IOracleCommand* nextCommand = pendingCommands_.front();

#if 0
      if (logbgo233) {
        if (logbgo115)
          LOG(TRACE) << "    * LoaderStateMachine::Step(): activeCommands_ (" << activeCommands_ << ") < simultaneousDownloads_ (" << simultaneousDownloads_ << ") --> will Schedule command addr " << std::hex << nextCommand << std::dec;
      }
#endif

      oracle_.Schedule(*this, nextCommand);
      pendingCommands_.pop_front();

      activeCommands_++;
    }
    else
    {
#if 0
      if (logbgo233) {
        if (logbgo115)
          LOG(TRACE) << "    * pendingCommands_.size() == " << pendingCommands_.size() << " LoaderStateMachine::Step(): activeCommands_ (" << activeCommands_ << ") >= simultaneousDownloads_ (" << simultaneousDownloads_ << ") --> will NOT Schedule anything";
      }
#endif
    }
  }


  void LoaderStateMachine::Clear()
  {
    for (PendingCommands::iterator it = pendingCommands_.begin();
         it != pendingCommands_.end(); ++it)
    {
      delete *it;
    }

    pendingCommands_.clear();
  }


  void LoaderStateMachine::HandleExceptionMessage(const OracleCommandExceptionMessage& message)
  {
    LOG(ERROR) << "LoaderStateMachine::HandleExceptionMessage: error in the state machine, stopping all processing";
    LOG(ERROR) << "Error: " << message.GetException().What() << " Details: " <<
      message.GetException().GetDetails();
      Clear();
  }

  template <typename T>
  void LoaderStateMachine::HandleSuccessMessage(const T& message)
  {
    assert(activeCommands_ > 0);
    activeCommands_--;

    try
    {
      dynamic_cast<State&>(message.GetOrigin().GetPayload()).Handle(message);
      Step();
    }
    catch (Orthanc::OrthancException& e)
    {
      LOG(ERROR) << "Error in the state machine, stopping all processing: " << 
        e.What() << " Details: " << e.GetDetails();
      Clear();
    }
  }


  LoaderStateMachine::LoaderStateMachine(IOracle& oracle,
                                         IObservable& oracleObservable) :
    IObserver(oracleObservable.GetBroker()),
    oracle_(oracle),
    active_(false),
    simultaneousDownloads_(4),
    activeCommands_(0)
  {
    oracleObservable.RegisterObserverCallback(
      new Callable<LoaderStateMachine, OrthancRestApiCommand::SuccessMessage>
      (*this, &LoaderStateMachine::HandleSuccessMessage));

    oracleObservable.RegisterObserverCallback(
      new Callable<LoaderStateMachine, GetOrthancImageCommand::SuccessMessage>
      (*this, &LoaderStateMachine::HandleSuccessMessage));

    oracleObservable.RegisterObserverCallback(
      new Callable<LoaderStateMachine, GetOrthancWebViewerJpegCommand::SuccessMessage>
      (*this, &LoaderStateMachine::HandleSuccessMessage));

    oracleObservable.RegisterObserverCallback(
      new Callable<LoaderStateMachine, OracleCommandExceptionMessage>
      (*this, &LoaderStateMachine::HandleExceptionMessage));
  }


  void LoaderStateMachine::SetSimultaneousDownloads(unsigned int count)
  {
    if (active_)
    {
      LOG(ERROR) << "LoaderStateMachine::SetSimultaneousDownloads called while active_ is true";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else if (count == 0)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);        
    }
    else
    {
      simultaneousDownloads_ = count;
    }
  }
}
