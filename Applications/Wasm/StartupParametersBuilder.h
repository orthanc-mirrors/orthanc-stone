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

#include <boost/program_options.hpp>
#include <tuple>

#if ORTHANC_ENABLE_SDL == 1
#error this file shall be included only with the ORTHANC_ENABLE_SDL set to 0
#endif

namespace OrthancStone
{
  // This class is used to generate boost program options from a dico.
  // In a Wasm context, startup options are passed as URI arguments that
  // are then passed to this class as a dico.
  // This class regenerates a fake command-line and parses it to produce
  // the same output as if the app was started at command-line.
  class StartupParametersBuilder
  {
    typedef std::list<std::tuple<std::string, std::string>> StartupParameters;
    StartupParameters startupParameters_;

  public:

    void Clear();
    // Please note that if a parameter is a flag-style one, the value that 
    // is passed should be an empty string
    void SetStartupParameter(const char* name, const char* value);
    void GetStartupParameters(
      boost::program_options::variables_map& parameters_, 
      const boost::program_options::options_description& options);
  };

}
