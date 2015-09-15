/* This file is part of the Tomographer project, which is distributed under the
 * terms of the MIT license.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 ETH Zurich, Institute for Theoretical Physics, Philippe Faist
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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
#include <tomographer/qit/util.h>


/** \file histogram.h
 *
 * \brief Definitions for Histogram Types.
 *
 * See also the \ref pageInterfaceHistogram.
 */

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

  /** \brief Pretty-print the histogram and return it as a string with horizontal bars
   *
   * \param max_width is the maximum line width the display should fit in.
   */
  inline std::string pretty_print(int max_width = 0) const
  {
    return histogram_pretty_print(*this, max_width);
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
  //! The Scalar (X-axis) Type. See \ref UniformBinsHistogram<Scalar_,CountType_>::Scalar.
  typedef Scalar_ Scalar;
  //! The Type used to keep track of counts. See \ref UniformBinsHistogram<Scalar_,CountType_>::CountType.
  typedef CountType_ CountType;

  //! Shortcut for our base class type.
  typedef UniformBinsHistogram<Scalar_, CountType_> Base_;
  /** \brief Shortcut for our base class' histogram parameters. See \ref
   *         UniformBinsHistogram<typename Scalar_, typename CountType_>::Params. */
  typedef typename Base_::Params Params;
  
  //! For the \ref pageInterfaceHistogram. This type of histogram does provide error bars
  static constexpr bool HasErrorBars = true;

  //! The error bars associated with each histogram bin
  Eigen::Array<CountType, Eigen::Dynamic, 1> delta;

  // make these accessible without having to use the "Base_::member" syntax all the time
  using Base_::params;
  using Base_::bins;
  using Base_::off_chart;


  /** \brief Constructor, with given histogram parameters.
   *
   * Constructs an empty histogram with the given parameters. All bin counts, off-chart
   * counts and \ref delta error bars are initialized to zero.
   */
  UniformBinsHistogramWithErrorBars(Params params = Params())
    : Base_(params), delta(Eigen::Array<CountType, Eigen::Dynamic, 1>::Zero(params.num_bins))
  {
  }

  /** \brief Constructor, with given histogram parameters.
   *
   * Constructs an empty histogram with the given parameters. All bin counts, off-chart
   * counts and \ref delta error bars are initialized to zero.
   */
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
  
  /** \brief For the \ref pageInterfaceHistogram. Get error bar for bin number \a i.
   *
   * This simply returns <code>delta(i)</code>.
   */
  inline CountType errorbar(std::size_t i) const
  {
    return delta(i);
  }

  /** \brief Print the histogram in human readable form
   *
   * Returns a string which can be e.g. printed to the terminal, which produces a visual
   * representation of the histogram.
   *
   * \param max_width the maximum line length the displayed histogram should fit in.
   */
  std::string pretty_print(int max_width = 0) const
  {
    return histogram_pretty_print(*this, max_width);
  }
  
};







/** \brief Combines several histograms (with same parameters) into an averaged histogram
 *
 * What this class does exactly is explained on the page \ref pageTheoryAveragedHistogram.
 *
 * The \a HistogramType is expected to be a \ref pageInterfaceHistogram -compliant
 * type. It may, or may not, come with its own error bars. If this is the case, then the
 * error bars are properly combined.
 *
 * You should add histograms to average with repeated calls to \ref add_histogram(). Then,
 * you should call \ref finalize(). Then this object, which inherits \ref
 * UniformBinsHistogramWithErrorBars, will be properly initialized with average counts and
 * error bars.
 *
 * \warning All the histograms added with \ref add_histogram() MUST have the same params,
 *          i.e. bin ranges and number of bins! The bin values are added up regardless of
 *          any parameters, simply because the \ref pageInterfaceHistogram does not expose
 *          any API for querying the params for a general histogram.
 *
 * This class itself complies with the \ref pageInterfaceHistogram.
 *
 * \warning Don't forget to call \ref finalize()! The bin counts \ref bins, off-chart
 *          count \ref off_chart and error bars \ref delta have UNDEFINED VALUES before
 *          calling \ref finalize().
 *
 *
 * \tparam HistgoramType_ the type of the individual histograms that we are
 *         averaging. This must comply with the \ref pageInterfaceHistogram
 *
 * \tparam RealAvgType the real scalar type used for averaging. You'll most likely want to
 *         choose \c double here. This can be different from the underlying \ref
 *         HistogramType's CountType, because that may be an integer type and not suitable
 *         for holding an average.
 */
template<typename HistogramType_, typename RealAvgType = double>
struct AveragedHistogram
  : public UniformBinsHistogramWithErrorBars<typename HistogramType_::Scalar, RealAvgType>
{
  /** \brief Type of the individual histograms we are averaging.
   *
   * This is the argument given as template parameter, and is expected to compily with
   * the \ref pageInterfaceHistogram.
   */
  typedef HistogramType_ HistogramType;
  //! Shortcut for our base class' type.
  typedef UniformBinsHistogramWithErrorBars<typename HistogramType_::Scalar, RealAvgType> Base_;

  //! The histogram parameters' type. See \ref UniformBinsHistogramWithErrorBars::Params
  typedef typename Base_::Params Params;
  //! The histogram's X-axis scalar type. See \ref UniformBinsHistogramWithErrorBars::Scalar
  typedef typename Base_::Scalar Scalar;
  //! The histogram' count type. This is exactly the same as \a RealAvgType.
  typedef typename Base_::CountType CountType;

  //! For the \ref pageInterfaceHistogram. This histogram type does provide error bars.
  static constexpr bool HasErrorBars = true;

  /** \brief The number of histograms averaged.
   *
   * This member records how many histograms have been added using \ref add_histogram()
   * since the object was created or was last \ref reset().
   */
  int num_histograms;


  /** \brief Constructs an AveragedHistogram with the given histogram parameters.
   *
   * The \a params are the histogram parameters (see \ref UniformBinsHistogram::Params) of
   * the individual histograms which will be averaged. Note that all those histograms must
   * have the same params.
   */
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
// get labels left of histogram (for the family of histograms with 'Params':
// UniformBinsHistogram, UniformBinsHistogramWithErrorBars etc.)
//
// common code for several specializations of histogram_pretty_print_label
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
// specialize histogram pretty-print labels using the code above
template<typename Scalar_, typename CountType_>
struct histogram_pretty_print_label<UniformBinsHistogram<Scalar_, CountType_> >
{
  typedef UniformBinsHistogram<Scalar_, CountType_> HistogramType;
  static inline void getLabels(std::vector<std::string> & labels, const HistogramType & hist)
  {
    histogram_get_labels_for_hist_params<HistogramType>(labels, hist);
  }
};
// specialize histogram pretty-print labels using the code above
template<typename Scalar_, typename CountType_>
struct histogram_pretty_print_label<UniformBinsHistogramWithErrorBars<Scalar_, CountType_> >
{
  typedef UniformBinsHistogramWithErrorBars<Scalar_, CountType_> HistogramType;
  static inline void getLabels(std::vector<std::string> & labels, const HistogramType & hist)
  {
    histogram_get_labels_for_hist_params<HistogramType>(labels, hist);
  }
};
// specialize histogram pretty-print labels using the code above
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

//! Internal Helper: format a pretty-print of a histogram.
//
// access this with public API using histogram_pretty_print().
//
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

template<typename HistogramType>
inline std::string histogram_short_bar_fmt(const HistogramType & histogram, const bool log_scale,
                                           const int max_width)
{
  std::string s = Tools::fmts("%.2g|", (double)histogram.bin_lower_value(0));
  std::string send = Tools::fmts("|%.2g", (double)histogram.bin_upper_value(histogram.num_bins()-1));
  if (histogram.off_chart > 0) {
    send += Tools::fmts(" [+%.1g off]", (double)histogram.off_chart);
  }
  
  const int maxbarwidth = max_width - s.size() - send.size();
  const int numdiv = (int)(std::ceil((float)histogram.num_bins() / maxbarwidth) + 0.5f);
  const int barwidth = (int)(std::ceil((float)histogram.num_bins() / numdiv) + 0.5f);

  Eigen::Array<typename HistogramType::CountType,Eigen::Dynamic,1> vec(barwidth);
  Eigen::ArrayXf veclog(barwidth);

  int k;
  float minlogval = 0;
  float maxlogval = 0;
  for (k = 0; k < barwidth; ++k) {
    vec(k) = histogram.bins.segment(numdiv*k, std::min((std::size_t)numdiv,
						       (std::size_t)(histogram.bins.size()-numdiv*k))).sum();
    if (vec(k) > 0) {
      if (log_scale) {
        veclog(k) = std::log((float)vec(k));
      } else {
        veclog(k) = (float)vec(k);
      }
      if (k == 0 || minlogval > veclog(k)) {
	minlogval = veclog(k);
      }
      if (k == 0 || maxlogval < veclog(k)) {
	maxlogval = veclog(k) + 1e-6f;
      }
    } else {
      veclog(k) = 0.f;
    }
  }

  // now, prepare string
  const std::string chars = ".-+ox%#";
  for (k = 0; k < barwidth; ++k) {
    if (vec(k) <= 0) {
      s += ' ';
    } else {
      int i = (int)(chars.size() * (veclog(k) - minlogval) / (maxlogval - minlogval));
      if (i < 0) { i = 0; }
      if (i >= (int)chars.size()) { i = chars.size()-1; }
      s += chars[i];
    }
  }

  s += send;
  
  return s;
}

/** \internal
 *
 * If max_width > 0, return as is.
 *
 * If max_width <= 0, return the width of the screen (or default width), minus the absolute
 * value of the given number. [E.g. if the screen width is 100, then if maxwidth=-4, then
 * return 96.]
 */
int maybe_default_col_width(int max_width = 0)
{
  if (max_width <= 0) {
    const int offset = max_width;
    // decide of a maximum width to display
    max_width = 100; // default maximum width
    // If the user provided a value for the terminal width, use it. Note that $COLUMNS is
    // not in the environment usually, so you have to set it manually with e.g.
    //    shell> export COLUMNS=$COLUMNS
    const char * cols_s = std::getenv("COLUMNS");
    if (cols_s != NULL) {
      max_width = std::atoi(cols_s);
    }
    max_width += offset; // if we had given, e.g. maxwidth=-4
  }
  return max_width;
}

} // namespace tomo_internal


/** \brief pretty-print the given histogram.
 *
 * This overload dumps the pretty print into the ostream \a str.
 *
 * If max_width <= 0, then the screen width (or default width) minus the given amount in
 * absolute value is used. The negative amount is useful if your line is already occupied
 * by some content.
 */
template<typename HistogramType>
inline void histogram_pretty_print(std::ostream & str, const HistogramType & histogram, int max_width = 0)
{
  eigen_assert(Tools::is_positive(histogram.params.num_bins));

  if (histogram.params.num_bins == 0) {
    str << "<empty histogram: no bins>\n";
    return;
  }

  max_width = tomo_internal::maybe_default_col_width(max_width);
  tomo_internal::histogram_pretty_printer<HistogramType>(histogram, max_width).pretty_print(str);
}

/** \brief Utility to display histograms for humans.
 *
 * This version of the function returns a formatted std::string.
 *
 * If max_width <= 0, then the screen width (or default width) minus the given amount in
 * absolute value is used. The negative amount is useful if your line is already occupied
 * by some content.
 */
template<typename HistogramType>
inline std::string histogram_pretty_print(const HistogramType & histogram, int max_width = 0)
{
  std::ostringstream ss;
  histogram_pretty_print<HistogramType>(ss, histogram, max_width);
  return ss.str();
}

/** \brief Format the histogram as a one-line bar.
 *
 * Produces a one-line visual representation of the histogram. The histogram is printed in
 * the given std::ostream.
 *
 * There's no final newline.
 *
 * If max_width <= 0, then the screen width (or default width) minus the given amount in
 * absolute value is used. The negative amount is useful if your line is already occupied
 * by some content.
 *
 * \returns The number of characters which are still left to fill a line with maximal
 *          width. More precisely, the maximum width is first determined from \a max_width
 *          and/or from the \c COLUMNS environment variable. Then histogram is formatted
 *          into a bar which fits in this width. The difference between the maximum width
 *          and the histogram length is returned.
 */
template<typename HistogramType>
inline int histogram_short_bar(std::ostream & str, const HistogramType & histogram,
			       bool log_scale = true, int max_width = 0)
{
  eigen_assert(Tools::is_positive(histogram.params.num_bins));

  max_width = tomo_internal::maybe_default_col_width(max_width);

  std::string s;
  if (histogram.params.num_bins == 0) {
    s = "<empty histogram: no bins>";
  } else {
    s = tomo_internal::histogram_short_bar_fmt<HistogramType>(histogram, log_scale, max_width);
  }

  str << s;
  return max_width - s.size();
}
/** \brief Format the histogram as a one-line bar.
 *
 * Produces a one-line visual representation of the histogram. Returns a \ref std::string.
 *
 * There's no final newline.
 *
 * If max_width <= 0, then the screen width (or default width) minus the given amount in
 * absolute value is used. The negative amount is useful if your line is already occupied
 * by some content.
 */
template<typename HistogramType>
inline std::string histogram_short_bar(const HistogramType & histogram, bool log_scale = true, int max_width = 0)
{
  eigen_assert(Tools::is_positive(histogram.params.num_bins));

  if (histogram.params.num_bins == 0) {
    return "<empty histogram: no bins>";
  }

  max_width = tomo_internal::maybe_default_col_width(max_width);
  return tomo_internal::histogram_short_bar_fmt<HistogramType>(histogram, log_scale, max_width);
}






} // namespace Tomographer




#endif
