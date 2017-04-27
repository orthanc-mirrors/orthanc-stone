/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017 Osimis, Belgium
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

#include "ISeriesLoader.h"
#include "../../Resources/Orthanc/Plugins/Samples/Common/IOrthancConnection.h"

#include <boost/shared_ptr.hpp>

namespace OrthancStone
{
  // This class sorts the slices from a given series, give access to
  // their geometry and individual frames, making the assumption that
  // there is a single frame in each instance of the series
  class OrthancSeriesLoader : public ISeriesLoader
  {
  private:
    class Slice;
    class SetOfSlices;

    OrthancPlugins::IOrthancConnection&  orthanc_;
    boost::shared_ptr<SetOfSlices>       slices_;
    ParallelSlices                       geometry_;
    Orthanc::PixelFormat                 format_;
    unsigned int                         width_;
    unsigned int                         height_;

    void CheckFrame(const Orthanc::ImageAccessor& frame) const;

  public:
    OrthancSeriesLoader(OrthancPlugins::IOrthancConnection& orthanc,
                        const std::string& series);
    
    virtual Orthanc::PixelFormat GetPixelFormat()
    {
      return format_;
    }

    virtual ParallelSlices& GetGeometry()
    {
      return geometry_;
    }

    virtual unsigned int GetWidth()
    {
      return width_;
    }

    virtual unsigned int GetHeight()
    {
      return height_;
    }

    virtual OrthancPlugins::IDicomDataset* DownloadDicom(size_t index);

    virtual Orthanc::ImageAccessor* DownloadFrame(size_t index);

    virtual Orthanc::ImageAccessor* DownloadJpegFrame(size_t index,
                                                      unsigned int quality);

    virtual bool IsJpegAvailable();
  };
}
