
#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include <cmath>

#include <Eigen/Core>


namespace Tomographer {



/** \brief Stores a histogram
 *
 * Splits the range of values \f$[\text{min},\text{max}]\f$ into \c num_bins number of
 * bins, and keeps counts of how many samples fell in which bin.
 */
template<typename Scalar_>
struct UniformBinsHistogram
{
  //! The scalar type of the "X"-axis of the histogram (usually \c double)
  typedef Scalar_ Scalar;

  /** \brief The parameters of a \ref UniformBinsHistogram
   *
   * This is the \f$[\text{min},\text{max}]\f$ range and the number of bins.
   */
  struct Params {
    //! obvious constructor
    Params(Scalar min_ = 0.0, Scalar max_ = 1.0, std::size_t num_bins_ = 50)
      : min(min_), max(max_), num_bins(num_bins_)
    {
    }
    //! Lower range value
    Scalar min;
    //! Upper range value
    Scalar max;
    //! Number of bins to split the range into
    std::size_t num_bins;

    //! Tests whether the given value is in the range of the histogram
    /**
     * \return \c true if \c value is finite (not inf or nan) and within the interval
     * \f$[\text{min},\text{max}[\f$.
     */
    inline bool is_within_bounds(Scalar value) const
    {
      //    printf("is_within_bounds(%.4g) : isfinite=%d, min=%.4g, max=%.4g",
      //           value, std::isfinite(value), params.min, params.max);
      return std::isfinite(value) && value >= min && value < max;
    }
    //! Returns which bin this value should be counted in (index in \ref bins array)
    inline int bin_index(Scalar value) const
    {
      if ( !is_within_bounds(value) ) {
        return -1;
      }
      return (int)((value-min) / (max-min) * num_bins);
    }
    //! Returns the value which a given bin index represents (lower bin value limit)
    inline Scalar bin_lower_value(int index) const
    {
      assert(index >= 0 && (std::size_t)index < num_bins);
      return min + index * (max-min) / num_bins;
    }
    //! Returns the width of a bin
    inline Scalar bin_resolution() const
    {
      return (max - min) / num_bins;
    }
  };

  //! Parameters of this histogram (range and # of bins)
  Params params;
  //! The counts for each bin
  Eigen::ArrayXi bins;
  //! The number of points that fell outside of the given range
  unsigned int off_chart;

  //! Constructor: stores the parameters and initializes the histogram to zero counts everywhere
  UniformBinsHistogram(Params p = Params())
    : params(p), bins(Eigen::ArrayXi::Zero(p.num_bins)), off_chart(0)
  {
  }

  //! Constructor: stores the parameters and initializes the histogram to zero counts everywhere
  UniformBinsHistogram(Scalar min_, Scalar max_, std::size_t num_bins)
    : params(min_, max_, num_bins), bins(Eigen::ArrayXi::Zero(num_bins)), off_chart(0)
  {
  }

  //! Resets the histogram to zero counts everywhere
  inline void reset()
  {
    bins.setZero();
    off_chart = 0;
  }

  //! Shorthand for <code>params->is_within_bounds(value)</code>
  inline bool is_within_bounds(Scalar value) const
  {
    return params.is_within_bounds(value);
  }
  //! Shorthand for <code>params->bin_index(value)</code>
  inline int bin_index(Scalar value) const
  {
    return params.bin_index(value);
  }
  //! Shorthand for <code>params->bin_lower_value(index)</code>
  inline Scalar bin_lower_value(int index) const
  {
    params.bin_lower_value(index);
  }
  //! Shorthand for <code>params->bin_resolution()</code>
  inline Scalar bin_resolution() const
  {
    return params.bin_resolution();
  }

  //! Record a new value in the histogram
  inline void record(Scalar value)
  {
    if ( !is_within_bounds(value) ) {
      ++off_chart;
      return;
    }
    ++bins( (int)((value-params.min) / (params.max-params.min) * bins.size()) );
  }

  //! Pretty-print the histogram and return it as a string with horizontal bars
  /**
   * \param max_bar_width is the maximum width (in number of characters) a full bar should
   * occupy.
   */
  inline std::string pretty_print(const int max_bar_width = 80) const
  {
    std::string s;
    assert(bins.size() >= 0);
    std::size_t Ntot = (std::size_t)bins.size();
    double barscale = (1.0+bins.maxCoeff()) / max_bar_width; // full bar is 80 chars wide
    for (std::size_t k = 0; k < Ntot; ++k) {
      assert(bins(k) >= 0);
      s += fmts("%-6.4g | %3d %s\n",
                params.min + k*(params.max-params.min)/Ntot,
                bins(k), std::string((int)(bins(k)/barscale+0.5), '*').c_str());
    }
    if (off_chart > 0) {
      s += fmts("   ... with another %lu points off chart.\n", (unsigned long)off_chart);
    }
    return s;
  }

};










} // namespace Tomographer




#endif
