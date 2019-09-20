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

#pragma once

#include "../Toolbox/DicomStructureSet2.h"
#include "../Messages/IMessage.h"
#include "../Messages/IObserver.h"
#include "../Messages/IObservable.h"
#include "../Oracle/OrthancRestApiCommand.h"

#include <boost/noncopyable.hpp>

namespace OrthancStone
{
  class IOracle;
  class IObservable;
  class OrthancRestApiCommand;
  class OracleCommandExceptionMessage;

  class DicomStructureSetLoader2 : public IObserver, public IObservable
  {
  public:
    ORTHANC_STONE_DEFINE_ORIGIN_MESSAGE(__FILE__, __LINE__, StructuresReady, DicomStructureSetLoader2);

    /**
    Warning: the structureSet, oracle and oracleObservable objects must live
    at least as long as this object (TODO: shared_ptr?)
    */
    DicomStructureSetLoader2(DicomStructureSet2& structureSet, IOracle& oracle, IObservable& oracleObservable);

    ~DicomStructureSetLoader2();

    void LoadInstance(const std::string& instanceId);

    /** Internal use */
    void LoadInstanceFromString(const std::string& body);

    void SetStructuresReady();
    bool AreStructuresReady() const;
  
  private:
    /**
    Called back by the oracle when data is ready!
    */
    void HandleSuccessMessage(const OrthancRestApiCommand::SuccessMessage& message);

    /**
    Called back by the oracle when shit hits the fan
    */
    void HandleExceptionMessage(const OracleCommandExceptionMessage& message);

    /**
    The structure set that will be (cleared and) filled with data from the 
    loader
    */
    DicomStructureSet2& structureSet_;

    IOracle&            oracle_;
    IObservable&        oracleObservable_;
    bool                structuresReady_;
  };
}
