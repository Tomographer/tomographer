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

#ifndef TOMOGRAPHER_DENSEDM_TSPACELLHWALKER_H
#define TOMOGRAPHER_DENSEDM_TSPACELLHWALKER_H

#include <cstddef>
#include <cmath>

#include <random>

#include <boost/math/constants/constants.hpp>

#include <tomographer/tools/loggers.h>
#include <tomographer/tools/needownoperatornew.h>
#include <tomographer/densedm/densellh.h>
#include <tomographer/densedm/dmtypes.h>
#include <tomographer/densedm/param_herm_x.h>
#include <tomographer/mhrw.h>

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




namespace tomo_internal {

template<typename DenseLLHType, typename = void>
struct DenseLLHInvoker
{
  typedef typename DenseLLHType::DMTypes DMTypes;
  typedef typename DMTypes::MatrixType MatrixType;
  typedef typename DMTypes::VectorParamType VectorParamType;
  typedef typename DenseLLHType::LLHValueType LLHValueType;

  const DenseLLHType & llh;
  const Tools::StoreIfEnabled<const ParamX<DMTypes>, ((int)DenseLLHType::LLHCalcType == (int)LLHCalcTypeX)> param_x;

  DenseLLHInvoker(const DenseLLHType & llh_)
    : llh(llh_),
      param_x(llh.dmt)
  {
  }

  // This implementation deals for \a DenseLLH objects which expose a \a logLikelihoodX()
  // function (see \ref pageInterfaceDenseLLH)
  TOMOGRAPHER_ENABLED_IF(DenseLLHType::LLHCalcType == LLHCalcTypeX)
  inline LLHValueType fnLogVal(const MatrixType & T) const
  {
    MatrixType rho(T*T.adjoint());
    VectorParamType x = param_x.value.HermToX(rho);
    LLHValueType llhval =  llh.logLikelihoodX(x);
    return llhval;
  }

  // This implementation deals for \a DenseLLH objects which expose a \a logLikelihoodRho()
  // function (see \ref pageInterfaceDenseLLH)
  TOMOGRAPHER_ENABLED_IF(DenseLLHType::LLHCalcType == LLHCalcTypeRho)
  inline LLHValueType fnLogVal(const MatrixType & T) const
  {
    LLHValueType llhval =  llh.logLikelihoodRho(T*T.adjoint());
    return llhval;
  }

};

} // namespace tomo_internal




/** \brief A random walk in the density matrix space of a Hilbert state space of a quantum
 *         system (uniform random unitary jumps; slower)
 *
 * The random walk explores the density operators on a Hilbert space of a given dimension,
 * according to the distribution given by the likelihood function on the Hilbert-schmidt
 * uniform prior.
 *
 * \tparam DenseLLHType A type satisfying the \ref pageInterfaceDenseLLH
 *
 * \tparam RngType A \c std::random random number \a generator (such as \ref std::mt19937)
 *
 * \tparam LoggerType A logger type (see \ref pageLoggers)
 */
template<typename DenseLLHType_, typename RngType_, typename LoggerType_>
class TOMOGRAPHER_EXPORT LLHMHWalker
  : public Tools::NeedOwnOperatorNew<typename DenseLLHType_::DMTypes::MatrixType>::ProviderType
{
public:
  //! The DenseLLH interface object type
  typedef DenseLLHType_ DenseLLHType;
  //! The random number generator type
  typedef RngType_ RngType;
  //! The logger type
  typedef LoggerType_ LoggerType;

  //! The data types of our problem
  typedef typename DenseLLHType::DMTypes DMTypes;
  //! The loglikelihood function value type (see \ref pageInterfaceDenseLLH e.g. \ref IndepMeasLLH)
  typedef typename DenseLLHType::LLHValueType LLHValueType;
  //! The matrix type for a density operator on our quantum system
  typedef typename DMTypes::MatrixType MatrixType;
  //! Type of an X-parameterization of a density operator (see \ref pageParamsX)
  typedef typename DMTypes::VectorParamType VectorParamType;
  //! The real scalar corresponding to our data types. Usually a \c double.
  typedef typename DMTypes::RealScalar RealScalar;
  //! The complex real scalar corresponding to our data types. Usually a \c std::complex<double>.
  typedef typename DMTypes::ComplexScalar ComplexScalar;

  /** \brief The real scalar type for the step size &mdash; this scalar type is the only
   *         parameter we need (see \ref pageInterfaceMHWalker)
   */
  typedef MHWalkerParamsStepSize<RealScalar> WalkerParams;

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

  const DenseLLHType & _llh;
  const tomo_internal::DenseLLHInvoker<DenseLLHType> _llhinvoker;
  RngType & _rng;
  std::normal_distribution<RealScalar> _normal_distr_rnd;

  LoggerType & _log;
  
  MatrixType _startpt;

public:

  /** \brief Constructor which just initializes the given fields
   *
   * The \a llh object is responsible for calculating the log-likelihood function of the
   * tomography experiment (see class documentation).
   * 
   * The \a startpt argument specifies the starting point for the random walk (matrix in
   * \ref pageParamsT).  If you provide a zero \a startpt here, then a random starting
   * point will be chosen using the \a rng random number generator to generate a random
   * point on the sphere.
   */
  LLHMHWalker(const MatrixType & startpt, const DenseLLHType & llh, RngType & rng, LoggerType & log_)
    : _llh(llh),
      _llhinvoker(llh),
      _rng(rng),
      _normal_distr_rnd(0.0, 1.0),
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
  inline const MatrixType & startPoint()
  {
    // It's fine to hard-code "1e-3" because for any type, valid T-matrices have norm == 1
    if (_startpt.norm() > 1e-3) {
      // nonzero matrix given: that's the starting point.
      return _startpt;
    }

    // zero matrix given: means to choose random starting point
    MatrixType T(_llh.dmt.initMatrixType());
    T = Tools::denseRandom<MatrixType>(
	_rng, _normal_distr_rnd, (Eigen::Index)_llh.dmt.dim(), (Eigen::Index)_llh.dmt.dim()
	);
    _startpt = T/T.norm(); // normalize to be on surface of the sphere

    _log.debug("TSpace::LLHMHWalker", [&](std::ostream & str) {
	str << "Chosen random start point T = \n" << _startpt;
      });

    // return start point
    return _startpt;
  }

  //! Callback for after thermalizing is done. No-op.
  inline void thermalizingDone()
  {
  }

  //! Callback for after random walk is finished. No-op.
  inline void done()
  {
  }

  /** \brief Calculate the logarithm of the Metropolis-Hastings function value.
   *
   * \return the log-likelihood, which is computed via the \a DenseLLH object.
   *
   */
  inline LLHValueType fnLogVal(const MatrixType & T) const
  {
    return _llhinvoker.fnLogVal(T);
  }

  //! Decides of a new point to jump to for the random walk
  inline MatrixType jumpFn(const MatrixType& cur_T, WalkerParams params)
  {
    MatrixType DeltaT(Tools::denseRandom<MatrixType>(
                          _rng, _normal_distr_rnd,
                          (Eigen::Index)_llh.dmt.dim(), (Eigen::Index)_llh.dmt.dim()
                          ));

    MatrixType new_T(cur_T + params.step_size * DeltaT);

    // renormalize to "project" onto the large T-space sphere
    new_T /= new_T.norm(); //Matrix<>.norm() is Frobenius norm.

    //    _log.longdebug("jump_fn(): step_size=%g, cur_T =\n%s\nDeltaT = \n%s\nnew_T = \n%s",
    //                   step_size, streamstr(cur_T).c_str(), streamstr(DeltaT).c_str(),
    //                   streamstr(new_T).c_str());

    // hope for Return Value Optimization by compiler
    return new_T;
  }

};



// /** \brief Jump parameters (step size) for \ref LLHMHWalkerLight
//  *
//  */
// template<typename AngleRealType_, typename JumpsCountIntType_ = std::int_fast16_t>
// struct LLHMHWalkerLightParams
// {
//   typedef AngleRealType_ AngleRealType;
//   typedef JumpsCountIntType_ JumpsCountIntType;

//   LLHMHWalkerLightParams(AngleRealType angle_step_size_, JumpsCountIntType num_elem_jumps_)
//     : angle_step_size(angle_step_size_),
//       num_elem_jumps(num_elem_jumps_)
//   {
//   }

//   // initialize from MHWalkerParamsStepSize
//   template<typename RealType>
//   LLHMHWalkerLightParams(MHWalkerParamsStepSize<RealType> angle_step_size_param)
//     : angle_step_size(step_size_param.step_size),
//       num_elem_jumps(1)
//   {
//   }

//   AngleRealType angle_step_size;
//   JumpsCountIntType num_elem_jumps;

// };

template<typename StepRealType>
using LLHMHWalkerLightParams = MHWalkerParamsStepSize<StepRealType>;


// template<typename AngleRealType_, typename JumpsCountIntType_>
// inline std::ostream & operator<<(std::ostream & stream,
//                                  const LLHMHWalkerLightParams<AngleRealType_,JumpsCountIntType_> & p)
// {
//   return stream << "angle_step_size=" << p.angle_step_size << ", num_elem_jumps="<<p.num_elem_jumps;
// }




/** \brief A random walk in the density matrix space of a Hilbert state space of a quantum
 *         system (elementary rotation jumps; faster)
 *
 * The random walk explores the density operators on a Hilbert space of a given dimension,
 * according to the distribution given by the likelihood function on the Hilbert-schmidt
 * uniform prior.
 *
 * In contrast to \ref LLHMHWalker, the random jumps of this \a MHWalker is not a uniform
 * random unitary jump, rather, it is a jump in a random elementary direction.  The
 * stationary distribution of the Markov chain is the same, so the same distribution is
 * explored, but because less calculations are involved this method can be much faster.
 *
 * \since Added in %Tomographer 5.0
 *
 * \tparam DenseLLHType A type satisfying the \ref pageInterfaceDenseLLH
 *
 * \tparam RngType A \c std::random random number \a generator (such as \ref std::mt19937)
 *
 * \tparam LoggerType A logger type (see \ref pageLoggers)
 */
template<typename DenseLLHType_, typename RngType_, typename LoggerType_>
         //typename JumpsCountIntType_ = std::int_fast16_t>
class TOMOGRAPHER_EXPORT LLHMHWalkerLight
  : public Tools::NeedOwnOperatorNew<typename DenseLLHType_::DMTypes::MatrixType>::ProviderType
{
public:
  //! The DenseLLH interface object type
  typedef DenseLLHType_ DenseLLHType;
  //! The random number generator type
  typedef RngType_ RngType;
  //! The logger type
  typedef LoggerType_ LoggerType;
//  //! Type for the number of jumps in a "step jump"
//  typedef JumpsCountIntType_ JumpsCountIntType;

  //! The data types of our problem
  typedef typename DenseLLHType::DMTypes DMTypes;
  //! The loglikelihood function value type (see \ref pageInterfaceDenseLLH e.g. \ref IndepMeasLLH)
  typedef typename DenseLLHType::LLHValueType LLHValueType;
  //! The matrix type for a density operator on our quantum system
  typedef typename DMTypes::MatrixType MatrixType;
  //! Type of an X-parameterization of a density operator (see \ref pageParamsX)
  typedef typename DMTypes::VectorParamType VectorParamType;
  //! The real scalar corresponding to our data types. Usually a \c double.
  typedef typename DMTypes::RealScalar RealScalar;
  //! The complex real scalar corresponding to our data types. Usually a \c std::complex<double>.
  typedef typename DMTypes::ComplexScalar ComplexScalar;

  /** \brief The real scalar type for the step size &mdash; this scalar type is the only
   *         parameter we need (see \ref pageInterfaceMHWalker)
   */
  typedef LLHMHWalkerLightParams<RealScalar//,JumpsCountIntType
                                 > WalkerParams;

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

  const DenseLLHType & _llh;
  const tomo_internal::DenseLLHInvoker<DenseLLHType> _llhinvoker;
  RngType & _rng;
  std::normal_distribution<RealScalar> _normal_distr_rnd;
  std::uniform_int_distribution<std::int_fast8_t> _jumptype_distr_rnd;
  std::uniform_int_distribution<Eigen::Index> _jumpdir_distr_rnd;

  Logger::LocalLogger<LoggerType> _llogger;
  
  MatrixType _startpt;

public:

  /** \brief Constructor which just initializes the given fields
   *
   * The \a llh object is responsible for calculating the log-likelihood function of the
   * tomography experiment (see class documentation).
   * 
   * The \a startpt argument specifies the starting point for the random walk (matrix in
   * \ref pageParamsT).  If you provide a zero \a startpt here, then a random starting
   * point will be chosen using the \a rng random number generator to generate a random
   * point on the sphere.
   */
  LLHMHWalkerLight(const MatrixType & startpt, const DenseLLHType & llh, RngType & rng, LoggerType & baselogger)
    : _llh(llh),
      _llhinvoker(llh),
      _rng(rng),
      _normal_distr_rnd(0.0, 1.0),
      _jumptype_distr_rnd(0, 2), // choice in {0, 1, 2}
      _jumpdir_distr_rnd(0, (Eigen::Index)llh.dmt.dim2()-1), // choice in {0,1,...,dim^2-1}
      _llogger("Tomographer::DenseDM::TSpace::LLHMHWalkerLight", baselogger),
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
    auto logger = _llogger.subLogger(TOMO_ORIGIN) ;
    logger.debug("Starting random walk");
  }

  //! Return the starting point given in the constructor, or a random start point
  inline const MatrixType & startPoint()
  {
    auto logger = _llogger.subLogger(TOMO_ORIGIN) ;

    // It's fine to hard-code "1e-3" because for any type, valid T-matrices have norm == 1
    if (_startpt.norm() > 1e-3) {
      // nonzero matrix given: that's the starting point.
      return _startpt;
    }

    // zero matrix given: means to choose random starting point
    MatrixType T(_llh.dmt.initMatrixType());
    T = Tools::denseRandom<MatrixType>(
	_rng, _normal_distr_rnd, (Eigen::Index)_llh.dmt.dim(), (Eigen::Index)_llh.dmt.dim()
	);
    _startpt = T/T.norm(); // normalize to be on surface of the sphere

    logger.debug([&](std::ostream & str) {
	str << "Chosen random start point T = \n" << _startpt;
      });

    // return start point
    return _startpt;
  }

  //! Callback for after thermalizing is done. No-op.
  inline void thermalizingDone()
  {
  }

  //! Callback for after random walk is finished. No-op.
  inline void done()
  {
  }

  /** \brief Calculate the logarithm of the Metropolis-Hastings function value.
   *
   * \return the log-likelihood, which is computed via the \a DenseLLH object.
   *
   */
  inline LLHValueType fnLogVal(const MatrixType & T) const
  {
    return _llhinvoker.fnLogVal(T);
  }

  //! Decides of a new point to jump to for the random walk
  inline MatrixType jumpFn(const MatrixType& cur_T, WalkerParams params)
  {
    MatrixType new_T(cur_T);

    auto logger = _llogger.subLogger(TOMO_ORIGIN) ;

    // repeat several times, to have some chance of the effect not just rotating the purification ...
    for (int j = 0; j < (int)_llh.dmt.dim(); ++j) {

      // select two random indices, and randomly select whether we apply an elementary x, y or z rotation
      Eigen::Index k1 = _jumpdir_distr_rnd(_rng);
      Eigen::Index k2;
      do { k2 = _jumpdir_distr_rnd(_rng); } while (k1 == k2); // choose also randomly, but different than k1.
      if (k1 > k2) { std::swap(k1, k2); } // ensure that k1 < k2
      std::int_fast8_t xyz = _jumptype_distr_rnd(_rng);

      RealScalar sina = params.step_size * _normal_distr_rnd(_rng);
      if (sina < -1) { sina = -1; }
      if (sina >  1) { sina =  1; }
      RealScalar cosa = std::sqrt(1 - sina*sina);
      // apply transformation
      Eigen::Matrix<ComplexScalar,2,2> tr2d;
      // remember: e^{i\phi(\vec{n}\cdot\vec{\sigma})} = \cos\phi \Ident + i\sin\phi (\vec{n}\cdot\vec{\sigma})
      switch (xyz) {
      case 0: // X-type rotation
        tr2d(0,0) = cosa;                   tr2d(0,1) = ComplexScalar(0,sina);
        tr2d(1,0) = ComplexScalar(0,sina);  tr2d(1,1) = cosa;
        break;
      case 1: // Y-type rotation
        tr2d(0,0) = cosa;   tr2d(0,1) = sina;
        tr2d(1,0) = -sina;  tr2d(1,1) = cosa;
        break;
      case 2: // Z-type rotation
        tr2d(0,0) = ComplexScalar(cosa, sina);  tr2d(0,1) = 0;
        tr2d(1,0) = 0;                          tr2d(1,1) = ComplexScalar(cosa, -sina);
        break;
      default:
        tomographer_assert(false && "Invalid rotation type number sampled!");
      }
      // apply the elementary rotation onto new_T, seen as a vector
      const Eigen::Index i1 = k1 / (Eigen::Index)_llh.dmt.dim();
      const Eigen::Index j1 = k1 % (Eigen::Index)_llh.dmt.dim();
      const Eigen::Index i2 = k2 / (Eigen::Index)_llh.dmt.dim();
      const Eigen::Index j2 = k2 % (Eigen::Index)_llh.dmt.dim();

      logger.longdebug([&](std::ostream & stream) {
          stream << "Elementary jump rotation: "
                 << "k1="<<k1<<" -> i1="<<i1<<" j1="<<j1<<"  k2="<<k2<<" -> i2="<<i2<<" j2="<<j2<<"\n"
                 << "tr2d=" << tr2d;
        }) ;

      const auto x = tr2d(0,0) * new_T(i1,j1) + tr2d(0,1) * new_T(i2,j2) ;
      const auto y = tr2d(1,0) * new_T(i1,j1) + tr2d(1,1) * new_T(i2,j2) ;
      new_T(i1,j1) = x;
      new_T(i2,j2) = y;
    }

    // ensure continued normalization
    new_T /= new_T.norm(); // norm is Frobenius norm

    // new_T is ready, return it
    return new_T;
  }

};






} // namespace TSpace
} // namespace DenseDM
} // namespace Tomographer


#endif
