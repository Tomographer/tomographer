
#ifndef TOMOGRAPHER_MHRW_BIN_ERR_H
#define TOMOGRAPHER_MHRW_BIN_ERR_H


#include <tomographer/qit/util.h>
#include <tomographer/tools/util.h>
#include <tomographer/tools/loggers.h>


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


/** \brief Simple binning analysis for determining error bars.
 *
 */
template<typename ValueType_, typename LoggerType_,
	 int NumTrackValues_ = Eigen::Dynamic, int NumLevels_ = Eigen::Dynamic,
         bool StoreBinMeans_ = true, typename CountIntType_ = int>
class BinningAnalysis
{
public:
  typedef ValueType_ ValueType;
  typedef CountIntType_ CountIntType;

  static constexpr int NumTrackValuesCTime = NumTrackValues_;
  static constexpr int NumLevelsCTime = NumLevels_;
  static constexpr int NumLevelsPlusOneCTime = (NumLevelsCTime == Eigen::Dynamic
						? Eigen::Dynamic
						: (NumLevelsCTime + 1));
  static constexpr int SamplesSizeCTime =
    tomo_internal::helper_samples_size<NumLevelsCTime, (NumLevelsCTime > 0 && NumLevelsCTime < 7)>::value;
  static constexpr bool StoreBinMeans = StoreBinMeans_;

  typedef Eigen::Array<ValueType, NumTrackValuesCTime, SamplesSizeCTime> SamplesArray;
  typedef Eigen::Array<ValueType, NumTrackValuesCTime, 1> BinMeansArray;
  typedef Eigen::Array<ValueType, NumTrackValuesCTime, NumLevelsPlusOneCTime> BinSqMeansArray;

  const Tools::static_or_dynamic<int, NumTrackValuesCTime> num_track_values;
  const Tools::static_or_dynamic<int, NumLevelsCTime> num_levels;
  const Tools::static_or_dynamic<CountIntType, SamplesSizeCTime> samples_size;

  //! Constants for error bar convergence analysis.  
  enum {
    //! Unable to determine whether the error bars have converged.
    UNKNOWN_CONVERGENCE = 0,
    //! The error bars appear to have converged.
    CONVERGED,
    //! The error bars don't seem to have converged.
    NOT_CONVERGED
  };

  typedef LoggerType_ LoggerType;

private:

  SamplesArray samples; // shape = (num_tracking, num_samples)

  // where we store the flushed values
  CountIntType n_flushes;
  Tools::store_if_enabled<BinMeansArray, StoreBinMeans> bin_means;   // shape = (num_tracking, 1)
  BinSqMeansArray bin_sqmeans; // shape = (num_tracking, num_levels+1)

  LoggerType & logger;

public:

  BinningAnalysis(int num_track_values_, int num_levels_, LoggerType & logger_)
    : num_track_values(num_track_values_),
      num_levels(num_levels_),
      samples_size(1 << num_levels()),
      samples(num_track_values(), samples_size()),
      n_flushes(0),
      bin_means(BinMeansArray::Zero(num_track_values())),
      bin_sqmeans(BinSqMeansArray::Zero(num_track_values(), num_levels()+1)),
      logger(logger_)
  {
    assert(Tools::is_positive(num_levels()));
    assert(Tools::is_power_of_two(samples_size()));
    assert(std::fabs(std::pow(num_levels(), 2) - samples_size()) < 1e-8);

    reset();
  }

  inline void reset()
  {
    n_flushes = 0;
    helper_reset_bin_means();
    bin_sqmeans = BinSqMeansArray::Zero(num_track_values(), num_levels());
    logger.longdebug("BinningAnalysis::reset()", "ready to go.");
  }
  
  template<typename CalcValType, bool dummy = true, 
	   typename std::enable_if<dummy && (NumTrackValuesCTime == 1), bool>::type dummy2 = true>
  inline void process_new_value(const CountIntType k, const CalcValType val)
  {
    // for a single value
    process_new_values(k, Eigen::Map<const Eigen::Array<CalcValType,1,1> >(&val));
  }

  template<typename Derived>
  inline void process_new_values(const CountIntType k, const Eigen::DenseBase<Derived> & vals)
  {
    const int ninbin = k % samples_size();

    // store the new values in the bins  [also if ninbin == 0]
    samples.col(ninbin) = vals;

    // see if we have to flush the bins (equivalent to `ninbin == samples_size()-1`)
    if ( (k+1) % samples_size() == 0) {
      
      // we have filled all bins. Flush them. Re-use the beginning of the samples[] array
      // to store the reduced bins while flushing them.
      logger.longdebug("BinningAnalysis::process_new_values()", [&](std::ostream & str) {
	  str << "Reached k = " << k << "; flushing bins. samples_size() = " << samples_size();
	});

      // the size of the samples at the current level of binning. Starts at samples_size,
      // and decreases by half at each higher level.

      for (int level = 0; level <= num_levels(); ++level) {

	const int binnedsize = 1 << (num_levels()-level);

	// mean and sqmean are already averages over this number of stored values [we'll
	// add another `binnedsize` now]
	const int n = binnedsize * n_flushes; 

	logger.longdebug("BinningAnalysis::process_new_values()", [&](std::ostream & str) {
	    str << "Processing binning level = " << level << ": binnedsize="<<binnedsize<<", "
		<< "n=" << n << "; n_flushes=" << n_flushes << "\n";
	    str << "\tbinned samples = \n" << samples.block(0,0,num_track_values(),binnedsize);
	  });

	for (int ksample = 0; ksample < binnedsize; ++ksample) {
	  bin_sqmeans.col(level) = (samples.col(ksample).cwiseProduct(samples.col(ksample))
				   + (n+ksample) * bin_sqmeans.col(level)) / (n+ksample+1);
	  if (ksample % 2 == 0 && binnedsize > 1) {
	    samples.col(ksample/2) = 0.5*(samples.col(ksample)+samples.col(ksample+1));
	  }
	}

      }

      //bin_means = (samples.col(0) + (n_flushes)*bin_means) / (n_flushes+1);
      helper_update_bin_means();

      logger.longdebug("BinningAnalysis::process_new_values()", [&](std::ostream & str) {
	  str << "Flushing #" << n_flushes << " done. bin_means is = \n" << bin_means << "\n"
	      << "\tbin_sqmeans is = \n" << bin_sqmeans << "\n";
	});

      ++n_flushes;
    }

  }
private:

  template<bool dummy = true,
           typename std::enable_if<(dummy && StoreBinMeans), bool>::type dummy2 = true>
  inline void helper_reset_bin_means()
  {
    bin_means.value = BinMeansArray::Zero(num_track_values());
  }
  template<bool dummy = true,
           typename std::enable_if<(dummy && !StoreBinMeans), bool>::type dummy2 = true>
  inline void helper_reset_bin_means() { }

  template<bool dummy = true,
           typename std::enable_if<(dummy && StoreBinMeans), bool>::type dummy2 = true>
  inline void helper_update_bin_means()
  {
    bin_means.value = (samples.col(0) + (n_flushes)*bin_means.value) / (n_flushes+1);
  }
  template<bool dummy = true,
           typename std::enable_if<(dummy && !StoreBinMeans), bool>::type dummy2 = true>
  inline void helper_update_bin_means() { }


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
           typename std::enable_if<(dummy && StoreBinMeans), bool>::type dummy2 = true>
  inline const BinMeansArray & get_bin_means() const { return bin_means.value; }

  /** \brief Get the raw average of the squared values observed, for each binning level.
   *
   * The vector <em>bin_sqmeans.col(0)</em> contains the raw average of the squares of the
   * raw values observed, <em>bin_sqmeans.col(1)</em> the raw average of the squares of
   * the values averaged 2 by 2 (i.e. at the first binning level), and so on.
   */
  inline const BinSqMeansArray & get_bin_sqmeans() const { return  bin_sqmeans; }


  /** \brief Calculate the standard deviations of samples at different binning levels.
   *
   * Return an array of shape <em>(num_track_values, num_levels)</em> where element
   * <em>(i,k)</em> corresponds to the standard deviation of the \a i -th value at binning
   * level \a k. Binning level \a k=0 corresponds to the standard deviation of the samples
   * (no binning).
   *
   * Use this variant of the function if this class doesn't store the bin means. If so,
   * you need to provide the value of the means explicitly to the parameter \a means.
   */
  template<typename Derived>
  inline BinSqMeansArray calc_stddev_levels(const Eigen::ArrayBase<Derived> & means) const
  {
    eigen_assert(means.rows() == num_track_values());
    eigen_assert(means.cols() == 1);
    const int n_levels_plus_one = num_levels()+1;
    const int n_track_values = num_track_values();
    return (
	bin_sqmeans - replicated<1,NumLevelsPlusOneCTime>(
            means.cwiseProduct(means).template cast<ValueType>(),
            // replicated by:
            1, n_levels_plus_one
            )
	).cwiseQuotient(
	    replicated<NumTrackValuesCTime,1>(
		powers_of_two<Eigen::Array<ValueType, 1, NumLevelsPlusOneCTime> >(num_levels()+1)
		.reverse(),
		// replicated by:
		n_track_values, 1
		) * n_flushes
	    ).cwiseSqrt();
  }


  /** \brief Calculate the standard deviations of samples at the last binning level.
   *
   * Return a vector of \a num_track_values elements, where the \a i -th item corresponds
   * to the standard deviation of the \a i -th value at binning level \a
   * num_levels.
   *
   * If the error bars converged (see \ref determine_error_convergence()), this should be
   * a good estimate of the error bars on the corresponding values.
   *
   * Use this variant of the function if this class doesn't store the bin means. If so,
   * you need to provide the value of the means explicitly to the parameter \a means.
   */
  template<typename Derived>
  inline BinMeansArray calc_stddev_lastlevel(const Eigen::ArrayBase<Derived> & means) const {
    eigen_assert(means.rows() == num_track_values());
    eigen_assert(means.cols() == 1);
    return (
	bin_sqmeans.col(num_levels()) - means.cwiseProduct(means).template cast<ValueType>()
	).cwiseSqrt() / std::sqrt(n_flushes);
  }
  
  /** \brief Calculate the standard deviations of samples at different binning levels.
   *
   * Return an array of shape <em>(num_track_values, num_levels)</em> where element
   * <em>(i,k)</em> corresponds to the standard deviation of the \a i -th value at binning
   * level \a k. Binning level \a k=0 corresponds to the standard deviation of the samples
   * (no binning).
   *
   * Use this variant of the function if this class stores the bin means. If this is not
   * the case, you will need to call the variant \ref calc_stddev_levels(const
   * Eigen::ArrayBase<Derived> & means) with the values of the means.
   */
  template<bool dummy = true,
           typename std::enable_if<(dummy && StoreBinMeans), bool>::type dummy2 = true>
  inline BinSqMeansArray calc_stddev_levels() const {
    return calc_stddev_levels(bin_means.value);
  }

  /** \brief Calculate the standard deviations of samples at the last binning level.
   *
   * Return a vector of \a num_track_values elements, where the \a i -th item corresponds
   * to the standard deviation of the \a i -th value at binning level \a
   * num_levels.
   *
   * If the error bars converged (see \ref determine_error_convergence()), this should be
   * a good estimate of the error bars on the corresponding values.
   *
   * Use this variant of the function if this class stores the bin means. If this is not
   * the case, you will need to call the variant \ref calc_stddev_lastlevel(const
   * Eigen::ArrayBase<Derived> & means) with the values of the means.
   */
  template<bool dummy = true,
           typename std::enable_if<(dummy && StoreBinMeans), bool>::type dummy2 = true>
  inline BinMeansArray calc_stddev_lastlevel() const {
    return calc_stddev_lastlevel(bin_means.value);
  }
  
  /** \brief Attempt to determine if the error bars have converged.
   *
   * Call this method after calculating the standard deviations for each level with \ref
   * calc_stddev_levels(). Use the return value of that function to feed in the input
   * here.
   *
   * \returns an array of integers, of length \a num_track_values, each set to one of \ref
   * CONVERGED, \ref NOT_CONVERGED or \ref CONVERGENCE_UNKNOWN.
   */
  inline Eigen::ArrayXi determine_error_convergence(const Eigen::Ref<const BinSqMeansArray> & stddev_levels)
  {
    Eigen::ArrayXi converged_status; // RVO will help

    // verify that indeed the errors have converged. Inspired from ALPS code, see
    // https://alps.comp-phys.org/svn/alps1/trunk/alps/src/alps/alea/simplebinning.h

    const int range = 4;
    if (num_levels() < range-1) {

      converged_status = Eigen::ArrayXi::Constant(UNKNOWN_CONVERGENCE, num_track_values());

    } else {

      converged_status = Eigen::ArrayXi::Constant(CONVERGED, num_track_values());

      const auto & stddevs = stddev_levels.col(num_levels());

      for (int level = num_levels()+1 - range; level <= num_levels(); ++level) {

	const auto & stddevs_thislevel = stddev_levels.col(level);
	for (int val_it = 0; val_it < num_track_values(); ++val_it) {
	  if (stddevs_thislevel(val_it) >= stddevs(val_it)) {
	    converged_status(val_it) = CONVERGED;
	  } else if (stddevs_thislevel(val_it) < 0.824 * stddevs(val_it)) {
	    converged_status(val_it) = NOT_CONVERGED;
	  } else if ((stddevs_thislevel(val_it) < 0.9 * stddevs(val_it)) &&
		     converged_status(val_it) != NOT_CONVERGED) {
	    converged_status(val_it) = UNKNOWN_CONVERGENCE;
	  }
	}

      }
    }

    return converged_status;
  }
};



} // namespace Tomographer















#endif
