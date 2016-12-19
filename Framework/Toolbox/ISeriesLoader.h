/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * In addition, as a special exception, the copyright holders of this
 * program give permission to link the code of its release with the
 * OpenSSL project's "OpenSSL" library (or with modified versions of it
 * that use the same license as the "OpenSSL" library), and distribute
 * the linked executables. You must obey the GNU General Public License
 * in all respects for all of the code used other than "OpenSSL". If you
 * modify file(s) with this exception, you may extend this exception to
 * your version of the file(s), but you are not obligated to do so. If
 * you do not wish to do so, delete this exception statement from your
 * version. If you delete this exception statement from all source files
 * in the program, then also delete it here.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


#pragma once

#include "ParallelSlices.h"

#include "IThreadSafety.h"
#include "../../Resources/Orthanc/Core/Images/ImageAccessor.h"
#include "../../Resources/Orthanc/Plugins/Samples/Common/IDicomDataset.h"

namespace OrthancStone
{
  // This class is NOT thread-safe
  class ISeriesLoader : public IThreadUnsafe
  {
  public:
    virtual ParallelSlices& GetGeometry() = 0;

    virtual Orthanc::PixelFormat GetPixelFormat() = 0;

    virtual unsigned int GetWidth() = 0;

    virtual unsigned int GetHeight() = 0;

    virtual OrthancPlugins::IDicomDataset* DownloadDicom(size_t index) = 0;

    // This downloads the frame from Orthanc. The resulting pixel
    // format must be Grayscale8, Grayscale16, SignedGrayscale16 or
    // RGB24. Orthanc Stone assumes the conversion of the photometric
    // interpretation is done by Orthanc.
    virtual Orthanc::ImageAccessor* DownloadFrame(size_t index) = 0;

    virtual Orthanc::ImageAccessor* DownloadJpegFrame(size_t index,
                                                      unsigned int quality) = 0;

    virtual bool IsJpegAvailable() = 0;
  };
}
