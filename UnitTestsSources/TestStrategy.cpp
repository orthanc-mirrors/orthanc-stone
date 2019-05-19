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


#include "gtest/gtest.h"

#include <Core/OrthancException.h>

#include <boost/noncopyable.hpp>
#include <vector>

namespace OrthancStone
{
  class IFetchingStrategy : public boost::noncopyable
  {
  public:
    virtual ~IFetchingStrategy()
    {
    }

    virtual unsigned int GetItemsCount() const = 0;

    virtual unsigned int GetMaxQuality() const = 0;

    virtual bool GetNext(unsigned int& item,
                         unsigned int& quality) = 0;

    virtual void SetCurrent(unsigned int item) = 0;

    // Ask the strategy to re-schedule the item with the lowest
    // priority in the fetching order. This allows to know which item
    // should be dropped from a cache.
    virtual void RecycleFurthest(unsigned int& item) = 0;
  };


  class IFetchingItemsSorter : public boost::noncopyable
  {
  public:
    virtual ~IFetchingItemsSorter()
    {
    }

    virtual unsigned int GetItemsCount() const = 0;

    // Sort a set of items given the current item
    virtual void Sort(std::vector<unsigned int>& target,
                      unsigned int current) = 0;
  };



  class BasicFetchingItemsSorter : public IFetchingItemsSorter
  {
  private:
    unsigned int  itemsCount_;

  public:
    BasicFetchingItemsSorter(unsigned int itemsCount) :
      itemsCount_(itemsCount)
    {
      if (itemsCount == 0)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
    }

    virtual unsigned int GetItemsCount() const
    {
      return itemsCount_;
    }

    virtual void Sort(std::vector<unsigned int>& target,
                      unsigned int current)
    {
      if (current >= itemsCount_)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }

      target.clear();
      target.reserve(itemsCount_);
      target.push_back(current);

      const unsigned int countBelow = current;
      const unsigned int countAbove = (itemsCount_ - 1) - current;
      const unsigned int n = std::min(countBelow, countAbove);

      for (unsigned int i = 1; i <= n; i++)
      {
        assert(current + i < itemsCount_ &&
               current >= i);
        target.push_back(current + i);
        target.push_back(current - i);
      }

      for (unsigned int i = current - n; i > 0; i--)
      {
        target.push_back(i - 1);
      }

      for (unsigned int i = current + n + 1; i < itemsCount_; i++)
      {
        target.push_back(i);
      }

      assert(target.size() == itemsCount_);
    }
  };


  class BasicFetchingStrategy : public IFetchingStrategy
  {
  private:
    class ContentItem
    {
    private:
      unsigned int  item_;
      unsigned int  quality_;

    public:
      ContentItem(unsigned int item,
           unsigned int quality) :
        item_(item),
        quality_(quality)
      {
      }

      unsigned int GetItem() const
      {
        return item_;
      }

      unsigned int GetQuality() const
      {
        return quality_;
      }
    };

    std::auto_ptr<IFetchingItemsSorter>  sorter_;
    std::vector<unsigned int>            nextQuality_;
    unsigned int                         maxQuality_;
    std::vector<ContentItem>             content_;
    size_t                               position_;
    unsigned int                         blockSize_;

    void Schedule(unsigned int item,
                unsigned int quality)
    {
      assert(item < GetItemsCount() &&
             quality <= maxQuality_);
      
      if (nextQuality_[item] <= quality)
      {
        content_.push_back(ContentItem(item, quality));
      }
    }
    
  public:
    BasicFetchingStrategy(IFetchingItemsSorter* sorter,   // Takes ownership
                          unsigned int maxQuality) :
      sorter_(sorter),
      maxQuality_(maxQuality),
      position_(0),
      blockSize_(2)
    {
      if (sorter == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }

      nextQuality_.resize(sorter_->GetItemsCount(), 0),   // Does not change along calls to "SetCurrent()"
      
      SetCurrent(0);
    }

    virtual unsigned int GetItemsCount() const
    {
      return sorter_->GetItemsCount();
    }

    virtual unsigned int GetMaxQuality() const
    {
      return maxQuality_;
    }

    void SetBlockSize(unsigned int size)
    {
      if (size <= 0)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }

      blockSize_ = size;
    }

    virtual bool GetNext(unsigned int& item,
                         unsigned int& quality)
    {
      if (position_ >= content_.size())
      {
        return false;
      }
      else
      {
        item = content_[position_].GetItem();       
        quality = content_[position_].GetQuality();

        assert(nextQuality_[item] <= quality);
        nextQuality_[item] = quality + 1;

        position_ ++;
        return true;
      }
    }
    
    virtual void SetCurrent(unsigned int item)
    {
      // TODO - This function is O(N) complexity where "N" is the
      // number of items times the max quality. Could use a LRU index.

      position_ = 0;
      
      std::vector<unsigned int> v;
      sorter_->Sort(v, item);

      assert(v.size() == GetItemsCount());

      if (v.size() == 0)
      {
        return;
      }
      
      content_.clear();
      content_.reserve(v.size() * maxQuality_);

      Schedule(v.front(), maxQuality_);

      for (unsigned int q = 0; q <= maxQuality_; q++)
      {
        unsigned int start = 1 + q * blockSize_;
        unsigned int end = start + blockSize_;

        if (q == maxQuality_ ||
            end > v.size())
        {
          end = v.size();
        }

        unsigned int a = 0;
        if (maxQuality_ >= q + 1)
        {
          a = maxQuality_ - q - 1;
        }
        
        for (unsigned int j = a; j <= maxQuality_; j++)
        {
          for (unsigned int i = start; i < end; i++)
          {
            Schedule(v[i], j);
          }
        }
      }
    }

    // Ask the strategy to re-schedule the item with the lowest
    // priority in the fetching order. This allows to know which item
    // should be dropped from a cache.
    virtual void RecycleFurthest(unsigned int& item)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
    }
  };
}



namespace
{
  class StrategyTester : public boost::noncopyable
  {
  private:
    std::map<unsigned int, unsigned int>  qualities_;

  public:
    bool IsValidCommand(unsigned int item,
                        unsigned int quality)
    {
      if (qualities_.find(item) != qualities_.end() &&
          qualities_[item] >= quality)
      {
        return false;
      }
      else
      {
        qualities_[item] = quality;
        return true;
      }
    }

    bool HasFinished(OrthancStone::BasicFetchingStrategy& strategy)
    {
      for (unsigned int i = 0; i < strategy.GetItemsCount(); i++)
      {
        if (qualities_.find(i) == qualities_.end() ||
            qualities_[i] != strategy.GetMaxQuality())
        {
          return false;
        }
      }

      return true;
    }
  };
}


TEST(BasicFetchingStrategy, Test1)
{
  ASSERT_THROW(OrthancStone::BasicFetchingStrategy(NULL, 0), Orthanc::OrthancException);
  ASSERT_THROW(OrthancStone::BasicFetchingStrategy(new OrthancStone::BasicFetchingItemsSorter(0), 0), Orthanc::OrthancException);

  {
    OrthancStone::BasicFetchingStrategy s(new OrthancStone::BasicFetchingItemsSorter(1), 0);
    unsigned int i, q;
    ASSERT_TRUE(s.GetNext(i, q));   ASSERT_EQ(0u, i);  ASSERT_EQ(0u, q);
    ASSERT_FALSE(s.GetNext(i, q));
  }

  {
    OrthancStone::BasicFetchingStrategy s(new OrthancStone::BasicFetchingItemsSorter(1), 5);
    unsigned int i, q;
    ASSERT_TRUE(s.GetNext(i, q));   ASSERT_EQ(0u, i);  ASSERT_EQ(5u, q);
    ASSERT_FALSE(s.GetNext(i, q));
  }

  {
    OrthancStone::BasicFetchingStrategy s(new OrthancStone::BasicFetchingItemsSorter(2), 2);
    unsigned int i, q;
    ASSERT_TRUE(s.GetNext(i, q));   ASSERT_EQ(0u, i);  ASSERT_EQ(2u, q);
    ASSERT_TRUE(s.GetNext(i, q));   ASSERT_EQ(1u, i);  ASSERT_EQ(1u, q);
    ASSERT_TRUE(s.GetNext(i, q));   ASSERT_EQ(1u, i);  ASSERT_EQ(2u, q);
    ASSERT_FALSE(s.GetNext(i, q));
  }

  {
    OrthancStone::BasicFetchingStrategy s(new OrthancStone::BasicFetchingItemsSorter(3), 2);
    unsigned int i, q;
    ASSERT_TRUE(s.GetNext(i, q));   ASSERT_EQ(0u, i);  ASSERT_EQ(2u, q);
    ASSERT_TRUE(s.GetNext(i, q));   ASSERT_EQ(1u, i);  ASSERT_EQ(1u, q);
    ASSERT_TRUE(s.GetNext(i, q));   ASSERT_EQ(2u, i);  ASSERT_EQ(1u, q);
    ASSERT_TRUE(s.GetNext(i, q));   ASSERT_EQ(1u, i);  ASSERT_EQ(2u, q);
    ASSERT_TRUE(s.GetNext(i, q));   ASSERT_EQ(2u, i);  ASSERT_EQ(2u, q);
    ASSERT_FALSE(s.GetNext(i, q));
  }

  {
    OrthancStone::BasicFetchingStrategy s(new OrthancStone::BasicFetchingItemsSorter(3), 2);
    s.SetBlockSize(1);
    s.SetCurrent(0);
    unsigned int i, q;
    ASSERT_TRUE(s.GetNext(i, q));   ASSERT_EQ(0u, i);  ASSERT_EQ(2u, q);
    ASSERT_TRUE(s.GetNext(i, q));   ASSERT_EQ(1u, i);  ASSERT_EQ(1u, q);
    ASSERT_TRUE(s.GetNext(i, q));   ASSERT_EQ(1u, i);  ASSERT_EQ(2u, q);
    ASSERT_TRUE(s.GetNext(i, q));   ASSERT_EQ(2u, i);  ASSERT_EQ(0u, q);
    ASSERT_TRUE(s.GetNext(i, q));   ASSERT_EQ(2u, i);  ASSERT_EQ(1u, q);
    ASSERT_TRUE(s.GetNext(i, q));   ASSERT_EQ(2u, i);  ASSERT_EQ(2u, q);
    ASSERT_FALSE(s.GetNext(i, q));
  }

  {
    OrthancStone::BasicFetchingStrategy s(new OrthancStone::BasicFetchingItemsSorter(5), 0);
    ASSERT_THROW(s.SetCurrent(5), Orthanc::OrthancException);
    s.SetCurrent(2);

    unsigned int i, q;
    ASSERT_TRUE(s.GetNext(i, q));   ASSERT_EQ(2u, i);  ASSERT_EQ(0u, q);
    ASSERT_TRUE(s.GetNext(i, q));   ASSERT_EQ(3u, i);  ASSERT_EQ(0u, q);
    ASSERT_TRUE(s.GetNext(i, q));   ASSERT_EQ(1u, i);  ASSERT_EQ(0u, q);
    ASSERT_TRUE(s.GetNext(i, q));   ASSERT_EQ(4u, i);  ASSERT_EQ(0u, q);
    ASSERT_TRUE(s.GetNext(i, q));   ASSERT_EQ(0u, i);  ASSERT_EQ(0u, q);
    ASSERT_FALSE(s.GetNext(i, q));
  }

  {
    OrthancStone::BasicFetchingStrategy s(new OrthancStone::BasicFetchingItemsSorter(5), 0);
    s.SetCurrent(4);

    unsigned int i, q;
    ASSERT_TRUE(s.GetNext(i, q));   ASSERT_EQ(4u, i);  ASSERT_EQ(0u, q);
    ASSERT_TRUE(s.GetNext(i, q));   ASSERT_EQ(3u, i);  ASSERT_EQ(0u, q);
    ASSERT_TRUE(s.GetNext(i, q));   ASSERT_EQ(2u, i);  ASSERT_EQ(0u, q);
    ASSERT_TRUE(s.GetNext(i, q));   ASSERT_EQ(1u, i);  ASSERT_EQ(0u, q);
    ASSERT_TRUE(s.GetNext(i, q));   ASSERT_EQ(0u, i);  ASSERT_EQ(0u, q);
    ASSERT_FALSE(s.GetNext(i, q));
  }
}


TEST(BasicFetchingStrategy, Test2)
{
  OrthancStone::BasicFetchingStrategy s(new OrthancStone::BasicFetchingItemsSorter(20), 2);
  ASSERT_EQ(20u, s.GetItemsCount());
  ASSERT_EQ(2u, s.GetMaxQuality());

  StrategyTester t;

  s.SetCurrent(10);

  unsigned int i, q;
  while (s.GetNext(i, q))
  {
    ASSERT_TRUE(t.IsValidCommand(i, q));
  }

  ASSERT_TRUE(t.HasFinished(s));
}




TEST(BasicFetchingItemsSorter, Small)
{
  ASSERT_THROW(OrthancStone::BasicFetchingItemsSorter(0), Orthanc::OrthancException);
  std::vector<unsigned int> v;

  {
    OrthancStone::BasicFetchingItemsSorter s(1);
    s.Sort(v, 0);
    ASSERT_EQ(1u, v.size());
    ASSERT_EQ(0u, v[0]);

    ASSERT_THROW(s.Sort(v, 1), Orthanc::OrthancException);
  }

  {
    OrthancStone::BasicFetchingItemsSorter s(2);
    s.Sort(v, 0);
    ASSERT_EQ(2u, v.size());
    ASSERT_EQ(0u, v[0]);
    ASSERT_EQ(1u, v[1]);

    s.Sort(v, 1);
    ASSERT_EQ(2u, v.size());
    ASSERT_EQ(1u, v[0]);
    ASSERT_EQ(0u, v[1]);

    ASSERT_THROW(s.Sort(v, 2), Orthanc::OrthancException);
  }

  {
    OrthancStone::BasicFetchingItemsSorter s(3);
    s.Sort(v, 0);
    ASSERT_EQ(3u, v.size());
    ASSERT_EQ(0u, v[0]);
    ASSERT_EQ(1u, v[1]);
    ASSERT_EQ(2u, v[2]);
    
    s.Sort(v, 1);
    ASSERT_EQ(3u, v.size());
    ASSERT_EQ(1u, v[0]);
    ASSERT_EQ(2u, v[1]);
    ASSERT_EQ(0u, v[2]);
    
    s.Sort(v, 2);
    ASSERT_EQ(3u, v.size());
    ASSERT_EQ(2u, v[0]);
    ASSERT_EQ(1u, v[1]);
    ASSERT_EQ(0u, v[2]);
    
    ASSERT_THROW(s.Sort(v, 3), Orthanc::OrthancException);
  }
}


TEST(BasicFetchingItemsSorter, Odd)
{
  OrthancStone::BasicFetchingItemsSorter s(7);
  std::vector<unsigned int> v;

  ASSERT_THROW(s.Sort(v, 7), Orthanc::OrthancException);

  {
    s.Sort(v, 0);
    ASSERT_EQ(7u, v.size());
    ASSERT_EQ(0u, v[0]);
    ASSERT_EQ(1u, v[1]);
    ASSERT_EQ(2u, v[2]);
    ASSERT_EQ(3u, v[3]);
    ASSERT_EQ(4u, v[4]);
    ASSERT_EQ(5u, v[5]);
    ASSERT_EQ(6u, v[6]);
  }

  {
    s.Sort(v, 1);
    ASSERT_EQ(7u, v.size());
    ASSERT_EQ(1u, v[0]);
    ASSERT_EQ(2u, v[1]);
    ASSERT_EQ(0u, v[2]);
    ASSERT_EQ(3u, v[3]);
    ASSERT_EQ(4u, v[4]);
    ASSERT_EQ(5u, v[5]);
    ASSERT_EQ(6u, v[6]);
  }

  {
    s.Sort(v, 2);
    ASSERT_EQ(7u, v.size());
    ASSERT_EQ(2u, v[0]);
    ASSERT_EQ(3u, v[1]);
    ASSERT_EQ(1u, v[2]);
    ASSERT_EQ(4u, v[3]);
    ASSERT_EQ(0u, v[4]);
    ASSERT_EQ(5u, v[5]);
    ASSERT_EQ(6u, v[6]);
  }

  {
    s.Sort(v, 3);
    ASSERT_EQ(7u, v.size());
    ASSERT_EQ(3u, v[0]);
    ASSERT_EQ(4u, v[1]);
    ASSERT_EQ(2u, v[2]);
    ASSERT_EQ(5u, v[3]);
    ASSERT_EQ(1u, v[4]);
    ASSERT_EQ(6u, v[5]);
    ASSERT_EQ(0u, v[6]);
  }

  {
    s.Sort(v, 4);
    ASSERT_EQ(7u, v.size());
    ASSERT_EQ(4u, v[0]);
    ASSERT_EQ(5u, v[1]);
    ASSERT_EQ(3u, v[2]);
    ASSERT_EQ(6u, v[3]);
    ASSERT_EQ(2u, v[4]);
    ASSERT_EQ(1u, v[5]);
    ASSERT_EQ(0u, v[6]);
  }

  {
    s.Sort(v, 5);
    ASSERT_EQ(7u, v.size());
    ASSERT_EQ(5u, v[0]);
    ASSERT_EQ(6u, v[1]);
    ASSERT_EQ(4u, v[2]);
    ASSERT_EQ(3u, v[3]);
    ASSERT_EQ(2u, v[4]);
    ASSERT_EQ(1u, v[5]);
    ASSERT_EQ(0u, v[6]);
  }

  {
    s.Sort(v, 6);
    ASSERT_EQ(7u, v.size());
    ASSERT_EQ(6u, v[0]);
    ASSERT_EQ(5u, v[1]);
    ASSERT_EQ(4u, v[2]);
    ASSERT_EQ(3u, v[3]);
    ASSERT_EQ(2u, v[4]);
    ASSERT_EQ(1u, v[5]);
    ASSERT_EQ(0u, v[6]);
  }
}


TEST(BasicFetchingItemsSorter, Even)
{
  OrthancStone::BasicFetchingItemsSorter s(6);
  std::vector<unsigned int> v;

  {
    s.Sort(v, 0);
    ASSERT_EQ(6u, v.size());
    ASSERT_EQ(0u, v[0]);
    ASSERT_EQ(1u, v[1]);
    ASSERT_EQ(2u, v[2]);
    ASSERT_EQ(3u, v[3]);
    ASSERT_EQ(4u, v[4]);
    ASSERT_EQ(5u, v[5]);
  }

  {
    s.Sort(v, 1);
    ASSERT_EQ(6u, v.size());
    ASSERT_EQ(1u, v[0]);
    ASSERT_EQ(2u, v[1]);
    ASSERT_EQ(0u, v[2]);
    ASSERT_EQ(3u, v[3]);
    ASSERT_EQ(4u, v[4]);
    ASSERT_EQ(5u, v[5]);
  }

  {
    s.Sort(v, 2);
    ASSERT_EQ(6u, v.size());
    ASSERT_EQ(2u, v[0]);
    ASSERT_EQ(3u, v[1]);
    ASSERT_EQ(1u, v[2]);
    ASSERT_EQ(4u, v[3]);
    ASSERT_EQ(0u, v[4]);
    ASSERT_EQ(5u, v[5]);
  }

  {
    s.Sort(v, 3);
    ASSERT_EQ(6u, v.size());
    ASSERT_EQ(3u, v[0]);
    ASSERT_EQ(4u, v[1]);
    ASSERT_EQ(2u, v[2]);
    ASSERT_EQ(5u, v[3]);
    ASSERT_EQ(1u, v[4]);
    ASSERT_EQ(0u, v[5]);
  }

  {
    s.Sort(v, 4);
    ASSERT_EQ(6u, v.size());
    ASSERT_EQ(4u, v[0]);
    ASSERT_EQ(5u, v[1]);
    ASSERT_EQ(3u, v[2]);
    ASSERT_EQ(2u, v[3]);
    ASSERT_EQ(1u, v[4]);
    ASSERT_EQ(0u, v[5]);
  }

  {
    s.Sort(v, 5);
    ASSERT_EQ(6u, v.size());
    ASSERT_EQ(5u, v[0]);
    ASSERT_EQ(4u, v[1]);
    ASSERT_EQ(3u, v[2]);
    ASSERT_EQ(2u, v[3]);
    ASSERT_EQ(1u, v[4]);
    ASSERT_EQ(0u, v[5]);
  }
}
