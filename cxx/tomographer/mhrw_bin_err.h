
#ifndef TOMOGRAPHER_MHRW_BIN_ERR_H
#define TOMOGRAPHER_MHRW_BIN_ERR_H


#include <tomographer/qit/util.h>
#include <tomographer/tools/util.h>
#include <tomographer/tools/loggers.h>

#include <boost/math/constants/constants.hpp>


namespace Tomographer {

namespace tomo_internal {
//
// internal helper to silence "left shift is negative" GCC warnings
//
template<int NumLevels, bool calculate>
struct helper_samples_size {
  enum {
    value = Eigen::Dynamic
  };
};
template<int NumLevels>
struct helper_samples_size<NumLevels,true> {
  enum {
    value = (1 << NumLevels)
  };
};
} // namespace tomo_internal



/** \brief Group template parameters for BinningAnalysis
 *
 * This class serves to group together the template parameters which determine the
 * behavior of \ref BinningAnalysis, as well as provide some common derived types and
 * utility definitions. This more or less acts like a traits class.
 *
 * See \ref pageTheoryBinningAnalysis for how binning analysis works, and \ref
 * BinningAnalysis for the actual implementation.
 *
 * \tparam NumTrackValues_ This class may be used to calculate error bars for different
 *         independent values. This is the number of independent values for which we are
 *         calculating error bars. May be \ref Eigen::Dynamic if this value is not known
 *         at compile-time and will be provided at run-time to the constructor of \ref
 *         BinningAnalysis.
 *
 * \tparam NumLevels_ The number of binning levels we should perform in our analysis. Use
 *         the special value \ref Eigen::Dynamic to specify that this value is not known
 *         at compile-time but will be specified at runtime to the constructor of \ref
 *         BinningAnalysis.
 *
 *         Note that this is the number of coarse-grainings. If you include the 0-th level
 *         which is the raw samples, we obtain <code>(NumLevels+1)</code> data sets. For
 *         example, for <code>NumLevels=1</code>, we have the raw samples and the first
 *         binned sequence.
 *
 * \tparam StoreBinSums_ In some cases, the sum of the samples for each value, needed to
 *         determine the mean values, is already calculated independently (e.g. using a
 *         \ref ValueHistogramMHRWStatsCollector). In this case, we don't need to compute
 *         it separately. Set this template parameter to \a false if you can provide the
 *         bin means directly to \ref BinningAnalysis::calc_error_levels(), in which case
 *         \ref BinningAnalysis won't itself keep track of the means of the samples.
 *
 * \tparam CountIntType_ The integer type used to count the number of samples. Usually you
 *         won't have any problems with \c int, except if you are really collecting a LOT
 *         of samples.
 *
 */
template<typename ValueType_, int NumTrackValues_ = Eigen::Dynamic, int NumLevels_ = Eigen::Dynamic,
         bool StoreBinSums_ = true, typename CountIntType_ = int>
struct BinningAnalysisParams
{
  /** \brief Type of the value which we are calculating error bars for. Also the type of
   *         the error bars themselves. */
  typedef ValueType_ ValueType;
  /** \brief Type used to count the number of samples. Usually \c int is fine, except if
   *         you are really taking a LOT of samples. */
  typedef CountIntType_ CountIntType;

  //! Number of values we are tracking/analyzing, if known at compile-time or \ref Eigen::Dynamic.
  static constexpr int NumTrackValuesCTime = NumTrackValues_;
  //! Number of binning levels in our binning analysis, if known at compile-time or \ref Eigen::Dynamic.
  static constexpr int NumLevelsCTime = NumLevels_;
  //! Number of binning levels in our binning analysis plus one, if known at compile-time or \ref Eigen::Dynamic.
  static constexpr int NumLevelsPlusOneCTime = (NumLevelsCTime == Eigen::Dynamic
						? Eigen::Dynamic
						: (NumLevelsCTime + 1));
  /** \brief Size of the buffer which holds the raw sequence, if of fixed size and known
   *         at compile-time
   *
   * The internal buffer which holds the raw samples has (in this implementation) to hold
   * all the samples until they can be "flushed" up to one sample at the last level. This
   * is exactly \f$ 2^{\mathrm{number\ of\ levels}} \f$. (In fact, we have one such buffer
   * per integration/value we're tracking.)
   *
   * If this number is small enough (~7 binning levels (?)), and is known at compile-time,
   * then the buffer is internally declared as a fixed-size Eigen type; otherwise it is
   * dynamically allocated on the heap.
   *
   * \todo This might change in the future. It would be better to have an "auto-flushing"
   *       mechanism as in ALPS:
   *       https://alps.comp-phys.org/svn/alps1/trunk/alps/src/alps/alea/simplebinning.h
   *       which uses a shorter buffer. This would avoid limiting the number of binning
   *       levels because of memory size.
   *
   * This property contains the size of the buffer if it is of fixed size, i.e. if the
   * binning level is known at compile-time and is not too large. Otherwise, it is set to
   * \ref Eigen::Dynamic.
   */
  static constexpr int SamplesSizeCTime =
    tomo_internal::helper_samples_size<NumLevelsCTime, (NumLevelsCTime > 0 && NumLevelsCTime < 7)>::value;

  /** \brief Whether we should store the bin sums, or whether they can be provided
   *         independently by the user.
   *
   * See class documentation for \ref BinningAnalysis and \ref BinningAnalysisParams.
   */
  static constexpr bool StoreBinSums = StoreBinSums_;

  /** \brief Type of the internal buffer for the raw samples.
   *
   * See \ref SamplesSizeCTime. This is a matrix of sample buffers for each integration
   * value we're tracking.
   *
   * \note This might change in the future, don't rely on this.
   */
  typedef Eigen::Array<ValueType, NumTrackValuesCTime, SamplesSizeCTime> SamplesArray;

  /** \brief Type used to store the sum of values.
   *
   * This is a linear array of fixed size \ref NumTrackValues (or \ref
   * Eigen::Dynamic). This is used to store one value for each tracked value, e.g. the sum
   * of all the samples.
   */
  typedef Eigen::Array<ValueType, NumTrackValuesCTime, 1> BinSumArray;
  /** \brief Type used to store the sum of squares of values at each binning level.
   *
   * This is a matrix of fixed size \ref NumTrackValues x \ref NumLevelsPlusOneCTime (or
   * \ref Eigen::Dynamic). This is used to store a list of values (one for each binning
   * level, including the 0-th) for each tracked value, e.g. the sum of all the samples.
   */
  typedef Eigen::Array<ValueType, NumTrackValuesCTime, NumLevelsPlusOneCTime> BinSumSqArray;

  //! Constants for error bar convergence analysis.  
  enum {
    //! Unable to determine whether the error bars have converged.
    UNKNOWN_CONVERGENCE = 0,
    //! The error bars appear to have converged.
    CONVERGED,
    //! The error bars don't seem to have converged.
    NOT_CONVERGED
  };
  
};

/** \brief Simple binning analysis for determining error bars.
 *
 * This class implements the theoretical procedure described in the theory page \ref
 * pageTheoryBinningAnalysis.
 *
 * This class in fact can perform binning analysis in parallel on several different
 * functions (or values you're integrating). This is useful, for example, to determine
 * error bars and run the binning analysis independently on each bin of a histogram (see
 * \ref ValueHistogramWithBinningMHRWStatsCollector).
 *
 * Several values here may be either specified at compile-time or at run-time, inspired by
 * Eigen's mechanism using \ref Eigen::Dynamic. For example, the number of independent
 * values to track may be fixed at compile-time in the \c NumTrackValues template
 * parameter of \ref BinningAnalysisParams which you specify as the \a Params parameter
 * here; however if the number of values will be determined dynamically at run-time, the
 * \c NumTrackValues template parameter should be set to \ref Eigen::Dynamic.
 *
 * Raw samples are added to the analysis by calling \ref process_new_values() with a list
 * of values, one new value per independent function integration that we are tracking. You
 * should call this function repeatedly, once per sample. This class then takes care of
 * adding those raw samples to an internal samples buffer and flushing this buffer
 * whenever is needed.
 *
 * The final results of the binning analysis can be obtained by calling \ref
 * calc_error_levels() to get the error bars at each binning level. Note that the analysis
 * will only include all the flushed samples, i.e. all samples up to a multiple of \ref
 * samples_size(). If additional samples were given which didn't fill the samples buffer,
 * they are ignored for the error analysis but they are included in \ref get_bin_means()
 * and \ref get_bin_sum().
 *
 * To calculate the error levels, this class also needs to keep track of the sum of all
 * samples to calculate their mean. However, this is often done independently, e.g. with a
 * ValueHistogramMHRWStatsCollector. In order to avoid calculating this twice, you may
 * tell this BinningAnalysis class NOT to worry about calculating the means of the samples
 * and that you will provide the sample means yourself. This is specified using the \a
 * StoreBinSums template parameter to \ref BinningAnalysisParams. In this case, you'll
 * need to use the variants of \ref calc_error_levels() and \ref calc_error_lastlevel()
 * with one argument, where you specify the bin means yourself.
 *
 * You may also access some other information such as the sum of the squares of the
 * samples at different binning levels (see \ref get_bin_sqmeans()).
 *
 * \tparam Params is a \ref BinningAnalysisParams with template parameters. This type
 *         specifies such types and values as the type of the values we're integrating,
 *         the number of independent functions we're integrating, and so on.
 *
 * \tparam LoggerType a logger type as usual, for us to provide information about what
 *         we're doing and how the values look like.
 *
 * \todo Further optimizations. For now, the raw samples are stored until they can be
 *       flushed up to the last level. Change this to do what ALPS' implementation does:
 *       flushing whenever it can, using a much shorter memory buffer, see
 *       https://alps.comp-phys.org/svn/alps1/trunk/alps/src/alps/alea/simplebinning.h .
 *
 */
template<typename Params, typename LoggerType_>
class BinningAnalysis
{
public:
  //! Type of the value(s) for which we are calculating error bars. See \ref BinningAnalysisParams::ValueType.
  typedef typename Params::ValueType ValueType;
  //! Type used to count the number of samples. See \ref BinningAnalysisParams::CountIntType.
  typedef typename Params::CountIntType CountIntType;

  /** \copybrief BinningAnalysisParams::NumTrackValuesCTime.
   * See \ref BinningAnalysisParams::NumTrackValuesCTime. 
   */
  static constexpr int NumTrackValuesCTime = Params::NumTrackValuesCTime;
  /** \copybrief BinningAnalysisParams::NumLevelsCTime.
   * See \ref BinningAnalysisParams::NumLevelsCTime.
   */
  static constexpr int NumLevelsCTime = Params::NumLevelsCTime;
  /** \copybrief BinningAnalysisParams::NumLevelsPlusOneCTime.
   * See \ref BinningAnalysisParams::NumLevelsPlusOneCTime.
   */
  static constexpr int NumLevelsPlusOneCTime = Params::NumLevelsPlusOneCTime;
  /** \copybrief BinningAnalysisParams::SamplesSizeCTime
   * See \ref BinningAnalysisParams::SamplesSizeCTime.
   */
  static constexpr int SamplesSizeCTime = Params::SamplesSizeCTime;
  /** \copybrief BinningAnalysisParams::StoreBinSums
   * See \ref BinningAnalysisParams::StoreBinSums.
   */
  static constexpr bool StoreBinSums = Params::StoreBinSums;

  /** \copybrief BinningAnalysisParams::SamplesArray
   * See \ref BinningAnalysisParams::SamplesArray.
   */
  typedef typename Params::SamplesArray SamplesArray;
  /** \copybrief BinningAnalysisParams::BinSumArray
   * See \ref BinningAnalysisParams::BinSumArray.
   */
  typedef typename Params::BinSumArray BinSumArray;
  /** \copybrief BinningAnalysisParams::BinSumSqArray
   * See \ref BinningAnalysisParams::BinSumSqArray.
   */
  typedef typename Params::BinSumSqArray BinSumSqArray;

  /** \brief The number of functions being tracked/analyzed.
   *
   * The number may be obtained by calling
   * <code>binning_analysis.num_track_values()</code>. This will either return the
   * compile-time fixed value \a NumTrackValuesCTime, or the value which was set
   * dynamically at runtime.
   *
   * \ref Eigen::Dynamic is never returned. See \ref Tools::static_or_dynamic.
   */
  const Tools::static_or_dynamic<int, NumTrackValuesCTime> num_track_values;
  /** \brief The number of levels in the binning analysis.
   *
   * The number may be obtained by calling
   * <code>binning_analysis.num_levels()</code>. This will either return the
   * compile-time fixed value \a NumTrackValuesCTime, or the value which was set
   * dynamically at runtime.
   *
   * \ref Eigen::Dynamic is never returned. See \ref Tools::static_or_dynamic.
   */
  const Tools::static_or_dynamic<int, NumLevelsCTime> num_levels;
  /** \brief The size of our samples buffer. (avoid using, might change in the future) 
   *
   * See \ref BinningAnalysisParams::SamplesSizeCTime. Avoid using this, this might change
   * in the future.
   */
  const Tools::static_or_dynamic<CountIntType, SamplesSizeCTime> samples_size;

  //! Constants for error bar convergence analysis.  
  enum {
    //! Unable to determine whether the error bars have converged.
    UNKNOWN_CONVERGENCE = Params::UNKNOWN_CONVERGENCE,
    //! The error bars appear to have converged.
    CONVERGED = Params::CONVERGED,
    //! The error bars don't seem to have converged.
    NOT_CONVERGED = Params::NOT_CONVERGED
  };

  //! Type of the logger we will be logging debugging messages to.
  typedef LoggerType_ LoggerType;

private:

  /** \brief The array in which we store samples that arrive from the simulation.
   *
   * This array has size \a samples_size() (for each tracking value). Once this array is
   * filled, it is <em>flushed</em>, i.e. the values are processed and stored as
   * appropriate in \ref bin_sum and \ref bin_sumsq.
   *
   * This array has \a num_tracking_values() rows and \a samples_size() columns.
   */
  SamplesArray samples;

  // where we store the flushed values
  
  /** \brief Number of samples seen.
   *
   * This is equal to the number of times \ref process_new_values() was called.
   */
  CountIntType n_samples;
  /** \brief Number of flushes.
   *
   * A flush corresponds to having filled all the samples in the sample vector (of size \a
   * samples_size()), and pushing new values into \ref bin_sum and \ref bin_sumsq.
   */
  CountIntType n_flushes;
  /** \brief Sum of all values seen.
   *
   * This is a column vector of \a num_tracking_values() entries.
   *
   * \note This member is only available if the template parameter \a StoreBinSums is set
   * to \c true.
   *
   * \note values are added to this array as soon as they are seen, not when the samples
   * array is flushed. In particular, if the total number of values is not a multiple of
   * \a samples_size(), then there will be samples counted into \a bin_sum but not into \a
   * bin_sumsq.
   */
  Tools::store_if_enabled<BinSumArray, StoreBinSums> bin_sum;
  /** \brief Sum of the squares of all flushed & processed values, at different binning
   * levels.
   *
   * This is a matrix of \a num_tracking_values() rows and <em>num_levels()+1</em>
   * columns.
   *
   */
  BinSumSqArray bin_sumsq;

  //! Just a boring logger...
  LoggerType & logger;

public:

  /** \brief Constructor.
   *
   * You must specify also the number of values that we will be tracking independently (\a
   * num_track_values) and the number of binning levels (\a num_levels). If compile-time
   * values have been provided as template parameters not equal to \ref Eigen::Dynamic,
   * these values MUST be equal to their compile-time given counterpart values.
   *
   * Specify also a reference to a logger for logging messages. See \ref pageLoggers. 
   *
   */
  BinningAnalysis(int num_track_values_, int num_levels_, LoggerType & logger_)
    : num_track_values(num_track_values_),
      num_levels(num_levels_),
      samples_size(1 << num_levels()),
      samples(num_track_values(), samples_size()),
      n_flushes(0),
      bin_sum(BinSumArray::Zero(num_track_values())),
      bin_sumsq(BinSumSqArray::Zero(num_track_values(), num_levels()+1)),
      logger(logger_)
  {
    assert(Tools::is_positive(num_levels()));
    assert(Tools::is_power_of_two(samples_size()));
    assert( (1<<num_levels()) == samples_size() );

    reset();
  }

  /** \brief Reset this object and start again as if freshly constructed.
   *
   * After calling \a reset(), you may use this object as if you had freshly constructed
   * it. All values are reset to zero and the samples buffer is emptied.
   */
  inline void reset()
  {
    n_flushes = 0;
    n_samples = 0;
    helper_reset_bin_sum();
    bin_sumsq = BinSumSqArray::Zero(num_track_values(), num_levels()+1);
    logger.longdebug("BinningAnalysis::reset()", "ready to go.");
  }
  
  /** \brief Process new raw samples.
   *
   * Call this function whenever you have a new sample with corresponding values. The
   * argument should evaluate to a column vector; each element of the vector corresponds
   * to the value of a function we're tracking. The length of the vector must equal \ref
   * num_track_values().
   *
   * This will add the samples to the internal raw sample buffer, and flush the buffer as
   * required.
   */
  template<typename Derived>
  inline void process_new_values(const Eigen::DenseBase<Derived> & vals)
  {
    const int ninbin = n_samples % samples_size();

    ++n_samples;

    eigen_assert(vals.rows() == num_track_values());
    eigen_assert(vals.cols() == 1);

    // store the new values in the bins  [also if ninbin == 0]
    samples.col(ninbin) = vals;

    // add to our sum of values, if applicable.
    helper_update_bin_sum(samples.col(ninbin));

    // see if we have to flush the bins (equivalent to `ninbin == samples_size()-1`)
    if ( ninbin == samples_size() - 1 ) {
      
      // we have filled all bins. Flush them. Re-use the beginning of the samples[] array
      // to store the reduced bins while flushing them.
      logger.longdebug("BinningAnalysis::process_new_values()", [&](std::ostream & str) {
	  str << "n_samples is now " << n_samples << "; flushing bins. samples_size() = " << samples_size();
	});

      // the size of the samples at the current level of binning. Starts at samples_size,
      // and decreases by half at each higher level.

      for (int level = 0; level <= num_levels(); ++level) {

	const int binnedsize = 1 << (num_levels()-level);

	logger.longdebug("BinningAnalysis::process_new_values()", [&](std::ostream & str) {
	    str << "Processing binning level = " << level << ": binnedsize="<<binnedsize
                << "; n_flushes=" << n_flushes << "\n";
	    str << "\tbinned samples = \n" << samples.block(0,0,num_track_values(),binnedsize);
	  });

	for (int ksample = 0; ksample < binnedsize; ++ksample) {
	  bin_sumsq.col(level) += samples.col(ksample).cwiseProduct(samples.col(ksample));
	  if (ksample % 2 == 0 && binnedsize > 1) {
	    samples.col(ksample/2) = boost::math::constants::half<ValueType>() *
              (samples.col(ksample) + samples.col(ksample+1));
	  }
	}

      }

      logger.longdebug("BinningAnalysis::process_new_values()", [&](std::ostream & str) {
	  str << "Flushing #" << n_flushes << " done. bin_sum is = \n" << bin_sum << "\n"
	      << "\tbin_sumsq is = \n" << bin_sumsq << "\n";
	});

      ++n_flushes;
    }

  }

  /** \brief Process a new value (if we're tracking a single function only)
   *
   * Use this variant of the function if we're tracking a single function only, so that
   * you don't have to specify a 1-element "array" to \ref process_new_values().
   */
  template<typename CalcValType, TOMOGRAPHER_ENABLED_IF_TMPL(NumTrackValuesCTime == 1)>
  inline void process_new_value(const CalcValType val)
  {
    // for a single value
    //process_new_values(Eigen::Map<const Eigen::Array<CalcValType,1,1> >(&val));
    process_new_values(Eigen::Array<CalcValType,1,1>::Constant(val));
  }



private:

  TOMOGRAPHER_ENABLED_IF(StoreBinSums)
  inline void helper_reset_bin_sum()
  {
    bin_sum.value = BinSumArray::Zero(num_track_values());
  }
  TOMOGRAPHER_ENABLED_IF(!StoreBinSums)
  inline void helper_reset_bin_sum() { }

  TOMOGRAPHER_ENABLED_IF(StoreBinSums)
  inline void helper_update_bin_sum(const Eigen::Ref<const Eigen::Array<ValueType, NumTrackValuesCTime, 1> > &
                                    new_samples)
  {
    bin_sum.value += new_samples;
  }
  template<typename Derived,
	   TOMOGRAPHER_ENABLED_IF_TMPL(!StoreBinSums)>
  inline void helper_update_bin_sum(const Eigen::DenseBase<Derived> & ) { }


public:

  // retrieve results.

  /** \brief Return the number of times the collected samples were flushed.
   *
   * This corresponds to the number of values of which the most coarse-grained binned
   * averaging consists of.
   */
  inline CountIntType get_n_flushes() const { return n_flushes; }

  /** \brief Get the average of each tracked value observed.
   *
   */
  TOMOGRAPHER_ENABLED_IF(StoreBinSums)
  inline auto get_bin_means() const
#ifndef TOMOGRAPHER_PARSED_BY_DOXYGEN
    -> decltype(BinSumArray() / ValueType(n_samples))
#endif
  {
    return bin_sum.value / ValueType(n_samples);
  }

  /** \brief Get the raw average of the squared values observed, for each binning level.
   *
   * The vector <em>bin_sqmeans.col(0)</em> contains the raw average of the squares of the
   * raw values observed, <em>bin_sqmeans.col(1)</em> the raw average of the squares of
   * the values averaged 2 by 2 (i.e. at the first binning level), and so on.
   */
  inline auto get_bin_sqmeans() const
#ifndef TOMOGRAPHER_PARSED_BY_DOXYGEN
    -> decltype(
	bin_sumsq.cwiseQuotient(n_flushes * replicated<NumTrackValuesCTime,1>(
				    powers_of_two<Eigen::Array<ValueType, NumLevelsPlusOneCTime, 1> >(num_levels()+1)
				    .transpose().reverse(),
				    // replicated by:
				    num_track_values(), 1
				    ))
	)
#endif
  {
    return bin_sumsq.cwiseQuotient(n_flushes * replicated<NumTrackValuesCTime,1>(
                                  powers_of_two<Eigen::Array<ValueType, NumLevelsPlusOneCTime, 1> >(num_levels()+1)
                                  .transpose().reverse(),
                                  // replicated by:
                                  num_track_values(), 1
                                  ));
  }


  /** \brief Get the sum of each tracked value observed.
   *
   * This is only available if the \a StoreBinSums template parameter was set to \c true.
   */
  TOMOGRAPHER_ENABLED_IF(StoreBinSums)
  inline const BinSumArray & get_bin_sum() const { return bin_sum.value; }

  /** \brief Get the raw sums of the squared values observed, at each binning level.
   *
   * The vector <em>bin_sumsq.col(0)</em> contains the raw sum of the squares of the
   * raw values observed, <em>bin_sumsq.col(1)</em> the raw sum of the squares of
   * the values averaged 2 by 2 (i.e. at the first binning level), and so on.
   */
  inline const BinSumSqArray & get_bin_sumsq() const {
    return bin_sumsq;
  }


  /** \brief Calculate the error bars of samples at different binning levels.
   *
   * Return an array of shape <em>(num_track_values, num_levels)</em> where element
   * <em>(i,k)</em> corresponds to the error of the \a i -th value at binning level \a
   * k. Binning level \a k=0 corresponds to the naive error bar from of the samples (no
   * binning).
   *
   * Use this variant of the function if this class doesn't store the bin means. If so,
   * you need to provide the value of the means explicitly to the parameter \a means.
   */
  template<typename Derived>
  inline BinSumSqArray calc_error_levels(const Eigen::ArrayBase<Derived> & means) const
  {
    eigen_assert(means.rows() == num_track_values());
    eigen_assert(means.cols() == 1);
    const int n_levels_plus_one = num_levels()+1;
    const int n_track_values = num_track_values();

    /** \todo this should be optimizable, using directly bin_sumsq and not effectively
     *        repeating the powers_of_two constants...
     */

    return (
	get_bin_sqmeans() - replicated<1,NumLevelsPlusOneCTime>(
            means.cwiseProduct(means).template cast<ValueType>(),
            // replicated by:
            1, n_levels_plus_one
            )
	).cwiseMax(0).cwiseQuotient(
	    // divide by the number of samples from which these bin-means were obtained, minus one.
	    replicated<NumTrackValuesCTime,1>(
		powers_of_two<Eigen::Array<ValueType, NumLevelsPlusOneCTime, 1> >(n_levels_plus_one)
		.transpose().reverse(),
		// replicated by:
		n_track_values, 1
		) * n_flushes
	    - Eigen::Array<ValueType, NumTrackValuesCTime, NumLevelsPlusOneCTime>::Constant(
		num_track_values(), num_levels()+1,
		1 // the constant...
		)
	    ).cwiseSqrt();
  }


  /** \brief Calculate the error bar of samples (from the last binning level).
   *
   * Return a vector of \a num_track_values elements, where the \a i -th item corresponds
   * to the error bar of the \a i -th value determined from binning level \a num_levels.
   *
   * If the error bars converged (see \ref determine_error_convergence()), this should be
   * a good estimate of the error bars on the corresponding values.
   *
   * Use this variant of the function if this class doesn't store the bin means. If so,
   * you need to provide the value of the means explicitly to the parameter \a means.
   */
  template<typename Derived>
  inline BinSumArray calc_error_lastlevel(const Eigen::ArrayBase<Derived> & means) const {
    eigen_assert(means.rows() == num_track_values());
    eigen_assert(means.cols() == 1);
    return (
	bin_sumsq.col(num_levels()) / ValueType(n_flushes) - means.cwiseProduct(means).template cast<ValueType>()
	).cwiseMax(0).cwiseSqrt() / std::sqrt(ValueType(n_flushes-1));
  }
  
  /** \brief Calculate the error bars of samples at different binning levels.
   *
   * Return an array of shape <em>(num_track_values, num_levels)</em> where element
   * <em>(i,k)</em> corresponds to the error of the \a i -th value at binning level \a
   * k. Binning level \a k=0 corresponds to the naive error bar from of the samples (no
   * binning).
   *
   * Use this variant of the function if this class stores the bin means. If this is not
   * the case, you will need to call the variant \ref calc_error_levels(const
   * Eigen::ArrayBase<Derived> & means) with the values of the means.
   */
  TOMOGRAPHER_ENABLED_IF(StoreBinSums)
  inline BinSumSqArray calc_error_levels() const {
    BinSumArray means = get_bin_means();
    return calc_error_levels(means);
  }

  /** \brief Calculate the error bar of samples (from the last binning level).
   *
   * Return a vector of \a num_track_values elements, where the \a i -th item corresponds
   * to the error bar of the \a i -th value determined from binning level \a num_levels.
   *
   * If the error bars converged (see \ref determine_error_convergence()), this should be
   * a good estimate of the error bars on the corresponding values.
   *
   * Use this variant of the function if this class stores the bin means. If this is not
   * the case, you will need to call the variant \ref calc_error_lastlevel(const
   * Eigen::ArrayBase<Derived> & means) with the values of the means.
   */
  TOMOGRAPHER_ENABLED_IF(StoreBinSums)
  inline BinSumArray calc_error_lastlevel() const {
    BinSumArray means = get_bin_means();
    return calc_error_lastlevel(means);
  }
  
  /** \brief Attempt to determine if the error bars have converged.
   *
   * Call this method after calculating the error bars for each level with \ref
   * calc_error_levels(). Use the return value of that function to feed in the input
   * here.
   *
   * \returns an array of integers, of length \a num_track_values, each set to one of \ref
   * CONVERGED, \ref NOT_CONVERGED or \ref CONVERGENCE_UNKNOWN.
   *
   * Contains code inspired by ALPS project, see
   * <a href="https://alps.comp-phys.org/svn/alps1/trunk/alps/src/alps/alea/simplebinning.h">https://alps.comp-phys.org/svn/alps1/trunk/alps/src/alps/alea/simplebinning.h</a>.
   */
  inline Eigen::ArrayXi determine_error_convergence(const Eigen::Ref<const BinSumSqArray> & error_levels) const
  {
    Eigen::ArrayXi converged_status(num_track_values()); // RVO will help

    eigen_assert(error_levels.rows() == num_track_values());
    eigen_assert(error_levels.cols() == num_levels() + 1);

    logger.longdebug("BinningAnalysis::determine_error_convergence", [&](std::ostream & str) {
	str << "error_levels = \n" << error_levels << "\n";
      });

    // verify that indeed the errors have converged. Inspired from ALPS code, see
    // https://alps.comp-phys.org/svn/alps1/trunk/alps/src/alps/alea/simplebinning.h

    const int range = 4;
    if (num_levels() < range-1) {

      converged_status = Eigen::ArrayXi::Constant(num_track_values(), UNKNOWN_CONVERGENCE);

    } else {

      converged_status = Eigen::ArrayXi::Constant(num_track_values(), CONVERGED);

      const auto & errors = error_levels.col(num_levels());

      for (int level = num_levels()+1 - range; level < num_levels(); ++level) {

	const auto & errors_thislevel = error_levels.col(level);

	logger.longdebug("BinningAnalysis::determine_error_convergence", [&](std::ostream & str) {
	    str << "About to study level " << level << ": at this point, converged_status = \n"
		<< converged_status << "\nand errors_thislevel = \n" << errors_thislevel;
	  });

	for (int val_it = 0; val_it < num_track_values(); ++val_it) {
	  if (errors_thislevel(val_it) >= errors(val_it) &&
	      converged_status(val_it) != NOT_CONVERGED) {
	    converged_status(val_it) = CONVERGED;
	  } else if (errors_thislevel(val_it) < 0.824 * errors(val_it)) {
	    converged_status(val_it) = NOT_CONVERGED;
	  } else if ((errors_thislevel(val_it) < 0.9 * errors(val_it)) &&
		     converged_status(val_it) != NOT_CONVERGED) {
	    converged_status(val_it) = UNKNOWN_CONVERGENCE;
	  }
	}

      }

    }

    logger.longdebug("BinningAnalysis::determine_error_convergence", [&](std::ostream & str) {
	str << "Done. converged_status [0=UNNOWN,1=CONVERGED,2=NOT CONVERGED] = \n"
	    << converged_status;
      });

    return converged_status;
  }
};



} // namespace Tomographer















#endif
