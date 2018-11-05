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


#include "gtest/gtest.h"

#include "../Framework/StoneException.h"



TEST(StoneExceptions, OrthancToStoneConversion)
{
  bool hasBeenCatched = false;
  try {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
  }
  catch (Orthanc::OrthancException& orthancException)
  {
    hasBeenCatched = true;
    OrthancStone::StoneOrthancException stoneException(orthancException);
    ASSERT_EQ(OrthancStone::ErrorCode_OrthancError, stoneException.GetErrorCode());
    ASSERT_EQ(Orthanc::ErrorCode_InternalError, stoneException.GetOrthancErrorCode());
  }

  ASSERT_TRUE(hasBeenCatched);
}
