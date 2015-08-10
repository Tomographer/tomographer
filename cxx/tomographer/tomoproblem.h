
#ifndef TOMOPROBLEM_H
#define TOMOPROBLEM_H


namespace Tomographer {


/** \brief Stores data for a tomography problem with independent measurements
 *
 * Stores data corresponding to a tomography probem. This is given by
 *
 *   - a state space dimension
 *
 *   - a list of POVM effects
 *
 *   - a list of frequencies, i.e. how many times each POVM effect was observed
 *
 *   - a Maximum Likelihood Estimate, i.e. the density operator which maximizes the
 *     likelihood function
 *
 * Additionally, we store also a factor by which to (artificially) amplify all the
 * frequencies by, i.e. to effectively fake more measuremnts with the same statistics.
 */
template<typename MatrQ_, typename LLHValueType_ = double, bool UseCLoopInstead = false>
struct IndepMeasTomoProblem
{
  typedef MatrQ_ MatrQ;
  typedef LLHValueType_ LLHValueType;

  //! The data types for this problem
  const MatrQ matq;

  //! The dimension of the Hilbert space
  const int dim;
  //! The square of the dimension of the Hilbert space, dim2=dim*dim
  const int dim2;
  //! The number of degrees of freedom, \f$ \text{Ndof} = \text{dim}\times\text{dim}-1 \f$.
  const int Ndof;

  //! A factor to artificially amplify the number of measurements by
  LLHValueType NMeasAmplifyFactor;

  //! The POVM Entries, parameterized with X-param (\ref param_herm_to_x())
  typename MatrQ::VectorParamListType Exn;
  //! The frequency list, i.e. number of times each POVM effect was observed
  typename MatrQ::FreqListType Nx;

  //! Maximum likelihood estimate as density matrix
  typename MatrQ::MatrixType rho_MLE;
  //! Maximum likelihood estimate as T-parameterized density matrix
  /**
   * The T-parameterization is such that \f$ \rho = T T^\dagger \f$ .
   */
  typename MatrQ::MatrixType T_MLE;
  //! X-Parameterized version of rho_MLE (\ref param_herm_to_x())
  typename MatrQ::VectorParamType x_MLE;

  /** \brief Constructs an IndepMeasTomoProblem instance
   *
   * \note The members \ref Exn and \ref Nx are left uninitialized, because we don't know
   * how many POVM effects there will be yet. This information will anyway probably be
   * read later from an input file or such.
   */
  IndepMeasTomoProblem(MatrQ matq_)
    : matq(matq_),
      dim(matq_.dim()),
      dim2(dim*dim),
      Ndof(dim2 - 1),
      NMeasAmplifyFactor(1),
      rho_MLE(matq.initMatrixType()),
      T_MLE(matq.initMatrixType()),
      x_MLE(matq.initVectorParamType())
  {
    // NOTE: Exn & N are left uninitialized, because we don't yet know how many POVM
    // effects there will be. (That will probably be read from a MAT file or such...)
    assert(MatrQ::FixedDim == Eigen::Dynamic || MatrQ::FixedDim == (int)matq.dim());
  }

  /** \brief Calculates the log-likelihood function
   *
   * \returns the value of the -2-log-likelihood function of this data at the point \a x,
   * defined as \f[
   *    \lambda(\texttt{x}) = -2\sum_k \texttt{Nx[k]}\,\ln\mathrm{tr}(\texttt{Exn[k]}\,\rho(\texttt{x})) .
   * \f]
   */
  inline LLHValueType calc_llh(const typename MatrQ::VectorParamType & x) const;
};



namespace tomo_internal
{
  /** \internal
   * \brief Helper class for \ref IndepMeasTomoProblem
   *
   * This class allows to statically decied how to calculate the LLH function. This can
   * either be done with Eigen's functional-like programming, or with a simple C \c for
   * loop.
   */
  template<typename MatrQ, typename LLHValueType, bool UseCLoopInstead>
  struct LLH_Calculator
  {
    /** \brief calculate the log-likelihood function
     *
     * Calculate the -2 * log-likelihood function for the given data at the given point \c
     * x in state space. \c x represents a density matrix parameterized with \ref
     * param_herm_to_x().
     *
     * This function will either be implemented with Eigen's functional-like notation, or
     * with a C-style \c for loop, depending on the template parameter \c UseCLoopInstead.
     */
    static inline LLHValueType calc_llh(const IndepMeasTomoProblem<MatrQ,LLHValueType,UseCLoopInstead> *data,
                                        const typename MatrQ::VectorParamType & x)
    {
      return -2 * data->NMeasAmplifyFactor * (
          data->Nx.template cast<LLHValueType>() * (data->Exn * x).array().log()
          ).sum();
    }
  };

  //
  // Specialization of helper for implementing the C for loop instead.
  //
  // no doxygen doc needed.
  //
  template<typename MatrQ, typename LLHValueType>
  struct LLH_Calculator<MatrQ, LLHValueType, true>
  {
    static inline LLHValueType calc_llh(const IndepMeasTomoProblem<MatrQ,LLHValueType,true> *data,
                                        const typename MatrQ::VectorParamType & x)
    {
      std::size_t k;
      typename MatrQ::RealScalar val = 0;
      for (k = 0; k < (std::size_t)data->Exn.rows(); ++k) {
        const typename MatrQ::RealScalar thisval = (data->Exn.row(k) * x);
        //std::cout << "k = "<<k<<"; thisval = "<<thisval << "\n";
        val += data->Nx[k] * std::log(thisval);
      }
      //std::cout << "val = " << val << "\n\n";
      return -2 * data->NMeasAmplifyFactor * val;
    }
  };
}


template<typename MatrQ, typename LLHValueType, bool UseCLoopInstead>
inline LLHValueType IndepMeasTomoProblem<MatrQ, LLHValueType, UseCLoopInstead>::calc_llh(
    const typename MatrQ::VectorParamType & x
    ) const
{
  return tomo_internal::LLH_Calculator<MatrQ, LLHValueType, UseCLoopInstead>::calc_llh(this, x);
}





} // namespace Tomographer



#endif
