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

#include "BasicApplicationContext.h"
#include <boost/program_options.hpp>

namespace OrthancStone
{
  class IBasicApplication : public boost::noncopyable
  {
  protected:
//    struct StartupOptionValue {
//      enum Type {
//        boolean,
//        string,
//        integer
//      };
//      Type type;
//      std::string value;

//      int asInt() {return std::stoi(value);}
//      bool asBool() {return value == "true"; }
//      std::string asString() {return value; }
//    };

//    struct StartupOptionDefinition {
//      std::string name;
//      std::string helpText;
//      std::string defaultValue;
//      StartupOptionValue::Type type;
//    };

//    typedef std::list<StartupOptionDefinition> StartupOptions;

//    StartupOptions startupOptions_;

  public:
    virtual ~IBasicApplication()
    {
    }

    virtual void DeclareStartupOptions(boost::program_options::options_description& options) = 0;
    virtual void Initialize(IStatusBar& statusBar,
                            const boost::program_options::variables_map& parameters) = 0;

    virtual BasicApplicationContext& CreateApplicationContext(Orthanc::WebServiceParameters& orthanc) = 0;


    virtual std::string GetTitle() const = 0;

//    virtual void Initialize(BasicApplicationContext& context,
//                            IStatusBar& statusBar,
//                            const std::map<std::string, std::string>& startupOptions) = 0;

    virtual void Finalize() = 0;

//protected:
//    virtual void DeclareStringStartupOption(const std::string& name, const std::string& defaultValue, const std::string& helpText);
//    virtual void DeclareIntegerStartupOption(const std::string& name, const int& defaultValue, const std::string& helpText);
//    virtual void DeclareBoolStartupOption(const std::string& name, bool defaultValue, const std::string& helpText);
  };

}
