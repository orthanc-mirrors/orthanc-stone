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


#include "CoordinateSystem3D.h"

#include "LinearAlgebra.h"
#include "GeometryToolbox.h"

#include <Core/Logging.h>
#include <Core/Toolbox.h>
#include <Core/OrthancException.h>

namespace OrthancStone
{
  void CoordinateSystem3D::CheckAndComputeNormal()
  {
    // DICOM expects normal vectors to define the axes: "The row and
    // column direction cosine vectors shall be normal, i.e., the dot
    // product of each direction cosine vector with itself shall be
    // unity."
    // http://dicom.nema.org/medical/dicom/current/output/chtml/part03/sect_C.7.6.2.html
    if (!LinearAlgebra::IsNear(boost::numeric::ublas::norm_2(axisX_), 1.0) ||
        !LinearAlgebra::IsNear(boost::numeric::ublas::norm_2(axisY_), 1.0))
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }

    // The vectors within "Image Orientation Patient" must be
    // orthogonal, according to the DICOM specification: "The row and
    // column direction cosine vectors shall be orthogonal, i.e.,
    // their dot product shall be zero."
    // http://dicom.nema.org/medical/dicom/current/output/chtml/part03/sect_C.7.6.2.html
    if (!LinearAlgebra::IsCloseToZero(boost::numeric::ublas::inner_prod(axisX_, axisY_)))
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }

    LinearAlgebra::CrossProduct(normal_, axisX_, axisY_);

    d_ = -(normal_[0] * origin_[0] + normal_[1] * origin_[1] + normal_[2] * origin_[2]);

    // Just a sanity check, it should be useless by construction
    assert(LinearAlgebra::IsNear(boost::numeric::ublas::norm_2(normal_), 1.0));
  }


  void CoordinateSystem3D::SetupCanonical()
  {
    LinearAlgebra::AssignVector(origin_, 0, 0, 0);
    LinearAlgebra::AssignVector(axisX_, 1, 0, 0);
    LinearAlgebra::AssignVector(axisY_, 0, 1, 0);
    CheckAndComputeNormal();
  }


  CoordinateSystem3D::CoordinateSystem3D(const Vector& origin,
                                         const Vector& axisX,
                                         const Vector& axisY) :
    origin_(origin),
    axisX_(axisX),
    axisY_(axisY)
  {
    CheckAndComputeNormal();
  }


  void CoordinateSystem3D::Setup(const std::string& imagePositionPatient,
                                 const std::string& imageOrientationPatient)
  {
    std::string tmpPosition = Orthanc::Toolbox::StripSpaces(imagePositionPatient);
    std::string tmpOrientation = Orthanc::Toolbox::StripSpaces(imageOrientationPatient);

    Vector orientation;
    if (!LinearAlgebra::ParseVector(origin_, tmpPosition) ||
        !LinearAlgebra::ParseVector(orientation, tmpOrientation) ||
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


  CoordinateSystem3D::CoordinateSystem3D(const OrthancPlugins::IDicomDataset& dicom)
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


  CoordinateSystem3D::CoordinateSystem3D(const Orthanc::DicomMap& dicom)
  {
    std::string a, b;

    if (dicom.CopyToString(a, Orthanc::DICOM_TAG_IMAGE_POSITION_PATIENT, false) &&
        dicom.CopyToString(b, Orthanc::DICOM_TAG_IMAGE_ORIENTATION_PATIENT, false))
    {
      Setup(a, b);
    }
    else
    {
      SetupCanonical();
    }    
  }


  Vector CoordinateSystem3D::MapSliceToWorldCoordinates(double x,
                                                        double y) const
  {
    return origin_ + x * axisX_ + y * axisY_;
  }


  double CoordinateSystem3D::ProjectAlongNormal(const Vector& point) const
  {
    return boost::numeric::ublas::inner_prod(point, normal_);
  }


  void CoordinateSystem3D::ProjectPoint(double& offsetX,
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


  bool CoordinateSystem3D::IntersectSegment(Vector& p,
                                            const Vector& edgeFrom,
                                            const Vector& edgeTo) const
  {
    return GeometryToolbox::IntersectPlaneAndSegment(p, normal_, d_, edgeFrom, edgeTo);
  }


  bool CoordinateSystem3D::IntersectLine(Vector& p,
                                         const Vector& origin,
                                         const Vector& direction) const
  {
    return GeometryToolbox::IntersectPlaneAndLine(p, normal_, d_, origin, direction);
  }
}