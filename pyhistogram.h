
#include "common.h"

#include <tomographer2/histogram.h>


namespace Py {

typedef Tomographer::UniformBinsHistogram<RealType, CountIntType> UniformBinsHistogram;

// UniformBinsWithErrorBars
typedef Tomographer::UniformBinsHistogramWithErrorBars<RealType, RealType> UniformBinsHistogramWithErrorBars;


// AveragedSimpleHistogram (underlying histogram w/o error bars)
typedef Tomographer::AveragedHistogram<UniformBinsHistogram, RealType> AveragedSimpleHistogram;
// void hlp_ash_addHistogram(AveragedSimpleHistogram & h, const UniformBinsHistogram & other) {
//   h.addHistogram(other);
// }
//void hlp_ash_finalize(AveragedSimpleHistogram & h) { h.finalize(); }
// AveragedErrorBarHistogram (underlying histogram with error bars)
typedef Tomographer::AveragedHistogram<UniformBinsHistogramWithErrorBars, RealType> AveragedErrorBarHistogram;
// void hlp_aebh_addHistogram(AveragedErrorBarHistogram & h, const UniformBinsHistogramWithErrorBars & other) {
//   h.addHistogram(other);
// }
//void hlp_aebh_finalize(AveragedErrorBarHistogram & h) { h.finalize(); }

} // namespace Py


void py_tomo_histogram();
