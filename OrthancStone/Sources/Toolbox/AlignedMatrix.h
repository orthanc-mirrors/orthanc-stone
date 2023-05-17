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


#pragma once

#include "SimdIncludes.h"

#if (ORTHANC_HAS_AVX2 == 1 || ORTHANC_HAS_SSE2 == 1 || ORTHANC_HAS_WASM_SIMD == 1)
#  define ORTHANC_HAS_MATRIX_PRODUCT_TRANSPOSED_VECTORIZED  1
#else
#  define ORTHANC_HAS_MATRIX_PRODUCT_TRANSPOSED_VECTORIZED  0
#endif


#include <boost/noncopyable.hpp>
#include <cassert>

namespace OrthancStone
{
  /**
   * 2D matrix whose rows are aligned for the largest SIMD
   * instructions that are available.
   **/
  class AlignedMatrix : public boost::noncopyable
  {
  private:
    class ProductTransposedVectorizedContext;
      
    unsigned int  rows_;
    unsigned int  cols_;
    size_t        pitch_;
    size_t        pitchFloatPointer_;
    float*        content_;

    void Setup(unsigned int rows,
               unsigned int cols);

  public:
    AlignedMatrix(unsigned int rows,
                  unsigned int cols)
    {
      Setup(rows, cols);
    }

    ~AlignedMatrix();

    unsigned int GetRows() const
    {
      return rows_;
    }

    unsigned int GetColumns() const
    {
      return cols_;
    }

    unsigned int GetPitch() const
    {
      return pitch_;
    }

    float* GetRowPointer(unsigned int row)
    {
      assert(row < rows_);
      return content_ + row * pitchFloatPointer_;
    }

    const float* GetRowPointer(unsigned int row) const
    {
      assert(row < rows_);
      return content_ + row * pitchFloatPointer_;
    }

    size_t GetIndex(unsigned int row,
                    unsigned int col) const
    {
      assert(row < rows_ && col < cols_);
      return row * pitchFloatPointer_ + col;
    }

    float GetValue(unsigned int row,
                   unsigned int col) const
    {
      return content_[GetIndex(row, col)];
    }

    void SetValue(unsigned int row,
                  unsigned int col,
                  float value) const
    {
      content_[GetIndex(row, col)] = value;
    }

    void AddValue(unsigned int row,
                  unsigned int col,
                  float value)
    {
      content_[GetIndex(row, col)] += value;
    }

    void FillZeros();

    // Computes "C = A * B" without SIMD operations
    static void ProductPlain(AlignedMatrix& c,
                             const AlignedMatrix& a,
                             const AlignedMatrix& b);

#if ORTHANC_HAS_MATRIX_PRODUCT_TRANSPOSED_VECTORIZED == 1
    // Computes "C = A * B^T" using SIMD operations
    static void ProductTransposedVectorized(AlignedMatrix& c,
                                            const AlignedMatrix& a,
                                            const AlignedMatrix& bt);
#endif
  };
}
