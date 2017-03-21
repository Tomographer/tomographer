/* This file is part of the Tomographer project, which is distributed under the
 * terms of the MIT license.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 ETH Zurich, Institute for Theoretical Physics, Philippe Faist
 * Copyright (c) 2017 Caltech, Institute for Quantum Information and Matter, Philippe Faist
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

#include <utility> // std::declval
#include <iostream>
#include <iomanip> // std::setprecision
#include <sstream> // std::stringstream
#include <stdexcept> // std::out_of_range
#include <type_traits> // std::enable_if
#include <algorithm> // std::max

#include <boost/math/constants/constants.hpp>

#include <Eigen/Core>

#include <tomographer/tools/fmt.h>
#include <tomographer/tools/eigenutil.h>
#include <tomographer/tools/cxxutil.h> // TOMOGRAPHER_ENABLED_IF, tomographer_assert(), getWidthForTerminalOutput
#include <tomographer/tools/needownoperatornew.h>


/** \file histogram.h
 *
 * \brief Definitions for Histogram Types.
 *
 * See also the \ref pageInterfaceHistogram.
 */

namespace Tomographer {



/** \brief The parameters of a \ref UniformBinsHistogram
 *
 * This is the \f$[\text{min},\text{max}]\f$ range along with the number of bins.
 */
template<typename Scalar_ = double>
TOMOGRAPHER_EXPORT struct UniformBinsHistogramParams
{
  //! The scalar type used to specify the "value" (horizongal axis) of the histogram
  typedef Scalar_ Scalar;

  //! The obvious constructor
  UniformBinsHistogramParams(Scalar min_ = 0.0, Scalar max_ = 1.0, std::size_t num_bins_ = 50)
    : min(min_), max(max_), num_bins(num_bins_)
  {
  }
  //! Copy constructor, from any other UniformBinsHistogram::Params type.
  template<typename Params2,
           // enforce Params-like type by checking that properties 'min','max','num_bins' exist:
           decltype((int)std::declval<const Params2>().min + (int)std::declval<const Params2>().max
                    + std::declval<Params2>().num_bins) dummyval = 0>
  UniformBinsHistogramParams(const Params2& other)
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
  inline bool isWithinBounds(Scalar value) const
  {
    return std::isfinite(value) && value >= min && value < max;
  }
  /** \brief Returns which bin this value should be counted in (index in the histogram's
   *         \a bins array)
   *
   * \note Raises \a std::out_of_range if the value is not in the range \f$
   * [\text{min},\text{max}[ \f$.
   */
  inline std::size_t binIndex(Scalar value) const
  {
    if ( !isWithinBounds(value) ) {
      throw std::out_of_range(streamstr("UniformBinsHistogramParams: Value "<<value
                                        <<" out of range ["<<min<<","<<max<<"["));
    }
    return binIndexUnsafe(value);
  }
  /** \brief Returns which bin this value should be counted in.
   *
   * This function blindly assumes its argument is within bounds, i.e. it must satisfy
   * <code>isWithinBounds(value)</code>.
   *
   * Use this function only if you're sure the value is within bounds, otherwise call
   * \ref binIndex().
   */
  inline std::size_t binIndexUnsafe(Scalar value) const
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
  inline Scalar binLowerValue(std::size_t index) const
  {
    tomographer_assert(Tools::isPositive(index) && (std::size_t)index < num_bins);
    return min + index * (max-min) / num_bins;
  }
  /** \brief Returns the value which a given bin index represents (center bin value)
   *
   * This is the value at the center of the bin.
   *
   * \note The index must be valid, i.e. <code>index >= 0 && index < num_bins</code>,
   * or you might get an <code>assert()</code> failure in your face.
   */
  inline Scalar binCenterValue(std::size_t index) const
  {
    tomographer_assert(Tools::isPositive(index) && (std::size_t)index < num_bins);
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
  inline Scalar binUpperValue(std::size_t index) const
  {
    tomographer_assert(Tools::isPositive(index) && (std::size_t)index < num_bins);
    return min + (index+1) * (max-min) / num_bins;
  }
  /** \brief Returns the width of a bin
   *
   * This is simply <code>(max - min) / num_bins</code>.
   */
  inline Scalar binResolution() const
  {
    return (max - min) / num_bins;
  }

  /** \brief Return an array of values corresponding to the center of each bin
   */
  inline Eigen::Array<Scalar, Eigen::Dynamic, 1> valuesCenter() const
  {
    const auto halfbinres = boost::math::constants::half<Scalar>() * binResolution();
    return Eigen::Array<Scalar, Eigen::Dynamic, 1>::LinSpaced(num_bins, min+halfbinres, max-halfbinres);
  }

  /** \brief Return an array of values corresponding to the lower value of each bin
   */
  inline Eigen::Array<Scalar, Eigen::Dynamic, 1> valuesLower() const
  {
    return Eigen::Array<Scalar, Eigen::Dynamic, 1>::LinSpaced(num_bins, min, max-binResolution());
  }

  /** \brief Return an array of values corresponding to the upper value of each bin
   */
  inline Eigen::Array<Scalar, Eigen::Dynamic, 1> valuesUpper() const
  {
    return Eigen::Array<Scalar, Eigen::Dynamic, 1>::LinSpaced(num_bins, min+binResolution(), max);
  }
};




/** \brief Stores a histogram
 *
 * Splits the range of values \f$[\text{min},\text{max}]\f$ into \c num_bins number of
 * bins, and keeps counts of how many samples fell in which bin.
 * 
 * Does not store any form of error bars. Complies with the \ref pageInterfaceHistogram.
 */
template<typename Scalar_, typename CountType_ = int>
TOMOGRAPHER_EXPORT class UniformBinsHistogram
  // inheriting from this has some advantages over EIGEN_MAKE_ALIGNED_OPERATOR_NEW, such
  // as not needing to explicitly declare the specialization
  // NeedOwnOperatorNew<UniformBinsHistogram>:
  //
  // -- really not needed because the matrices are dynamically sized
  //
  //  : public virtual Tools::NeedOwnOperatorNew<Eigen::Array<CountType,Eigen::Dynamic,1> >::ProviderType
{
public:
  //! The scalar type of the "X"-axis of the histogram (usually \c double)
  typedef Scalar_ Scalar;

  //! The type that serves to count how many hits in each bin
  typedef CountType_ CountType;

  //! This histogram type does not provide error bars (see \ref pageInterfaceHistogram)
  static constexpr bool HasErrorBars = false;

  //! The type for specifying parameters of this histogram (limits, number of bins)
  typedef UniformBinsHistogramParams<Scalar_> Params;

  //! Parameters of this histogram (range and # of bins)
  Params params;
  //! The counts for each bin
  Eigen::Array<CountType, Eigen::Dynamic, 1> bins;
  //! The number of points that fell outside of the given range
  CountType off_chart;

  //! Constructor: stores the parameters and initializes the histogram to zero counts everywhere
  template<typename Params2 = Params,
           // enforce Params-like type by checking that properties 'min','max','num_bins' exist:
           decltype((int)std::declval<const Params2>().min + (int)std::declval<const Params2>().max
                    + std::declval<Params2>().num_bins) dummyval = 0>
  UniformBinsHistogram(Params2 p = Params())
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
  template<typename HistogramType,// and enforce it's indeed a histogram type by testing its 'HasErrorBars' property:
           TOMOGRAPHER_ENABLED_IF_TMPL(HistogramType::HasErrorBars == 0 || HistogramType::HasErrorBars == 1)>
  UniformBinsHistogram(const HistogramType & other)
    : params(other.params), bins(other.params.num_bins), off_chart(other.off_chart)
  {
    bins = other.bins.template cast<CountType>();
  }

  //! Resets the histogram to zero counts everywhere (including the off-chart counts)
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
   *     dense, have one column and exactly \ref numBins() rows.
   *
   * \param off_chart_ if provided, then set the \ref off_chart count to this
   *     number. Otherwise, reset the \ref off_chart counts to zero.
   */
  template<typename EigenType>
  inline void load(const Eigen::DenseBase<EigenType> & x, CountType off_chart_ = 0)
  {
    tomographer_assert(x.cols() == 1);
    tomographer_assert((std::size_t)x.rows() == params.num_bins);
    bins = x.derived().template cast<CountType>();
    off_chart = off_chart_;
  }

  /** \brief Add data to the histogram.
   *
   * \param x is an Eigen Vector or 1-D Array from which to load data to add to
   *     the histogram counts. It must be (dense) Eigen::ArrayBase-derived type,
   *     have one column and exactly \ref numBins() rows.
   *
   * \param off_chart_ if provided, add this amount to the \ref off_chart counts.
   */
  template<typename EigenType>
  inline void add(const Eigen::ArrayBase<EigenType> & x, CountType off_chart_ = 0)
  {
    // the argument must be of ArrayBase type (as opposed to load() where we can
    // also accept MatrixBase types) because Eigen doesn't allow operator+=
    // between Arrays and Matrices, but has an operator= .
    tomographer_assert(x.cols() == 1);
    tomographer_assert((std::size_t)x.rows() == params.num_bins);
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
    tomographer_assert(x.bins.cols() == 1);
    tomographer_assert((std::size_t)x.bins.rows() == params.num_bins);
    tomographer_assert(std::fabs(x.params.min - params.min) < 1e-8);
    tomographer_assert(std::fabs(x.params.max - params.max) < 1e-8);
    bins += x.bins.template cast<CountType>();
    off_chart += x.off_chart;
  }

  //! Shorthand for <code>params.num_bins</code>
  inline std::size_t numBins() const
  {
    return params.num_bins;
  }

  //! Shorthand for <code>bins(i)</code>
  inline CountType count(std::size_t i) const
  {
    return bins(i);
  }

  //! Shorthand for Params::isWithinBounds()
  inline bool isWithinBounds(Scalar value) const
  {
    return params.isWithinBounds(value);
  }
  //! Shorthand for Params::binIndex()
  inline std::size_t binIndex(Scalar value) const
  {
    return params.binIndex(value);
  }
  //! Shorthand for Params::binLowerValue()
  inline Scalar binLowerValue(int index) const
  {
    return params.binLowerValue(index);
  }
  //! Shorthand for Params::binCenterValue()
  inline Scalar binCenterValue(std::size_t index) const
  {
    return params.binCenterValue(index);
  }
  //! Shorthand for Params::binUpperValue()
  inline Scalar binUpperValue(std::size_t index) const
  {
    return params.binUpperValue(index);
  }

  //! Shorthand for Params::binResolution()
  inline Scalar binResolution() const
  {
    return params.binResolution();
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
    if ( !isWithinBounds(value) ) {
      ++off_chart;
      return std::numeric_limits<std::size_t>::max();
    }
    // calling bin_index_unsafe because we have already checked that value is in range.
    const std::size_t index = params.binIndexUnsafe(value);
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
    if ( !isWithinBounds(value) ) {
      off_chart += weight;
      return std::numeric_limits<std::size_t>::max();
    }
    // calling bin_index_unsafe is safe here because we have already checked that value is
    // in range.
    const std::size_t index = params.binIndexUnsafe(value);
    bins(index) += weight;
    return index;
  }


  /** \brief Calculate the total weight stored in this histogram
   *
   * Calculates the relevant normalization factor by which the bin counts (and possibly
   * error bars) should be divided in order to get a proper normalized probability density
   * on the real line.
   *
   * This function calculates
   * \f[
   *   \mathit{normalization} ~=~ \texttt{off_chart} ~+~
   *       \texttt{binResolution()} \times \sum_i \texttt{bins[}i\texttt{]}
   * \f]
   *
   */
  template<typename NewCountType = decltype(Scalar(1) + CountType(1))>
  inline NewCountType normalization() const
  {
    // DON'T DO NewCountType(binResolution())*NewCountType(bins.sum()) as we may loose
    // precision (if NewCountType=int, for example)
    return NewCountType(off_chart) + NewCountType(binResolution() * bins.sum());
  }

  /** \brief Get a normalized version of this histogram
   *
   * This function divides the bins and the off chart counts by the appropriate
   * normalization() and stores these values into a new returned histogram.
   *
   */
  template<typename NewCountType = Scalar>
  inline UniformBinsHistogram<Scalar, NewCountType> normalized() const
  {
    UniformBinsHistogram<Scalar, NewCountType> h(params);
    const NewCountType f = normalization<NewCountType>();
    h.load(bins.template cast<NewCountType>() / f, NewCountType(off_chart) / f);
    return h;
  }

  /** \brief Pretty-print the histogram and return it as a string with horizontal bars
   *
   * \param max_width is the maximum line width the display should fit in.
   */
  inline std::string prettyPrint(int max_width = 0) const
  {
    return histogramPrettyPrint(*this, max_width);
  }

};




/** \brief Stores a histogram along with error bars
 *
 * Builds on top of \ref UniformBinsHistogram<Scalar,CountType> to store error bars
 * corresponding to each bin.
 */
template<typename Scalar_, typename CountType_ = double>
TOMOGRAPHER_EXPORT class UniformBinsHistogramWithErrorBars
  : public UniformBinsHistogram<Scalar_, CountType_>
  //    public virtual Tools::EigenAlignedOperatorNewProvider -- no need for dynamically-sized matrices
{
public:
  //! The Scalar (X-axis) Type. See UniformBinsHistogram::Scalar.
  typedef Scalar_ Scalar;
  //! The Type used to keep track of counts. See UniformBinsHistogram::CountType.
  typedef CountType_ CountType;

  //! Shortcut for our base class type.
  typedef UniformBinsHistogram<Scalar_, CountType_> Base_;
  /** \brief Shortcut for our base class' histogram parameters. See
   *         UniformBinsHistogram::Params.
   */
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
    delta.resize(Base_::numBins());
    delta.setZero();
  }

  /** \brief For the \ref pageInterfaceHistogram. Get error bar for bin number \a i.
   *
   * This simply returns <code>delta(i)</code>.
   */
  inline CountType errorBar(std::size_t i) const
  {
    return delta(i);
  }

  
  /** \brief Load data for the histogram. Uses current histogram parameters, just sets the bin
   * counts and the error bars.
   *
   * \param d is an Eigen Vector or 1-D Array from which to load the data for the bin
   *     counts. It must be dense, have one column and exactly \ref numBins() rows.
   *
   * \param derr is an Eigen Vector or 1-D Array from which to load the data for the error
   *     bars. It must be dense, have one column and exactly \ref numBins() rows.
   *
   * \param off_chart_ if provided, then set the \ref off_chart count to this
   *     number. Otherwise, reset the \ref off_chart counts to zero.
   */
  template<typename EigenType, typename EigenType2 = EigenType>
  inline void load(const Eigen::DenseBase<EigenType> & d,
                   const Eigen::DenseBase<EigenType2> & derr,
                   CountType off_chart_ = 0)
  {
    Base_::load(d, off_chart_);
    tomographer_assert(derr.cols() == 1);
    tomographer_assert((std::size_t)derr.rows() == params.num_bins);
    delta = derr.derived().template cast<CountType>();
  }


  /** \brief Get a normalized version of this histogram
   *
   * This function divides the bins, the error bars and the off chart counts by the
   * appropriate normalization() and stores these values into a new returned histogram.
   *
   */
  template<typename NewCountType = Scalar>
  inline UniformBinsHistogramWithErrorBars<Scalar, NewCountType> normalized() const
  {
    UniformBinsHistogramWithErrorBars<Scalar, NewCountType> h(params);
    const NewCountType f = Base_::template normalization<NewCountType>();
    h.load(bins.template cast<NewCountType>() / f,
           delta.template cast<NewCountType>() / f,
           NewCountType(off_chart) / f);
    return h;
  }


private:
  /** \brief disable the add() method, which doesn't take care of error bars. To combine
   *         histograms into an averaged histogram, use \ref AveragedHistogram.
   */
  template<typename... Args>
  inline void add(Args && ... )
  {
  }
public:


  /** \brief Print the histogram in human readable form
   *
   * Returns a string which can be e.g. printed to the terminal, which produces a visual
   * representation of the histogram.
   *
   * \param max_width the maximum line length the displayed histogram should fit in.
   */
  std::string prettyPrint(int max_width = 0) const
  {
    return histogramPrettyPrint(*this, max_width);
  }
  
};
// static members:
template<typename Scalar_, typename CountType_>
constexpr bool UniformBinsHistogramWithErrorBars<Scalar_,CountType_>::HasErrorBars;






/** \brief Combines several histograms (with same parameters) into an averaged histogram
 *
 * What this class does exactly is explained on the page \ref pageTheoryAveragedHistogram.
 *
 * The \a HistogramType is expected to be a \ref pageInterfaceHistogram -compliant
 * type. It may, or may not, come with its own error bars. If this is the case, then the
 * error bars are properly combined.
 *
 * You should add histograms to average with repeated calls to \ref addHistogram().  Then,
 * you should call \ref finalize().  Then this object, which inherits \ref
 * UniformBinsHistogramWithErrorBars, will be properly initialized with average counts and
 * error bars.
 *
 * \warning All the histograms added with \ref addHistogram() MUST have the same params,
 *          i.e. bin ranges and number of bins! The bin values are added up regardless of
 *          any parameters, simply because the \ref pageInterfaceHistogram does not expose
 *          any API for querying the params for a general histogram.
 *
 * This class itself complies with the \ref pageInterfaceHistogram.
 *
 * \warning Don't forget to call \ref finalize()! The bin counts in \ref
 *          UniformBinsHistogram::bins "bins", off-chart count \ref off_chart and error
 *          bars \ref delta have UNDEFINED VALUES before calling \ref finalize().
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
TOMOGRAPHER_EXPORT class AveragedHistogram
  : public UniformBinsHistogramWithErrorBars<typename HistogramType_::Scalar, RealAvgType>
{
public:
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
   * This member records how many histograms have been added using \ref addHistogram()
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

  /** \brief Constructs an AveragedHistogram with the given histogram parameters.
   *
   * This overload is provided for convenience and has the same effect as the other
   * constructor.
   */
  AveragedHistogram(Scalar min, Scalar max, std::size_t num_bins)
    : Base_(min, max, num_bins), num_histograms(0)
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

  /** \brief Add a new histogram in the data series
   *
   * Add a new histogram to include into the final averaged histogram.  Call this function
   * repeatedly with different histograms you want to average together.  When you're
   * finished adding histograms, you must call \ref finalize() in order to finalize the
   * averaging; until then the members \ref UniformBinsHistogram::bins "bins" and \ref
   * delta (as well as \ref count(), \ref errorBar() etc.) yield undefined values.
   *
   * \note \a histogram must have the same range and number of bins as the params
   *       specified to the constructor of this class.
   *
   * This implementation deals with the case where the individual histograms (such as \a
   * histogram) don't have themselves error bars.
   */
  TOMOGRAPHER_ENABLED_IF(!HistogramType::HasErrorBars)
  inline void addHistogram(const HistogramType& histogram)
  {
    // bins collects the sum of the histograms
    // delta for now collects the sum of squares. delta will be normalized in finalize().

    tomographer_assert((typename HistogramType::CountType)histogram.numBins() ==
                       (typename HistogramType::CountType)Base_::numBins());

    for (std::size_t k = 0; k < histogram.numBins(); ++k) {
      RealAvgType binvalue = histogram.count(k);
      Base_::bins(k) += binvalue;
      Base_::delta(k) += binvalue * binvalue;
    }

    Base_::off_chart += histogram.off_chart;
    ++num_histograms;
  }
  /** \brief Finalize the averaging procedure
   *
   * Call this function once you have called \ref addHistogram() for all the histograms
   * you wanted to average together.
   *
   * Only after this function was called may you access the averaged histogram data (in
   * \ref UniformBinsHistogram::bins "bins", \ref delta, \ref count() etc.).  Before
   * calling finalize(), those methods return undefined results.
   *
   * Calling \ref addHistogram() after finalize() will yield undefined results.
   *
   * This implementation deals with the case where the individual histograms (such as \a
   * histogram) don't have themselves error bars.
   */
  TOMOGRAPHER_ENABLED_IF(!HistogramType::HasErrorBars)
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

  /** \brief Add a new histogram in the data series
   *
   * Add a new histogram to include into the final averaged histogram.  Call this function
   * repeatedly with different histograms you want to average together.  When you're
   * finished adding histograms, you must call \ref finalize() in order to finalize the
   * averaging; until then the members \ref UniformBinsHistogram::bins "bins" and \ref
   * delta (as well as \ref count(), \ref errorBar() etc.) yield undefined values.
   *
   * \note \a histogram must have the same range and number of bins as the params
   *       specified to the constructor of this class.
   *
   * This implementation deals with the case where the individual histograms (such as \a
   * histogram) do have themselves error bars.
   */
  TOMOGRAPHER_ENABLED_IF(HistogramType::HasErrorBars)
  inline void addHistogram(const HistogramType& histogram)
  {
    // bins collects the sum of the histograms
    // delta for now collects the sum of squares. delta will be normalized in finished().

    tomographer_assert((typename HistogramType::CountType)histogram.numBins() == Base_::numBins());

    for (std::size_t k = 0; k < histogram.numBins(); ++k) {
      RealAvgType binvalue = histogram.count(k);
      Base_::bins(k) += binvalue;
      RealAvgType bindelta = histogram.errorBar(k);
      Base_::delta(k) += bindelta*bindelta;
    }

    Base_::off_chart += histogram.off_chart;
    ++num_histograms;
  }
  /** \brief Finalize the averaging procedure
   *
   * Call this function once you have called \ref addHistogram() for all the histograms
   * you wanted to average together.
   *
   * Only after this function was called may you access the averaged histogram data (in
   * \ref UniformBinsHistogram::bins "bins", \ref delta, \ref count() etc.).  Before
   * calling finalize(), those methods return undefined results.
   *
   * Calling \ref addHistogram() after finalize() will yield undefined results.
   *
   * This implementation deals with the case where the individual histograms (such as \a
   * histogram) do have themselves error bars.
   */
  TOMOGRAPHER_ENABLED_IF(HistogramType::HasErrorBars)
  inline void finalize()
  {
    Base_::bins /= num_histograms;
    Base_::off_chart /= num_histograms;

    Base_::delta = Base_::delta.sqrt();
    Base_::delta /= num_histograms;
  }

};







// -----------------------------------------------------------------------------
// Pretty Print Histogram Utilities
// -----------------------------------------------------------------------------



namespace tomo_internal {
// internal helpers
//
// find maximum value in the list only among those values which are finite (not inf or nan)
//
template<typename Scalar, typename Fn>
inline Scalar max_finite_value(const std::size_t num_items, Fn val_generator,
			       const Scalar default_val = Scalar(1.0))
{
  bool has_first_val = false;
  Scalar max_val = default_val;
  for (std::size_t k = 0; k < num_items; ++k) {
    const Scalar this_val = val_generator(k);
    if (std::isfinite(this_val) && (!has_first_val || this_val > max_val)) {
      max_val = this_val;
      has_first_val = true;
    }
  }
  return max_val;
}

//
// get labels left of histogram (generic HistogramType interface: no information, just bin #)
//
template<typename HistogramType>
struct histogram_pretty_print_label
{
  static inline void getLabels(std::vector<std::string> & labels, const HistogramType & hist)
  {
    labels.resize(hist.numBins());
    for (std::size_t k = 0; k < hist.numBins(); ++k) {
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
  labels.resize(hist.numBins());

  const double max_label_val = std::max(hist.binCenterValue(0), hist.binCenterValue(hist.numBins()-1));
  const int powten = (int)std::floor(std::log10(max_label_val));
  const int relprecision = 4;
  const int precision = (powten > relprecision) ? 0 : (relprecision - powten - 1);

  for (std::size_t k = 0; k < hist.numBins(); ++k) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(precision) << std::right << hist.binCenterValue(k);
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

  TOMOGRAPHER_ENABLED_IF(!HistogramType::HasErrorBars)
  static inline void getStrValues(std::vector<std::string> & svalues, const HistogramType & hist)
  {
    svalues.resize(hist.numBins());

    double max_val = max_finite_value(hist.numBins(), [&](std::size_t k) { return hist.count(k); }, 1.0);

    const int powten = (int)std::floor(std::log10(max_val));
    const int relprecision = 3;
    const int precision = abs_precision_for(powten, relprecision);
    const int w = (precision > 0) ? (precision+powten+1) : (relprecision+2);

    for (std::size_t k = 0; k < hist.numBins(); ++k) {
      std::ostringstream ss;
      ss << std::setprecision(precision) << std::fixed << std::right << std::setw(w)
	 << hist.count(k);
      svalues[k] = ss.str();
    }
  }

  TOMOGRAPHER_ENABLED_IF(HistogramType::HasErrorBars)
  static inline void getStrValues(std::vector<std::string> & svalues, const HistogramType & hist)
  {
    svalues.resize(hist.numBins());

    double max_val = max_finite_value(hist.numBins(), [&](std::size_t k) { return hist.count(k); }, 1.0);

    const int powten = (int)std::floor(std::log10(max_val)); // floor of log_{10}(...)
    const int relprecision = 3;
    const int precision = abs_precision_for(powten, relprecision);
    const int w = (precision > 0) ? (precision+powten+2) : (relprecision+2);

    for (std::size_t k = 0; k < hist.numBins(); ++k) {
      std::ostringstream ss;
      ss << std::setprecision(precision) << std::fixed << std::right << std::setw(w)
	 << hist.count(k) << " +- "
	 << std::setprecision(abs_precision_for(powten-1, relprecision-1)) << std::setw(w)
	 << hist.errorBar(k);
      svalues[k] = ss.str();
    }
  }
private:
  static inline int abs_precision_for(const int powten, const int relprecision)
  {
    return (powten >= relprecision) ? 0 : (relprecision - powten - 1);
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

    labels.resize(hist.numBins());
    svalues.resize(hist.numBins());

    histogram_pretty_print_label<HistogramType>::getLabels(labels, hist);
    histogram_pretty_print_value<HistogramType>::getStrValues(svalues, hist);

    bool has_maxval = false;
    for (std::size_t k = 0; k < hist.numBins(); ++k) {
      const double val = maxval(k);

      if (std::isfinite(val) && (!has_maxval || val > max_value)) {
	max_value = val;
	has_maxval = true;
      }
      if (k == 0 || (int)labels[k].size() > maxlabelwidth) {
	maxlabelwidth = (int)labels[k].size();
      }
      if (k == 0 || (int)svalues[k].size() > maxsvaluewidth) {
	maxsvaluewidth = (int)svalues[k].size();
      }
    }
    if (!has_maxval) {
      max_value = 1.0;
    }

    max_bar_width = max_width - maxlabelwidth - maxsvaluewidth - (int)lsep.size() - (int)rsep.size();
    if (max_bar_width < 2) {
      max_bar_width = 2;
    }
    barscale = ((max_value > 0) ? (max_value / max_bar_width) : 1.0);
  }

  inline int value_to_bar_length(double val) const
  {
    if (val < 0 || !std::isfinite(val)) {
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
    tomographer_assert(vs >= 0);
    tomographer_assert(vs < (int)s.size());
    tomographer_assert(ve >= 0);
    tomographer_assert(ve < (int)s.size());
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

    for (k = 0; k < hist.numBins(); ++k) {
      str << std::setw(maxlabelwidth) << labels[k] << lsep
	  << make_bar(k) << rsep << std::setw(maxsvaluewidth) << svalues[k] << "\n";
    }
  }

private:
  // maxval(k): how much this bar may extend in length
  TOMOGRAPHER_ENABLED_IF(!HistogramType::HasErrorBars)
  inline double maxval(const std::size_t k) const
  {
    return (double)hist.count(k);
  }
  TOMOGRAPHER_ENABLED_IF(HistogramType::HasErrorBars)
  inline double maxval(const std::size_t k) const
  {
    return (double)(hist.count(k) + hist.errorBar(k));
  }
  // make_bar(k): produce the histogram bar in characters...
  TOMOGRAPHER_ENABLED_IF(!HistogramType::HasErrorBars)
  inline std::string make_bar(std::size_t k) const
  {
    std::string sbar(max_bar_width, ' ');
    fill_str_len(sbar, 0.0, hist.count(k), '*', 0, 0);
    return sbar;
  }
  TOMOGRAPHER_ENABLED_IF(HistogramType::HasErrorBars)
  inline std::string make_bar(std::size_t k) const
  {
    std::string sbar(max_bar_width, ' ');
    const double binval = hist.count(k);
    const double binerr = hist.errorBar(k);
    fill_str_len(sbar, 0.0, binval - binerr, '*', '*', '*');
    fill_str_len(sbar, binval - binerr, binval + binerr, '-', '|', '|');
    return sbar;
  }
};

template<typename HistogramType>
inline std::string histogram_short_bar_fmt(const HistogramType & histogram, const bool log_scale,
                                           const int max_width)
{
  std::string s = Tools::fmts("%.2g|", (double)histogram.binLowerValue(0));
  std::string send = Tools::fmts("|%.2g", (double)histogram.binUpperValue(histogram.numBins()-1));
  if (histogram.off_chart > 0) {
    send += Tools::fmts(" [+%.1g off]", (double)histogram.off_chart);
  }
  
  const int maxbarwidth = max_width - (int)s.size() - (int)send.size();
  const int numdiv = (int)(std::ceil((float)histogram.numBins() / maxbarwidth) + 0.5f);
  const int barwidth = (int)(std::ceil((float)histogram.numBins() / numdiv) + 0.5f);

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
      if (i >= (int)chars.size()) { i = (int)chars.size()-1; }
      s += chars[i];
    }
  }

  s += send;
  
  return s;
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
inline void histogramPrettyPrint(std::ostream & str, const HistogramType & histogram, int max_width = 0)
{
  tomographer_assert(Tools::isPositive(histogram.params.num_bins));

  if (histogram.params.num_bins == 0) {
    str << "<empty histogram: no bins>\n";
    return;
  }

  max_width = Tools::getWidthForTerminalOutput(max_width);
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
inline std::string histogramPrettyPrint(const HistogramType & histogram, int max_width = 0)
{
  std::ostringstream ss;
  histogramPrettyPrint<HistogramType>(ss, histogram, max_width);
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
inline int histogramShortBar(std::ostream & str, const HistogramType & histogram,
                             bool log_scale = true, int max_width = 0)
{
  tomographer_assert(Tools::isPositive(histogram.params.num_bins));

  max_width = Tools::getWidthForTerminalOutput(max_width);

  std::string s;
  if (histogram.params.num_bins == 0) {
    s = "<empty histogram: no bins>";
  } else {
    s = tomo_internal::histogram_short_bar_fmt<HistogramType>(histogram, log_scale, max_width);
  }

  str << s;
  return max_width - (int)s.size();
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
inline std::string histogramShortBar(const HistogramType & histogram, bool log_scale = true, int max_width = 0)
{
  tomographer_assert(Tools::isPositive(histogram.params.num_bins));

  if (histogram.params.num_bins == 0) {
    return "<empty histogram: no bins>";
  }

  max_width = Tools::getWidthForTerminalOutput(max_width);
  return tomo_internal::histogram_short_bar_fmt<HistogramType>(histogram, log_scale, max_width);
}


/** \brief Format the histogram as a one-line bar, with some surrounding info
 *
 * Produces a one-line visual representation of the histogram, streamed into an
 * std::ostream.  The bar is surrounded by some info, given as two strings \a head and \a
 * tail. A final newline is inserted.
 *
 * More precisely, this function outputs \a head, then the histogram bar, and then \a
 * tail, while ensuring that the full width of everything reaches exactly \a
 * full_max_width.  If max_width <= 0, then the screen width (or default width) minus the
 * given amount in absolute value is used instead. The negative amount is useful if your
 * line is already occupied by some content.
 *
 * See also \ref histogramShortBarWithInfo(std::string, const HistogramType&, std::string, bool, int)
 */
template<typename HistogramType>
inline void histogramShortBarWithInfo(std::ostream & str,
                                      std::string head,
                                      const HistogramType& hist,
                                      std::string tail,
                                      bool log_scale = true,
                                      int full_max_width = 0)
{
  full_max_width = Tools::getWidthForTerminalOutput(full_max_width);

  str << head;
  const int w = histogramShortBar(str, hist, log_scale, full_max_width - (int)head.size() - (int)tail.size());
  str << std::setw(w + (int)tail.size()) << std::right << tail << "\n";
}

/** \brief Format the histogram as a one-line bar, with some surrounding info
 *
 * Produces a one-line visual representation of the histogram, returned as a string.  The
 * bar is surrounded by some info, given as two strings \a head and \a tail. A final
 * newline is inserted.
 *
 * More precisely, this function outputs \a head, then the histogram bar, and then \a
 * tail, while ensuring that the full width of everything reaches exactly \a
 * full_max_width.  If max_width <= 0, then the screen width (or default width) minus the
 * given amount in absolute value is used instead. The negative amount is useful if your
 * line is already occupied by some content.
 *
 * See also \ref histogramShortBarWithInfo(std::ostream&, std::string, const HistogramType&, std::string, bool, int)
 */
template<typename HistogramType>
inline std::string histogramShortBarWithInfo(std::string head,
                                             const HistogramType& hist,
                                             std::string tail,
                                             bool log_scale = true,
                                             int full_max_width = 0)
{
  std::ostringstream ss;
  histogramShortBarWithInfo(ss, head, hist, tail, log_scale, full_max_width);
  return ss.str();
}







} // namespace Tomographer




#endif
