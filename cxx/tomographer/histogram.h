
#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include <cmath>
#include <cstdlib>

#include <stdexcept> // std::out_of_range

#include <Eigen/Core>

#include <tomographer/tools/fmt.h>


namespace Tomographer {



/** \brief Stores a histogram
 *
 * Splits the range of values \f$[\text{min},\text{max}]\f$ into \c num_bins number of
 * bins, and keeps counts of how many samples fell in which bin.
 */
template<typename Scalar_, typename CountIntType_ = unsigned int>
struct UniformBinsHistogram
{
  //! The scalar type of the "X"-axis of the histogram (usually \c double)
  typedef Scalar_ Scalar;

  //! The type that serves to count how many hits in each bin
  typedef CountIntType_ CountIntType;

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
      return (std::size_t)((value-min) / (max-min) * num_bins);
    }
    /** \brief Returns the value which a given bin index represents (lower bin value
     * limit)
     *
     * \note The index must be valid, i.e. <code>index >= 0 && index < num_bins</code>,
     * or you might get an <code>assert()</code> failure in your face.
     */
    inline Scalar bin_lower_value(std::size_t index) const
    {
      assert(/*index >= 0 && */ (std::size_t)index < num_bins);
      return min + index * (max-min) / num_bins;
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
  Eigen::Array<CountIntType, Eigen::Dynamic, 1> bins;
  //! The number of points that fell outside of the given range
  CountIntType off_chart;

  //! Constructor: stores the parameters and initializes the histogram to zero counts everywhere
  UniformBinsHistogram(Params p = Params())
    : params(p), bins(Eigen::Array<CountIntType,Eigen::Dynamic,1>::Zero(p.num_bins)),
      off_chart(0)
  {
  }

  //! Constructor: stores the parameters and initializes the histogram to zero counts everywhere
  UniformBinsHistogram(Scalar min_, Scalar max_, std::size_t num_bins)
    : params(min_, max_, num_bins), bins(Eigen::Array<CountIntType,Eigen::Dynamic,1>::Zero(num_bins)),
      off_chart(0)
  {
  }

  //! Resets the histogram to zero counts everywhere
  inline void reset()
  {
    bins.setZero();
    off_chart = 0;
  }

  //! Shorthand for <code>params.num_bins</code>
  inline std::size_t num_bins() const
  {
    return params.num_bins;
  }

  //! Shorthand for <code>bins(i)</code>
  inline CountIntType count(std::size_t i) const
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
    params.bin_lower_value(index);
  }
  //! Shorthand for <code>params.bin_resolution()</code>
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
      assert(Tools::is_positive(bins(k)));
      s += Tools::fmts("%-6.4g | %3d %s\n",
		       params.min + k*(params.max-params.min)/Ntot,
		       bins(k), std::string((int)(bins(k)/barscale+0.5), '*').c_str());
    }
    if (off_chart > 0) {
      s += Tools::fmts("   ... with another %lu points off chart.\n", (unsigned long)off_chart);
    }
    return s;
  }

};



/** \brief Combines several histograms (with same parameters) into an averaged histogram
 *
 * What is expected from \a HistogramType is type Params with field num_bins, and methods
 * num_bins() and count(i).
 *
 * \note Check out the source how this works. First call the constructor, or reset(), then
 * call add_histogram() as much as you need, DON'T FORGET TO CALL finalize(), and then you
 * may read out meaningful results in final_histogram, std_dev, off_chart and
 * num_histograms.
 *  
 */
template<typename HistogramType_, typename RealAvgType = double>
struct AveragedHistogram
{
  typedef HistogramType_ HistogramType;
  typedef typename HistogramType::Params HistogramParamsType;

  HistogramParamsType params;
  Eigen::ArrayXd final_histogram;
  Eigen::ArrayXd std_dev;
  double off_chart;

  int num_histograms;

  AveragedHistogram(const HistogramParamsType& params_ = HistogramParamsType())
    : params(params_)
  {
    reset(params);
  }

  inline void reset(const HistogramParamsType& params_)
  {
    params = params_;
    final_histogram = Eigen::ArrayXd::Zero(params.num_bins);
    std_dev = Eigen::ArrayXd::Zero(params.num_bins);
    off_chart = 0.0;
    num_histograms = 0;
  }

  inline void add_histogram(const HistogramType& histogram)
  {
    // final_histogram collects the sum of the histograms
    // std_dev for now collects the sum of squares. std_dev will be normalized in run_finished().

    eigen_assert(histogram.num_bins() == params.num_bins);
    eigen_assert(histogram.num_bins() == final_histogram.rows());
    eigen_assert(histogram.num_bins() == std_dev.rows());

    for (std::size_t k = 0; k < histogram.num_bins(); ++k) {
      RealAvgType binvalue = histogram.count(k);
      final_histogram(k) += binvalue;
      std_dev(k) += binvalue * binvalue;
    }

    off_chart += histogram.off_chart;
    ++num_histograms;
  }

  inline void finalize()
  {
    final_histogram /= num_histograms;
    std_dev /= num_histograms;
    off_chart /= num_histograms;

    // std_dev = sqrt(< X^2 > - < X >^2) / sqrt(Nrepeats)
    auto finhist2 = final_histogram*final_histogram; // for array, this is c-wise product
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
    assert(final_histogram.size() >= 0);
    std::size_t Ntot = (std::size_t)final_histogram.size();
    // max_width - formatting widths (see below) - some leeway
    const unsigned int max_bar_width = max_width - (6+3+4+5+4+5) - 5;
    double barscale = (1.0+final_histogram.maxCoeff()) / max_bar_width; // full bar is max_bar_width chars wide
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
      assert(final_histogram(k) >= 0);
      std::string sline(max_bar_width, ' ');
      fill_str_len(sline, 0.0, final_histogram(k) - std_dev(k), '*', '*');
      fill_str_len(sline, final_histogram(k) - std_dev(k), final_histogram(k) + std_dev(k), '-', '|');
      
      s += Tools::fmts("%-6.4g | %s    %5.1f +- %5.1f\n",
		       params.min + k*(params.max-params.min)/Ntot,
		       sline.c_str(),
		       final_histogram(k), std_dev(k)
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
