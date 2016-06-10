/* This file is part of the Tomographer project, which is distributed under the
 * terms of the MIT license.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 ETH Zurich, Institute for Theoretical Physics, Philippe Faist
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

#ifndef TOMOGRAPHER_DENSEDM_TSPACELLHWALKER_H
#define TOMOGRAPHER_DENSEDM_TSPACELLHWALKER_H

#include <cstddef>
#include <cmath>

#include <random>

#include <boost/math/constants/constants.hpp>

#include <tomographer2/tools/loggers.h>
#include <tomographer2/densedm/densellh.h>
#include <tomographer2/densedm/dmtypes.h>
#include <tomographer2/densedm/param_herm_x.h>
#include <tomographer2/mhrw.h>

/** \file tspacellhwalker.h
 *
 * \brief Definitions for a Metropolis-Hastings random walk on a quantum state space with
 *        dense matrix type
 *
 * See mainly \ref Tomographer::DenseDM::TSpace::LLHMHWalker.
 */

namespace Tomographer {
namespace DenseDM {
namespace TSpace {

/** \brief A random walk in the density matrix space of a Hilbert state space of a quantum
 * system
 *
 * The random walk explores the density operators on a Hilbert space of a given dimension,
 * according to the distribution given by the likelihood function on the Hilbert-schmidt
 * uniform prior.
 *
 * \tparam DenseLLH a type satisfying the LLH type interface (\ref pageInterfaceLLH)
 *
 *
 * \tparam Rng a \c std::random random number \a generator (such as \ref std::mt19937)
 *
 * \tparam MHRWStatsCollector a type implementing a StatsCollector interface (\ref
 *      pageInterfaceMHRWStatsCollector)
 */
template<typename DenseLLH, typename Rng, typename LoggerType>
class LLHMHWalker
{
public:
  //! The data types of our problem
  typedef typename DenseLLH::DMTypes DMTypes;
  //! The loglikelihood function value type (see \ref pageInterfaceLLH e.g. \ref IndepMeasLLH)
  typedef typename DenseLLH::LLHValueType LLHValueType;
  //! The matrix type for a density operator on our quantum system
  typedef typename DMTypes::MatrixType MatrixType;
  //! Type of an X-parameterization of a density operator (see \ref pageParamsX)
  typedef typename DMTypes::VectorParamType VectorParamType;
  //! The real scalar corresponding to our data types. Usually a \c double.
  typedef typename DMTypes::RealScalar RealScalar;
  //! The complex real scalar corresponding to our data types. Usually a \c std::complex<double>.
  typedef typename DMTypes::ComplexScalar ComplexScalar;

  /** \brief The real scalar type for the step size &mdash; this is our scalar type (see
   *         \ref pageInterfaceMHWalker)
   */
  typedef RealScalar StepRealType;

  //! Provided for MHRandomWalk. A point in our random walk = a density matrix
  typedef MatrixType PointType;
  //! Provided for MHRandomWalk. The function value type is the loglikelihood value type
  typedef LLHValueType FnValueType;
  //! see \ref pageInterfaceMHWalker
  enum {
    /** \brief We will calculate the log-likelihood function, which is the logarithm of
     *         the Metropolis-Hastings function we should be calculating
     */
    UseFnSyntaxType = MHUseFnLogValue
  };

private:

  const DenseLLH & _llh;
  Rng & _rng;
  std::normal_distribution<RealScalar> _normal_distr_rnd;
  const ParamX<DMTypes> _px;

  LoggerType & _log;
  
  MatrixType _startpt;

public:

  /** \brief Constructor which just initializes the given fields
   *
   * \param startpt A starting point for the random walk (matrix in \ref pageParamT).  If
   *     you provide a zero \a startpt here, then a random starting point will be chosen
   *     using the \a rng random number generator to generate a random point on the
   *     sphere.
   */
  LLHMHWalker(const MatrixType & startpt, const DenseLLH & llh, Rng & rng,
                        LoggerType & log_)
    : _llh(llh),
      _rng(rng),
      _normal_distr_rnd(0.0, 1.0),
      _px(llh.dmt),
      _log(log_),
      _startpt(startpt)
  {
  }

  /** \brief Provided for \ref MHRandomWalk. Initializes some fields and prepares for a
   *     random walk.
   *
   * \note If the \a startpt given to the constructor is zero (or has very small norm),
   * then a uniformly random starting point is chosen. The starting point is chosen
   * randomly according to the Haar-invariant measure on the state space. This corresponds
   * to choosing a uniform point on the sphere corresponding to the T-parameterization of
   * the density matrices.
   */
  inline void init()
  {
    _log.debug("TSpace::LLHMHWalker", "Starting random walk");
  }

  //! Return the starting point given in the constructor, or a random start point
  inline const MatrixType & startpoint()
  {
    if (_startpt.norm() > 1e-3) {
      // nonzero matrix given: that's the starting point
      return _startpt;
    }

    // zero matrix given: means to choose random starting point
    MatrixType T(_llh.dmt.initMatrixType());
    T = Tools::dense_random<MatrixType>(
	_rng, _normal_distr_rnd, _llh.dmt.dim(), _llh.dmt.dim()
	);
    _startpt = T/T.norm(); // normalize to be on surface of the sphere

    _log.debug("TSpace::LLHMHWalker", [&T](std::ostream & str) {
	str << "Chosen random start point T = \n" << T;
      });

    // return start point
    return _startpt;
  }

  //! Callback for after thermalizing is done. No-op.
  inline void thermalizing_done()
  {
  }

  //! Callback for after random walk is finished. No-op.
  inline void done()
  {
  }

  /** \brief Calculate the logarithm of the Metropolis-Hastings function value.
   *
   * \return the log-likelihood, which is computed via the DenseLLH object.
   *
   * This implementation deals for DenseLLH objects which expose a \a logLikelihoodX()
   * function (see \ref pageInterfaceDenseLLH)
   */
  TOMOGRAPHER_ENABLED_IF(DenseLLH::LLHCalcType == LLHCalcTypeX)
  inline LLHValueType fnlogval(const MatrixType & T) const
  {
    MatrixType rho(T*T.adjoint());
    VectorParamType x = _px.HermToX(rho);
    LLHValueType llhval =  _llh.logLikelihoodX(x);
    // _log.longdebug("fnlogval(%s) = %g\n", streamcstr(x.transpose()), llhval);
    return llhval;
  }

  /** \brief Calculate the logarithm of the Metropolis-Hastings function value.
   *
   * \return the log-likelihood, which is computed via the DenseLLH object.
   *
   * This implementation deals for DenseLLH objects which expose a \a logLikelihoodRho()
   * function (see \ref pageInterfaceDenseLLH)
   */
  TOMOGRAPHER_ENABLED_IF(DenseLLH::LLHCalcType == LLHCalcTypeRho)
  inline LLHValueType fnlogval(const MatrixType & T) const
  {
    LLHValueType llhval =  _llh.logLikelihoodRho(T*T.adjoint());
    // _log.longdebug("fnlogval( %s ) = %g\n", streamcstr(T*T.adjoint()), llhval);
    return llhval;
  }


  //! Decides of a new point to jump to for the random walk
  inline MatrixType jump_fn(const MatrixType& cur_T, RealScalar step_size)
  {
    MatrixType DeltaT(Tools::dense_random<MatrixType>(
                          _rng, _normal_distr_rnd, _llh.dmt.dim(), _llh.dmt.dim()
                          ));

    MatrixType new_T(cur_T + step_size * DeltaT);

    // renormalize to "project" onto the large T-space sphere
    new_T /= new_T.norm(); //Matrix<>.norm() is Frobenius norm.

    //    _log.longdebug("jump_fn(): step_size=%g, cur_T =\n%s\nDeltaT = \n%s\nnew_T = \n%s",
    //                   step_size, streamstr(cur_T).c_str(), streamstr(DeltaT).c_str(),
    //                   streamstr(new_T).c_str());

    // hope for Return Value Optimization by compiler
    return new_T;
  }

};



} // namespace TSpace
} // namespace DenseDM
} // namespace Tomographer


#endif
