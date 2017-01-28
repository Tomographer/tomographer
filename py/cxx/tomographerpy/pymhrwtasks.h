
#ifndef PYMHRWTASKS_H
#define PYMHRWTASKS_H

#include <string>

#include <tomographerpy/common.h>

#include <tomographer/mhrw.h>
#include <tomographer/mhrw_bin_err.h>
#include <tomographer/mhrwtasks.h>

#include <tomographerpy/pyhistogram.h>
#include <tomographerpy/pymhrw.h>


namespace Py {

namespace tomo_internal {
struct DummyValueCalculator {
  typedef RealType ValueType;
  template<typename PointType>
  inline ValueType getValue(const PointType & ) const { return 0; }
};
} // tomo_internal

typedef Tomographer::ValueHistogramWithBinningMHRWStatsCollectorParams<
    tomo_internal::DummyValueCalculator, CountIntType, RealType, Eigen::Dynamic, Eigen::Dynamic
  >::Result
  ValueHistogramWithBinningMHRWStatsCollectorResult;



typedef Tomographer::MHRWTasks::MHRandomWalkTaskResult<
    ValueHistogramWithBinningMHRWStatsCollectorResult, CountIntType, RealType
  >
  MHRandomWalkValueHistogramTaskResult;


} // Py











#endif
