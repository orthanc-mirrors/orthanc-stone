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

#include "../OrthancStone/Sources/Toolbox/Internals/OrientedIntegerLine2D.h"
#include "../OrthancStone/Sources/Toolbox/Internals/RectanglesIntegerProjection.h"
#include "../OrthancStone/Sources/Toolbox/SegmentTree.h"

#include <Logging.h>
#include <OrthancException.h>


namespace
{
  typedef Orthanc::SingleValueObject<int>  Counter;
  
  class CounterFactory : public OrthancStone::SegmentTree::IPayloadFactory
  {
  private:
    int value_;
    
  public:
    CounterFactory(int value) :
      value_(value)
    {
    }
    
    virtual Orthanc::IDynamicObject* Create() ORTHANC_OVERRIDE
    {
      return new Counter(value_);
    }
  };

  class IncrementVisitor : public OrthancStone::SegmentTree::IVisitor
  {
  private:
    int increment_;

  public:
    IncrementVisitor(int increment) :
      increment_(increment)
    {
    }

    virtual void Visit(const OrthancStone::SegmentTree& node,
                       bool fullyInside) ORTHANC_OVERRIDE
    {
      if (fullyInside)
      {
        Counter& payload = node.GetTypedPayload<Counter>();

        if (payload.GetValue() + increment_ < 0)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
        }
        else
        {
          payload.SetValue(payload.GetValue() + increment_);
        }
      }
    }
  };
}


TEST(SegmentTree, Create)
{
  CounterFactory factory(42);
  OrthancStone::SegmentTree root(4u, 15u, factory);   // Check out Figure 1.1 (page 14) from textbook
  
  ASSERT_EQ(4u, root.GetLowBound());
  ASSERT_EQ(15u, root.GetHighBound());
  ASSERT_FALSE(root.IsLeaf());
  ASSERT_EQ(42, root.GetTypedPayload<Counter>().GetValue());
  ASSERT_EQ(21u, root.CountNodes());

  const OrthancStone::SegmentTree* n = &root.GetLeftChild();
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

  ASSERT_TRUE(root.FindLeaf(3) == NULL);
  for (size_t i = 4; i < 15; i++)
  {
    n = root.FindLeaf(i);
    ASSERT_TRUE(n != NULL);
    ASSERT_TRUE(n->IsLeaf());
    ASSERT_EQ(i, n->GetLowBound());
    ASSERT_EQ(i + 1, n->GetHighBound());
    ASSERT_EQ(42, n->GetTypedPayload<Counter>().GetValue());
  }
  ASSERT_TRUE(root.FindLeaf(15) == NULL);
}


static bool CheckCounter(const OrthancStone::SegmentTree& node,
                         int expectedValue)
{
  if (node.GetTypedPayload<Counter>().GetValue() != expectedValue)
  {
    return false;
  }
  else if (node.IsLeaf())
  {
    return true;
  }
  else
  {
    return (CheckCounter(node.GetLeftChild(), expectedValue) &&
            CheckCounter(node.GetRightChild(), expectedValue));
  }
}


#if 0
static void Print(const OrthancStone::SegmentTree& node,
                  unsigned int indent)
{
  for (size_t i = 0; i < indent; i++)
    printf("    ");
  printf("(%lu,%lu): %d\n", node.GetLowBound(), node.GetHighBound(), node.GetTypedPayload<Counter>().GetValue());
  if (!node.IsLeaf())
  {
    Print(node.GetLeftChild(), indent + 1);
    Print(node.GetRightChild(), indent + 1);
  }
}
#endif


TEST(SegmentTree, Visit)
{
  CounterFactory factory(0);
  OrthancStone::SegmentTree root(4u, 15u, factory);   // Check out Figure 1.1 (page 14) from textbook

  ASSERT_TRUE(CheckCounter(root, 0));

  IncrementVisitor plus(1);
  IncrementVisitor minus(-1);

  root.VisitSegment(0, 20, plus);
  ASSERT_EQ(1, root.GetTypedPayload<Counter>().GetValue());
  ASSERT_TRUE(CheckCounter(root.GetLeftChild(), 0));
  ASSERT_TRUE(CheckCounter(root.GetRightChild(), 0));

  root.VisitSegment(0, 20, plus);
  ASSERT_EQ(2, root.GetTypedPayload<Counter>().GetValue());
  ASSERT_TRUE(CheckCounter(root.GetLeftChild(), 0));
  ASSERT_TRUE(CheckCounter(root.GetRightChild(), 0));

  root.VisitSegment(0, 20, minus);
  root.VisitSegment(0, 20, minus);
  ASSERT_TRUE(CheckCounter(root, 0));

  root.VisitSegment(8, 11, plus);
  ASSERT_EQ(0, root.FindNode(4, 15)->GetTypedPayload<Counter>().GetValue());
  ASSERT_EQ(0, root.FindNode(4, 9)->GetTypedPayload<Counter>().GetValue());
  ASSERT_EQ(0, root.FindNode(4, 6)->GetTypedPayload<Counter>().GetValue());
  ASSERT_EQ(0, root.FindNode(4, 5)->GetTypedPayload<Counter>().GetValue());
  ASSERT_EQ(0, root.FindNode(5, 6)->GetTypedPayload<Counter>().GetValue());
  ASSERT_EQ(0, root.FindNode(6, 9)->GetTypedPayload<Counter>().GetValue());
  ASSERT_EQ(0, root.FindNode(6, 7)->GetTypedPayload<Counter>().GetValue());
  ASSERT_EQ(0, root.FindNode(7, 9)->GetTypedPayload<Counter>().GetValue());
  ASSERT_EQ(0, root.FindNode(7, 8)->GetTypedPayload<Counter>().GetValue());
  ASSERT_EQ(1, root.FindNode(8, 9)->GetTypedPayload<Counter>().GetValue());
  ASSERT_EQ(0, root.FindNode(9, 15)->GetTypedPayload<Counter>().GetValue());
  ASSERT_EQ(0, root.FindNode(9, 12)->GetTypedPayload<Counter>().GetValue());
  ASSERT_EQ(1, root.FindNode(9, 10)->GetTypedPayload<Counter>().GetValue());
  ASSERT_EQ(0, root.FindNode(10, 12)->GetTypedPayload<Counter>().GetValue());
  ASSERT_EQ(1, root.FindNode(10, 11)->GetTypedPayload<Counter>().GetValue());
  ASSERT_EQ(0, root.FindNode(11, 12)->GetTypedPayload<Counter>().GetValue());
  ASSERT_EQ(0, root.FindNode(12, 15)->GetTypedPayload<Counter>().GetValue());
  ASSERT_EQ(0, root.FindNode(12, 13)->GetTypedPayload<Counter>().GetValue());
  ASSERT_EQ(0, root.FindNode(13, 15)->GetTypedPayload<Counter>().GetValue());
  ASSERT_EQ(0, root.FindNode(13, 14)->GetTypedPayload<Counter>().GetValue());
  ASSERT_EQ(0, root.FindNode(14, 15)->GetTypedPayload<Counter>().GetValue());
  
  root.VisitSegment(9, 11, minus);
  ASSERT_EQ(0, root.FindNode(4, 15)->GetTypedPayload<Counter>().GetValue());
  ASSERT_EQ(0, root.FindNode(4, 9)->GetTypedPayload<Counter>().GetValue());
  ASSERT_EQ(0, root.FindNode(4, 6)->GetTypedPayload<Counter>().GetValue());
  ASSERT_EQ(0, root.FindNode(4, 5)->GetTypedPayload<Counter>().GetValue());
  ASSERT_EQ(0, root.FindNode(5, 6)->GetTypedPayload<Counter>().GetValue());
  ASSERT_EQ(0, root.FindNode(6, 9)->GetTypedPayload<Counter>().GetValue());
  ASSERT_EQ(0, root.FindNode(6, 7)->GetTypedPayload<Counter>().GetValue());
  ASSERT_EQ(0, root.FindNode(7, 9)->GetTypedPayload<Counter>().GetValue());
  ASSERT_EQ(0, root.FindNode(7, 8)->GetTypedPayload<Counter>().GetValue());
  ASSERT_EQ(1, root.FindNode(8, 9)->GetTypedPayload<Counter>().GetValue());
  ASSERT_TRUE(CheckCounter(root.GetRightChild(), 0));

  root.VisitSegment(8, 9, minus);
  ASSERT_TRUE(CheckCounter(root, 0));
}


TEST(UnionOfRectangles, RectanglesIntegerProjection)
{
  std::list<OrthancStone::Extent2D> rectangles;
  rectangles.push_back(OrthancStone::Extent2D(10, 20, 30, 40));

  {
    OrthancStone::Internals::RectanglesIntegerProjection h(rectangles, true);
    ASSERT_EQ(2u, h.GetEndpointsCount());
    ASSERT_EQ(10, h.GetEndpointCoordinate(0));
    ASSERT_EQ(30, h.GetEndpointCoordinate(1));
    ASSERT_EQ(1u, h.GetProjectedRectanglesCount());
    ASSERT_EQ(0u, h.GetProjectedRectangleLow(0));
    ASSERT_EQ(1u, h.GetProjectedRectangleHigh(0));

    ASSERT_THROW(h.GetEndpointCoordinate(2), Orthanc::OrthancException);
    ASSERT_THROW(h.GetProjectedRectangleLow(1), Orthanc::OrthancException);
    ASSERT_THROW(h.GetProjectedRectangleHigh(1), Orthanc::OrthancException);
  }

  {
    OrthancStone::Internals::RectanglesIntegerProjection h(rectangles, false);
    ASSERT_EQ(2u, h.GetEndpointsCount());
    ASSERT_EQ(20, h.GetEndpointCoordinate(0));
    ASSERT_EQ(40, h.GetEndpointCoordinate(1));
    ASSERT_EQ(1u, h.GetProjectedRectanglesCount());
    ASSERT_EQ(0u, h.GetProjectedRectangleLow(0));
    ASSERT_EQ(1u, h.GetProjectedRectangleHigh(0));
  }

  rectangles.push_back(OrthancStone::Extent2D(20, 30, 40, 50));

  {
    OrthancStone::Internals::RectanglesIntegerProjection h(rectangles, true);
    ASSERT_EQ(4u, h.GetEndpointsCount());
    ASSERT_EQ(10, h.GetEndpointCoordinate(0));
    ASSERT_EQ(20, h.GetEndpointCoordinate(1));
    ASSERT_EQ(30, h.GetEndpointCoordinate(2));
    ASSERT_EQ(40, h.GetEndpointCoordinate(3));
    ASSERT_EQ(2u, h.GetProjectedRectanglesCount());
    ASSERT_EQ(0u, h.GetProjectedRectangleLow(0));
    ASSERT_EQ(2u, h.GetProjectedRectangleHigh(0));
    ASSERT_EQ(1u, h.GetProjectedRectangleLow(1));
    ASSERT_EQ(3u, h.GetProjectedRectangleHigh(1));
  }

  {
    OrthancStone::Internals::RectanglesIntegerProjection h(rectangles, false);
    ASSERT_EQ(4u, h.GetEndpointsCount());
    ASSERT_EQ(20, h.GetEndpointCoordinate(0));
    ASSERT_EQ(30, h.GetEndpointCoordinate(1));
    ASSERT_EQ(40, h.GetEndpointCoordinate(2));
    ASSERT_EQ(50, h.GetEndpointCoordinate(3));
    ASSERT_EQ(2u, h.GetProjectedRectanglesCount());
    ASSERT_EQ(0u, h.GetProjectedRectangleLow(0));
    ASSERT_EQ(2u, h.GetProjectedRectangleHigh(0));
    ASSERT_EQ(1u, h.GetProjectedRectangleLow(1));
    ASSERT_EQ(3u, h.GetProjectedRectangleHigh(1));
  }
}


static void Convert(std::vector<size_t>& horizontal,
                    std::vector<size_t>& vertical,
                    const OrthancStone::Internals::OrientedIntegerLine2D::Chain& chain)
{
  horizontal.clear();
  vertical.clear();

  for (OrthancStone::Internals::OrientedIntegerLine2D::Chain::const_iterator
         it = chain.begin(); it != chain.end(); ++it)
  {
    horizontal.push_back(it->first);
    vertical.push_back(it->second);
  }
}


TEST(UnionOfRectangles, ExtractChains)
{
  std::vector<OrthancStone::Internals::OrientedIntegerLine2D> edges;
  edges.push_back(OrthancStone::Internals::OrientedIntegerLine2D(0, 0, 10, 0));
  edges.push_back(OrthancStone::Internals::OrientedIntegerLine2D(10, 0, 10, 20));
  edges.push_back(OrthancStone::Internals::OrientedIntegerLine2D(10, 20, 0, 20));

  std::list<OrthancStone::Internals::OrientedIntegerLine2D::Chain> chains;
  OrthancStone::Internals::OrientedIntegerLine2D::ExtractChains(chains, edges);

  std::vector<size_t> h, v;

  ASSERT_EQ(1u, chains.size());

  Convert(h, v, chains.front());
  ASSERT_EQ(4u, h.size());
  ASSERT_EQ(0u, h[0]);
  ASSERT_EQ(10u, h[1]);
  ASSERT_EQ(10u, h[2]);
  ASSERT_EQ(0u, h[3]);
  ASSERT_EQ(4u, v.size());
  ASSERT_EQ(0u, v[0]);
  ASSERT_EQ(0u, v[1]);
  ASSERT_EQ(20u, v[2]);
  ASSERT_EQ(20u, v[3]);

  edges.push_back(OrthancStone::Internals::OrientedIntegerLine2D(5, 5, 10, 5));
  OrthancStone::Internals::OrientedIntegerLine2D::ExtractChains(chains, edges);

  ASSERT_EQ(2u, chains.size());
  
  Convert(h, v, chains.front());
  ASSERT_EQ(4u, h.size());
  ASSERT_EQ(0u, h[0]);
  ASSERT_EQ(10u, h[1]);
  ASSERT_EQ(10u, h[2]);
  ASSERT_EQ(0u, h[3]);
  ASSERT_EQ(4u, v.size());
  ASSERT_EQ(0u, v[0]);
  ASSERT_EQ(0u, v[1]);
  ASSERT_EQ(20u, v[2]);
  ASSERT_EQ(20u, v[3]);
  
  Convert(h, v, chains.back());
  ASSERT_EQ(2u, h.size());
  ASSERT_EQ(5u, h[0]);
  ASSERT_EQ(10u, h[1]);
  ASSERT_EQ(2u, v.size());
  ASSERT_EQ(5u, v[0]);
  ASSERT_EQ(5u, v[1]);

  edges.push_back(OrthancStone::Internals::OrientedIntegerLine2D(0, 20, 5, 5));
  OrthancStone::Internals::OrientedIntegerLine2D::ExtractChains(chains, edges);

  ASSERT_EQ(1u, chains.size());
  
  Convert(h, v, chains.front());
  ASSERT_EQ(6u, h.size());
  ASSERT_EQ(0u, h[0]);
  ASSERT_EQ(10u, h[1]);
  ASSERT_EQ(10u, h[2]);
  ASSERT_EQ(0u, h[3]);
  ASSERT_EQ(5u, h[4]);
  ASSERT_EQ(10u, h[5]);
  ASSERT_EQ(6u, v.size());
  ASSERT_EQ(0u, v[0]);
  ASSERT_EQ(0u, v[1]);
  ASSERT_EQ(20u, v[2]);
  ASSERT_EQ(20u, v[3]);
  ASSERT_EQ(5u, v[4]);
  ASSERT_EQ(5u, v[5]);
}
