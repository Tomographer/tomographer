
#ifndef TOMOGRAPHER_MHRW_BIN_ERR_H
#define TOMOGRAPHER_MHRW_BIN_ERR_H


#include <tomographer/tools/util.h>
#include <tomographer/tools/loggers.h>
#include <tomographer/mhrw.h>


namespace Tomographer {


namespace tomo_internal {
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
}

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
  static constexpr int SamplesSizeCTime =
    tomo_internal::helper_samples_size<NumLevels_, (NumLevels_ > 0 && NumLevels_ < 10)>::value;
  static constexpr bool StoreBinMeans = StoreBinMeans_;

  typedef Eigen::Array<ValueType, NumTrackValuesCTime, SamplesSizeCTime> SamplesArray;
  typedef Eigen::Array<ValueType, NumTrackValuesCTime, 1> BinMeansArray;
  typedef Eigen::Array<ValueType, NumTrackValuesCTime, NumLevelsCTime> BinSqMeansArray;

  typedef LoggerType_ LoggerType;


  const Tools::static_or_dynamic<int, NumTrackValuesCTime> num_track_values;
  const Tools::static_or_dynamic<int, NumLevelsCTime> num_levels;
  const Tools::static_or_dynamic<CountIntType, SamplesSizeCTime> samples_size;

private:

  SamplesArray samples; // shape = (num_tracking, num_samples)

  // where we store the flushed values
  CountIntType n_flushes;
  Tools::store_if_enabled<BinMeansArray, StoreBinMeans> bin_means;   // shape = (num_tracking, num_levels)
  BinSqMeansArray bin_sqmeans; // shape = (num_tracking, num_levels)

  LoggerType & logger;

public:

  BinningAnalysis(int num_track_values_, int num_levels_, LoggerType & logger_)
    : num_track_values(num_track_values_),
      num_levels(num_levels_),
      samples_size(1 << num_levels()),
      samples(num_track_values(), samples_size()),
      n_flushes(0),
      bin_means(BinMeansArray::Zero(num_track_values())),
      bin_sqmeans(BinSqMeansArray::Zero(num_track_values(), num_levels())),
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
      int binnedsize = samples_size();

      for (int level = 0; level < num_levels(); ++level) {

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

	binnedsize >>= 1;
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
  template<typename Derived, bool dummy = true,
           typename std::enable_if<dummy && (NumLevelsCTime != Eigen::Dynamic), bool>::type dummy2 = true>
  inline BinSqMeansArray calc_stddev_levels(const Eigen::ArrayBase<Derived> & means) const {
    eigen_assert(means.rows() == num_track_values());
    eigen_assert(means.cols() == 1);
    return (bin_sqmeans - (means.cwiseProduct(means)).template replicate<1, NumLevelsCTime>()).cwiseSqrt();
  }

  template<typename Derived, bool dummy = true,
           typename std::enable_if<dummy && (NumLevelsCTime == Eigen::Dynamic), bool>::type dummy2 = true>
  inline BinSqMeansArray calc_stddev_levels(const Eigen::ArrayBase<Derived> & means) const {
    eigen_assert(means.rows() == num_track_values());
    eigen_assert(means.cols() == 1);
    return (bin_sqmeans - (means.cwiseProduct(means)).replicate(1, num_levels())).cwiseSqrt();
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
  
};



} // namespace Tomographer















#endif
