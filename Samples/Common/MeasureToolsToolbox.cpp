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

#include "MeasureToolsToolbox.h"

#include <boost/math/constants/constants.hpp>

namespace
{
  double g_pi = boost::math::constants::pi<double>();
}

namespace OrthancStone
{

  void AddSquare(PolylineSceneLayer::Chain& chain,
    const Scene2D&      scene,
    const ScenePoint2D& centerS,
    const double&       sideLength)
  {
    chain.clear();
    chain.reserve(4);
    ScenePoint2D centerC = centerS.Apply(scene.GetSceneToCanvasTransform());
    //TODO: take DPI into account
    double handleLX = centerC.GetX() - sideLength / 2;
    double handleTY = centerC.GetY() - sideLength / 2;
    double handleRX = centerC.GetX() + sideLength / 2;
    double handleBY = centerC.GetY() + sideLength / 2;
    ScenePoint2D LTC(handleLX, handleTY);
    ScenePoint2D RTC(handleRX, handleTY);
    ScenePoint2D RBC(handleRX, handleBY);
    ScenePoint2D LBC(handleLX, handleBY);

    ScenePoint2D startLT = LTC.Apply(scene.GetCanvasToSceneTransform());
    ScenePoint2D startRT = RTC.Apply(scene.GetCanvasToSceneTransform());
    ScenePoint2D startRB = RBC.Apply(scene.GetCanvasToSceneTransform());
    ScenePoint2D startLB = LBC.Apply(scene.GetCanvasToSceneTransform());

    chain.push_back(startLT);
    chain.push_back(startRT);
    chain.push_back(startRB);
    chain.push_back(startLB);
  }

  void AddArc(
      PolylineSceneLayer::Chain& chain
    , const Scene2D&      scene
    , const ScenePoint2D& p1
    , const ScenePoint2D& c
    , const ScenePoint2D& p2
    , const double&       radiusS
    , const bool          clockwise
    , const int           subdivisionsCount)
  {
    double p1cAngle = atan2(p1.GetY() - c.GetY(), p1.GetX() - c.GetX());
    double p2cAngle = atan2(p2.GetY() - c.GetY(), p2.GetX() - c.GetX());
    AddArc(
      chain, scene, c, radiusS, p1cAngle, p2cAngle, 
      clockwise, subdivisionsCount);
  }

  double RadiansToDegrees(double angleRad)
  {
    static const double factor = 180.0 / g_pi;
    return angleRad * factor;
  }

  void GetPositionOnBisectingLine(
    ScenePoint2D&       result
    , const ScenePoint2D& p1
    , const ScenePoint2D& c
    , const ScenePoint2D& p2
    , const double d)
  {
    // TODO: fix correct half-plane
    double p1cAngle = atan2(p1.GetY() - c.GetY(), p1.GetX() - c.GetX());
    double p2cAngle = atan2(p2.GetY() - c.GetY(), p2.GetX() - c.GetX());
    double angle = 0.5*(p1cAngle + p2cAngle);
    double unitVectorX = cos(angle);
    double unitVectorY = sin(angle);
    double posX = c.GetX() + d * unitVectorX;
    double posY = c.GetX() + d * unitVectorY;
    result = ScenePoint2D(posX, posY);
  }
   
  void AddArc(
      PolylineSceneLayer::Chain& chain
    , const Scene2D&      scene
    , const ScenePoint2D& centerS
    , const double&       radiusS
    , const double        startAngleRad
    , const double        endAngleRad
    , const bool          clockwise
    , const int           subdivisionsCount)
  {
    double startAngleRadN = NormalizeAngle(startAngleRad);
    double endAngleRadN = NormalizeAngle(endAngleRad);

    double angle1Rad = std::min(startAngleRadN, endAngleRadN);
    double angle2Rad = std::max(startAngleRadN, endAngleRadN);

    // now we are sure angle1Rad < angle2Rad
    // this means that if we draw from 1 to 2, it will be clockwise (
    // increasing angles).
    // let's fix this:
    if (!clockwise)
    {
      angle2Rad -= 2 * g_pi;
      // now we are sure angle2Rad < angle1Rad (since they were normalized) 
      // and, thus, going from 1 to 2 means the angle values will DECREASE,
      // which is the definition of anticlockwise
    }

    chain.clear();
    chain.reserve(subdivisionsCount + 1);

    double angleIncr = (angle2Rad - angle1Rad)
      / static_cast<double>(subdivisionsCount);

    double theta = angle1Rad;
    for (int i = 0; i < subdivisionsCount + 1; ++i)
    {
      double offsetX = radiusS * cos(theta);
      double offsetY = radiusS * sin(theta);
      double pointX = centerS.GetX() + offsetX;
      double pointY = centerS.GetY() + offsetY;
      chain.push_back(ScenePoint2D(pointX, pointY));
      theta += angleIncr;
    }
  }

  void AddCircle(PolylineSceneLayer::Chain& chain,
    const Scene2D&      scene,
    const ScenePoint2D& centerS,
    const double&       radiusS,
    const int           numSubdivisions)
  {
    //ScenePoint2D centerC = centerS.Apply(scene.GetSceneToCanvasTransform());
    //TODO: take DPI into account

    // TODO: automatically compute the number for segments for smooth 
    // display based on the radius in pixels.

    chain.clear();
    chain.reserve(numSubdivisions);

    double angleIncr = (2.0 * g_pi)
      / static_cast<double>(numSubdivisions);

    double theta = 0;
    for (int i = 0; i < numSubdivisions; ++i)
    {
      double offsetX = radiusS * cos(theta);
      double offsetY = radiusS * sin(theta);
      double pointX = centerS.GetX() + offsetX;
      double pointY = centerS.GetY() + offsetY;
      chain.push_back(ScenePoint2D(pointX, pointY));
      theta += angleIncr;
    }
  }

  double NormalizeAngle(double angle)
  {
    double retAngle = angle;
    while (retAngle < 0)
      retAngle += 2 * g_pi;
    while (retAngle >= 2 * g_pi)
      retAngle -= 2 * g_pi;
    return retAngle;
  }

  double MeasureAngle(const ScenePoint2D& p1, const ScenePoint2D& c, const ScenePoint2D& p2)
  {
    double p1cAngle = atan2(p1.GetY() - c.GetY(), p1.GetX() - c.GetX());
    double p2cAngle = atan2(p2.GetY() - c.GetY(), p2.GetX() - c.GetX());
    double delta = p2cAngle - p1cAngle;
    return NormalizeAngle(delta);
  }


#if 0
  void AddEllipse(PolylineSceneLayer::Chain& chain,
    const Scene2D&      scene,
    const ScenePoint2D& centerS,
    const double&       halfHAxis,
    const double&       halfVAxis)
  {
    chain.clear();
    chain.reserve(4);
    ScenePoint2D centerC = centerS.Apply(scene.GetSceneToCanvasTransform());
    //TODO: take DPI into account
    double handleLX = centerC.GetX() - sideLength / 2;
    double handleTY = centerC.GetY() - sideLength / 2;
    double handleRX = centerC.GetX() + sideLength / 2;
    double handleBY = centerC.GetY() + sideLength / 2;
    ScenePoint2D LTC(handleLX, handleTY);
    ScenePoint2D RTC(handleRX, handleTY);
    ScenePoint2D RBC(handleRX, handleBY);
    ScenePoint2D LBC(handleLX, handleBY);

    ScenePoint2D startLT = LTC.Apply(scene.GetCanvasToSceneTransform());
    ScenePoint2D startRT = RTC.Apply(scene.GetCanvasToSceneTransform());
    ScenePoint2D startRB = RBC.Apply(scene.GetCanvasToSceneTransform());
    ScenePoint2D startLB = LBC.Apply(scene.GetCanvasToSceneTransform());

    chain.push_back(startLT);
    chain.push_back(startRT);
    chain.push_back(startRB);
    chain.push_back(startLB);
}
#endif
}
