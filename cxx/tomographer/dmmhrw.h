
#ifndef DMLLHINTEGRATOR_H
#define DMLLHINTEGRATOR_H

#include <cstddef>
#include <cmath>

#include <random>

#include <tomographer/qit/util.h>
#include <tomographer/qit/dist.h>
#include <tomographer/tomoproblem.h>
#include <tomographer/tools/loggers.h>
#include <tomographer/mhrw.h>

namespace Tomographer {


/** \brief A random walk in the density matrix space of a Hilbert state space of a quantum
 * system
 *
 * The random walk explores the density operators on a Hilbert space of a given dimension,
 * according to the distribution given by the likelihood function on the Hilbert-schmidt
 * uniform prior.
 *
 * \tparam TomoProblem the tomography data, expected to be a priori a \ref
 *      IndepMeasTomoProblem
 *
 * \tparam Rng a \c std::random random number \a generator (such as \ref std::mt19937)
 *
 * \tparam MHRWStatsCollector a type implementing a StatsCollector interface (\ref
 *      pageInterfaceMHRWStatsCollector)
 */
template<typename TomoProblem, typename Rng, typename Log>
class DMStateSpaceLLHMHWalker
{
public:
  //! The data types of our problem
  typedef typename TomoProblem::MatrQ MatrQ;
  //! The loglikelihood function value type (see \ref IndepMeasTomoProblem)
  typedef typename TomoProblem::LLHValueType LLHValueType;
  //! The matrix type for a density operator on our quantum system
  typedef typename MatrQ::MatrixType MatrixType;
  //! Type of an X-parameterization of a density operator (see \ref param_x_to_herm())
  typedef typename MatrQ::VectorParamType VectorParamType;
  //! The real scalar corresponding to our data types. Usually a \c double.
  typedef typename MatrQ::RealScalar RealScalar;
  //! The complex real scalar corresponding to our data types. Usually a \c std::complex<double>.
  typedef typename MatrQ::ComplexScalar ComplexScalar;

  //! Provided for MHRandomWalk. A point in our random walk = a density matrix
  typedef MatrixType PointType;
  //! Provided for MHRandomWalk. The function value type is the loglikelihood value type
  typedef LLHValueType FnValueType;
  enum {
    /** \brief We will calculate the log-likelihood function, which is the logarithm of
     * the Metropolis-Hastings function we should be calculating
     */
    UseFnSyntaxType = MHUseFnLogValue
  };

private:

  const TomoProblem & _tomo;
  Rng & _rng;
  std::normal_distribution<RealScalar> _normal_distr_rnd;

  Log & _log;
  
  MatrixType _startpt;

public:

  /** \brief Constructor which just initializes the given fields
   *
   * If you provide a zero \a startpt here, then a random starting point will be chosen
   * using the \a rng random number generator to generate a random point on the sphere.
   */
  DMStateSpaceLLHMHWalker(const MatrixType & startpt, const TomoProblem & tomo, Rng & rng,
			  Log & log_)
    : _tomo(tomo),
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
    _log.debug("DMStateSpaceLLHMHWalker", "Starting random walk");
  }

  //! Return the starting point given in the constructor, or a random start point
  inline const MatrixType & startpoint()
  {
    if (_startpt.norm() > 1e-3) {
      // nonzero matrix given: that's the starting point
      return _startpt;
    }

    // zero matrix given: means to choose random starting point
    MatrixType T = _tomo.matq.initMatrixType();
    T = dense_random<MatrixType>(
	_rng, _normal_distr_rnd, _tomo.matq.dim(), _tomo.matq.dim()
	);
    _startpt = T/T.norm(); // normalize to be on surface of the sphere

    _log.debug("DMStateSpaceLLHMHWalker", [&T](std::ostream & str) {
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
   * \return <code>-0.5 * (-2-log-likelihood)</code>, where the
   * <code>-2-log-likelihood</code> is computed using \ref
   * IndepMeasTomoProblem::calc_llh()
   */
  inline LLHValueType fnlogval(const MatrixType & T)
  {
    MatrixType rho = _tomo.matq.initMatrixType();
    rho.noalias() = T*T.adjoint();

    VectorParamType x = _tomo.matq.initVectorParamType();
    param_herm_to_x(x, rho);

    LLHValueType llhval = -0.5 * _tomo.calc_llh(x);
    // _log.longdebug("fnlogval(%s) = %g\n", streamcstr(x.transpose()), llhval);
    return llhval;
  }

  //! Decides of a new point to jump to for the random walk
  inline MatrixType jump_fn(const MatrixType& cur_T, RealScalar step_size)
  {
    MatrixType DeltaT = dense_random<MatrixType>(
        _rng, _normal_distr_rnd, _tomo.matq.dim(), _tomo.matq.dim()
        );

    MatrixType new_T = _tomo.matq.initMatrixType();

    new_T.noalias() = cur_T + step_size * DeltaT;

    // renormalize to "project" onto the large T-space sphere
    new_T /= new_T.norm(); //Matrix<>.norm() is Frobenius norm.

    //    _log.longdebug("jump_fn(): step_size=%g, cur_T =\n%s\nDeltaT = \n%s\nnew_T = \n%s",
    //                   step_size, streamstr(cur_T).c_str(), streamstr(DeltaT).c_str(),
    //                   streamstr(new_T).c_str());

    // hope for Return Value Optimization by compiler
    return new_T;
  }

};





/** \brief Calculate the fidelity to a reference state for each sample
 *
 */
template<typename TomoProblem_, typename FidValueType_ = double>
class FidelityToRefCalculator
{
public:
  typedef TomoProblem_ TomoProblem;
  typedef typename TomoProblem::MatrQ MatrQ;
  typedef typename MatrQ::MatrixType MatrixType;

  //! For ValueCalculator interface : value type
  typedef FidValueType_ ValueType;

private:
  MatrixType _ref_T;

public:
  //! Constructor, take the reference state to be the MLE
  FidelityToRefCalculator(const TomoProblem & tomo)
    : _ref_T(tomo.matq.initMatrixType())
  {
    _ref_T = tomo.T_MLE;
  }
  //! Constructor, the reference state is T_ref (in \ref pageParamsT)
  FidelityToRefCalculator(const TomoProblem & tomo, const MatrixType & T_ref)
    : _ref_T(tomo.matq.initMatrixType())
  {
    _ref_T = T_ref;
  }

  inline ValueType getValue(const MatrixType & T) const
  {
    return fidelity_T<ValueType>(T, _ref_T);
  }
};

/** \brief Calculate the "purified distance" to a reference state for each sample
 *
 * The purified distance \f$ P(\rho,\sigma) \f$ for two normalized states is defined as
 * \f[
 *   P\left(\rho,\sigma\right) = \sqrt{1 - F^2\left(\rho,\sigma\right)}\ .
 * \f]
 */
template<typename TomoProblem_, typename ValueType_ = double>
class PurifDistToRefCalculator
{
public:
  typedef TomoProblem_ TomoProblem;
  typedef typename TomoProblem::MatrQ MatrQ;
  typedef typename MatrQ::MatrixType MatrixType;

  //! For ValueCalculator interface : value type
  typedef ValueType_ ValueType;

private:
  MatrixType _ref_T;

public:
  //! Constructor, take the reference state to be the MLE
  PurifDistToRefCalculator(const TomoProblem & tomo)
    : _ref_T(tomo.matq.initMatrixType())
  {
    _ref_T = tomo.T_MLE;
  }
  //! Constructor, the reference state is T_ref (in \ref pageParamsT)
  PurifDistToRefCalculator(const TomoProblem & tomo, const MatrixType & T_ref)
    : _ref_T(tomo.matq.initMatrixType())
  {
    _ref_T = T_ref;
  }

  inline ValueType getValue(const MatrixType & T) const
  {
    auto F = fidelity_T<ValueType>(T, _ref_T);
    return std::sqrt(1.0 - F*F);
  }
};

/** \brief Calculate the trace distance to a reference state for each sample
 *
 */
template<typename TomoProblem_, typename TrDistValueType_ = double>
class TrDistToRefCalculator
{
public:
  typedef TomoProblem_ TomoProblem;
  typedef typename TomoProblem::MatrQ MatrQ;
  typedef typename MatrQ::MatrixType MatrixType;

  //! For ValueCalculator interface : value type
  typedef TrDistValueType_ ValueType;

private:
  MatrixType _ref_rho;

public:
  //! Constructor, take the reference state to be the MLE
  TrDistToRefCalculator(const TomoProblem & tomo)
    : _ref_rho(tomo.matq.initMatrixType())
  {
    _ref_rho = tomo.rho_MLE;
  }
  //! Constructor, the reference state is \a rho_ref
  TrDistToRefCalculator(const TomoProblem & tomo, const MatrixType& rho_ref)
    : _ref_rho(tomo.matq.initMatrixType())
  {
    _ref_rho = rho_ref;
  }

  inline ValueType getValue(const MatrixType & T) const
  {
    return 0.5 * (T*T.adjoint() - _ref_rho).jacobiSvd().singularValues().sum();
  }
};

/** \brief Calculate expectation value of an observable for each sample
 *
 */
template<typename TomoProblem_>
class ObservableValueCalculator
{
public:
  typedef TomoProblem_ TomoProblem;
  typedef typename TomoProblem::MatrQ MatrQ;
  typedef typename MatrQ::MatrixType MatrixType;
  typedef typename MatrQ::VectorParamType VectorParamType;

  //! For ValueCalculator interface : value type
  typedef typename MatrQ::RealScalar ValueType;

private:
  const TomoProblem & _tomo;

  //! The observable we wish to watch the exp value with (in \ref pageParamsX)
  VectorParamType _A_x;

  //  MatrixType _A; // not needed

public:
  //! Constructor directly accepting \a A as a hermitian matrix
  ObservableValueCalculator(const TomoProblem & tomo, const MatrixType & A)
    : _tomo(tomo),
      _A_x(tomo.matq.initVectorParamType())
  {
    param_herm_to_x(_A_x, A);
  }

  //! Constructor directly accepting the X parameterization of \a A
  ObservableValueCalculator(const TomoProblem & tomo, const VectorParamType & A_x)
    : _tomo(tomo),
      _A_x(tomo.matq.initVectorParamType())
  {
    _A_x = A_x;
  }

  inline ValueType getValue(const MatrixType & T) const
  {
    MatrixType rho = _tomo.matq.initMatrixType();
    rho = T*T.adjoint();
    VectorParamType x = _tomo.matq.initVectorParamType();
    param_herm_to_x(x, rho);
    return _A_x.transpose() * x;
  }
};



} // namespace Tomographer


#endif
