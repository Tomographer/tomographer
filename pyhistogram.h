
#include "common.h"

#include <tomographer2/histogram.h>


namespace Py {

typedef Tomographer::UniformBinsHistogram<RealType, CountIntType> UniformBinsHistogram;

typedef Tomographer::UniformBinsHistogram<RealType, RealType> UniformBinsRealHistogram;

// UniformBinsWithErrorBars
typedef Tomographer::UniformBinsHistogramWithErrorBars<RealType, RealType> UniformBinsHistogramWithErrorBars;


// AveragedSimpleHistogram (underlying histogram w/o error bars)
typedef Tomographer::AveragedHistogram<UniformBinsHistogram, RealType> AveragedSimpleHistogram;

typedef Tomographer::AveragedHistogram<UniformBinsRealHistogram, RealType> AveragedSimpleRealHistogram;

typedef Tomographer::AveragedHistogram<UniformBinsHistogramWithErrorBars, RealType> AveragedErrorBarHistogram;


} // namespace Py


void py_tomo_histogram();
