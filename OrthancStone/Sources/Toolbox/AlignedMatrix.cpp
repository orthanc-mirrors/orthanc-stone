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


#include "AlignedMatrix.h"

#include <OrthancException.h>

#include <string.h>

namespace OrthancStone
{
  static unsigned int Ceiling(unsigned int a,
                              unsigned int b)
  {
    if (a % b == 0)
    {
      return a / b;
    }
    else
    {
      return a / b + 1;
    }
  }


  void AlignedMatrix::Setup(unsigned int rows,
                            unsigned int cols)
  {
    assert(sizeof(float) == 4);
    
    if (rows == 0 ||
        cols == 0)
    {
      rows_ = 0;
      cols_ = 0;
      pitch_ = 0;
      pitchFloatPointer_ = 0;
      content_ = NULL;
    }
    else
    {
      rows_ = rows;
      cols_ = cols;
      pitch_ = Ceiling(cols * sizeof(float), ORTHANC_MEMORY_ALIGNMENT) * ORTHANC_MEMORY_ALIGNMENT;
      pitchFloatPointer_ = pitch_ / sizeof(float);
      
      void* tmp = NULL;
      if (posix_memalign(&tmp, ORTHANC_MEMORY_ALIGNMENT, rows_ * pitch_) != 0)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotEnoughMemory);
      }

      assert(reinterpret_cast<intptr_t>(tmp) % ORTHANC_MEMORY_ALIGNMENT == 0);
      assert(pitch_ % ORTHANC_MEMORY_ALIGNMENT == 0);
      assert(pitch_ % sizeof(float) == 0);
      assert((rows_ * pitch_) % ORTHANC_MEMORY_ALIGNMENT == 0);
      
      content_ = static_cast<float*>(tmp);
    }
  }


  AlignedMatrix::~AlignedMatrix()
  {
    if (content_ != NULL)
    {
      free(content_);
    }
  }

  
  void AlignedMatrix::FillZeros()
  {
    memset(content_, 0, rows_ * pitch_);
  }


  void AlignedMatrix::ProductPlain(AlignedMatrix& c,
                                   const AlignedMatrix& a,
                                   const AlignedMatrix& b)
  {
    if (c.GetRows() != a.GetRows() ||
        c.GetColumns() != b.GetColumns() ||
        a.GetColumns() != b.GetRows())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageSize);
    }
  
    const unsigned int M = c.GetRows();
    const unsigned int N = c.GetColumns();
    const unsigned int K = a.GetColumns();

    c.FillZeros();
  
    for (unsigned int i = 0; i < M; i++)
    {
      // Loop over "k" to be more cache-friendly
      // https://sahnimanas.github.io/post/anatomy-of-a-high-performance-convolution/
      for (unsigned int k = 0; k < K; k++)
      {
        for (unsigned int j = 0; j < N; j++)
        {
          c.AddValue(i, j, a.GetValue(i, k) * b.GetValue(k, j));
        }
      }
    }
  }


#if ORTHANC_HAS_MATRIX_PRODUCT_TRANSPOSED_VECTORIZED == 1
  // Computes "C = A*B^T"
  class AlignedMatrix::ProductTransposedVectorizedContext : public boost::noncopyable
  {
  private:
    unsigned int vectorizedSteps_;
    uint8_t      finalSteps_;
  
  public:
    ORTHANC_FORCE_INLINE
    ProductTransposedVectorizedContext(const AlignedMatrix& a)
    {
#if ORTHANC_HAS_AVX2 == 1
      const unsigned int blockSize = 8;
#elif ORTHANC_HAS_SSE2 == 1 || ORTHANC_HAS_WASM_SIMD == 1
      const unsigned int blockSize = 4;
#else
#       error No supported SIMD instruction set
#endif
        
      vectorizedSteps_ = a.GetColumns() / blockSize;
      finalSteps_ = a.GetColumns() - vectorizedSteps_ * blockSize;
    }

    ORTHANC_FORCE_INLINE
    float Apply(const float* ap,
                const float* btp) const noexcept
    {
      float result;
        
#if ORTHANC_HAS_AVX2 == 1
      __m256 accumulator = _mm256_set1_ps(0);

      for (unsigned int k = 0; k < vectorizedSteps_; k++)
      {
        __m256 a = _mm256_load_ps(ap);
        __m256 b = _mm256_load_ps(btp);
        //accumulator = _mm256_add_ps(accumulator, _mm256_mul_ps(a, b));
        accumulator = _mm256_fmadd_ps(a, b, accumulator);  // Requires the "-mfma" compiler flag

        ap += 8;
        btp += 8;
      }
        
      float tmp[8] __attribute__ ((aligned (ORTHANC_MEMORY_ALIGNMENT)));
      _mm256_store_ps(tmp, accumulator);
      result = tmp[0] + tmp[1] + tmp[2] + tmp[3] + tmp[4] + tmp[5] + tmp[6] + tmp[7];

#elif ORTHANC_HAS_SSE2 == 1
      __m128 accumulator = _mm_set1_ps(0);

      for (unsigned int k = 0; k < vectorizedSteps_; k++)
      {
        __m128 a = _mm_load_ps(ap);
        __m128 b = _mm_load_ps(btp);
        accumulator = _mm_add_ps(accumulator, _mm_mul_ps(a, b));
        ap += 4;
        btp += 4;
      }

#if 1
      float tmp[4] __attribute__ ((aligned (ORTHANC_MEMORY_ALIGNMENT)));
      _mm_storeu_ps(tmp, accumulator);
      result = tmp[0] + tmp[1] + tmp[2] + tmp[3];
#else
      // This trickier version is theoretically faster, but no much difference in practice
      const __m128 sum2 = _mm_add_ps(accumulator, _mm_shuffle_ps(accumulator, accumulator, _MM_SHUFFLE(2, 3, 0, 1)));
      const __m128 sum1 = _mm_add_ps(sum2, _mm_shuffle_ps(sum2, sum2, _MM_SHUFFLE(0, 1, 2, 3)));
      result = _mm_cvtss_f32(sum1);
#endif

#elif ORTHANC_HAS_WASM_SIMD == 1
      v128_t accumulator = wasm_f32x4_splat(0);

      for (unsigned int k = 0; k < vectorizedSteps_; k++)
      {
        v128_t a = wasm_v128_load(ap);
        v128_t b = wasm_v128_load(btp);
        accumulator = wasm_f32x4_add(accumulator, wasm_f32x4_mul(a, b));
        ap += 4;
        btp += 4;
      }        
        
#if 1
      float tmp[4];
      wasm_v128_store(tmp, accumulator);
      result = tmp[0] + tmp[1] + tmp[2] + tmp[3];
#else
      const v128_t sum2 = wasm_f32x4_add(accumulator, wasm_i32x4_shuffle(accumulator, accumulator, 2, 3, 0, 0));
      const v128_t sum1 = wasm_f32x4_add(sum2, wasm_i32x4_shuffle(sum2, sum2, 1, 0, 0, 0));
      result = wasm_f32x4_extract_lane(sum1, 0);
#endif

#else
#       error No supported SIMD instruction set
#endif
        
      for (uint8_t k = 0; k < finalSteps_; k++)
      {
        result += (*ap) * (*btp);
        ap++;
        btp++;
      }
        
      return result;
    }
  };
#endif
    

#if ORTHANC_HAS_MATRIX_PRODUCT_TRANSPOSED_VECTORIZED == 1
  void AlignedMatrix::ProductTransposedVectorized(AlignedMatrix& c,
                                                  const AlignedMatrix& a,
                                                  const AlignedMatrix& bt)
  {
    if (c.GetRows() != a.GetRows() ||
        c.GetColumns() != bt.GetRows() ||
        a.GetColumns() != bt.GetColumns())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageSize);
    }

    AlignedMatrix::ProductTransposedVectorizedContext context(a);

    const unsigned int M = a.GetRows();
    const unsigned int N = bt.GetRows();

    const size_t rowSizeA = a.GetPitch() / sizeof(float);
    const size_t rowSizeB = bt.GetPitch() / sizeof(float);
  
    const float* ap = a.GetRowPointer(0);
    for (unsigned int i = 0; i < M; i++)
    {
      float* cp = c.GetRowPointer(i);
    
      const float* btp = bt.GetRowPointer(0);
      for (unsigned int j = 0; j < N; j++, cp++)
      {
        *cp = context.Apply(ap, btp);
        btp += rowSizeB;
      }

      ap += rowSizeA;
    }
  }
#endif
}
