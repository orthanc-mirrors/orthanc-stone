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


#include "SliceGeometry.h"

#include "GeometryToolbox.h"

#include "../../Resources/Orthanc/Core/Logging.h"
#include "../../Resources/Orthanc/Core/Toolbox.h"
#include "../../Resources/Orthanc/Core/OrthancException.h"

namespace OrthancStone
{
  void SliceGeometry::CheckAndComputeNormal()
  {
    // DICOM expects normal vectors to define the axes: "The row and
    // column direction cosine vectors shall be normal, i.e., the dot
    // product of each direction cosine vector with itself shall be
    // unity."
    // http://dicom.nema.org/medical/dicom/current/output/chtml/part03/sect_C.7.6.2.html
    if (!GeometryToolbox::IsNear(boost::numeric::ublas::norm_2(axisX_), 1.0) ||
        !GeometryToolbox::IsNear(boost::numeric::ublas::norm_2(axisY_), 1.0))
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }

    // The vectors within "Image Orientation Patient" must be
    // orthogonal, according to the DICOM specification: "The row and
    // column direction cosine vectors shall be orthogonal, i.e.,
    // their dot product shall be zero."
    // http://dicom.nema.org/medical/dicom/current/output/chtml/part03/sect_C.7.6.2.html
    if (!GeometryToolbox::IsCloseToZero(boost::numeric::ublas::inner_prod(axisX_, axisY_)))
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }

    GeometryToolbox::CrossProduct(normal_, axisX_, axisY_);

    // Just a sanity check, it should be useless by construction
    assert(GeometryToolbox::IsNear(boost::numeric::ublas::norm_2(normal_), 1.0));
  }


  void SliceGeometry::SetupCanonical()
  {
    GeometryToolbox::AssignVector(origin_, 0, 0, 0);
    GeometryToolbox::AssignVector(axisX_, 1, 0, 0);
    GeometryToolbox::AssignVector(axisY_, 0, 1, 0);
    CheckAndComputeNormal();
  }


  SliceGeometry::SliceGeometry(const Vector& origin,
                               const Vector& axisX,
                               const Vector& axisY) :
    origin_(origin),
    axisX_(axisX),
    axisY_(axisY)
  {
    CheckAndComputeNormal();
  }


  void SliceGeometry::Setup(const std::string& imagePositionPatient,
                            const std::string& imageOrientationPatient)
  {
    std::string tmpPosition = Orthanc::Toolbox::StripSpaces(imagePositionPatient);
    std::string tmpOrientation = Orthanc::Toolbox::StripSpaces(imageOrientationPatient);

    Vector orientation;
    if (!GeometryToolbox::ParseVector(origin_, tmpPosition) ||
        !GeometryToolbox::ParseVector(orientation, tmpOrientation) ||
        origin_.size() != 3 ||
        orientation.size() != 6)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }

    axisX_.resize(3);
    axisX_[0] = orientation[0];
    axisX_[1] = orientation[1];
    axisX_[2] = orientation[2];

    axisY_.resize(3);
    axisY_[0] = orientation[3];
    axisY_[1] = orientation[4];
    axisY_[2] = orientation[5];

    CheckAndComputeNormal();
  }   


  SliceGeometry::SliceGeometry(const OrthancPlugins::IDicomDataset& dicom)
  {
    std::string a, b;

    if (dicom.GetStringValue(a, OrthancPlugins::DICOM_TAG_IMAGE_POSITION_PATIENT) &&
        dicom.GetStringValue(b, OrthancPlugins::DICOM_TAG_IMAGE_ORIENTATION_PATIENT))
    {
      Setup(a, b);
    }
    else
    {
      SetupCanonical();
    }
  }   


  Vector SliceGeometry::MapSliceToWorldCoordinates(double x,
                                                   double y) const
  {
    return origin_ + x * axisX_ + y * axisY_;
  }


  double SliceGeometry::ProjectAlongNormal(const Vector& point) const
  {
    return boost::numeric::ublas::inner_prod(point, normal_);
  }


  void SliceGeometry::ProjectPoint(double& offsetX,
                                   double& offsetY,
                                   const Vector& point) const
  {
    // Project the point onto the slice
    Vector projection;
    GeometryToolbox::ProjectPointOntoPlane(projection, point, normal_, origin_);

    // As the axes are orthonormal vectors thanks to
    // CheckAndComputeNormal(), the following dot products give the
    // offset of the origin of the slice wrt. the origin of the
    // reference plane https://en.wikipedia.org/wiki/Vector_projection
    offsetX = boost::numeric::ublas::inner_prod(axisX_, projection - origin_);
    offsetY = boost::numeric::ublas::inner_prod(axisY_, projection - origin_);
  }
}
