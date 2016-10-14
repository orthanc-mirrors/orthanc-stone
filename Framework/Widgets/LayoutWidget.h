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

#include "WidgetBase.h"


namespace OrthancStone
{
  class LayoutWidget : 
    public WidgetBase,
    public IWidget::IChangeObserver
  {
  private:
    class LayoutMouseTracker;
    class ChildWidget;

    std::vector<ChildWidget*>     children_;
    bool                          isHorizontal_;
    bool                          started_;
    unsigned int                  width_;
    unsigned int                  height_;
    std::auto_ptr<IMouseTracker>  mouseTracker_;
    unsigned int                  paddingLeft_;
    unsigned int                  paddingTop_;
    unsigned int                  paddingRight_;
    unsigned int                  paddingBottom_;
    unsigned int                  paddingInternal_;

    void ComputeChildrenExtents();

  protected:
    virtual bool HasUpdateThread() const 
    {
      return false;
    }

    virtual void UpdateStep();


  public:
    LayoutWidget();

    virtual ~LayoutWidget();

    virtual void NotifyChange(const IWidget& widget);

    void SetHorizontal();

    void SetVertical();

    void SetPadding(unsigned int left,
                    unsigned int top,
                    unsigned int right,
                    unsigned int bottom,
                    unsigned int spacing);
    
    void SetPadding(unsigned int padding);

    unsigned int GetPaddingLeft() const
    {
      return paddingLeft_;
    }

    unsigned int GetPaddingTop() const
    {
      return paddingTop_;
    }

    unsigned int GetPaddingRight() const
    {
      return paddingRight_;
    }

    unsigned int GetPaddingBottom() const
    {
      return paddingBottom_;
    }

    unsigned int GetPaddingInternal() const
    {
      return paddingInternal_;
    }

    IWidget& AddWidget(IWidget* widget);  // Takes ownership

    virtual void SetStatusBar(IStatusBar& statusBar);

    virtual void ResetStatusBar();

    virtual void Start();

    virtual void Stop();

    virtual void SetSize(unsigned int width,
                         unsigned int height);

    virtual bool Render(Orthanc::ImageAccessor& surface);
    
    virtual IMouseTracker* CreateMouseTracker(MouseButton button,
                                              int x,
                                              int y,
                                              KeyboardModifiers modifiers);

    virtual void RenderMouseOver(Orthanc::ImageAccessor& target,
                                 int x,
                                 int y);

    virtual void MouseWheel(MouseWheelDirection direction,
                            int x,
                            int y,
                            KeyboardModifiers modifiers);

    virtual void KeyPressed(char key,
                            KeyboardModifiers modifiers);
  };
}
