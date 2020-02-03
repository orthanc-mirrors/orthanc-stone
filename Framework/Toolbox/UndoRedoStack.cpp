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


#include "UndoRedoStack.h"

#include <Core/OrthancException.h>

#include <cassert>

namespace OrthancStone
{
  void UndoRedoStack::Clear(UndoRedoStack::Stack::iterator from)
  {
    for (Stack::iterator it = from; it != stack_.end(); ++it)
    {
      assert(*it != NULL);
      delete *it;
    }

    stack_.erase(from, stack_.end());
  }


  UndoRedoStack::UndoRedoStack() :
    current_(stack_.end())
  {
  }

  
  UndoRedoStack::~UndoRedoStack()
  {
    Clear(stack_.begin());
  }

  
  void UndoRedoStack::Add(ICommand* command)
  {
    if (command == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }
      
    Clear(current_);

    stack_.push_back(command);
    current_ = stack_.end();
  }

  
  void UndoRedoStack::Undo()
  {
    if (current_ != stack_.begin())
    {
      --current_;
        
      assert(*current_ != NULL);
      (*current_)->Undo();
    }
  }

  void UndoRedoStack::Redo()
  {
    if (current_ != stack_.end())
    {
      assert(*current_ != NULL);
      (*current_)->Redo();

      ++current_;
    }
  }
}
