/* This file is part of the Tomographer project, which is distributed under the
 * terms of the MIT license.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 ETH Zurich, Institute for Theoretical Physics, Philippe Faist
 * Copyright (c) 2017 Caltech, Institute for Quantum Information and Matter, Philippe Faist
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


#ifndef TOMOGRAPHER_DENSEDM_TSPACEFIGOFMERIT_H
#define TOMOGRAPHER_DENSEDM_TSPACEFIGOFMERIT_H


#include <tomographer/tools/needownoperatornew.h>
#include <tomographer/densedm/dmtypes.h>
#include <tomographer/densedm/distmeasures.h>
#include <tomographer/densedm/param_herm_x.h>


/** \file tspacefigofmerit.h
 *
 * \brief Implement some standard figures of merit for when integrating in T space densely
 *        represented density matrices
 */


namespace Tomographer {
namespace DenseDM {
namespace TSpace {


/** \brief Calculate the fidelity to a reference state for each sample
 *
 * This calculates the "root" fidelity as per Nielsen & Chuang.
 *
 * \see fidelityT(rho,sigma)
 */
template<typename DMTypes_, typename ValueType_ = double>
TOMOGRAPHER_EXPORT class FidelityToRefCalculator
  : public virtual Tools::NeedOwnOperatorNew<typename DMTypes_::MatrixType>::ProviderType
{
public:
  typedef DMTypes_ DMTypes;
  typedef typename DMTypes::MatrixType MatrixType;
  typedef typename DMTypes::MatrixTypeConstRef MatrixTypeConstRef;

  //! For ValueCalculator interface : value type
  typedef ValueType_ ValueType;

private:
  MatrixType _ref_T;

public:
  //! Constructor, the reference state is T_ref (in \ref pageParamsT)
  FidelityToRefCalculator(MatrixTypeConstRef T_ref)
    : _ref_T(T_ref)
  {
  }

  //! Calculate the fidelity of the state represented by T to the reference state
  inline ValueType getValue(MatrixTypeConstRef T) const
  {
    return fidelityT<ValueType>(T, _ref_T);
  }
};




/** \brief Calculate the "purified distance" to a reference state for each sample
 *
 * The purified distance \f$ P(\rho,\sigma) \f$ for two normalized states is defined as
 * \f[
 *   P\left(\rho,\sigma\right) = \sqrt{1 - F^2\left(\rho,\sigma\right)}\ .
 * \f]
 */
template<typename DMTypes_, typename ValueType_ = double>
TOMOGRAPHER_EXPORT class PurifDistToRefCalculator
  : public virtual Tools::NeedOwnOperatorNew<typename DMTypes_::MatrixType>::ProviderType
{
public:
  typedef DMTypes_ DMTypes;
  typedef typename DMTypes::MatrixType MatrixType;
  typedef typename DMTypes::MatrixTypeConstRef MatrixTypeConstRef;

  //! For ValueCalculator interface : value type
  typedef ValueType_ ValueType;

private:
  MatrixType _ref_T;

public:
  //! Constructor, the reference state is T_ref (in \ref pageParamsT)
  PurifDistToRefCalculator(MatrixTypeConstRef T_ref)
    : _ref_T(T_ref)
  {
  }

  //! Calculate the purified distance of the state represented by T to the reference state
  inline ValueType getValue(MatrixTypeConstRef T) const
  {
    ValueType F = fidelityT<ValueType>(T, _ref_T);
    if (F >= ValueType(1)) {
      return 0;
    }
    return std::sqrt(ValueType(1) - F*F);
  }
};

/** \brief Calculate the trace distance to a reference state for each sample
 *
 */
template<typename DMTypes_, typename ValueType_ = double>
TOMOGRAPHER_EXPORT class TrDistToRefCalculator
  : public virtual Tools::NeedOwnOperatorNew<typename DMTypes_::MatrixType>::ProviderType
{
public:
  typedef DMTypes_ DMTypes;
  typedef typename DMTypes::MatrixType MatrixType;
  typedef typename DMTypes::MatrixTypeConstRef MatrixTypeConstRef;

  //! For ValueCalculator interface : value type
  typedef ValueType_ ValueType;

private:
  MatrixType _ref_rho;

public:
  //! Constructor, the reference state is \a rho_ref
  TrDistToRefCalculator(MatrixTypeConstRef rho_ref)
    : _ref_rho(rho_ref)
  {
  }

  //! Calculate the trace distance of the state represented by T to the reference state
  inline ValueType getValue(MatrixTypeConstRef T) const
  {
    return traceDistance<ValueType>(T*T.adjoint(), _ref_rho);
  }
};



/** \brief Calculate expectation value of an observable for each sample
 *
 */
template<typename DMTypes_>
TOMOGRAPHER_EXPORT class ObservableValueCalculator
  : public virtual Tools::NeedOwnOperatorNew<typename DMTypes_::VectorParamType>::ProviderType
{
public:
  typedef DMTypes_ DMTypes;
  typedef typename DMTypes::MatrixType MatrixType;
  typedef typename DMTypes::MatrixTypeConstRef MatrixTypeConstRef;
  typedef typename DMTypes::VectorParamType VectorParamType;
  typedef typename DMTypes::VectorParamTypeConstRef VectorParamTypeConstRef;

  //! For ValueCalculator interface : value type
  typedef typename DMTypes::RealScalar ValueType;

private:
  //! The parametrization object, allowing us to convert rho to its \ref pageParamsX
  ParamX<DMTypes> _param_x;

  //! The observable we wish to watch the expectation value with (in \ref pageParamsX)
  VectorParamType _A_x;

public:
  //! Constructor directly accepting \a A as a hermitian matrix
  ObservableValueCalculator(DMTypes dmt, MatrixTypeConstRef A)
    : _param_x(dmt), _A_x(_param_x.HermToX(A))
  {
  }
  //! Constructor directly accepting the X parameterization of \a A
  ObservableValueCalculator(DMTypes dmt, VectorParamTypeConstRef A_x)
    : _param_x(dmt), _A_x(A_x)
  {
  }

  //! Calculate the expectation value of the observable for the state represented by T
  inline ValueType getValue(MatrixTypeConstRef T) const
  {
    return _A_x.transpose() * _param_x.HermToX(T*T.adjoint());
  }
};


} // namespace TSpace
} // namespace DenseDM
} // namespace Tomographer


#endif
