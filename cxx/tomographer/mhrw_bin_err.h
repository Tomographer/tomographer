
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
 */
template<typename ValueType_, int NumTrackValues_ = Eigen::Dynamic, int NumLevels_ = Eigen::Dynamic,
         bool StoreBinSums_ = true, typename CountIntType_ = int>
struct BinningAnalysisParams
{
  typedef ValueType_ ValueType;
  typedef CountIntType_ CountIntType;

  static constexpr int NumTrackValuesCTime = NumTrackValues_;
  static constexpr int NumLevelsCTime = NumLevels_;
  static constexpr int NumLevelsPlusOneCTime = (NumLevelsCTime == Eigen::Dynamic
						? Eigen::Dynamic
						: (NumLevelsCTime + 1));
  static constexpr int SamplesSizeCTime =
    tomo_internal::helper_samples_size<NumLevelsCTime, (NumLevelsCTime > 0 && NumLevelsCTime < 7)>::value;
  static constexpr bool StoreBinSums = StoreBinSums_;

  typedef Eigen::Array<ValueType, NumTrackValuesCTime, SamplesSizeCTime> SamplesArray;
  typedef Eigen::Array<ValueType, NumTrackValuesCTime, 1> BinSumArray;
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
 * \tparam Params must be a BinningAnalysisParams-based type
 * \tparam LoggerType a logger type as usual
 *
 * \todo OPTIMIZE THE WAY WE STORE STUFF!!! TAKE MORE IDEAS FROM ALPS'
 * simplebinning.h. [Store sum of squares, not its mean, etc.] ********* Working on
 * this. Check that it's fine????
 */
template<typename Params, typename LoggerType_>
class BinningAnalysis
{
public:
  typedef typename Params::ValueType ValueType;
  typedef typename Params::CountIntType CountIntType;

  static constexpr int NumTrackValuesCTime = Params::NumTrackValuesCTime;
  static constexpr int NumLevelsCTime = Params::NumLevelsCTime;
  static constexpr int NumLevelsPlusOneCTime = Params::NumLevelsPlusOneCTime;
  static constexpr int SamplesSizeCTime = Params::SamplesSizeCTime;
  static constexpr bool StoreBinSums = Params::StoreBinSums;

  typedef typename Params::SamplesArray SamplesArray;
  typedef typename Params::BinSumArray BinSumArray;
  typedef typename Params::BinSumSqArray BinSumSqArray;

  const Tools::static_or_dynamic<int, NumTrackValuesCTime> num_track_values;
  const Tools::static_or_dynamic<int, NumLevelsCTime> num_levels;
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

  inline void reset()
  {
    n_flushes = 0;
    n_samples = 0;
    helper_reset_bin_sum();
    bin_sumsq = BinSumSqArray::Zero(num_track_values(), num_levels()+1);
    logger.longdebug("BinningAnalysis::reset()", "ready to go.");
  }
  
  template<typename CalcValType, bool dummy = true, 
	   typename std::enable_if<dummy && (NumTrackValuesCTime == 1), bool>::type dummy2 = true>
  inline void process_new_value(const CalcValType val)
  {
    // for a single value
    process_new_values(Eigen::Map<const Eigen::Array<CalcValType,1,1> >(&val));
  }

  template<typename Derived>
  inline void process_new_values(const Eigen::DenseBase<Derived> & vals)
  {
    const int ninbin = n_samples % samples_size();

    ++n_samples;

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

private:

  template<bool dummy = true,
           typename std::enable_if<(dummy && StoreBinSums), bool>::type dummy2 = true>
  inline void helper_reset_bin_sum()
  {
    bin_sum.value = BinSumArray::Zero(num_track_values());
  }
  template<bool dummy = true,
           typename std::enable_if<(dummy && !StoreBinSums), bool>::type dummy2 = true>
  inline void helper_reset_bin_sum() { }

  template<bool dummy = true,
           typename std::enable_if<(dummy && StoreBinSums), bool>::type dummy2 = true>
  inline void helper_update_bin_sum(const Eigen::Ref<const Eigen::Array<ValueType, NumTrackValuesCTime, 1> > &
                                    new_samples)
  {
    bin_sum.value += new_samples;
  }
  template<typename Derived,
           bool dummy = true,
           typename std::enable_if<(dummy && !StoreBinSums), bool>::type dummy2 = true>
  inline void helper_update_bin_sum(const Eigen::DenseBase<Derived> & ) { }


public:

  // retrieve results.

  /** \brief Return the number of times the collected samples were flushed.
   *
   * This corresponds to the number of values from which the most coarse-grained binned
   * averaging consists of.
   */
  inline CountIntType get_n_flushes() const { return n_flushes; }

  /** \brief Get the average of each tracked value observed.
   *
   */
  template<bool dummy = true,
           typename std::enable_if<(dummy && StoreBinSums), bool>::type dummy2 = true>
  inline const auto get_bin_means() const -> decltype(BinSumArray() / ValueType(n_samples))
  {
    return bin_sum.value / ValueType(n_samples);
  }

  /** \brief Get the raw average of the squared values observed, for each binning level.
   *
   * The vector <em>bin_sqmeans.col(0)</em> contains the raw average of the squares of the
   * raw values observed, <em>bin_sqmeans.col(1)</em> the raw average of the squares of
   * the values averaged 2 by 2 (i.e. at the first binning level), and so on.
   */
  inline const auto get_bin_sqmeans() const -> decltype(
      bin_sumsq.cwiseQuotient(n_flushes * replicated<NumTrackValuesCTime,1>(
                             powers_of_two<Eigen::Array<ValueType, NumLevelsPlusOneCTime, 1> >(num_levels()+1)
                             .transpose().reverse(),
                             // replicated by:
                             num_track_values(), 1
                             ))
      )
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
   */
  template<bool dummy = true,
           typename std::enable_if<(dummy && StoreBinSums), bool>::type dummy2 = true>
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
  template<bool dummy = true,
           typename std::enable_if<(dummy && StoreBinSums), bool>::type dummy2 = true>
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
  template<bool dummy = true,
           typename std::enable_if<(dummy && StoreBinSums), bool>::type dummy2 = true>
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
   */
  inline Eigen::ArrayXi determine_error_convergence(const Eigen::Ref<const BinSumSqArray> & error_levels)
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
