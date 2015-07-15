
#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include <cmath>
#include <cstdlib>

#include <iostream>
#include <iomanip> // std::setprecision
#include <sstream> // std::stringstream
#include <stdexcept> // std::out_of_range

#include <boost/math/constants/constants.hpp>

#include <Eigen/Core>

#include <tomographer/tools/fmt.h>


namespace Tomographer {



/** \brief Stores a histogram
 *
 * Splits the range of values \f$[\text{min},\text{max}]\f$ into \c num_bins number of
 * bins, and keeps counts of how many samples fell in which bin.
 */
template<typename Scalar_, typename CountType_ = unsigned int>
struct UniformBinsHistogram
{
  //! The scalar type of the "X"-axis of the histogram (usually \c double)
  typedef Scalar_ Scalar;

  //! The type that serves to count how many hits in each bin
  typedef CountType_ CountType;

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

    /** \brief Tests whether the given value is in the range of the histogram
     *
     * \return \c true if \c value is finite (not inf or nan) and within the interval
     * \f$[\text{min},\text{max}[\f$.
     */
    inline bool is_within_bounds(Scalar value) const
    {
      return std::isfinite(value) && value >= min && value < max;
    }
    /** \brief Returns which bin this value should be counted in (index in \ref bins array)
     *
     * \note Raises \a std::out_of_range if the value is not in the range \f$
     * [\text{min},\text{max}[ \f$.
     */
    inline std::size_t bin_index(Scalar value) const
    {
      if ( !is_within_bounds(value) ) {
        throw std::out_of_range(streamstr("UniformBinsHistogram::Params: Value "<<value
					  <<" out of range ["<<min<<","<<max<<"["));
      }
      return bin_index_unsafe(value);
    }
    /** \brief Returns which bin this value should be counted in.
     *
     * This function blindly assumes its argument is within bounds, i.e. it must satisfy
     * <code>is_within_bounds(value)</code>.
     *
     * Use this function only if you're sure the value is within bounds, otherwise call
     * \ref bin_index().
     */
    inline std::size_t bin_index_unsafe(Scalar value) const
    {
      return (std::size_t)((value-min) / (max-min) * num_bins);
    }
    /** \brief Returns the value which a given bin index represents (lower bin value
     * limit)
     *
     * This is the value at the left edge of the bin.
     *
     * \note The index must be valid, i.e. <code>index >= 0 && index < num_bins</code>,
     * or you might get an <code>assert()</code> failure in your face.
     */
    inline Scalar bin_lower_value(std::size_t index) const
    {
      assert(Tools::is_positive(index) && (std::size_t)index < num_bins);
      return min + index * (max-min) / num_bins;
    }
    /** \brief Returns the value which a given bin index represents (center bin value)
     *
     * This is the value at the center of the bin.
     *
     * \note The index must be valid, i.e. <code>index >= 0 && index < num_bins</code>,
     * or you might get an <code>assert()</code> failure in your face.
     */
    inline Scalar bin_center_value(std::size_t index) const
    {
      assert(Tools::is_positive(index) && (std::size_t)index < num_bins);
      return min + (index+boost::math::constants::half<Scalar>()) * (max-min) / num_bins;
    }
    /** \brief Returns the value which a given bin index represents (upper bin value
     * limit)
     *
     * This is the value at the right edge of the bin.
     *
     * \note The index must be valid, i.e. <code>index >= 0 && index < num_bins</code>,
     * or you might get an <code>assert()</code> failure in your face.
     */
    inline Scalar bin_upper_value(std::size_t index) const
    {
      assert(Tools::is_positive(index) && (std::size_t)index < num_bins);
      return min + (index+1) * (max-min) / num_bins;
    }
    /** \brief Returns the width of a bin
     *
     * This is simply <code>(max - min) / num_bins</code>.
     */
    inline Scalar bin_resolution() const
    {
      return (max - min) / num_bins;
    }
  };

  //! Parameters of this histogram (range and # of bins)
  Params params;
  //! The counts for each bin
  Eigen::Array<CountType, Eigen::Dynamic, 1> bins;
  //! The number of points that fell outside of the given range
  CountType off_chart;

  //! Constructor: stores the parameters and initializes the histogram to zero counts everywhere
  UniformBinsHistogram(Params p = Params())
    : params(p), bins(Eigen::Array<CountType,Eigen::Dynamic,1>::Zero(p.num_bins)),
      off_chart(0)
  {
  }

  //! Constructor: stores the parameters and initializes the histogram to zero counts everywhere
  UniformBinsHistogram(Scalar min_, Scalar max_, std::size_t num_bins)
    : params(min_, max_, num_bins), bins(Eigen::Array<CountType,Eigen::Dynamic,1>::Zero(num_bins)),
      off_chart(0)
  {
  }

  //! Resets the histogram to zero counts everywhere
  inline void reset()
  {
    bins.setZero();
    off_chart = 0;
  }

  /** \brief Load data for the histogram. Uses current histogram parameters, just sets the bin
   * counts.
   *
   * \param x is an Eigen Vector or 1-D Array from which to load the data. It must be
   *     dense, have one column and exactly \ref num_bins() rows.
   *
   * \param off_chart if provided, then set the \ref off_chart count to this
   *     number. Otherwise, reset the \ref off_chart counts to zero.
   */
  template<typename EigenType>
  inline void load(const Eigen::DenseBase<EigenType> & x, CountType off_chart_ = 0)
  {
    eigen_assert(x.cols() == 1);
    eigen_assert((std::size_t)x.rows() == params.num_bins);
    bins = x.derived().template cast<CountType>();
    off_chart = off_chart_;
  }

  /** \brief Add data to the histogram.
   *
   * \param x is an Eigen Vector or 1-D Array from which to load data to add to the
   *     histogram counts. It must be dense, have one column and exactly \ref num_bins()
   *     rows.
   *
   * \param off_chart if provided, add this amount to the \ref off_chart counts.
   */
  template<typename EigenType>
  inline void add(const Eigen::DenseBase<EigenType> & x, CountType off_chart_ = 0)
  {
    eigen_assert(x.cols() == 1);
    eigen_assert((std::size_t)x.rows() == params.num_bins);
    bins += x.derived().template cast<CountType>();
    off_chart += off_chart_;
  }

  /** \brief Add data to the histogram.
   *
   * Adds the values contained in the other histogram \a x to the current histogram. This
   * also updates the \ref off_chart counts.
   *
   * \warning The histogram \a x must have the same params as the current one. An
   * assertion check is performed that this is the case (up to some small tolerance).
   */
  template<typename OtherScalar, typename OtherCountType>
  inline void add(const UniformBinsHistogram<OtherScalar,OtherCountType> & x)
  {
    eigen_assert(x.bins.cols() == 1);
    eigen_assert((std::size_t)x.bins.rows() == params.num_bins);
    eigen_assert(std::fabs(x.params.min - params.min) < 1e-8);
    eigen_assert(std::fabs(x.params.max - params.max) < 1e-8);
    bins += x.bins.template cast<CountType>();
    off_chart += x.off_chart;
  }

  //! Shorthand for <code>params.num_bins</code>
  inline std::size_t num_bins() const
  {
    return params.num_bins;
  }

  //! Shorthand for <code>bins(i)</code>
  inline CountType count(std::size_t i) const
  {
    return bins(i);
  }

  //! Shorthand for <code>params.is_within_bounds(value)</code>
  inline bool is_within_bounds(Scalar value) const
  {
    return params.is_within_bounds(value);
  }
  //! Shorthand for <code>params.bin_index(value)</code>
  inline std::size_t bin_index(Scalar value) const
  {
    return params.bin_index(value);
  }
  //! Shorthand for <code>params.bin_lower_value(index)</code>
  inline Scalar bin_lower_value(int index) const
  {
    return params.bin_lower_value(index);
  }
  //! Shorthand for <code>params.bin_center_value(index)</code>
  inline Scalar bin_center_value(std::size_t index) const
  {
    return params.bin_center_value(index);
  }
  //! Shorthand for <code>params.bin_upper_value(index)</code>
  inline Scalar bin_upper_value(std::size_t index) const
  {
    return params.bin_upper_value(index);
  }

  //! Shorthand for <code>params.bin_resolution()</code>
  inline Scalar bin_resolution() const
  {
    return params.bin_resolution();
  }

  /** \brief Record a new value in the histogram
   *
   * This adds one to the bin corresponding to the given \a value.
   *
   * If the value is out of the histogram range, then \a off_chart is incremented by one.
   *
   * Returns the index of the bin in which the value was added, or \a
   * "std::numeric_limits<std::size_t>::max()" if off-chart.
   */
  inline std::size_t record(Scalar value)
  {
    if ( !is_within_bounds(value) ) {
      ++off_chart;
      return std::numeric_limits<std::size_t>::max();
    }
    // calling bin_index_unsafe because we have already checked that value is in range.
    const std::size_t index = params.bin_index_unsafe(value);
    ++bins( index );
    return index;
  }

  /** \brief Record a new value in the histogram, with a certain weight.
   *
   * This adds \a weight to the histogram bin corresponding to the given \a value.
   *
   * If the value is out of the histogram range, then \a off_chart is incremented by \a
   * weight.
   *
   * Returns the index of the bin in which the value was added, or \a
   * "std::numeric_limits<std::size_t>::max()" if off-chart.
   */
  inline std::size_t record(Scalar value, CountType weight)
  {
    if ( !is_within_bounds(value) ) {
      off_chart += weight;
      return std::numeric_limits<std::size_t>::max();
    }
    // calling bin_index_unsafe is safe here because we have already checked that value is
    // in range.
    const std::size_t index = params.bin_index_unsafe(value);
    bins(index) += weight;
    return index;
  }

  //! Pretty-print the histogram and return it as a string with horizontal bars
  /**
   * \param max_bar_width is the maximum width (in number of characters) a full bar should
   * occupy.
   */
  inline std::string pretty_print(int max_bar_width = 0) const
  {
    assert(Tools::is_positive(params.num_bins));

    if (params.num_bins == 0) {
      return std::string("<empty histogram: no bins>\n");
    }

    if (max_bar_width == 0) {
      // decide of a maximum width to display
      max_bar_width = 80; // default maximum width
      // If the user provided a value for the terminal width, use it. Note that $COLUMNS is
      // not in the environment usually, so you have to set it manually with e.g.
      //    shell> export COLUMNS=$COLUMNS
      const char * cols_s = std::getenv("COLUMNS");
      if (cols_s != NULL) {
	max_bar_width = std::atoi(cols_s) - 20;
      }
    }

    assert(bins.size() >= 0);
    std::size_t Ntot = (std::size_t)bins.size();
    double barscale = 1.0;
    if (bins.maxCoeff() > 0) {
      // if all values are zero, the division later by barscale gives a division by zero
      // and crashes stuff
      barscale = (double)bins.maxCoeff() / max_bar_width; // full bar is 80 chars wide
    }
    std::stringstream s;
    for (std::size_t k = 0; k < Ntot; ++k) {
      //assert(Tools::is_positive(bins(k))); // don't abort() because of this...
      s.width(6);
      s << std::setprecision(4) << std::left << (double)(params.min + k*(params.max-params.min)/Ntot);
      s << " | ";
      s.width(3);
      s << std::setprecision(2) << std::right << bins(k) << " ";
      s.width(0);
      int strwid = (int)(bins(k)/barscale + 0.5);
      if (strwid < 0) {
	strwid = 0;
      } else if (strwid > max_bar_width) {
	strwid = max_bar_width;
      }
      s << std::string(strwid >= 0 ? strwid : 0, '*') << "\n";

      //      s += Tools::fmts("%-6.4g | %3ld %s\n",
      //		       (double)(params.min + k*(params.max-params.min)/Ntot),
      //		       (long)bins(k), std::string((int)(bins(k)/barscale+0.5), '*').c_str());
    }
    if (off_chart > 0) {
      //      s += Tools::fmts("   ... with another %lu points off chart.\n", (unsigned long)off_chart);
      s << "   ... with another " << off_chart << " points off chart.\n";
    }
    return s.str();
  }

};



/** \brief Combines several histograms (with same parameters) into an averaged histogram
 *
 * What is expected from \a HistogramType is type Params with field num_bins, and methods
 * num_bins() and count(i).
 *
 * \note Check out the source how this works. First call the constructor, or reset(), then
 * call add_histogram() as much as you need, DON'T FORGET TO CALL finalize(), and then you
 * may read out meaningful results in bins, std_dev, off_chart and
 * num_histograms.
 *  
 */
template<typename HistogramType_, typename RealAvgType = double>
struct AveragedHistogram
{
  typedef HistogramType_ HistogramType;
  typedef typename HistogramType::Params HistogramParamsType;

  HistogramParamsType params;
  Eigen::Array<RealAvgType, Eigen::Dynamic, 1> bins;
  Eigen::Array<RealAvgType, Eigen::Dynamic, 1> std_dev;
  RealAvgType off_chart;

  int num_histograms;

  AveragedHistogram(const HistogramParamsType& params_ = HistogramParamsType())
    : params(params_)
  {
    reset(params);
  }

  //! Resets the data and sets new params.
  inline void reset(const HistogramParamsType& params_)
  {
    params = params_;
    bins = Eigen::Array<RealAvgType, Eigen::Dynamic, 1>::Zero(params.num_bins);
    std_dev = Eigen::Array<RealAvgType, Eigen::Dynamic, 1>::Zero(params.num_bins);
    off_chart = 0.0;
    num_histograms = 0;
  }

  //! Resets the data keeping the exisiting params.
  inline void reset()
  {
    bins = Eigen::Array<RealAvgType, Eigen::Dynamic, 1>::Zero(params.num_bins);
    std_dev = Eigen::Array<RealAvgType, Eigen::Dynamic, 1>::Zero(params.num_bins);
    off_chart = 0.0;
    num_histograms = 0;
  }

  inline void add_histogram(const HistogramType& histogram)
  {
    // bins collects the sum of the histograms
    // std_dev for now collects the sum of squares. std_dev will be normalized in run_finished().

    eigen_assert((typename HistogramType::CountType)histogram.num_bins() == params.num_bins);
    eigen_assert((typename HistogramType::CountType)histogram.num_bins() == bins.rows());
    eigen_assert((typename HistogramType::CountType)histogram.num_bins() == std_dev.rows());

    for (std::size_t k = 0; k < histogram.num_bins(); ++k) {
      RealAvgType binvalue = histogram.count(k);
      bins(k) += binvalue;
      std_dev(k) += binvalue * binvalue;
    }

    off_chart += histogram.off_chart;
    ++num_histograms;
  }

  inline void finalize()
  {
    bins /= num_histograms;
    std_dev /= num_histograms;
    off_chart /= num_histograms;

    // std_dev = sqrt(< X^2 > - < X >^2) / sqrt(Nrepeats)
    auto finhist2 = bins*bins; // for array, this is c-wise product
    std_dev = ( (std_dev - finhist2) / num_histograms ).sqrt();
  }

  std::string pretty_print(int max_width = 0) const
  {
    if (max_width == 0) {
      // decide of a maximum width to display
      max_width = 100; // default maximum width
      // If the user provided a value for the terminal width, use it. Note that $COLUMNS is
      // not in the environment usually, so you have to set it manually with e.g.
      //    shell> export COLUMNS=$COLUMNS
      const char * cols_s = std::getenv("COLUMNS");
      if (cols_s != NULL) {
	max_width = std::atoi(cols_s);
      }
    }

    std::string s;
    assert(bins.size() >= 0);
    std::size_t Ntot = (std::size_t)bins.size();
    // max_width - formatting widths (see below) - some leeway
    const unsigned int max_bar_width = max_width - (6+3+4+5+4+5) - 5;
    double barscale = (1.0+bins.maxCoeff()) / max_bar_width; // full bar is max_bar_width chars wide
    assert(barscale > 0);
    auto val_to_bar_len = [max_bar_width,barscale](double val) -> unsigned int {
      if (val < 0) {
	val = 0;
      }
      unsigned int l = (unsigned int)(val/barscale+0.5);
      if (l >= max_bar_width) {
	return max_bar_width-1;
      }
      return l;
    };
    auto fill_str_len = [val_to_bar_len](std::string & s, double valstart, double valend, char c, char cside) {
      unsigned int vs = val_to_bar_len(valstart);
      unsigned int ve = val_to_bar_len(valend);
      assert(vs < s.size() && ve < s.size());
      for (unsigned int j = vs+1; j < ve; ++j) {
	s[j] = c;
      }
      s[vs] = cside;
      s[ve] = cside;
    };
    
    for (std::size_t k = 0; k < Ntot; ++k) {
      assert(bins(k) >= 0);
      std::string sline(max_bar_width, ' ');
      fill_str_len(sline, 0.0, bins(k) - std_dev(k), '*', '*');
      fill_str_len(sline, bins(k) - std_dev(k), bins(k) + std_dev(k), '-', '|');
      
      s += Tools::fmts("%-6.4g | %s    %5.1f +- %5.1f\n",
		       params.min + k*(params.max-params.min)/Ntot,
		       sline.c_str(),
		       bins(k), std_dev(k)
	  );
    }
    if (off_chart > 1e-6) {
      s += Tools::fmts("   ... with another (average) %.4g points off chart.\n", (double)off_chart);
    }
    return s;
  }
};








} // namespace Tomographer




#endif
