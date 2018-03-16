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


#include "FiniteProjectiveCamera.h"

#include "GeometryToolbox.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>

namespace OrthancStone
{
  void FiniteProjectiveCamera::ComputeMInverse()
  {
    using namespace boost::numeric::ublas;

    // inv(M) = inv(K * R) = inv(R) * inv(K) = R' * inv(K). This
    // matrix is always invertible, by definition of finite
    // projective cameras (page 157).
    Matrix kinv;
    LinearAlgebra::InvertUpperTriangularMatrix(kinv, k_);
    minv_ = prod(trans(r_), kinv);
  }

    
  void FiniteProjectiveCamera::Setup(const Matrix& k,
                                     const Matrix& r,
                                     const Vector& c)
  {
    if (k.size1() != 3 ||
        k.size2() != 3 ||
        !LinearAlgebra::IsCloseToZero(k(1, 0)) ||
        !LinearAlgebra::IsCloseToZero(k(2, 0)) ||
        !LinearAlgebra::IsCloseToZero(k(2, 1)))
    {
      LOG(ERROR) << "Invalid intrinsic parameters";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    if (r.size1() != 3 ||
        r.size2() != 3)
    {
      LOG(ERROR) << "Invalid size for a 3D rotation matrix";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    if (!LinearAlgebra::IsRotationMatrix(r, 100.0 * std::numeric_limits<float>::epsilon()))
    {
      LOG(ERROR) << "Invalid rotation matrix";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    if (c.size() != 3)
    {
      LOG(ERROR) << "Invalid camera center";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    k_ = k;
    r_ = r;
    c_ = c;

    ComputeMInverse();
      
    Matrix tmp = LinearAlgebra::IdentityMatrix(3);
    tmp.resize(3, 4);
    tmp(0, 3) = -c[0];
    tmp(1, 3) = -c[1];
    tmp(2, 3) = -c[2];

    p_ = LinearAlgebra::Product(k, r, tmp);

    assert(p_.size1() == 3 &&
           p_.size2() == 4);

  }

    
  void FiniteProjectiveCamera::Setup(const Matrix& p)
  {
    if (p.size1() != 3 ||
        p.size2() != 4)
    {
      LOG(ERROR) << "Invalid camera matrix";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    p_ = p;

    // M is the left 3x3 submatrix of "P"
    Matrix m = p;
    m.resize(3, 3);

    // p4 is the last column of "P"
    Vector p4(3);
    p4[0] = p(0, 3);
    p4[1] = p(1, 3);
    p4[2] = p(2, 3);

    // The RQ decomposition is explained on page 157
    LinearAlgebra::RQDecomposition3x3(k_, r_, m);
    ComputeMInverse();

    c_ = LinearAlgebra::Product(-minv_, p4);
  }


  FiniteProjectiveCamera::FiniteProjectiveCamera(const double k[9],
                                                 const double r[9],
                                                 const double c[3])
  {
    Matrix kk, rr;
    Vector cc;

    LinearAlgebra::FillMatrix(kk, 3, 3, k);
    LinearAlgebra::FillMatrix(rr, 3, 3, r);
    LinearAlgebra::FillVector(cc, 3, c);

    Setup(kk, rr, cc);
  }


  FiniteProjectiveCamera::FiniteProjectiveCamera(const double p[12])
  {
    Matrix pp;
    LinearAlgebra::FillMatrix(pp, 3, 4, p);
    Setup(pp);
  }


  Vector FiniteProjectiveCamera::GetRayDirection(double x,
                                                 double y) const
  {
    // This derives from Equation (6.14) on page 162, taking "mu =
    // 1" and noticing that "-inv(M)*p4" corresponds to the camera
    // center in finite projective cameras

    // The (x,y) coordinates on the imaged plane, as an homogeneous vector
    Vector xx(3);
    xx[0] = x;
    xx[1] = y;
    xx[2] = 1.0;

    return boost::numeric::ublas::prod(minv_, xx);
  }



  static Vector SetupApply(const Vector& v,
                           bool infinityAllowed)
  {
    if (v.size() == 3)
    {
      // Vector "v" in non-homogeneous coordinates, add the homogeneous component
      Vector vv;
      LinearAlgebra::AssignVector(vv, v[0], v[1], v[2], 1.0);
      return vv;
    }
    else if (v.size() == 4)
    {
      // Vector "v" is already in homogeneous coordinates

      if (!infinityAllowed &&
          LinearAlgebra::IsCloseToZero(v[3]))
      {
        LOG(ERROR) << "Cannot apply a finite projective camera to a "
                   << "point at infinity with this method";
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }

      return v;
    }
    else
    {
      LOG(ERROR) << "The input vector must represent a point in 3D";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
  }

  
  void FiniteProjectiveCamera::ApplyFinite(double& x,
                                           double& y,
                                           const Vector& v) const
  {
    Vector p = boost::numeric::ublas::prod(p_, SetupApply(v, false));    

    if (LinearAlgebra::IsCloseToZero(p[2]))
    {
      // Point at infinity: Should not happen with a finite input point
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
    else
    {
      x = p[0] / p[2];
      y = p[1] / p[2];
    }
  }

  
  Vector FiniteProjectiveCamera::ApplyGeneral(const Vector& v) const
  {
    return boost::numeric::ublas::prod(p_, SetupApply(v, true));
  }


  static Vector AddHomogeneousCoordinate(const Vector& p)
  {
    assert(p.size() == 3);
    return LinearAlgebra::CreateVector(p[0], p[1], p[2], 1);
  }


  FiniteProjectiveCamera::FiniteProjectiveCamera(const Vector& camera,
                                                 const Vector& principalPoint,
                                                 double angle,
                                                 unsigned int imageWidth,
                                                 unsigned int imageHeight,
                                                 double pixelSpacingX,
                                                 double pixelSpacingY)
  {
    if (camera.size() != 3 ||
        principalPoint.size() != 3 ||
        LinearAlgebra::IsCloseToZero(pixelSpacingX) ||
        LinearAlgebra::IsCloseToZero(pixelSpacingY))
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    
    const double focal = boost::numeric::ublas::norm_2(camera - principalPoint);

    if (LinearAlgebra::IsCloseToZero(focal))
    {
      LOG(ERROR) << "Camera lies on the image plane";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
      
    Matrix a;
    LinearAlgebra::AlignVectorsWithRotation(a, camera - principalPoint,
                                            LinearAlgebra::CreateVector(0, 0, -1));

    Matrix r = LinearAlgebra::Product(GeometryToolbox::CreateRotationMatrixAlongZ(angle), a);

    Matrix k = LinearAlgebra::ZeroMatrix(3, 3);
    k(0,0) = focal / pixelSpacingX;
    k(1,1) = focal / pixelSpacingY;
    k(0,2) = static_cast<double>(imageWidth) / 2.0;
    k(1,2) = static_cast<double>(imageHeight) / 2.0;
    k(2,2) = 1;

    Setup(k, r, camera);
 
    {
      // Sanity checks
      Vector v1 = LinearAlgebra::Product(p_, AddHomogeneousCoordinate(camera));
      Vector v2 = LinearAlgebra::Product(p_, AddHomogeneousCoordinate(principalPoint));

      if (!LinearAlgebra::IsCloseToZero(v1[2]) ||   // Camera is mapped to singularity
          LinearAlgebra::IsCloseToZero(v2[2]))
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }

      // The principal point must be mapped to the center of the image
      v2 /= v2[2];

      if (!LinearAlgebra::IsNear(v2[0], static_cast<double>(imageWidth) / 2.0) ||
          !LinearAlgebra::IsNear(v2[1], static_cast<double>(imageHeight) / 2.0))
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
    }
  }
}
