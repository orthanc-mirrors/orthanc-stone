/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017 Osimis, Belgium
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

#include "GeometryToolbox.h"
#include "../../Resources/Orthanc/Plugins/Samples/Common/IDicomDataset.h"

namespace OrthancStone
{
  // Geometry of a 3D plane, NOT thread-safe
  class SliceGeometry
  {
  private:
    Vector    origin_;
    Vector    normal_;
    Vector    axisX_;
    Vector    axisY_;

    void CheckAndComputeNormal();

    void Setup(const std::string& imagePositionPatient,
               const std::string& imageOrientationPatient);

    void SetupCanonical();

  public:
    SliceGeometry()
    {
      SetupCanonical();
    }

    SliceGeometry(const Vector& origin,
                  const Vector& axisX,
                  const Vector& axisY);

    SliceGeometry(const OrthancPlugins::IDicomDataset& dicom);

    SliceGeometry(const std::string& imagePositionPatient,
                  const std::string& imageOrientationPatient)
    {
      Setup(imagePositionPatient, imageOrientationPatient);
    }

    const Vector& GetNormal() const
    {
      return normal_;
    }

    const Vector& GetOrigin() const
    {
      return origin_;
    }

    const Vector& GetAxisX() const
    {
      return axisX_;
    }

    const Vector& GetAxisY() const
    {
      return axisY_;
    }

    Vector MapSliceToWorldCoordinates(double x,
                                      double y) const;
    
    double ProjectAlongNormal(const Vector& point) const;

    void ProjectPoint(double& offsetX,
                      double& offsetY,
                      const Vector& point) const;
  };
}
