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

#include <boost/noncopyable.hpp>

namespace OrthancStone
{
  /**
   * Dummy interface to explicitely tag the interfaces whose derived
   * class must be thread-safe. The different methods of such classes
   * might be simlultaneously invoked by several threads, and should
   * be properly protected by mutexes.
   **/
  class IThreadSafe : public boost::noncopyable
  {
  public:
    virtual ~IThreadSafe()
    {
    }
  };


  /**
   * Dummy interface to explicitely tag the interfaces that are NOT
   * expected to be thread-safe. The Orthanc Stone framework ensures
   * that at most one method of such classes will be invoked at a
   * given time. Such classes are automatically protected by the
   * Orthanc Stone framework wherever required.
   **/
  class IThreadUnsafe : public boost::noncopyable
  {
  public:
    virtual ~IThreadUnsafe()
    {
    }
  };
}
