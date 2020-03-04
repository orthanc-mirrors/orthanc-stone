/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2020 Osimis S.A., Belgium
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

#include "../../Scene2DViewport/ViewportController.h"
#include "FixedPointAligner.h"

namespace OrthancStone
{
  namespace Internals
  {
    FixedPointAligner::FixedPointAligner(IViewport& viewport,
                                         const ScenePoint2D& p) 
      : viewport_(viewport)
      , canvas_(p)
    {
      std::unique_ptr<IViewport::ILock> lock(viewport_.Lock());
      pivot_ = canvas_.Apply(lock->GetController().GetCanvasToSceneTransform());
    }

    
    void FixedPointAligner::Apply()
    {
      std::unique_ptr<IViewport::ILock> lock(viewport_.Lock());
      ScenePoint2D p = canvas_.Apply(
        lock->GetController().GetCanvasToSceneTransform());

      lock->GetController().SetSceneToCanvasTransform(
        AffineTransform2D::Combine(
          lock->GetController().GetSceneToCanvasTransform(),
          AffineTransform2D::CreateOffset(p.GetX() - pivot_.GetX(),
                                          p.GetY() - pivot_.GetY())));
      lock->Invalidate();
    }
  }
}
