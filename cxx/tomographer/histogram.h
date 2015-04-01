
#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include <cmath>

#include <Eigen/Core>


template<typename Scalar_>
struct UniformBinsHistogram
{
  typedef Scalar_ Scalar;

  struct Params {
    Params(Scalar min_ = 0.0, Scalar max_ = 1.0, size_t num_bins_ = 50)
      : min(min_), max(max_), num_bins(num_bins_)
    {
    }
    Scalar min;
    Scalar max;
    size_t num_bins;
  };

  Params params;
  Eigen::ArrayXi bins;
  size_t off_chart;

  UniformBinsHistogram(Params p = Params())
    : params(p), bins(p.num_bins), off_chart(0)
  {
  }

  UniformBinsHistogram(Scalar min_, Scalar max_, size_t num_bins)
    : params(min_, max_, num_bins), bins(num_bins), off_chart(0)
  {
  }

  inline void reset()
  {
    bins.setZero();
    off_chart = 0;
  }

  inline bool is_within_bounds(Scalar value)
  {
    //    printf("is_within_bounds(%.4g) : isfinite=%d, min=%.4g, max=%.4g",
    //           value, std::isfinite(value), params.min, params.max);
    return std::isfinite(value) && value >= params.min && value < params.max;
  }
  inline int bin_index(Scalar value)
  {
    if ( !is_within_bounds(value) ) {
      return -1;
    }
    return (int)((value-params.min) / (params.max-params.min) * bins.size());
  }
  inline void record(Scalar value)
  {
    if ( !is_within_bounds(value) ) {
      ++off_chart;
      return;
    }
    ++bins( (int)((value-params.min) / (params.max-params.min) * bins.size()) );
  }


  inline std::string pretty_print(const int max_bar_width = 80) const
  {
    std::string s;
    size_t Ntot = bins.size();
    double barscale = (1.0+bins.maxCoeff()) / max_bar_width; // full bar is 80 chars wide
    for (size_t k = 0; k < Ntot; ++k) {
      s += fmts("%-6.4g | %3d %s\n",
                params.min + k*(params.max-params.min)/Ntot,
                bins(k), std::string((int)(bins(k)/barscale+0.5), '*').c_str());
    }
    if (off_chart > 0) {
      s += fmts("   ... with another %lu points off chart.\n", off_chart);
    }
    return s;
  }

};

























#endif
