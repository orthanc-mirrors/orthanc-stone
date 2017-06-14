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

#include "WorldSceneWidget.h"

namespace OrthancStone
{
  namespace Samples
  {
    class TestWorldSceneWidget : public WorldSceneWidget
    {
    private:
      class Interactor;

      std::auto_ptr<Interactor>   interactor_;
      bool                        animate_;
      unsigned int                count_;

    protected:
      virtual bool RenderScene(CairoContext& context,
                               const ViewportGeometry& view);

    public:
      TestWorldSceneWidget(bool animate);

      virtual Extent GetSceneExtent();

      virtual bool HasUpdateContent() const
      {
        return animate_;
      }

      virtual void UpdateContent();

      virtual bool HasRenderMouseOver()
      {
        return true;
      }
    };
  }
}
