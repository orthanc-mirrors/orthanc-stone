/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2022 Osimis S.A., Belgium
 * Copyright (C) 2021-2022 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 **/


#include <gtest/gtest.h>

#include "../Sources/Toolbox/Internals/OrientedIntegerLine2D.h"
#include "../Sources/Toolbox/Internals/RectanglesIntegerProjection.h"
#include "../Sources/Toolbox/SegmentTree.h"
#include "../Sources/Toolbox/UnionOfRectangles.h"

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


TEST(UnionOfRectangles, Textbook)
{
  // This is Figure 8.12 from textbook

  std::list<OrthancStone::Extent2D>  rectangles;
  rectangles.push_back(OrthancStone::Extent2D(1, 3, 13, 5));
  rectangles.push_back(OrthancStone::Extent2D(3, 1, 7, 12));
  rectangles.push_back(OrthancStone::Extent2D(5, 7, 11, 10));
  rectangles.push_back(OrthancStone::Extent2D(10, 2, 14, 8));
  rectangles.push_back(OrthancStone::Extent2D(3, 3, 4, 3));  // empty rectangle

  for (unsigned int fillHole = 0; fillHole < 2; fillHole++)
  {
    if (fillHole)
    {
      rectangles.push_back(OrthancStone::Extent2D(6.5, 4.5, 10.5, 7.5));
    }
    
    std::list< std::vector<OrthancStone::ScenePoint2D> > contours;
    OrthancStone::UnionOfRectangles::Apply(contours, rectangles);

    ASSERT_EQ(fillHole ? 1u : 2u, contours.size());
    ASSERT_EQ(17u, contours.front().size());
    ASSERT_TRUE(contours.front()[0].IsEqual(OrthancStone::ScenePoint2D(3, 12)));
    ASSERT_TRUE(contours.front()[1].IsEqual(OrthancStone::ScenePoint2D(7, 12)));
    ASSERT_TRUE(contours.front()[2].IsEqual(OrthancStone::ScenePoint2D(7, 10)));
    ASSERT_TRUE(contours.front()[3].IsEqual(OrthancStone::ScenePoint2D(11, 10)));
    ASSERT_TRUE(contours.front()[4].IsEqual(OrthancStone::ScenePoint2D(11, 8)));
    ASSERT_TRUE(contours.front()[5].IsEqual(OrthancStone::ScenePoint2D(14, 8)));
    ASSERT_TRUE(contours.front()[6].IsEqual(OrthancStone::ScenePoint2D(14, 2)));
    ASSERT_TRUE(contours.front()[7].IsEqual(OrthancStone::ScenePoint2D(10, 2)));
    ASSERT_TRUE(contours.front()[8].IsEqual(OrthancStone::ScenePoint2D(10, 3)));
    ASSERT_TRUE(contours.front()[9].IsEqual(OrthancStone::ScenePoint2D(7, 3)));
    ASSERT_TRUE(contours.front()[10].IsEqual(OrthancStone::ScenePoint2D(7, 1)));
    ASSERT_TRUE(contours.front()[11].IsEqual(OrthancStone::ScenePoint2D(3, 1)));
    ASSERT_TRUE(contours.front()[12].IsEqual(OrthancStone::ScenePoint2D(3, 3)));
    ASSERT_TRUE(contours.front()[13].IsEqual(OrthancStone::ScenePoint2D(1, 3)));
    ASSERT_TRUE(contours.front()[14].IsEqual(OrthancStone::ScenePoint2D(1, 5)));
    ASSERT_TRUE(contours.front()[15].IsEqual(OrthancStone::ScenePoint2D(3, 5)));
    ASSERT_TRUE(contours.front()[16].IsEqual(OrthancStone::ScenePoint2D(3, 12)));

    if (!fillHole)
    {
      ASSERT_EQ(5u, contours.back().size());
      ASSERT_TRUE(contours.back()[0].IsEqual(OrthancStone::ScenePoint2D(10, 7)));
      ASSERT_TRUE(contours.back()[1].IsEqual(OrthancStone::ScenePoint2D(7, 7)));
      ASSERT_TRUE(contours.back()[2].IsEqual(OrthancStone::ScenePoint2D(7, 5)));
      ASSERT_TRUE(contours.back()[3].IsEqual(OrthancStone::ScenePoint2D(10, 5)));
      ASSERT_TRUE(contours.back()[4].IsEqual(OrthancStone::ScenePoint2D(10, 7)));
    }
  }
}


#if 0
static void SaveSvg(const std::list< std::vector<OrthancStone::ScenePoint2D> >& contours)
{
  float ww = 15;
  float hh = 13;

  FILE* fp = fopen("test.svg", "w");
  fprintf(fp, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n");
  fprintf(fp, "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n");
  fprintf(fp, "<svg width=\"%f\" height=\"%f\" viewBox=\"0 0 %f %f\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\">\n", 100.0f*ww, 100.0f*hh, ww, hh);

  // http://thenewcode.com/1068/Making-Arrows-in-SVG
  fprintf(fp, "<defs>\n");
  fprintf(fp, "<marker id=\"arrowhead\" markerWidth=\"2\" markerHeight=\"3\" \n");
  fprintf(fp, "refX=\"2\" refY=\"1.5\" orient=\"auto\">\n");
  fprintf(fp, "<polygon points=\"0 0, 2 1.5, 0 3\" />\n");
  fprintf(fp, "</marker>\n");
  fprintf(fp, "</defs>\n");

  fprintf(fp, "<rect fill=\"#fff\" stroke=\"#000\" x=\"0\" y=\"0\" width=\"%f\" height=\"%f\"/>\n", ww, hh);

  for (std::list< std::vector<OrthancStone::ScenePoint2D> >::const_iterator
         it = contours.begin(); it != contours.end(); ++it)
  {
    for (size_t i = 0; i + 1 < it->size(); i++)
    {
      float x1 = (*it)[i].GetX();
      float x2 = (*it)[i + 1].GetX();
      float y1 = (*it)[i].GetY();
      float y2 = (*it)[i + 1].GetY();
      
      fprintf(fp, "<line x1=\"%f\" y1=\"%f\" x2=\"%f\" y2=\"%f\" stroke=\"blue\" stroke-width=\"0.05\" marker-end=\"url(#arrowhead)\"/>\n", x1, y1, x2, y2);
    }
  }
  fprintf(fp, "</svg>\n");
  
  fclose(fp);
}
#endif


TEST(UnionOfRectangles, EdgeCases)
{
  {
    std::list<OrthancStone::Extent2D>  rectangles;

    std::list< std::vector<OrthancStone::ScenePoint2D> > contours;
    OrthancStone::UnionOfRectangles::Apply(contours, rectangles);

    ASSERT_EQ(0u, contours.size());
  }

  {
    std::list<OrthancStone::Extent2D>  rectangles;
    rectangles.push_back(OrthancStone::Extent2D(3, 3, 4, 3));  // empty rectangle

    std::list< std::vector<OrthancStone::ScenePoint2D> > contours;
    OrthancStone::UnionOfRectangles::Apply(contours, rectangles);

    ASSERT_EQ(0u, contours.size());
  }

  {
    std::list<OrthancStone::Extent2D>  rectangles;
    rectangles.push_back(OrthancStone::Extent2D(1, 1, 2, 2));

    std::list< std::vector<OrthancStone::ScenePoint2D> > contours;
    OrthancStone::UnionOfRectangles::Apply(contours, rectangles);

    ASSERT_EQ(1u, contours.size());
    ASSERT_EQ(5u, contours.front().size());
    ASSERT_TRUE(contours.front()[0].IsEqual(OrthancStone::ScenePoint2D(1, 2)));
    ASSERT_TRUE(contours.front()[1].IsEqual(OrthancStone::ScenePoint2D(2, 2)));
    ASSERT_TRUE(contours.front()[2].IsEqual(OrthancStone::ScenePoint2D(2, 1)));
    ASSERT_TRUE(contours.front()[3].IsEqual(OrthancStone::ScenePoint2D(1, 1)));
    ASSERT_TRUE(contours.front()[4].IsEqual(OrthancStone::ScenePoint2D(1, 2)));
  }

  {
    std::list<OrthancStone::Extent2D>  rectangles;
    rectangles.push_back(OrthancStone::Extent2D(1, 1, 2, 2));
    rectangles.push_back(OrthancStone::Extent2D(1, 3, 2, 4));

    std::list< std::vector<OrthancStone::ScenePoint2D> > contours;
    OrthancStone::UnionOfRectangles::Apply(contours, rectangles);

    ASSERT_EQ(2u, contours.size());

    ASSERT_EQ(5u, contours.front().size());
    ASSERT_TRUE(contours.front()[0].IsEqual(OrthancStone::ScenePoint2D(1, 4)));
    ASSERT_TRUE(contours.front()[1].IsEqual(OrthancStone::ScenePoint2D(2, 4)));
    ASSERT_TRUE(contours.front()[2].IsEqual(OrthancStone::ScenePoint2D(2, 3)));
    ASSERT_TRUE(contours.front()[3].IsEqual(OrthancStone::ScenePoint2D(1, 3)));
    ASSERT_TRUE(contours.front()[4].IsEqual(OrthancStone::ScenePoint2D(1, 4)));

    ASSERT_EQ(5u, contours.back().size());
    ASSERT_TRUE(contours.back()[0].IsEqual(OrthancStone::ScenePoint2D(1, 2)));
    ASSERT_TRUE(contours.back()[1].IsEqual(OrthancStone::ScenePoint2D(2, 2)));
    ASSERT_TRUE(contours.back()[2].IsEqual(OrthancStone::ScenePoint2D(2, 1)));
    ASSERT_TRUE(contours.back()[3].IsEqual(OrthancStone::ScenePoint2D(1, 1)));
    ASSERT_TRUE(contours.back()[4].IsEqual(OrthancStone::ScenePoint2D(1, 2)));
  }

  {
    std::list<OrthancStone::Extent2D>  rectangles;
    rectangles.push_back(OrthancStone::Extent2D(1, 4, 4, 6));
    rectangles.push_back(OrthancStone::Extent2D(4, 6, 7, 8));
    rectangles.push_back(OrthancStone::Extent2D(4, 2, 7, 4));
    rectangles.push_back(OrthancStone::Extent2D(7, 4, 10, 6));
  
    std::list< std::vector<OrthancStone::ScenePoint2D> > contours;
    OrthancStone::UnionOfRectangles::Apply(contours, rectangles);

    ASSERT_EQ(1u, contours.size());
    ASSERT_EQ(17u, contours.front().size());
    ASSERT_TRUE(contours.front()[0].IsEqual(OrthancStone::ScenePoint2D(4, 8)));
    ASSERT_TRUE(contours.front()[1].IsEqual(OrthancStone::ScenePoint2D(7, 8)));
    ASSERT_TRUE(contours.front()[2].IsEqual(OrthancStone::ScenePoint2D(7, 6)));
    ASSERT_TRUE(contours.front()[3].IsEqual(OrthancStone::ScenePoint2D(10, 6)));
    ASSERT_TRUE(contours.front()[4].IsEqual(OrthancStone::ScenePoint2D(10, 4)));
    ASSERT_TRUE(contours.front()[5].IsEqual(OrthancStone::ScenePoint2D(7, 4)));
    ASSERT_TRUE(contours.front()[6].IsEqual(OrthancStone::ScenePoint2D(7, 2)));
    ASSERT_TRUE(contours.front()[7].IsEqual(OrthancStone::ScenePoint2D(4, 2)));
    ASSERT_TRUE(contours.front()[8].IsEqual(OrthancStone::ScenePoint2D(4, 4)));
    ASSERT_TRUE(contours.front()[9].IsEqual(OrthancStone::ScenePoint2D(7, 4)));
    ASSERT_TRUE(contours.front()[10].IsEqual(OrthancStone::ScenePoint2D(7, 6)));
    ASSERT_TRUE(contours.front()[11].IsEqual(OrthancStone::ScenePoint2D(4, 6)));
    ASSERT_TRUE(contours.front()[12].IsEqual(OrthancStone::ScenePoint2D(4, 4)));
    ASSERT_TRUE(contours.front()[13].IsEqual(OrthancStone::ScenePoint2D(1, 4)));
    ASSERT_TRUE(contours.front()[14].IsEqual(OrthancStone::ScenePoint2D(1, 6)));
    ASSERT_TRUE(contours.front()[15].IsEqual(OrthancStone::ScenePoint2D(4, 6)));
    ASSERT_TRUE(contours.front()[16].IsEqual(OrthancStone::ScenePoint2D(4, 8)));
  }
  
  {
    std::list<OrthancStone::Extent2D>  rectangles;
    rectangles.push_back(OrthancStone::Extent2D(1, 4, 4, 6));
    rectangles.push_back(OrthancStone::Extent2D(4, 4, 7, 6));
  
    std::list< std::vector<OrthancStone::ScenePoint2D> > contours;
    OrthancStone::UnionOfRectangles::Apply(contours, rectangles);

    ASSERT_EQ(1u, contours.size());
    ASSERT_EQ(5u, contours.front().size());
    ASSERT_TRUE(contours.front()[0].IsEqual(OrthancStone::ScenePoint2D(1, 6)));
    ASSERT_TRUE(contours.front()[1].IsEqual(OrthancStone::ScenePoint2D(7, 6)));
    ASSERT_TRUE(contours.front()[2].IsEqual(OrthancStone::ScenePoint2D(7, 4)));
    ASSERT_TRUE(contours.front()[3].IsEqual(OrthancStone::ScenePoint2D(1, 4)));
    ASSERT_TRUE(contours.front()[4].IsEqual(OrthancStone::ScenePoint2D(1, 6)));
  }
  
  {
    std::list<OrthancStone::Extent2D>  rectangles;
    rectangles.push_back(OrthancStone::Extent2D(1, 4, 4, 6));
    rectangles.push_back(OrthancStone::Extent2D(1, 6, 4, 8));
  
    std::list< std::vector<OrthancStone::ScenePoint2D> > contours;
    OrthancStone::UnionOfRectangles::Apply(contours, rectangles);

    ASSERT_EQ(1u, contours.size());
    ASSERT_EQ(5u, contours.front().size());
    ASSERT_TRUE(contours.front()[0].IsEqual(OrthancStone::ScenePoint2D(1, 8)));
    ASSERT_TRUE(contours.front()[1].IsEqual(OrthancStone::ScenePoint2D(4, 8)));
    ASSERT_TRUE(contours.front()[2].IsEqual(OrthancStone::ScenePoint2D(4, 4)));
    ASSERT_TRUE(contours.front()[3].IsEqual(OrthancStone::ScenePoint2D(1, 4)));
    ASSERT_TRUE(contours.front()[4].IsEqual(OrthancStone::ScenePoint2D(1, 8)));
  }
  
  {
    std::list<OrthancStone::Extent2D>  rectangles;
    rectangles.push_back(OrthancStone::Extent2D(1, 1, 2, 2));
    rectangles.push_back(OrthancStone::Extent2D(4, 4, 7, 6));
    rectangles.push_back(OrthancStone::Extent2D(4, 6, 7, 8));

    std::list< std::vector<OrthancStone::ScenePoint2D> > contours;
    OrthancStone::UnionOfRectangles::Apply(contours, rectangles);
    
    ASSERT_EQ(2u, contours.size());
    
    ASSERT_EQ(5u, contours.front().size());
    ASSERT_TRUE(contours.front()[0].IsEqual(OrthancStone::ScenePoint2D(4, 8)));
    ASSERT_TRUE(contours.front()[1].IsEqual(OrthancStone::ScenePoint2D(7, 8)));
    ASSERT_TRUE(contours.front()[2].IsEqual(OrthancStone::ScenePoint2D(7, 4)));
    ASSERT_TRUE(contours.front()[3].IsEqual(OrthancStone::ScenePoint2D(4, 4)));
    ASSERT_TRUE(contours.front()[4].IsEqual(OrthancStone::ScenePoint2D(4, 8)));
    
    ASSERT_EQ(5u, contours.back().size());
    ASSERT_TRUE(contours.back()[0].IsEqual(OrthancStone::ScenePoint2D(1, 2)));
    ASSERT_TRUE(contours.back()[1].IsEqual(OrthancStone::ScenePoint2D(2, 2)));
    ASSERT_TRUE(contours.back()[2].IsEqual(OrthancStone::ScenePoint2D(2, 1)));
    ASSERT_TRUE(contours.back()[3].IsEqual(OrthancStone::ScenePoint2D(1, 1)));
    ASSERT_TRUE(contours.back()[4].IsEqual(OrthancStone::ScenePoint2D(1, 2)));
  }

  {
    std::list<OrthancStone::Extent2D>  rectangles;
    rectangles.push_back(OrthancStone::Extent2D(1, 4, 4, 6));
    rectangles.push_back(OrthancStone::Extent2D(6, 4, 9, 6));
    rectangles.push_back(OrthancStone::Extent2D(4, 6, 7, 8));
    rectangles.push_back(OrthancStone::Extent2D(4, 2, 7, 6));

    std::list< std::vector<OrthancStone::ScenePoint2D> > contours;
    OrthancStone::UnionOfRectangles::Apply(contours, rectangles);

    ASSERT_EQ(1u, contours.size());
    ASSERT_EQ(13u, contours.front().size());
    ASSERT_TRUE(contours.front()[0].IsEqual(OrthancStone::ScenePoint2D(4, 8)));
    ASSERT_TRUE(contours.front()[1].IsEqual(OrthancStone::ScenePoint2D(7, 8)));
    ASSERT_TRUE(contours.front()[2].IsEqual(OrthancStone::ScenePoint2D(7, 6)));
    ASSERT_TRUE(contours.front()[3].IsEqual(OrthancStone::ScenePoint2D(9, 6)));
    ASSERT_TRUE(contours.front()[4].IsEqual(OrthancStone::ScenePoint2D(9, 4)));
    ASSERT_TRUE(contours.front()[5].IsEqual(OrthancStone::ScenePoint2D(7, 4)));
    ASSERT_TRUE(contours.front()[6].IsEqual(OrthancStone::ScenePoint2D(7, 2)));
    ASSERT_TRUE(contours.front()[7].IsEqual(OrthancStone::ScenePoint2D(4, 2)));
    ASSERT_TRUE(contours.front()[8].IsEqual(OrthancStone::ScenePoint2D(4, 4)));
    ASSERT_TRUE(contours.front()[9].IsEqual(OrthancStone::ScenePoint2D(1, 4)));
    ASSERT_TRUE(contours.front()[10].IsEqual(OrthancStone::ScenePoint2D(1, 6)));
    ASSERT_TRUE(contours.front()[11].IsEqual(OrthancStone::ScenePoint2D(4, 6)));
    ASSERT_TRUE(contours.front()[12].IsEqual(OrthancStone::ScenePoint2D(4, 8)));
  }

  {
    std::list<OrthancStone::Extent2D>  rectangles;
    rectangles.push_back(OrthancStone::Extent2D(1, 4, 4, 6));
    rectangles.push_back(OrthancStone::Extent2D(4, 6, 7, 8));
    rectangles.push_back(OrthancStone::Extent2D(4, 2, 7, 6));

    std::list< std::vector<OrthancStone::ScenePoint2D> > contours;
    OrthancStone::UnionOfRectangles::Apply(contours, rectangles);

    ASSERT_EQ(1u, contours.size());
    ASSERT_EQ(9u, contours.front().size());
    ASSERT_TRUE(contours.front()[0].IsEqual(OrthancStone::ScenePoint2D(4, 8)));
    ASSERT_TRUE(contours.front()[1].IsEqual(OrthancStone::ScenePoint2D(7, 8)));
    ASSERT_TRUE(contours.front()[2].IsEqual(OrthancStone::ScenePoint2D(7, 2)));
    ASSERT_TRUE(contours.front()[3].IsEqual(OrthancStone::ScenePoint2D(4, 2)));
    ASSERT_TRUE(contours.front()[4].IsEqual(OrthancStone::ScenePoint2D(4, 4)));
    ASSERT_TRUE(contours.front()[5].IsEqual(OrthancStone::ScenePoint2D(1, 4)));
    ASSERT_TRUE(contours.front()[6].IsEqual(OrthancStone::ScenePoint2D(1, 6)));
    ASSERT_TRUE(contours.front()[7].IsEqual(OrthancStone::ScenePoint2D(4, 6)));
    ASSERT_TRUE(contours.front()[8].IsEqual(OrthancStone::ScenePoint2D(4, 8)));
  }

  {
    std::list<OrthancStone::Extent2D>  rectangles;
    rectangles.push_back(OrthancStone::Extent2D(2, 2, 4, 4));
    rectangles.push_back(OrthancStone::Extent2D(3, 3, 5, 5));

    std::list< std::vector<OrthancStone::ScenePoint2D> > contours;
    OrthancStone::UnionOfRectangles::Apply(contours, rectangles);

    ASSERT_EQ(1u, contours.size());
    ASSERT_EQ(9u, contours.front().size());
    ASSERT_TRUE(contours.front()[0].IsEqual(OrthancStone::ScenePoint2D(3, 5)));
    ASSERT_TRUE(contours.front()[1].IsEqual(OrthancStone::ScenePoint2D(5, 5)));
    ASSERT_TRUE(contours.front()[2].IsEqual(OrthancStone::ScenePoint2D(5, 3)));
    ASSERT_TRUE(contours.front()[3].IsEqual(OrthancStone::ScenePoint2D(4, 3)));
    ASSERT_TRUE(contours.front()[4].IsEqual(OrthancStone::ScenePoint2D(4, 2)));
    ASSERT_TRUE(contours.front()[5].IsEqual(OrthancStone::ScenePoint2D(2, 2)));
    ASSERT_TRUE(contours.front()[6].IsEqual(OrthancStone::ScenePoint2D(2, 4)));
    ASSERT_TRUE(contours.front()[7].IsEqual(OrthancStone::ScenePoint2D(3, 4)));
    ASSERT_TRUE(contours.front()[8].IsEqual(OrthancStone::ScenePoint2D(3, 5)));
  }

  {
    std::list<OrthancStone::Extent2D>  rectangles;
    rectangles.push_back(OrthancStone::Extent2D(2, 2, 4, 4));
    rectangles.push_back(OrthancStone::Extent2D(3, 1, 5, 3));

    std::list< std::vector<OrthancStone::ScenePoint2D> > contours;
    OrthancStone::UnionOfRectangles::Apply(contours, rectangles);

    ASSERT_EQ(1u, contours.size());
    ASSERT_EQ(9u, contours.front().size());
    ASSERT_TRUE(contours.front()[0].IsEqual(OrthancStone::ScenePoint2D(2, 4)));
    ASSERT_TRUE(contours.front()[1].IsEqual(OrthancStone::ScenePoint2D(4, 4)));
    ASSERT_TRUE(contours.front()[2].IsEqual(OrthancStone::ScenePoint2D(4, 3)));
    ASSERT_TRUE(contours.front()[3].IsEqual(OrthancStone::ScenePoint2D(5, 3)));
    ASSERT_TRUE(contours.front()[4].IsEqual(OrthancStone::ScenePoint2D(5, 1)));
    ASSERT_TRUE(contours.front()[5].IsEqual(OrthancStone::ScenePoint2D(3, 1)));
    ASSERT_TRUE(contours.front()[6].IsEqual(OrthancStone::ScenePoint2D(3, 2)));
    ASSERT_TRUE(contours.front()[7].IsEqual(OrthancStone::ScenePoint2D(2, 2)));
    ASSERT_TRUE(contours.front()[8].IsEqual(OrthancStone::ScenePoint2D(2, 4)));
  }

  {
    std::list<OrthancStone::Extent2D>  rectangles;
    rectangles.push_back(OrthancStone::Extent2D(2, 2, 4, 4));
    rectangles.push_back(OrthancStone::Extent2D(1, 1, 3, 3));

    std::list< std::vector<OrthancStone::ScenePoint2D> > contours;
    OrthancStone::UnionOfRectangles::Apply(contours, rectangles);

    ASSERT_EQ(1u, contours.size());
    ASSERT_EQ(9u, contours.front().size());
    ASSERT_TRUE(contours.front()[0].IsEqual(OrthancStone::ScenePoint2D(2, 4)));
    ASSERT_TRUE(contours.front()[1].IsEqual(OrthancStone::ScenePoint2D(4, 4)));
    ASSERT_TRUE(contours.front()[2].IsEqual(OrthancStone::ScenePoint2D(4, 2)));
    ASSERT_TRUE(contours.front()[3].IsEqual(OrthancStone::ScenePoint2D(3, 2)));
    ASSERT_TRUE(contours.front()[4].IsEqual(OrthancStone::ScenePoint2D(3, 1)));
    ASSERT_TRUE(contours.front()[5].IsEqual(OrthancStone::ScenePoint2D(1, 1)));
    ASSERT_TRUE(contours.front()[6].IsEqual(OrthancStone::ScenePoint2D(1, 3)));
    ASSERT_TRUE(contours.front()[7].IsEqual(OrthancStone::ScenePoint2D(2, 3)));
    ASSERT_TRUE(contours.front()[8].IsEqual(OrthancStone::ScenePoint2D(2, 4)));
  }

  {
    std::list<OrthancStone::Extent2D>  rectangles;
    rectangles.push_back(OrthancStone::Extent2D(2, 2, 4, 4));
    rectangles.push_back(OrthancStone::Extent2D(1, 3, 3, 5));

    std::list< std::vector<OrthancStone::ScenePoint2D> > contours;
    OrthancStone::UnionOfRectangles::Apply(contours, rectangles);

    ASSERT_EQ(1u, contours.size());
    ASSERT_EQ(9u, contours.front().size());
    ASSERT_TRUE(contours.front()[0].IsEqual(OrthancStone::ScenePoint2D(1, 5)));
    ASSERT_TRUE(contours.front()[1].IsEqual(OrthancStone::ScenePoint2D(3, 5)));
    ASSERT_TRUE(contours.front()[2].IsEqual(OrthancStone::ScenePoint2D(3, 4)));
    ASSERT_TRUE(contours.front()[3].IsEqual(OrthancStone::ScenePoint2D(4, 4)));
    ASSERT_TRUE(contours.front()[4].IsEqual(OrthancStone::ScenePoint2D(4, 2)));
    ASSERT_TRUE(contours.front()[5].IsEqual(OrthancStone::ScenePoint2D(2, 2)));
    ASSERT_TRUE(contours.front()[6].IsEqual(OrthancStone::ScenePoint2D(2, 3)));
    ASSERT_TRUE(contours.front()[7].IsEqual(OrthancStone::ScenePoint2D(1, 3)));
    ASSERT_TRUE(contours.front()[8].IsEqual(OrthancStone::ScenePoint2D(1, 5)));
  }

  {
    std::list<OrthancStone::Extent2D>  rectangles;
    rectangles.push_back(OrthancStone::Extent2D(2, 2, 3, 3));
    rectangles.push_back(OrthancStone::Extent2D(3, 1, 4, 2));

    std::list< std::vector<OrthancStone::ScenePoint2D> > contours;
    OrthancStone::UnionOfRectangles::Apply(contours, rectangles);

    ASSERT_EQ(1u, contours.size());
    ASSERT_EQ(9u, contours.front().size());
    ASSERT_TRUE(contours.front()[0].IsEqual(OrthancStone::ScenePoint2D(2, 3)));
    ASSERT_TRUE(contours.front()[1].IsEqual(OrthancStone::ScenePoint2D(3, 3)));
    ASSERT_TRUE(contours.front()[2].IsEqual(OrthancStone::ScenePoint2D(3, 2)));
    ASSERT_TRUE(contours.front()[3].IsEqual(OrthancStone::ScenePoint2D(4, 2)));
    ASSERT_TRUE(contours.front()[4].IsEqual(OrthancStone::ScenePoint2D(4, 1)));
    ASSERT_TRUE(contours.front()[5].IsEqual(OrthancStone::ScenePoint2D(3, 1)));
    ASSERT_TRUE(contours.front()[6].IsEqual(OrthancStone::ScenePoint2D(3, 2)));
    ASSERT_TRUE(contours.front()[7].IsEqual(OrthancStone::ScenePoint2D(2, 2)));
    ASSERT_TRUE(contours.front()[8].IsEqual(OrthancStone::ScenePoint2D(2, 3)));
  }

  {
    std::list<OrthancStone::Extent2D>  rectangles;
    rectangles.push_back(OrthancStone::Extent2D(2, 2, 3, 3));
    rectangles.push_back(OrthancStone::Extent2D(3, 3, 4, 4));

    std::list< std::vector<OrthancStone::ScenePoint2D> > contours;
    OrthancStone::UnionOfRectangles::Apply(contours, rectangles);

    ASSERT_EQ(1u, contours.size());
    ASSERT_EQ(9u, contours.front().size());
    ASSERT_TRUE(contours.front()[0].IsEqual(OrthancStone::ScenePoint2D(3, 4)));
    ASSERT_TRUE(contours.front()[1].IsEqual(OrthancStone::ScenePoint2D(4, 4)));
    ASSERT_TRUE(contours.front()[2].IsEqual(OrthancStone::ScenePoint2D(4, 3)));
    ASSERT_TRUE(contours.front()[3].IsEqual(OrthancStone::ScenePoint2D(3, 3)));
    ASSERT_TRUE(contours.front()[4].IsEqual(OrthancStone::ScenePoint2D(3, 2)));
    ASSERT_TRUE(contours.front()[5].IsEqual(OrthancStone::ScenePoint2D(2, 2)));
    ASSERT_TRUE(contours.front()[6].IsEqual(OrthancStone::ScenePoint2D(2, 3)));
    ASSERT_TRUE(contours.front()[7].IsEqual(OrthancStone::ScenePoint2D(3, 3)));
    ASSERT_TRUE(contours.front()[8].IsEqual(OrthancStone::ScenePoint2D(3, 4)));
  }

  {
    std::list<OrthancStone::Extent2D>  rectangles;
    rectangles.push_back(OrthancStone::Extent2D(2, 2, 3, 3));
    rectangles.push_back(OrthancStone::Extent2D(1, 3, 2, 4));

    std::list< std::vector<OrthancStone::ScenePoint2D> > contours;
    OrthancStone::UnionOfRectangles::Apply(contours, rectangles);

    ASSERT_EQ(1u, contours.size());
    ASSERT_EQ(9u, contours.front().size());
    ASSERT_TRUE(contours.front()[0].IsEqual(OrthancStone::ScenePoint2D(1, 4)));
    ASSERT_TRUE(contours.front()[1].IsEqual(OrthancStone::ScenePoint2D(2, 4)));
    ASSERT_TRUE(contours.front()[2].IsEqual(OrthancStone::ScenePoint2D(2, 3)));
    ASSERT_TRUE(contours.front()[3].IsEqual(OrthancStone::ScenePoint2D(3, 3)));
    ASSERT_TRUE(contours.front()[4].IsEqual(OrthancStone::ScenePoint2D(3, 2)));
    ASSERT_TRUE(contours.front()[5].IsEqual(OrthancStone::ScenePoint2D(2, 2)));
    ASSERT_TRUE(contours.front()[6].IsEqual(OrthancStone::ScenePoint2D(2, 3)));
    ASSERT_TRUE(contours.front()[7].IsEqual(OrthancStone::ScenePoint2D(1, 3)));
    ASSERT_TRUE(contours.front()[8].IsEqual(OrthancStone::ScenePoint2D(1, 4)));
  }

  {
    std::list<OrthancStone::Extent2D>  rectangles;
    rectangles.push_back(OrthancStone::Extent2D(2, 2, 3, 3));
    rectangles.push_back(OrthancStone::Extent2D(1, 1, 2, 2));

    std::list< std::vector<OrthancStone::ScenePoint2D> > contours;
    OrthancStone::UnionOfRectangles::Apply(contours, rectangles);

    ASSERT_EQ(1u, contours.size());
    ASSERT_EQ(9u, contours.front().size());
    ASSERT_TRUE(contours.front()[0].IsEqual(OrthancStone::ScenePoint2D(2, 3)));
    ASSERT_TRUE(contours.front()[1].IsEqual(OrthancStone::ScenePoint2D(3, 3)));
    ASSERT_TRUE(contours.front()[2].IsEqual(OrthancStone::ScenePoint2D(3, 2)));
    ASSERT_TRUE(contours.front()[3].IsEqual(OrthancStone::ScenePoint2D(2, 2)));
    ASSERT_TRUE(contours.front()[4].IsEqual(OrthancStone::ScenePoint2D(2, 1)));
    ASSERT_TRUE(contours.front()[5].IsEqual(OrthancStone::ScenePoint2D(1, 1)));
    ASSERT_TRUE(contours.front()[6].IsEqual(OrthancStone::ScenePoint2D(1, 2)));
    ASSERT_TRUE(contours.front()[7].IsEqual(OrthancStone::ScenePoint2D(2, 2)));
    ASSERT_TRUE(contours.front()[8].IsEqual(OrthancStone::ScenePoint2D(2, 3)));
  }

  {
    std::list<OrthancStone::Extent2D>  rectangles;
    rectangles.push_back(OrthancStone::Extent2D(2, 2, 3, 5));
    rectangles.push_back(OrthancStone::Extent2D(1, 3, 4, 4));

    std::list< std::vector<OrthancStone::ScenePoint2D> > contours;
    OrthancStone::UnionOfRectangles::Apply(contours, rectangles);
    
    ASSERT_EQ(1u, contours.size());
    ASSERT_EQ(13u, contours.front().size());
    ASSERT_TRUE(contours.front()[0].IsEqual(OrthancStone::ScenePoint2D(2, 5)));
    ASSERT_TRUE(contours.front()[1].IsEqual(OrthancStone::ScenePoint2D(3, 5)));
    ASSERT_TRUE(contours.front()[2].IsEqual(OrthancStone::ScenePoint2D(3, 4)));
    ASSERT_TRUE(contours.front()[3].IsEqual(OrthancStone::ScenePoint2D(4, 4)));
    ASSERT_TRUE(contours.front()[4].IsEqual(OrthancStone::ScenePoint2D(4, 3)));
    ASSERT_TRUE(contours.front()[5].IsEqual(OrthancStone::ScenePoint2D(3, 3)));
    ASSERT_TRUE(contours.front()[6].IsEqual(OrthancStone::ScenePoint2D(3, 2)));
    ASSERT_TRUE(contours.front()[7].IsEqual(OrthancStone::ScenePoint2D(2, 2)));
    ASSERT_TRUE(contours.front()[8].IsEqual(OrthancStone::ScenePoint2D(2, 3)));
    ASSERT_TRUE(contours.front()[9].IsEqual(OrthancStone::ScenePoint2D(1, 3)));
    ASSERT_TRUE(contours.front()[10].IsEqual(OrthancStone::ScenePoint2D(1, 4)));
    ASSERT_TRUE(contours.front()[11].IsEqual(OrthancStone::ScenePoint2D(2, 4)));
    ASSERT_TRUE(contours.front()[12].IsEqual(OrthancStone::ScenePoint2D(2, 5)));
  }
}
