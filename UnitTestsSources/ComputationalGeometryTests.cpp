/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2022 Osimis S.A., Belgium
 * Copyright (C) 2021-2022 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
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


#include <gtest/gtest.h>

#include "../OrthancStone/Sources/Toolbox/SegmentTree.h"

#include <Logging.h>
#include <OrthancException.h>


namespace
{
  class CounterFactory : public OrthancStone::SegmentTree::IPayloadFactory
  {
  public:
    virtual Orthanc::IDynamicObject* Create()
    {
      return new Orthanc::SingleValueObject<int>(42);
    }
  };
}


TEST(SegmentTree, Basic)
{
  CounterFactory factory;
  OrthancStone::SegmentTree root(4u, 15u, factory);   // Check out Figure 1.1 (page 14) from textbook
  
  ASSERT_EQ(4u, root.GetLowBound());
  ASSERT_EQ(15u, root.GetHighBound());
  ASSERT_FALSE(root.IsLeaf());
  ASSERT_EQ(42, root.GetTypedPayload< Orthanc::SingleValueObject<int> >().GetValue());
  ASSERT_EQ(21u, root.CountNodes());

  OrthancStone::SegmentTree* n = &root.GetLeftChild();
  ASSERT_EQ(4u, n->GetLowBound());
  ASSERT_EQ(9u, n->GetHighBound());
  ASSERT_FALSE(n->IsLeaf());
  ASSERT_EQ(9u, n->CountNodes());

  n = &root.GetLeftChild().GetLeftChild();
  ASSERT_EQ(4u, n->GetLowBound());
  ASSERT_EQ(6u, n->GetHighBound());
  ASSERT_FALSE(n->IsLeaf());
  ASSERT_EQ(3u, n->CountNodes());

  n = &root.GetLeftChild().GetLeftChild().GetLeftChild();
  ASSERT_EQ(4u, n->GetLowBound());
  ASSERT_EQ(5u, n->GetHighBound());
  ASSERT_TRUE(n->IsLeaf());
  ASSERT_THROW(n->GetLeftChild(), Orthanc::OrthancException);
  ASSERT_THROW(n->GetRightChild(), Orthanc::OrthancException);
  ASSERT_EQ(1u, n->CountNodes());

  n = &root.GetLeftChild().GetLeftChild().GetRightChild();
  ASSERT_EQ(5u, n->GetLowBound());
  ASSERT_EQ(6u, n->GetHighBound());
  ASSERT_TRUE(n->IsLeaf());
  ASSERT_EQ(1u, n->CountNodes());

  n = &root.GetLeftChild().GetRightChild();
  ASSERT_EQ(6u, n->GetLowBound());
  ASSERT_EQ(9u, n->GetHighBound());
  ASSERT_FALSE(n->IsLeaf());
  ASSERT_EQ(5u, n->CountNodes());

  n = &root.GetLeftChild().GetRightChild().GetLeftChild();
  ASSERT_EQ(6u, n->GetLowBound());
  ASSERT_EQ(7u, n->GetHighBound());
  ASSERT_TRUE(n->IsLeaf());
  ASSERT_EQ(1u, n->CountNodes());

  n = &root.GetLeftChild().GetRightChild().GetRightChild();
  ASSERT_EQ(7u, n->GetLowBound());
  ASSERT_EQ(9u, n->GetHighBound());
  ASSERT_FALSE(n->IsLeaf());
  ASSERT_EQ(3u, n->CountNodes());

  n = &root.GetLeftChild().GetRightChild().GetRightChild().GetLeftChild();
  ASSERT_EQ(7u, n->GetLowBound());
  ASSERT_EQ(8u, n->GetHighBound());
  ASSERT_TRUE(n->IsLeaf());
  ASSERT_EQ(1u, n->CountNodes());

  n = &root.GetLeftChild().GetRightChild().GetRightChild().GetRightChild();
  ASSERT_EQ(8u, n->GetLowBound());
  ASSERT_EQ(9u, n->GetHighBound());
  ASSERT_TRUE(n->IsLeaf());
  ASSERT_EQ(1u, n->CountNodes());

  n = &root.GetRightChild();
  ASSERT_EQ(9u, n->GetLowBound());
  ASSERT_EQ(15u, n->GetHighBound());
  ASSERT_FALSE(n->IsLeaf());
  ASSERT_EQ(11u, n->CountNodes());

  n = &root.GetRightChild().GetLeftChild();
  ASSERT_EQ(9u, n->GetLowBound());
  ASSERT_EQ(12u, n->GetHighBound());
  ASSERT_FALSE(n->IsLeaf());
  ASSERT_EQ(5u, n->CountNodes());

  n = &root.GetRightChild().GetLeftChild().GetLeftChild();
  ASSERT_EQ(9u, n->GetLowBound());
  ASSERT_EQ(10u, n->GetHighBound());
  ASSERT_TRUE(n->IsLeaf());
  ASSERT_EQ(1u, n->CountNodes());

  n = &root.GetRightChild().GetLeftChild().GetRightChild();
  ASSERT_EQ(10u, n->GetLowBound());
  ASSERT_EQ(12u, n->GetHighBound());
  ASSERT_FALSE(n->IsLeaf());
  ASSERT_EQ(3u, n->CountNodes());

  n = &root.GetRightChild().GetLeftChild().GetRightChild().GetLeftChild();
  ASSERT_EQ(10u, n->GetLowBound());
  ASSERT_EQ(11u, n->GetHighBound());
  ASSERT_TRUE(n->IsLeaf());
  ASSERT_EQ(1u, n->CountNodes());

  n = &root.GetRightChild().GetLeftChild().GetRightChild().GetRightChild();
  ASSERT_EQ(11u, n->GetLowBound());
  ASSERT_EQ(12u, n->GetHighBound());
  ASSERT_TRUE(n->IsLeaf());
  ASSERT_EQ(1u, n->CountNodes());

  n = &root.GetRightChild().GetRightChild();
  ASSERT_EQ(12u, n->GetLowBound());
  ASSERT_EQ(15u, n->GetHighBound());
  ASSERT_FALSE(n->IsLeaf());
  ASSERT_EQ(5u, n->CountNodes());

  n = &root.GetRightChild().GetRightChild().GetLeftChild();
  ASSERT_EQ(12u, n->GetLowBound());
  ASSERT_EQ(13u, n->GetHighBound());
  ASSERT_TRUE(n->IsLeaf());
  ASSERT_EQ(1u, n->CountNodes());

  n = &root.GetRightChild().GetRightChild().GetRightChild();
  ASSERT_EQ(13u, n->GetLowBound());
  ASSERT_EQ(15u, n->GetHighBound());
  ASSERT_FALSE(n->IsLeaf());
  ASSERT_EQ(3u, n->CountNodes());

  n = &root.GetRightChild().GetRightChild().GetRightChild().GetLeftChild();
  ASSERT_EQ(13u, n->GetLowBound());
  ASSERT_EQ(14u, n->GetHighBound());
  ASSERT_TRUE(n->IsLeaf());
  ASSERT_EQ(1u, n->CountNodes());

  n = &root.GetRightChild().GetRightChild().GetRightChild().GetRightChild();
  ASSERT_EQ(14u, n->GetLowBound());
  ASSERT_EQ(15u, n->GetHighBound());
  ASSERT_TRUE(n->IsLeaf());
  ASSERT_EQ(1u, n->CountNodes());
}
