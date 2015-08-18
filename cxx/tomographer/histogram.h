
#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include <cmath>
#include <cstdlib>

#include <iostream>
#include <iomanip> // std::setprecision
#include <sstream> // std::stringstream
#include <stdexcept> // std::out_of_range
#include <type_traits> // std::enable_if

#include <boost/math/constants/constants.hpp>

#include <Eigen/Core>

#include <tomographer/tools/fmt.h>


namespace Tomographer {


/** \brief Stores a histogram
 *
 * Splits the range of values \f$[\text{min},\text{max}]\f$ into \c num_bins number of
 * bins, and keeps counts of how many samples fell in which bin.
 * 
 * Does not store any form of error bars. Compiles with the \ref pageInterfaceHistogram.
 */
template<typename Scalar_, typename CountType_ = unsigned int>
struct UniformBinsHistogram
{
  //! The scalar type of the "X"-axis of the histogram (usually \c double)
  typedef Scalar_ Scalar;

  //! The type that serves to count how many hits in each bin
  typedef CountType_ CountType;

  //! This histogram type does not provide error bars (see \ref pageInterfaceHistogram)
  static constexpr bool HasErrorBars = false;

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
    //! Copy constructor, from any other UniformBinsHistogram::Params type.
    template<typename OtherParams/*,
             typename std::enable_if<(tomo_internal::is_histogram_params_type<OtherParams>::value), bool>::type
             dummy2 = true*/>
    Params(const OtherParams& other)
      : min(other.min), max(other.max), num_bins(other.num_bins)
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
      eigen_assert(Tools::is_positive(index) && (std::size_t)index < num_bins);
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
      eigen_assert(Tools::is_positive(index) && (std::size_t)index < num_bins);
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
      eigen_assert(Tools::is_positive(index) && (std::size_t)index < num_bins);
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

  //! Constructor: copy another histogram type
  template<typename HistogramType>
  UniformBinsHistogram(const HistogramType & other)
    : params(other.params), bins(other.params.num_bins), off_chart(other.off_chart)
  {
    bins = other.bins.template cast<CountType>();
  }

  //! Resets the histogram to zero counts everywhere
  inline void reset()
  {
    bins.resize(params.num_bins);
    bins.setZero();
    off_chart = 0;
  }

  /** \brief Load data for the histogram. Uses current histogram parameters, just sets the bin
   * counts.
   *
   * \param x is an Eigen Vector or 1-D Array from which to load the data. It must be
   *     dense, have one column and exactly \ref num_bins() rows.
   *
   * \param off_chart_ if provided, then set the \ref off_chart count to this
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
   * \param x is an Eigen Vector or 1-D Array from which to load data to add to
   *     the histogram counts. It must be (dense) Eigen::ArrayBase-derived type,
   *     have one column and exactly \ref num_bins() rows.
   *
   * \param off_chart_ if provided, add this amount to the \ref off_chart counts.
   */
  template<typename EigenType>
  inline void add(const Eigen::ArrayBase<EigenType> & x, CountType off_chart_ = 0)
  {
    // the argument must be of ArrayBase type (as opposed to load() where we can
    // also accept MatrixBase types) because Eigen doesn't allow operator+=
    // between Arrays and Matrices, but has an operator= .
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
   * \param max_width is the maximum width (in number of characters) a full bar should
   * occupy.
   */
  inline std::string pretty_print(int max_width = 0) const
  {
    return histogram_pretty_print(*this, max_width);
    // eigen_assert(Tools::is_positive(params.num_bins));

    // if (params.num_bins == 0) {
    //   return std::string("<empty histogram: no bins>\n");
    // }

    // if (max_bar_width == 0) {
    //   // decide of a maximum width to display
    //   max_bar_width = 80; // default maximum width
    //   // If the user provided a value for the terminal width, use it. Note that $COLUMNS is
    //   // not in the environment usually, so you have to set it manually with e.g.
    //   //    shell> export COLUMNS=$COLUMNS
    //   const char * cols_s = std::getenv("COLUMNS");
    //   if (cols_s != NULL) {
    // 	max_bar_width = std::atoi(cols_s) - 20;
    //   }
    // }

    // eigen_assert(bins.size() >= 0);
    // std::size_t Ntot = (std::size_t)bins.size();
    // double barscale = 1.0;
    // if (bins.maxCoeff() > 0) {
    //   // if all values are zero, the division later by barscale gives a division by zero
    //   // and crashes stuff
    //   barscale = (double)bins.maxCoeff() / max_bar_width; // full bar is 80 chars wide
    // }
    // std::stringstream s;
    // for (std::size_t k = 0; k < Ntot; ++k) {
    //   //assert(Tools::is_positive(bins(k))); // don't abort() because of this...
    //   eigen_assert(Tools::is_positive(bins(k)));
    //   s.width(6);
    //   s << std::setprecision(4) << std::left << (params.min + k*(params.max-params.min)/Ntot);
    //   s << " | ";
    //   s.width(3);
    //   s << std::setprecision(2) << std::right << bins(k) << " ";
    //   s.width(0);
    //   int strwid = (int)(bins(k)/barscale + 0.5);
    //   if (strwid < 0) {
    // 	strwid = 0;
    //   } else if (strwid > max_bar_width) {
    // 	strwid = max_bar_width;
    //   }
    //   s << std::string(strwid >= 0 ? strwid : 0, '*') << "\n";

    //   //      s += Tools::fmts("%-6.4g | %3ld %s\n",
    //   //		       (double)(params.min + k*(params.max-params.min)/Ntot),
    //   //		       (long)bins(k), std::string((int)(bins(k)/barscale+0.5), '*').c_str());
    // }
    // if (off_chart > 0) {
    //   //      s += Tools::fmts("   ... with another %lu points off chart.\n", (unsigned long)off_chart);
    //   s << "   ... with another " << off_chart << " points off chart.\n";
    // }
    // return s.str();
  }

};



/** \brief Stores a histogram along with error bars
 *
 * Builds on top of \ref UniformBinsHistogram<Scalar,CountType> to store error bars
 * corresponding to each bin.
 */
template<typename Scalar_, typename CountType_ = double>
struct UniformBinsHistogramWithErrorBars : public UniformBinsHistogram<Scalar_, CountType_>
{
  typedef Scalar_ Scalar;
  typedef CountType_ CountType;

  typedef UniformBinsHistogram<Scalar_, CountType_> Base_;
  typedef typename Base_::Params Params;
  
  static constexpr bool HasErrorBars = true;

  //! The error bars associated with each histogram bin
  Eigen::Array<CountType, Eigen::Dynamic, 1> delta;

  // make these accessible without having to use the "Base_::member" syntax all the time
  using Base_::params;
  using Base_::bins;
  using Base_::off_chart;


  UniformBinsHistogramWithErrorBars(Params params = Params())
    : Base_(params), delta(Eigen::Array<CountType, Eigen::Dynamic, 1>::Zero(params.num_bins))
  {
  }

  UniformBinsHistogramWithErrorBars(Scalar min, Scalar max, std::size_t num_bins)
    : Base_(min, max, num_bins), delta(Eigen::Array<CountType, Eigen::Dynamic, 1>::Zero(num_bins))
  {
  }

  /** \brief Resets the histogram to zero counts everywhere, and zero error bars.
   *
   * \warning the corresponding base class has NO virtual methods. So don't see this class
   * inheritance as object polymorphism. If you need to reset a uniform bins histogram
   * with error bars, you need to know its type.
   */
  inline void reset()
  {
    Base_::reset();
    delta.resize(Base_::num_bins());
    delta.setZero();
  }
  

  inline CountType errorbar(std::size_t i) const
  {
    return delta(i);
  }


  std::string pretty_print(int max_width = 0) const
  {
    return histogram_pretty_print(*this, max_width);
    // eigen_assert(Tools::is_positive(params.num_bins));

    // if (max_bar_width == 0) {
    //   // decide of a maximum width to display
    //   max_bar_width = 70; // default maximum width
    //   // If the user provided a value for the terminal width, use it. Note that $COLUMNS is
    //   // not in the environment usually, so you have to set it manually with e.g.
    //   //    shell> export COLUMNS=$COLUMNS
    //   const char * cols_s = std::getenv("COLUMNS");
    //   if (cols_s != NULL) {
    // 	max_bar_width = std::atoi(cols_s) - 30;
    //   }
    // }

    // eigen_assert(bins.size() >= 0);
    // std::size_t Ntot = (std::size_t)bins.size();
    // double barscale = 1.0;
    // if (bins.maxCoeff() > 0) {
    //   // if all values are zero, the division later by barscale gives a division by zero
    //   // and a nan/inf mess
    //   barscale = (double)bins.maxCoeff() / max_bar_width;
    // }
    // auto val_to_bar_len = [max_bar_width,barscale](double val) -> unsigned int {
    //   if (val < 0) {
    // 	val = 0;
    //   }
    //   int l = (int)(val/barscale+0.5);
    //   if (l >= max_bar_width) {
    // 	return max_bar_width-1;
    //   }
    //   return l;
    // };
    // auto fill_str_len = [val_to_bar_len](std::string & s, double valstart, double valend, char c, char cside) {
    //   unsigned int vs = val_to_bar_len(valstart);
    //   unsigned int ve = val_to_bar_len(valend);
    //   eigen_assert(vs < s.size() && ve < s.size());
    //   for (unsigned int j = vs+1; j < ve; ++j) {
    // 	s[j] = c;
    //   }
    //   s[vs] = cside;
    //   s[ve] = cside;
    // };
    
    // std::string s;
    // for (std::size_t k = 0; k < Ntot; ++k) {
    //   eigen_assert(Tools::is_positive(bins(k)));
    //   std::string sline(max_bar_width, ' ');
    //   fill_str_len(sline, 0.0, bins(k) - delta(k), '*', '*');
    //   fill_str_len(sline, bins(k) - delta(k), bins(k) + delta(k), '-', '|');
    //   s += Tools::fmts("%-6.4g | %s    %5.1f +- %5.1f\n",
    // 		       (double)(params.min + k*(params.max-params.min)/Ntot),
    // 		       sline.c_str(),
    // 		       (double)bins(k), (double)delta(k)
    // 	  );
    // }
    // if (off_chart > 1e-6) {
    //   s += Tools::fmts("   ... with another (average) %.4g points off chart.\n", (double)off_chart);
    // }
    // return s;
  }
  
};







/** \brief Combines several histograms (with same parameters) into an averaged histogram
 *
 * The \a HistogramType is expected to be a \ref pageInterfaceHistogram -compliant
 * type. It may, or may not, come with its own error bars. If this is the case, then the
 * error bars are properly combined.
 *
 * \note Check out the source how this works. First call the constructor, or reset(), then
 * call add_histogram() as much as you need, DON'T FORGET TO CALL finalize(), and then you
 * may read out meaningful results in bins, delta, off_chart and
 * num_histograms.
 *  
 */
template<typename HistogramType_, typename RealAvgType = double>
struct AveragedHistogram
  : public UniformBinsHistogramWithErrorBars<typename HistogramType_::Scalar, RealAvgType>
{
  typedef HistogramType_ HistogramType;
  typedef UniformBinsHistogramWithErrorBars<typename HistogramType_::Scalar, RealAvgType> Base_;

  typedef typename Base_::Params Params;
  typedef typename Base_::Scalar Scalar;
  typedef typename Base_::CountType CountType;

  static constexpr bool HasErrorBars = true;

  int num_histograms;

  AveragedHistogram(const Params& params = Params())
    : Base_(params), num_histograms(0)
  {
  }

  /** \brief Resets the data and sets new params.
   *
   * \warning the corresponding base class has NO virtual methods. So don't see this class
   * inheritance as object polymorphism. If you need to reset a uniform bins histogram
   * with error bars, you need to know its type.
   */
  inline void reset(const Params& params_)
  {
    Base_::params = params_;
    Base_::reset();
    num_histograms = 0;
  }

  /** \brief Resets the data keeping the exisiting params.
   *
   * \warning the corresponding base class has NO virtual methods. So don't see this class
   * inheritance as object polymorphism. If you need to reset a uniform bins histogram
   * with error bars, you need to know its type.
   */
  inline void reset()
  {
    Base_::reset();
    num_histograms = 0;
  }

  // ---------------------------------------------------------------------------
  // Implementation in case the added histograms don't have their own error bars
  // ---------------------------------------------------------------------------

  template<bool dummy = true,
           typename std::enable_if<dummy && (!HistogramType::HasErrorBars), bool>::type dummy2 = true>
  inline void add_histogram(const HistogramType& histogram)
  {
    // bins collects the sum of the histograms
    // delta for now collects the sum of squares. delta will be normalized in run_finished().

    eigen_assert((typename HistogramType::CountType)histogram.num_bins() ==
                 (typename HistogramType::CountType)Base_::num_bins());

    for (std::size_t k = 0; k < histogram.num_bins(); ++k) {
      RealAvgType binvalue = histogram.count(k);
      Base_::bins(k) += binvalue;
      Base_::delta(k) += binvalue * binvalue;
    }

    Base_::off_chart += histogram.off_chart;
    ++num_histograms;
  }
  template<bool dummy = true,
           typename std::enable_if<dummy && (!HistogramType::HasErrorBars), bool>::type dummy2 = true>
  inline void finalize()
  {
    Base_::bins /= num_histograms;
    Base_::delta /= num_histograms;
    Base_::off_chart /= num_histograms;

    // delta = sqrt(< X^2 > - < X >^2) / sqrt(Nrepeats-1)
    auto finhist2 = Base_::bins*Base_::bins; // for array, this is c-wise product
    Base_::delta = ( (Base_::delta - finhist2) / (num_histograms-1) ).sqrt();
  }

  // ---------------------------------------------------------------------------
  // Implementation in case the added histograms do have their own error bars
  // ---------------------------------------------------------------------------

  template<bool dummy = true,
           typename std::enable_if<dummy && (HistogramType::HasErrorBars), bool>::type dummy2 = true>
  inline void add_histogram(const HistogramType& histogram)
  {
    // bins collects the sum of the histograms
    // delta for now collects the sum of squares. delta will be normalized in run_finished().

    eigen_assert((typename HistogramType::CountType)histogram.num_bins() == Base_::num_bins());

    for (std::size_t k = 0; k < histogram.num_bins(); ++k) {
      RealAvgType binvalue = histogram.count(k);
      Base_::bins(k) += binvalue;
      RealAvgType bindelta = histogram.errorbar(k);
      Base_::delta(k) += bindelta*bindelta;
    }

    Base_::off_chart += histogram.off_chart;
    ++num_histograms;
  }
  template<bool dummy = true,
           typename std::enable_if<dummy && (HistogramType::HasErrorBars), bool>::type dummy2 = true>
  inline void finalize()
  {
    Base_::bins /= num_histograms;
    Base_::off_chart /= num_histograms;

    Base_::delta = Base_::delta.sqrt();
    Base_::delta /= num_histograms;
  }

};





namespace tomo_internal {
// internal helpers
//
// get labels left of histogram (generic HistogramType interface: no information, just bin #)
template<typename HistogramType>
struct histogram_pretty_print_label
{
  static inline void getLabels(std::vector<std::string> & labels, const HistogramType & hist)
  {
    labels.resize(hist.num_bins());
    for (std::size_t k = 0; k < hist.num_bins(); ++k) {
      labels[k] = std::to_string(k);
    }
  }
};
// get labels left of histogram (UniformBinsHistogram)
template<typename HistogramType>
inline void histogram_get_labels_for_hist_params(std::vector<std::string> & labels, const HistogramType& hist)
{
  labels.resize(hist.num_bins());

  const double max_label_val = std::max(hist.bin_center_value(0), hist.bin_center_value(hist.num_bins()-1));
  const int powten = (int)std::floor(std::log10(max_label_val));
  const int relprecision = 4;
  const int precision = (powten > relprecision) ? 0 : (relprecision - powten - 1);

  for (std::size_t k = 0; k < hist.num_bins(); ++k) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(precision) << std::right << hist.bin_center_value(k);
    labels[k] = ss.str();
  }
}
template<typename Scalar_, typename CountType_>
struct histogram_pretty_print_label<UniformBinsHistogram<Scalar_, CountType_> >
{
  typedef UniformBinsHistogram<Scalar_, CountType_> HistogramType;
  static inline void getLabels(std::vector<std::string> & labels, const HistogramType & hist)
  {
    histogram_get_labels_for_hist_params<HistogramType>(labels, hist);
  }
};
template<typename Scalar_, typename CountType_>
struct histogram_pretty_print_label<UniformBinsHistogramWithErrorBars<Scalar_, CountType_> >
{
  typedef UniformBinsHistogramWithErrorBars<Scalar_, CountType_> HistogramType;
  static inline void getLabels(std::vector<std::string> & labels, const HistogramType & hist)
  {
    histogram_get_labels_for_hist_params<HistogramType>(labels, hist);
  }
};
template<typename BaseHistogramType_, typename RealAvgType_>
struct histogram_pretty_print_label<AveragedHistogram<BaseHistogramType_, RealAvgType_> >
{
  typedef AveragedHistogram<BaseHistogramType_, RealAvgType_> HistogramType;
  static inline void getLabels(std::vector<std::string> & labels, const HistogramType & hist)
  {
    histogram_get_labels_for_hist_params<HistogramType>(labels, hist);
  }
};

// format bin counts nicely
template<typename HistogramType>
struct histogram_pretty_print_value
{
  const HistogramType & hist;
  histogram_pretty_print_value(const HistogramType & hist_) : hist(hist_) { }

  template<bool dummy = true,
	   typename std::enable_if<dummy && !HistogramType::HasErrorBars, bool>::type dummy2 = true>
  static inline void getStrValues(std::vector<std::string> & svalues, const HistogramType & hist)
  {
    svalues.resize(hist.num_bins());

    std::size_t k;
    double max_val = 1.0;
    for (k = 0; k < hist.num_bins(); ++k) {
      if (k == 0 || hist.count(k) > max_val) {
	max_val = hist.count(k);
      }
    }

    const int powten = (int)std::floor(std::log10(max_val));
    const int relprecision = 3;
    const int precision = abs_precision_for(powten, relprecision);
    const int w = (precision > 0) ? (precision+powten+1) : (relprecision+2);

    for (std::size_t k = 0; k < hist.num_bins(); ++k) {
      std::stringstream ss;
      ss << std::setprecision(precision) << std::fixed << std::right << std::setw(w)
	 << hist.count(k);
      svalues[k] = ss.str();
    }
  }
  template<bool dummy = true,
	   typename std::enable_if<dummy && HistogramType::HasErrorBars, bool>::type dummy2 = true>
  static inline void getStrValues(std::vector<std::string> & svalues, const HistogramType & hist)
  {
    svalues.resize(hist.num_bins());

    std::size_t k;
    double max_val = 1.0;
    for (k = 0; k < hist.num_bins(); ++k) {
      if (k == 0 || hist.count(k) > max_val) {
	max_val = hist.count(k);
      }
    }

    const int powten = (int)std::floor(std::log10(max_val)); // floor of log_{10}(...)
    const int relprecision = 3;
    const int precision = abs_precision_for(powten, relprecision);
    const int w = (precision > 0) ? (precision+powten+1) : (relprecision+2);

    for (k = 0; k < hist.num_bins(); ++k) {
      std::stringstream ss;
      ss << std::setprecision(precision) << std::fixed << std::right << std::setw(w)
	 << hist.count(k)
	 << " +- "
	 << std::setprecision(abs_precision_for(powten-1, relprecision-1)) << std::setw(w)
	 << hist.errorbar(k);
      svalues[k] = ss.str();
    }
  }
private:
  static inline int abs_precision_for(const int powten, const int relprecision)
  {
    return (powten > relprecision) ? 0 : (relprecision - powten - 1);
  }
};

template<typename HistogramType>
struct histogram_pretty_printer
{
  const HistogramType & hist;
  const int max_width;

  const std::string lsep;
  const std::string rsep;

  std::vector<std::string> labels;
  int maxlabelwidth;

  std::vector<std::string> svalues;
  int maxsvaluewidth;

  double max_value;

  int max_bar_width;
  double barscale;

  histogram_pretty_printer(const HistogramType & hist_, const int max_width_)
    : hist(hist_), max_width(max_width_), lsep(" |"), rsep("  ")
  {
    // first pass:
    //  -  determine the maximum value attained in the histogram
    //  -  determine maximum width of formatted label & value fields.

    labels.resize(hist.num_bins());
    svalues.resize(hist.num_bins());

    histogram_pretty_print_label<HistogramType>::getLabels(labels, hist);
    histogram_pretty_print_value<HistogramType>::getStrValues(svalues, hist);

    std::size_t k;
    for (k = 0; k < hist.num_bins(); ++k) {
      double val = maxval(k);

      if (k == 0 || val > max_value) {
	max_value = val;
      }
      if (k == 0 || (int)labels[k].size() > maxlabelwidth) {
	maxlabelwidth = labels[k].size();
      }
      if (k == 0 || (int)svalues[k].size() > maxsvaluewidth) {
	maxsvaluewidth = svalues[k].size();
      }
    }

    max_bar_width = max_width - maxlabelwidth - maxsvaluewidth - lsep.size() - rsep.size();
    if (max_bar_width < 2) {
      max_bar_width = 2;
    }
    barscale = ((max_value > 0) ? (max_value / max_bar_width) : 1.0);
  }

  inline int value_to_bar_length(double val) const
  {
    if (val < 0) {
      val = 0;
    }
    int l = (int)(val/barscale+0.5);
    if (l >= max_bar_width) {
      return max_bar_width-1;
    }
    return l;
  }

  inline void fill_str_len(std::string & s, double valstart, double valend,
			   char c, char clside, char crside) const
  {
    int vs = value_to_bar_length(valstart);
    int ve = value_to_bar_length(valend);
    eigen_assert(vs >= 0);
    eigen_assert(vs < (int)s.size());
    eigen_assert(ve >= 0);
    eigen_assert(ve < (int)s.size());
    for (int j = vs; j < ve; ++j) {
      s[j] = c;
    }
    if (clside && crside && clside != crside && vs == ve) {
      if (ve < (int)s.size()-1) {
	++ve;
      } else if (vs > 1) {
	--vs;
      }
    }
    if (clside) {
      s[vs] = clside;
    }
    if (crside) {
      s[ve] = crside;
    }
  }

  inline void pretty_print(std::ostream & str) const
  {
    // perform now second pass:
    //   - display the histogram line by line, with the calculated widths.

    std::size_t k;

    for (k = 0; k < hist.num_bins(); ++k) {
      str << std::setw(maxlabelwidth) << labels[k] << lsep
	  << make_bar(k) << rsep << std::setw(maxsvaluewidth) << svalues[k] << "\n";
    }
  }

private:
  // maxval(k): how much this bar may extend in length
  template<bool dummy = true,
	   typename std::enable_if<dummy && !HistogramType::HasErrorBars, bool>::type dummy2 = true>
  inline double maxval(const std::size_t k) const
  {
    return (double)hist.count(k);
  }
  template<bool dummy = true,
	   typename std::enable_if<dummy && HistogramType::HasErrorBars, bool>::type dummy2 = true>
  inline double maxval(const std::size_t k) const
  {
    return (double)(hist.count(k) + hist.errorbar(k));
  }
  // make_bar(k): produce the histogram bar in characters...
  template<bool dummy = true,
	   typename std::enable_if<dummy && !HistogramType::HasErrorBars, bool>::type dummy2 = true>
  inline std::string make_bar(std::size_t k) const
  {
    std::string sbar(max_bar_width, ' ');
    fill_str_len(sbar, 0.0, hist.count(k), '*', 0, 0);
    return sbar;
  }
  template<bool dummy = true,
	   typename std::enable_if<dummy && HistogramType::HasErrorBars, bool>::type dummy2 = true>
  inline std::string make_bar(std::size_t k) const
  {
    std::string sbar(max_bar_width, ' ');
    const double binval = hist.count(k);
    const double binerr = hist.errorbar(k);
    fill_str_len(sbar, 0.0, binval - binerr, '*', '*', '*');
    fill_str_len(sbar, binval - binerr, binval + binerr, '-', '|', '|');
    return sbar;
  }
};

} // namespace tomo_internal


/** \brief pretty-print the given histogram.
 *
 * This overload dumps the pretty print into the ostream \a str.
 */
template<typename HistogramType>
inline void histogram_pretty_print(std::ostream & str, const HistogramType & histogram, int max_width = 0)
{
  eigen_assert(Tools::is_positive(histogram.params.num_bins));

  if (histogram.params.num_bins == 0) {
    str << "<empty histogram: no bins>\n";
    return;
  }

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

  tomo_internal::histogram_pretty_printer<HistogramType>(histogram, max_width).pretty_print(str);
}

/** \brief Utility to display histograms for humans.
 *
 * This version of the function returns a formatted std::string.
 */
template<typename HistogramType>
inline std::string histogram_pretty_print(const HistogramType & histogram, int max_width = 0)
{
  std::ostringstream ss;
  histogram_pretty_print<HistogramType>(ss, histogram, max_width);
  return ss.str();
}








} // namespace Tomographer




#endif
