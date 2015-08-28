
#ifndef TEST_MH_RANDOM_WALK_H
#define TEST_MH_RANDOM_WALK_H

#include <random>

#include <tomographer/mhrw.h>

#include <Eigen/Core>


//
// Necessary utilities to perform a random walk on a lattice. just to perform test cases
// with integers so that we're sure we have deterministic results (e.g. from different
// compilers)
//
template<typename IntType, typename Rng>
struct TestLatticeMHRWBase
{
  typedef Eigen::Matrix<IntType,Eigen::Dynamic,1> PointType;
  typedef double RealScalar;
  
  PointType latticeDims;
  Rng rng;

  template<typename Der>
  TestLatticeMHRWBase(const Eigen::DenseBase<Der> & dims, typename Rng::result_type seed = 0)
    : latticeDims(dims.rows()), rng(seed)
  {
    latticeDims = dims;
  }

  inline void init() { }

  inline PointType startpoint()
  {
    return PointType::Zero(latticeDims.size());
  }

  inline void thermalizing_done() { }

  inline void done() { }

  inline PointType jump_fn(const PointType & curpt, const RealScalar step_size)
  {
    IntType istep = IntType(1+step_size);
    std::uniform_int_distribution<int> rnddist(-istep, istep);
    
    PointType newpt(latticeDims.size());
    for (int k = 0; k < latticeDims.size(); ++k) {
      IntType newcoord = curpt(k) + rnddist(rng);
      while (newcoord < 0) {
	newcoord += latticeDims(k);
      }
      while (newcoord >= latticeDims(k)) {
	newcoord -= latticeDims(k);
      }
      newpt(k) = newcoord;
    }

    return newpt;
  }

};


// M-H Random walk on a lattice (Periodic Boundary Conditions), using a Gaussian peak with
// some parameters as distribution.
//
// Complies to MHWalker type interface.
//
template<typename IntType, typename Rng = std::mt19937>
struct TestLatticeMHRWGaussPeak
  : public TestLatticeMHRWBase<IntType, Rng>
{
  typedef Eigen::Matrix<IntType,Eigen::Dynamic,Eigen::Dynamic> SigmaType;
  typedef typename TestLatticeMHRWBase<IntType,Rng>::PointType PointType;

  SigmaType Sigma;
  PointType Offset;

  template<typename Der1, typename Der2, typename Der3>
  TestLatticeMHRWGaussPeak(const Eigen::DenseBase<Der1> & dims, const Eigen::DenseBase<Der2>& Sigma_,
			   const Eigen::DenseBase<Der3> Offset_,
			   int seed = 0)
    : TestLatticeMHRWBase<IntType,Rng>(dims, seed),
      Sigma(dims.size(), dims.size()),
      Offset(dims.size())
  {
    Sigma = Sigma_;
    Offset = Offset_;
  }

  typedef int FnValueType; // make everything deterministic.
  enum { UseFnSyntaxType = Tomographer::MHUseFnLogValue } ;

  inline FnValueType fnlogval(const PointType & pt)
  {
    return (pt - Offset).transpose() * Sigma * (pt - Offset);
  }

};


#endif
