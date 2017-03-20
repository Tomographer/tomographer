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

#ifndef TOMOGRAPHER_DENSEDM_INDEPMEASLLH_H
#define TOMOGRAPHER_DENSEDM_INDEPMEASLLH_H

#include <cstddef>
#include <string>
#include <iomanip> // std::setprecision, std::setw and friends.

#include <Eigen/Eigen>

#include <tomographer/tools/cxxutil.h> // StaticOrDynamic, TOMOGRAPHER_ENABLED_IF
#include <tomographer/tools/needownoperatornew.h>
#include <tomographer/tools/fmt.h> // streamstr
#include <tomographer/densedm/dmtypes.h>
#include <tomographer/densedm/param_herm_x.h>
#include <tomographer/densedm/densellh.h>

/** \file indepmeasllh.h
 * \brief C++ types and functions for calculating the log-likelihood for independent measurements
 *
 */


namespace Tomographer {
namespace DenseDM {




/** \brief C++ types and functions for calculating the log-likelihood for POVM effects
 * which can be written as a product of individual effects
 *
 * Implements the \ref pageInterfaceDenseLLH.
 */
template<typename DMTypes_, typename LLHValueType_ = typename DMTypes_::RealScalar,
         typename IntFreqType_ = int, int FixedMaxParamList_ = Eigen::Dynamic,
	 bool UseNMeasAmplifyFactor_ = false>
TOMOGRAPHER_EXPORT class IndepMeasLLH
// : public Tools::NeedEigenAlignedOperatorNew::ProviderType -- not needed, matrices are Eigen::Dynamic for now
{
public:
  //! The \ref DMTypes in use here
  typedef DMTypes_ DMTypes;
  //! Type used to calculate the log-likelihood function (see \ref pageInterfaceDenseLLH)
  typedef LLHValueType_ LLHValueType;
  //! Type used to store integer measurement counts
  typedef IntFreqType_ IntFreqType;
  //! Maximum number of POVM effects, fixed at compile-time or \ref TutorialMatrixClass "Eigen::Dynamic"
  static constexpr int FixedMaxParamList = FixedMaxParamList_;
  //! Whether the dimension is specified dynamically at run-time or statically at compile-time
  static constexpr bool IsDynamicMaxParamList = (FixedMaxParamList_ == Eigen::Dynamic);
  //! Whether we allow NMeasAmplifyFactor to be set
  static constexpr bool UseNMeasAmplifyFactor = UseNMeasAmplifyFactor_;

  /** \brief Declare some stuff as part of the \ref pageInterfaceDenseLLH compliance
   *
   * See \ref DenseDM::LLHCalcTypeX and \ref pageInterfaceDenseLLH for details.
   */
  enum {
    //! Declare that this DenseLLH object exposes a logLikelihoodX() method.
    LLHCalcType = LLHCalcTypeX
  } ;

  /** \brief dynamic Matrix with rows = dim*dim Vectors (row-major)
   * [maximum FixedMaxParamList rows, or Dynamic]
   */
  typedef Eigen::Matrix<typename DMTypes::RealScalar, Eigen::Dynamic, DMTypes::FixedDim2,
                        Eigen::RowMajor, FixedMaxParamList, DMTypes::FixedDim2>  VectorParamListType;
  //! Const ref to a VectorParamListType
  typedef const Eigen::Ref<const VectorParamListType> & VectorParamListTypeConstRef;

  /** \brief Type used to index entries in VectorParamListType (and also used for indexing
   *         entries in FreqListType)
   */
  typedef typename VectorParamListType::Index IndexType;

  /** \brief dynamic Array of integers [maximum FixedMaxParamList entries or Dynamic]
   */
  typedef Eigen::Array<IntFreqType, Eigen::Dynamic, 1,
                       0 /*Options*/, FixedMaxParamList, 1>  FreqListType;
  //! Const ref to a FreqListType
  typedef const Eigen::Ref<const FreqListType> & FreqListTypeConstRef;

  
  /** \brief Simple constructor
   *
   * The measurement data is initialially empty.  You may call \ref addMeasEffect() or
   * \ref setMeas() to specify the measurement data.
   */
  inline IndepMeasLLH(DMTypes dmt_)
    : dmt(dmt_), _Exn(VectorParamListType::Zero(0, dmt.dim2())), _Nx(FreqListType::Zero(0)),
      _NMeasAmplifyFactor(1)
  {
  }

  /** \brief Constructor with full measurement data
   *
   * The measurement data is set to \a Exn_ and \a Nx_ via a call to \ref setMeas().
   */
  inline IndepMeasLLH(DMTypes dmt_, VectorParamListTypeConstRef Exn_, FreqListTypeConstRef Nx_)
    : dmt(dmt_), _Exn(VectorParamListType::Zero(0, dmt.dim2())), _Nx(FreqListType::Zero(0)), _NMeasAmplifyFactor(1)
  {
    setMeas(Exn_, Nx_);
  }

  //! The \ref DMTypes object, storing e.g. the dimension of the problem.
  const DMTypes dmt;

  /** \brief The number of POVM effects in the list
   *
   * Returns the size of the list of POVM effects.  They are not guaranteed to be
   * different (although this is usually the case); that depends on what the user
   * specified to \ref setMeas().
   */
  inline const IndexType numEffects() const { return _Exn.rows(); }

  /** \brief  The stored individual POVM effects, in \ref pageParamsX
   *
   * This is an \ref Eigen::Array type (2-D), whose rows are the x-parameterization
   * representation of the POVM effects.
   *
   * The number of rows of \ref Exn must be equal to the number of rows of \ref Nx.  Use
   * \ref setMeas() or \ref addMeasEffect() to populate measurements.
   *
   * See also \ref Exn(IndexType i) const
   */
  inline const VectorParamListType & Exn() const { return _Exn; }

  /** \brief  The i-th stored POVM effect, in \ref pageParamsX
   *
   * Returns the \a i-th stored POVM effect, in X-parameterization, as a column vector.
   *
   * See also \ref Exn()
   */
  inline Eigen::Ref<const typename DMTypes::VectorParamType>  Exn(IndexType i) const
  {
    return _Exn.row(i).transpose();
  }


  /** \brief The stored frequency counts for each individual POVM effect
   *
   * This is an \ref Eigen::Array object (1-D), of an integer type, which stores the
   * number of times the corresponding POVM effect (given in the corresponding row in \ref
   * Exn) was observed in the experiment.
   *
   * The number of rows of \ref Exn must be equal to the number of rows of \ref Nx.  Use
   * \ref setMeas() or \ref addMeasEffect to specify the measurement data.
   *
   * See also \ref Nx(IndexType i) const
   */
  inline const FreqListType & Nx() const { return _Nx; }

  /** \brief  The number of counts stored for the i-th POVM effect
   *
   * Returns the frequency count corresponding to the \a i-th POVM effect, i.e. the number
   * of times this POVM effect was observed in the experiment.
   *
   * See also \ref Nx()
   */
  inline const IntFreqType Nx(IndexType i) const { return _Nx(i); }

  /** \brief Reset the measurement vectors and frequency counts
   *
   * Resets the measurement data to an empty list.  Consequently the return values of \ref
   * Exn() and \ref Nx() are subsequently empty vectors.
   */
  inline void resetMeas()
  {
    _Exn.resize(0, dmt.dim2());
    _Nx.resize(0);
  }

  /** \brief Store a measurement POVM effect along with a frequency count
   *
   * \param E_x the X-parameterization of the POVM effect corresponding to the observed
   *            measurement outcome
   * \param n   the number of times this POVM effect was observed in the experiment
   *
   * \param check_validity Perform some sanity checks, verifying the \a E_x and \a n are
   *                       valid inputs
   *
   * If \a n is zero, then the measurement effect is ignored and not stored.  This is
   * because the outcome was never observed in the experiment; furthermore we can't have
   * zero entries in the \ref Nx() vector because that would cause "0*log(0)" terms in our
   * logLikelihoodX() calculation and create a mess of NaNs And Infs.
   *
   */
  inline void addMeasEffect(typename DMTypes::VectorParamTypeConstRef E_x, IntFreqType n,
			    bool check_validity = true)
  {
    // no change if frequency count for the added effect == 0
    if (n == 0) {
      return;
    }

    tomographer_assert(n > 0);

    if (check_validity) { // check validity of measurement data
      typename DMTypes::MatrixType E_m(dmt.initMatrixType());
      E_m = ParamX<DMTypes>(dmt).XToHerm(E_x);
      _check_effect(E_m);
    }

    IndexType newi = _Exn.rows();
    tomographer_assert(newi == _Nx.rows());

    _Exn.conservativeResize(newi + 1, Eigen::NoChange);
    _Exn.row(newi) = E_x.transpose();
    _Nx.conservativeResize(newi + 1, Eigen::NoChange);
    _Nx(newi) = n;

    tomographer_assert(_Exn.rows() == _Nx.rows());
  }

  /** \brief Store a POVM measurement effect with frequency count
   *
   * This overload accepts the POVM effect specified as a dense matrix instead of a \ref
   * pageParamsX.
   *
   * See \ref addMeasEffect(typename DMTypes::VectorParamTypeConstRef E_x, IntFreqType n,
   * bool check_validity = true).
   *
   */
  inline void addMeasEffect(typename DMTypes::MatrixTypeConstRef E_m, IntFreqType n,
			    bool check_validity = true)
  {
    tomographer_assert(E_m.rows() == E_m.cols());
    tomographer_assert(E_m.rows() == (IndexType)dmt.dim());

    // skip if no frequency count
    if (n == 0) {
      return;
    }

    if (check_validity) { // check validity of measurement data
      tomographer_assert(n > 0);
      _check_effect(E_m);
    }

    addMeasEffect(Tomographer::DenseDM::ParamX<DMTypes>(dmt).HermToX(E_m), n, false);
  }

  /** \brief Specify the full measurement data at once
   *
   *
   */
  inline void setMeas(VectorParamListTypeConstRef Exn_, FreqListTypeConstRef Nx_, bool check_validity = true)
  {
    tomographer_assert(Exn_.cols() == (IndexType)dmt.dim2());
    tomographer_assert(Exn_.rows() == Nx_.rows());
    tomographer_assert(Nx_.cols() == 1);

    if ((Nx_ > 0).all()) {
      // all measurements are OK, so we can just copy the data.
      _Exn.resize(Exn_.rows(), dmt.dim2());
      _Exn = Exn_;
      _Nx.resize(Nx_.rows(), 1);
      _Nx = Nx_;
    } else {
      // otherwise, we have to add all measurements one by one, and filter out the zero-count ones.
      resetMeas();
      for (IndexType i = 0; i < Exn_.rows(); ++i) {
	addMeasEffect(Exn_.row(i), Nx_(i), false);
      }
    }
    if (check_validity) {
      checkAllMeas();
    }
  }

  inline void checkAllMeas() const
  {
    tomographer_assert(_Exn.cols() == (IndexType)dmt.dim2());
    tomographer_assert(_Exn.rows() == _Nx.rows());
    tomographer_assert(_Nx.cols() == 1);

    for (IndexType i = 0; i < _Exn.rows(); ++i) {

      tomographer_assert(_Nx(i) > 0);

      typename DMTypes::MatrixType E_m(dmt.initMatrixType());
      E_m = ParamX<DMTypes>(dmt).XToHerm(_Exn.row(i).transpose());
      _check_effect(E_m);
    }
  }
  inline void checkEffect(IndexType i) const
  {
    tomographer_assert(_Exn.cols() == (IndexType)dmt.dim2());
    tomographer_assert(_Exn.rows() == _Nx.rows());
    tomographer_assert(_Nx.cols() == 1);
    tomographer_assert(i >= 0 && i < _Exn.rows());
    tomographer_assert(_Nx(i) > 0);

    typename DMTypes::MatrixType E_m(dmt.initMatrixType());
    E_m = ParamX<DMTypes>(dmt).XToHerm(_Exn.row(i).transpose());
    _check_effect(E_m);
  }

private:
  inline void _check_effect(typename DMTypes::MatrixType E_m) const
  {
    if ( ! (double( (E_m - E_m.adjoint()).norm() ) < 1e-8) ) { // matrix not Hermitian
      throw InvalidMeasData(streamstr("POVM effect is not hermitian : E_m =\n"
				      << std::setprecision(10) << E_m));
    }
    Eigen::SelfAdjointEigenSolver<typename DMTypes::MatrixType> slv(E_m);
    const double mineigval = slv.eigenvalues().minCoeff();
    if ( ! (mineigval >= -1e-12) ) { // not positive semidef
      throw InvalidMeasData(streamstr("POVM effect is not positive semidefinite (min eigval="<<mineigval<<") : E_m =\n"
				      << std::setprecision(10) << E_m));
    }
    if ( ! (double(E_m.norm()) > 1e-6) ) { // POVM effect is zero
      throw InvalidMeasData(streamstr("POVM effect is zero : E_m =\n" << E_m));
    }
  }

public:

  /** \brief Factor by which to artificially amplify the number of measurements.
   *
   * This should always be set to one for any physical experiment.  It is only useful for
   * tests.
   */
  TOMOGRAPHER_ENABLED_IF(UseNMeasAmplifyFactor)
  inline LLHValueType NMeasAmplifyFactor() const { return _NMeasAmplifyFactor.value; }

  //! Basic implementation of NMeasAmplifyFactor() if this feature is disabled
  TOMOGRAPHER_ENABLED_IF(!UseNMeasAmplifyFactor)
  inline LLHValueType NMeasAmplifyFactor() const { return LLHValueType(1); }

  /** \brief Set the factor by which to artificially multiply all frequency counts
   *
   * Only use this for tests and debugging.  It does not have any meaning for actual
   * measurement data, as with this you are altering the numbers that you observed in the
   * experiment!
   */
  TOMOGRAPHER_ENABLED_IF(UseNMeasAmplifyFactor)
  inline void setNMeasAmplifyFactor(LLHValueType val) { _NMeasAmplifyFactor.value = val; }
  

  /** \brief Calculates the log-likelihood function, in X parameterization
   *
   * \returns the value of the log-likelihood function of this data at the point \a x,
   * defined as \f[
   *    \log\Lambda(\texttt{x}) = \sum_k \texttt{Nx[k]}\,\ln\mathrm{tr}(\texttt{Exn[k]}\,\rho(\texttt{x})) .
   * \f]
   *
   * \note this does not include a sometimes conventional factor \f$ -2\f$.
   */
  inline LLHValueType logLikelihoodX(typename DMTypes::VectorParamTypeConstRef x) const
  {
    return _mult_by_nmeasfactor(
	(_Nx.template cast<LLHValueType>() * (_Exn * x).template cast<LLHValueType>().array().log()).sum()
	);
  }

private:
  template<typename Expr, TOMOGRAPHER_ENABLED_IF_TMPL(UseNMeasAmplifyFactor)>
  inline auto _mult_by_nmeasfactor(Expr&& expr) const -> decltype(LLHValueType(1) * expr)
  {
    return _NMeasAmplifyFactor.value * expr;
  }
  template<typename Expr, TOMOGRAPHER_ENABLED_IF_TMPL(!UseNMeasAmplifyFactor)>
  inline auto _mult_by_nmeasfactor(Expr&& expr) const -> decltype(std::forward<Expr>(expr))
  {
    return std::forward<Expr>(expr);
  }

private:
  //! Store the data returned by \ref Exn() 
  VectorParamListType _Exn;
  //! Store the data returned by \ref Nx() 
  FreqListType _Nx;

  //! Number by which to artificially amplify the frequency vector (for tests)
  Tomographer::Tools::StoreIfEnabled<LLHValueType, UseNMeasAmplifyFactor> _NMeasAmplifyFactor;
};
// define static members:
template<typename DMTypes_, typename LLHValueType_,
         typename IntFreqType_, int FixedMaxParamList_,
	 bool UseNMeasAmplifyFactor_>
constexpr int
IndepMeasLLH<DMTypes_,LLHValueType_,IntFreqType_,FixedMaxParamList_,UseNMeasAmplifyFactor_>::FixedMaxParamList;
template<typename DMTypes_, typename LLHValueType_,
         typename IntFreqType_, int FixedMaxParamList_,
	 bool UseNMeasAmplifyFactor_>
constexpr bool
IndepMeasLLH<DMTypes_,LLHValueType_,IntFreqType_,FixedMaxParamList_,UseNMeasAmplifyFactor_>::IsDynamicMaxParamList;
template<typename DMTypes_, typename LLHValueType_,
         typename IntFreqType_, int FixedMaxParamList_,
	 bool UseNMeasAmplifyFactor_>
constexpr bool
IndepMeasLLH<DMTypes_,LLHValueType_,IntFreqType_,FixedMaxParamList_,UseNMeasAmplifyFactor_>::UseNMeasAmplifyFactor;





} // namespace DenseDM
} // namespace Tomographer








#endif
