
#ifndef TOMOPROBLEM_H
#define TOMOPROBLEM_H




/** Tomography probem, given by a list of POVM effects with their frequencies.
 */
template<typename MatrQ_, typename LLHValueType_ = double, bool UseCLoopInstead = false>
struct IndepMeasTomoProblem
{
  typedef MatrQ_ MatrQ;
  typedef LLHValueType_ LLHValueType;

  const MatrQ matq;

  const int dim; //!< the dimension of the Hilbert space
  const int dim2; //!< the square of the dimension of the Hilbert space, dim2=dim*dim
  const int Ndof; //!< the number of degrees of freedom, Ndof = dim*dim-1

  LLHValueType NMeasAmplifyFactor; //!< A factor to artificially amplify the number of measurements by

  typename MatrQ::VectorParamListType Exn; //!< POVM Entries, parameterized with X-param
  typename MatrQ::FreqListType Nx; //!< frequency list

  typename MatrQ::MatrixType rho_MLE; //!< Maximum likelihood estimate as density matrix
  typename MatrQ::MatrixType T_MLE; //!< Maximum likelihood estimate as T-parameterized density matrix
  typename MatrQ::VectorParamType x_MLE; //!< X-Parameterized version of rho_MLE

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
    assert(MatrQ::FixedDim == Eigen::Dynamic || MatrQ::FixedDim == matq.dim());
  }

  inline LLHValueType calc_llh(const typename MatrQ::VectorParamType & x) const;
};



namespace tomo_internal
{
  template<typename MatrQ, typename LLHValueType, bool UseCLoopInstead>
  struct LLH_Calculator
  {
    static inline LLHValueType calc_llh(const IndepMeasTomoProblem<MatrQ,LLHValueType,UseCLoopInstead> *data,
                                        const typename MatrQ::VectorParamType & x)
    {
      return -2 * data->NMeasAmplifyFactor * (
          data->Nx.template cast<LLHValueType>() * (data->Exn * x).array().log()
          ).sum();
    }
  };

  template<typename MatrQ, typename LLHValueType>
  struct LLH_Calculator<MatrQ, LLHValueType, true>
  {
    static inline LLHValueType calc_llh(const IndepMeasTomoProblem<MatrQ,LLHValueType,true> *data,
                                        const typename MatrQ::VectorParamType & x)
    {
      size_t k;
      typename MatrQ::RealScalar val = 0;
      for (k = 0; k < (size_t)data->Exn.rows(); ++k) {
        const typename MatrQ::RealScalar thisval = (data->Exn.row(k) * x);
        //std::cout << "k = "<<k<<"; thisval = "<<thisval << "\n";
        val += data->Nx[k] * log(thisval);
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








#endif
