/* This file is part of the Tomographer project, which is distributed under the
 * terms of the MIT license.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 ETH Zurich, Institute for Theoretical Physics, Philippe Faist
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _TOMOGRAPHER_DENSEDM_INDEPMEASLLH_H
#define _TOMOGRAPHER_DENSEDM_INDEPMEASLLH_H

#include <cstddef>
#include <cassert>

#include <Eigen/Eigen>

#include <tomographer2/tools/cxxutil.h> // static_or_dynamic, TOMOGRAPHER_ENABLED_IF
#include <tomographer2/densedm/dmtypes.h>

/** \file indepmeasllh.h
 * \brief C++ types and functions for calculating the log-likelihood for independent measurements
 *
 */


namespace Tomographer {
namespace DenseDM {


/** \brief C++ types and functions for calculating the log-likelihood for independent measurements
 *
 */
template<typename DMTypes_, typename LLHValueType_ = typename DMTypes_::RealScalar,
         typename IntFreqType_ = int, int FixedMaxParamList_ = Eigen::Dynamic>
struct IndepMeasLLH
{
  //! The \ref DMTypes in use here
  typedef DMTypes_ DMTypes;
  //! Type used to calculate the log-likelihood function
  typedef LLHValueType_ LLHValueType;
  //! Type used to store integer measurement counts
  typedef IntFreqType_ IntFreqType;
  //! Maximum number of POVM effects, fixed at compile-time or \ref Eigen::Dynamic
  static constexpr int FixedMaxParamList = FixedMaxParamList_;
  //! Whether the dimension is specified dynamically at run-time or statically at compile-time
  static constexpr bool IsDynamicMaxParamList = (FixedMaxParamList_ == Eigen::Dynamic);


  /** \brief dynamic Matrix with rows = dim*dim Vectors (row-major)
   * [maximum FixedMaxParamList rows, or Dynamic]
   */
  typedef Eigen::Matrix<typename DMTypes::RealScalar, Eigen::Dynamic, DMTypes::FixedDim2,
                        Eigen::RowMajor, FixedMaxParamList, DMTypes::FixedDim2>  VectorParamListType;
  //! Const ref to a VectorParamListType
  typedef const Eigen::Ref<const VectorParamListType> & VectorParamListTypeConstRef;

  /** \brief dynamic Array of integers [maximum FixedMaxParamList entries or Dynamic]
   */
  typedef Eigen::Array<IntFreqType, Eigen::Dynamic, 1,
                       0 /*Options*/, FixedMaxParamList, 1>  FreqListType;
  //! Const ref to a FreqListType
  typedef const Eigen::Ref<const FreqListType> & FreqListTypeConstRef;

  
  /** \brief Constructor [only for statically-fixed dim]
   *
   */
  inline IndepMeasLLH(DMTypes dmt_)
    : dmt(dmt_), Exn(VectorParamListType::Zero(0, dmt.dim2())), Nx(FreqListType::Zero(0)),
      NMeasAmplifyFactor(1)
  {
  }

  /** \brief Constructor [only for statically-fixed dim]
   *
   */
  inline IndepMeasLLH(DMTypes dmt_, VectorParamListTypeConstRef Exn_, FreqListTypeConstRef Nx_)
    : dmt(dmt_), Exn(Exn_), Nx(Nx_), NMeasAmplifyFactor(1)
  {
  }

  //! The \ref DMTypes object, storing e.g. the dimension of the problem.
  const DMTypes dmt;

  VectorParamListType Exn;
  FreqListType Nx;

  inline void initMeasVector(std::size_t len)
  {
    Exn.resize(len, dmt.dim2());
    Exn.setZero();
    Nx.resize(len);
    Nx.setZero();
  }

  LLHValueType NMeasAmplifyFactor;


  /** \brief Calculates the log-likelihood function
   *
   * \returns the value of the -2-log-likelihood function of this data at the point \a x,
   * defined as \f[
   *    \lambda(\texttt{x}) = -2\sum_k \texttt{Nx[k]}\,\ln\mathrm{tr}(\texttt{Exn[k]}\,\rho(\texttt{x})) .
   * \f]
   */
  inline LLHValueType calc_llh(typename DMTypes::VectorParamTypeConstRef x) const
  {
    return -2 * NMeasAmplifyFactor * (
        Nx.template cast<LLHValueType>() * (Exn * x).template cast<LLHValueType>().array().log()
        ).sum();
  }

};





} // namespace DenseDM
} // namespace Tomographer








#endif
