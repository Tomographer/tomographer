/* This file is part of the Tomographer project, which is distributed under the
 * terms of the MIT license.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 ETH Zurich, Institute for Theoretical Physics, Philippe Faist
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

#ifndef TOMOGRAPHER_MHRWSTATSCOLLECTORS_H
#define TOMOGRAPHER_MHRWSTATSCOLLECTORS_H

#include <cstddef>

#include <limits>
#include <tuple>
#include <utility>
#include <type_traits>
#include <typeinfo>


#include <tomographer2/tools/cxxutil.h>
#include <tomographer2/histogram.h>

/** \file mhrwstatscollectors.h
 *
 * Definitions for stats collectors -- see \ref pageInterfaceMHRWStatsCollector, as well
 * as for example \ref ValueHistogramMHRWStatsCollector etc.
 *
 */


namespace Tomographer {


/** \brief A simple MHRWStatsCollector interface which combines several stats collectors
 *
 * A \ref MHRandomWalk object expects one instance of a \a MHRWStatsCollector (see \ref
 * pageInterfaceMHRWStatsCollector); in case you wish to provide several stats collectors,
 * you should use a MultipleMHRWStatsCollectors instance which combines all your preferred
 * stats collectors.
 *
 * The obscure variadic templating of this class should not scare you&mdash;it's
 * relatively straightforward to use:
 * \code
 *   // some stat collectors
 *   MyStatCollector1 statcoll1(..);
 *   MyStatCollector2 statcoll2(..);
 *   MyStatCollector3 statcoll3(..);
 *
 *   // we combine them into one single "stat collector interface", which we can then
 *   // give as argument to the MHRandomWalk object:
 *   MultipleMHRWStatsCollectors<MyStatCollector1,MyStatCollector2,MyStatCollector3>
 *       multistatcollector(statcoll1, statcoll2, statcoll3);
 *
 *   MHRandomWalk<...> myrandomwalk(..., multistatcollector,...);
 *   // now, the random walk will call the callbacks in `multistatcollector', which
 *   // will relay the callbacks to all the given stat collectors `statcoll*'.
 * \endcode
 *
 * The number of stat collectors that were defined is accessible through the constant
 * enumeration value \ref NumStatColl.
 *
 */
template<typename... MHRWStatsCollectors>
class MultipleMHRWStatsCollectors
{
public:
  typedef std::tuple<MHRWStatsCollectors&...> MHRWStatsCollectorsRefTupleType;
  typedef std::tuple<MHRWStatsCollectors...> MHRWStatsCollectorsTupleType;

  //! The number of stats collectors we are tracking
  static constexpr int NumStatColl = sizeof...(MHRWStatsCollectors);

private:
  MHRWStatsCollectorsRefTupleType statscollectors;

public:

  MultipleMHRWStatsCollectors(MHRWStatsCollectors&... statscollectors_)
    : statscollectors(statscollectors_...)
  {
  }

  // method to get a particular stats collector
  template<int I>
  inline const typename std::tuple_element<I, MHRWStatsCollectorsTupleType>::type & getStatsCollector()
  {
    return std::get<I>(statscollectors);
  }

  // init() callback

  template<int I = 0>
  inline typename std::enable_if<I < NumStatColl, void>::type init()
  {
    std::get<I>(statscollectors).init();
    init<I+1>();
  }
  template<int I = 0>
  inline typename std::enable_if<I == NumStatColl, void>::type init()
  {
  }

  // thermalizing_done() callback

  template<int I = 0>
  inline typename std::enable_if<I < NumStatColl, void>::type thermalizing_done()
  {
    std::get<I>(statscollectors).thermalizing_done();
    thermalizing_done<I+1>();
  }
  template<int I = 0>
  inline typename std::enable_if<I == NumStatColl, void>::type thermalizing_done()
  {
  }

  // done() callback

  template<int I = 0>
  inline typename std::enable_if<I < NumStatColl, void>::type done()
  {
    std::get<I>(statscollectors).done();
    done<I+1>();
  }
  template<int I = 0>
  inline typename std::enable_if<I == NumStatColl, void>::type done()
  {
  }


  // raw_move() callback

  template<typename CountIntType, typename PointType, typename FnValueType, typename MHRandomWalk, int I = 0>
  inline typename std::enable_if<I < NumStatColl, void>::type raw_move(
      CountIntType k, bool is_thermalizing, bool is_live_iter, bool accepted,
      double a, const PointType & newpt, FnValueType newptval,
      const PointType & curpt, FnValueType curptval,
      MHRandomWalk & rw
      )
  {
    std::get<I>(statscollectors).raw_move(
        k, is_thermalizing, is_live_iter, accepted, a,
        newpt, newptval, curpt, curptval, rw
        );
    raw_move<CountIntType, PointType, FnValueType, MHRandomWalk, I+1>(
        k, is_thermalizing, is_live_iter, accepted, a,
        newpt, newptval, curpt, curptval, rw
        );
  }
  template<typename CountIntType, typename PointType, typename FnValueType, typename MHRandomWalk, int I = 0>
  inline typename std::enable_if<I == NumStatColl, void>::type raw_move(
      CountIntType, bool, bool, bool, double, const PointType &, FnValueType,
      const PointType &, FnValueType, MHRandomWalk &
      )
  {
  }


  // process_sample() callback

  template<typename CountIntType, typename PointType, typename FnValueType, typename MHRandomWalk, int I = 0>
  inline typename std::enable_if<I < NumStatColl, void>::type process_sample(
      CountIntType k, CountIntType n, const PointType & curpt, FnValueType curptval, MHRandomWalk & rw
      )
  {
    std::get<I>(statscollectors).process_sample(k, n, curpt, curptval, rw);
    process_sample<CountIntType, PointType, FnValueType, MHRandomWalk, I+1>(k, n, curpt, curptval, rw);
  }

  template<typename CountIntType, typename PointType, typename FnValueType, typename MHRandomWalk, int I = 0>
  inline typename std::enable_if<I == NumStatColl, void>::type process_sample(
      CountIntType, CountIntType, const PointType &, FnValueType, MHRandomWalk &
      )
  {
  }

};




// -----------------



/** \brief A StatsCollector which builds a histogram of values calculated with a
 * ValueCalculator for each data sample point
 *
 * \a ValueHistogramMHRWStatsCollector complies both with the \ref
 * pageInterfaceMHRWStatsCollector and the \ref pageInterfaceResultable.
 *
 * This stats collector is suitable for tracking statistics during a \ref MHRandomWalk.
 *
 * The ValueCalculator is a type expected to implement the \ref
 * pageInterfaceValueCalculator.  The argument of the \a getValue() method should be the
 * same type as the point type of the random walk; the current point of the random walk is
 * passed on as is.
 *
 */
template<typename ValueCalculator_,
	 typename LoggerType = Logger::VacuumLogger,
	 typename HistogramType_ = UniformBinsHistogram<typename ValueCalculator_::ValueType>
	 >
class ValueHistogramMHRWStatsCollector
{
public:
  /** \brief The type which calculates the interesting value. Should be of type interface \ref
   * pageInterfaceValueCalculator.
   */
  typedef ValueCalculator_ ValueCalculator;

  //! The type to use to represent a calculated distance
  typedef typename ValueCalculator::ValueType ValueType;

  //! The type of the histogram. Usually a \ref UniformBinsHistogram with \a ValueType range type
  typedef HistogramType_ HistogramType;

  //! Required for compliance with \ref pageInterfaceResultable type
  typedef HistogramType_ Result;

  //! Structure which holds the parameters of the histogram we're recording
  typedef typename HistogramType::Params HistogramParams;

private:

  //! Store the histogram
  HistogramType _histogram;

  /** \brief The value calculator which we will invoke to get the interesting value.
   *
   * The type should implement the \ref pageInterfaceValueCalculator interface.
   */
  ValueCalculator _vcalc;

  LoggerType & _logger;

public:
  //! Simple constructor, initializes with the given values
  ValueHistogramMHRWStatsCollector(HistogramParams histogram_params,
				   const ValueCalculator & vcalc,
				   LoggerType & logger)
    : _histogram(histogram_params),
      _vcalc(vcalc),
      _logger(logger)
  {
  }

  //! Get the histogram data collected so far. See \ref HistogramType.
  inline const HistogramType & histogram() const
  {
    return _histogram;
  }

  /** \brief Get the histogram data collected so far. This method is needed for \ref
   * pageInterfaceResultable compliance.
   */
  inline const Result & getResult() const
  {
    return _histogram;
  }

  // stats collector part

  //! Part of the \ref pageInterfaceMHRWStatsCollector. Initializes the histogram to zeros.
  inline void init()
  {
    // reset our array
    _histogram.reset();
  }
  //! Part of the \ref pageInterfaceMHRWStatsCollector. No-op.
  inline void thermalizing_done()
  {
  }
  /** \brief Part of the \ref pageInterfaceMHRWStatsCollector.
   *
   * If you call this function with \a PrintHistogram=true (the default), then this will
   * display the final histogram in the logger at logging level \a Logger::LONGDEBUG.
   *
   * If this function is called with \a PrintHistogram=false, then this is a no-op.
   */
  template<bool PrintHistogram = true>
  inline void done()
  {
    if (PrintHistogram) {
      if (_logger.enabled_for(Logger::LONGDEBUG)) {
	// _logger.longdebug("ValueHistogramMHRWStatsCollector", "done()");
	_logger.longdebug("ValueHistogramMHRWStatsCollector",
		       "Done walking & collecting stats. Here's the histogram:\n"
		       + _histogram.pretty_print());
      }
    }
  }

  //! Part of the \ref pageInterfaceMHRWStatsCollector. No-op.
  template<typename CountIntType, typename PointType, typename LLHValueType, typename MHRandomWalk>
  void raw_move(CountIntType k, bool /*is_thermalizing*/, bool /*is_live_iter*/, bool /*accepted*/,
                double /*a*/, const PointType & /*newpt*/, LLHValueType /*newptval*/,
                const PointType & /*curpt*/, LLHValueType /*curptval*/, MHRandomWalk & /*mh*/)
  {
    _logger.longdebug("ValueHistogramMHRWStatsCollector", [&](std::ostream & stream) {
	stream << "raw_move(): k=" << k;
      });
  }

  //! Part of the \ref pageInterfaceMHRWStatsCollector. Records the sample in the histogram.
  template<typename CountIntType, typename PointType, typename LLHValueType, typename MHRandomWalk>
  std::size_t process_sample(CountIntType k, CountIntType n, const PointType & curpt,
                             LLHValueType /*curptval*/, MHRandomWalk & /*mh*/)
  {
    ValueType val = _vcalc.getValue(curpt);

    _logger.longdebug("ValueHistogramMHRWStatsCollector", [&](std::ostream & stream) {
	stream << "in process_sample(): "
	       << "k=" << k << ", n=" << n << ", val=" << val
	       << " [with ValueType=" << typeid(ValueType).name() << "]" ;
      });

    return _histogram.record(val);

    //_logger.longdebug("ValueHistogramMHRWStatsCollector", "process_sample() finished");
  }
 

};



/** \brief Traits-like class for ValueHistogramWithBinningMHRWStatsCollector.
 *
 * Collects the template parameters for use with \ref
 * ValueHistogramWithBinningMHRWStatsCollector.
 *
 * Provides also some trait properties, such as the corresponding result type \ref Result.
 */
template<typename ValueCalculator_,
         typename CountIntType_ = int,
         typename CountRealAvgType_ = double,
         int NumTrackValues_ = Eigen::Dynamic,
         int NumLevels_ = Eigen::Dynamic
	 >
struct ValueHistogramWithBinningMHRWStatsCollectorParams
{
  /** \brief The type of the \ref pageInterfaceValueCalculator which calculates the value
   * of which we're collecting a histogram. */
  typedef ValueCalculator_ ValueCalculator;
  /** \brief Type used to count the number of hits in each bin. */
  typedef CountIntType_ CountIntType;
  /** \brief Type used to store the averages of the histogram bins. */
  typedef CountRealAvgType_ CountRealAvgType;

  //! Number of values we're tracking, i.e. the number of bins in the histogram [for now]
  static constexpr int NumTrackValues = NumTrackValues_;
  //! The number of levels in the binning analysis. See \ref BinningAnalysis.
  static constexpr int NumLevels = NumLevels_;

  //! The type of a value calculated by the \ref pageInterfaceValueCalculator.
  typedef typename ValueCalculator::ValueType ValueType;
  
  //! The relevant \ref BinningAnalysis parameters for us.
  typedef BinningAnalysisParams<ValueType,NumTrackValues,NumLevels,false/*StoreBinSums*/,CountIntType>
  BinningAnalysisParamsType;

  /** \brief The Base Histogram Type.
   *
   * The type of the histogram which simply stores the bin counts.
   */
  typedef UniformBinsHistogram<typename ValueCalculator::ValueType, CountIntType> BaseHistogramType;
  /** \brief The Final Histogram Type (with error bars).
   *
   */
  typedef UniformBinsHistogramWithErrorBars<typename ValueCalculator::ValueType, CountRealAvgType> HistogramType;
  typedef typename HistogramType::Params HistogramParams;

  /** \brief Result type of the corresponding ValueHistogramWithBinningMHRWStatsCollector
   *
   * Stores a histogram with error bars, detailed information about error bars at different binning
   * levels, and information about the convergence of these error bars.
   */
  struct Result
  {
    //! Simple default constructor (e.g. to use as std::vector<Result>).
    explicit Result()
      : hist(), error_levels(), converged_status()
    {
    }

    //! Constructor which initializes the fields from the histogram and binning analysis type.
    template<typename BinningAnalysisType>
    Result(HistogramParams p, const BinningAnalysisType & b)
      : hist(p),
	error_levels(b.num_track_values(), b.num_levels()+1),
	converged_status(Eigen::ArrayXi::Constant(b.num_track_values(), BinningAnalysisType::UNKNOWN_CONVERGENCE))
    {
      tomographer_assert(converged_status.rows() == b.num_track_values() && converged_status.cols() == 1);
    }

    //! Histogram, already with error bars
    HistogramType hist;
    //! Detailed error bars for all binning levels
    typename BinningAnalysisParamsType::BinSumSqArray error_levels;
    /** \brief Information of convergence status of the error bars (see e.g. \ref
     * BinningAnalysisParamsType::CONVERGED)
     */
    Eigen::ArrayXi converged_status;

    //! Dump values, error bars and convergence status in human-readable form into ostream
    inline void dump_convergence_analysis(std::ostream & str) const
    {
      for (int k = 0; k < converged_status.size(); ++k) {
	str << "\tval[" << std::setw(3) << k << "] = "
	    << std::setw(12) << hist.bins(k)
	    << " +- " << std::setw(12) << hist.delta(k);
	if (converged_status(k) == BinningAnalysisParamsType::CONVERGED) {
	  str << "  [CONVERGED]";
	} else if (converged_status(k) == BinningAnalysisParamsType::NOT_CONVERGED) {
	  str << "  [NOT CONVERGED]";
	} else if (converged_status(k) == BinningAnalysisParamsType::UNKNOWN_CONVERGENCE) {
	  str << "  [UNKNOWN]";
	} else {
	  str << "  [UNKNOWN CONVERGENCE STATUS: " << converged_status(k) << "]";
	}
	str << "\n";
      }
    }

    //! Dump values, error bars and convergence status in human-readable form as string
    inline std::string dump_convergence_analysis() const
    {
      std::stringstream ss;
      dump_convergence_analysis(ss);
      return ss.str();
    }

  };

};

/** \brief Collect a histogram of values from a MH random walk, with binning analysis.
 *
 * The \a Params template parameter must be a
 * ValueHistogramWithBinningMHRWStatsCollectorParams with the relevant template
 * parameters.
 *
 */
template<typename Params,
	 typename LoggerType_ = Logger::VacuumLogger
         >
class ValueHistogramWithBinningMHRWStatsCollector
{
public:

  //! See \ref ValueHistogramWithBinningMHRWStatsCollectorParams::ValueCalculator .
  typedef typename Params::ValueCalculator ValueCalculator;
  //! See \ref ValueHistogramWithBinningMHRWStatsCollectorParams::CountIntType .
  typedef typename Params::CountIntType CountIntType;
  //! See \ref ValueHistogramWithBinningMHRWStatsCollectorParams::CountRealAvgType .
  typedef typename Params::CountRealAvgType CountRealAvgType;
  
  /** \brief Somewhere where this object may log what it's doing. */
  typedef LoggerType_ LoggerType;

  //! See \ref ValueHistogramWithBinningMHRWStatsCollectorParams::BaseHistogramType .
  typedef typename Params::BaseHistogramType BaseHistogramType;
  //! See \ref ValueHistogramWithBinningMHRWStatsCollectorParams::HistogramParams .
  typedef typename Params::HistogramParams HistogramParams;
    
  //! See \ref ValueHistogramWithBinningMHRWStatsCollectorParams::ValueType .
  typedef typename Params::ValueType ValueType;
  
  //! See \ref ValueHistogramWithBinningMHRWStatsCollectorParams::BinningAnalysisParamsType .
  typedef typename Params::BinningAnalysisParamsType BinningAnalysisParamsType;
  //! See \ref ValueHistogramWithBinningMHRWStatsCollectorParams::BinningAnalysisType .
  typedef BinningAnalysis<BinningAnalysisParamsType, LoggerType> BinningAnalysisType;
    
  //! See \ref ValueHistogramWithBinningMHRWStatsCollectorParams::NumTrackValues .
  static constexpr int NumTrackValuesCTime = Params::NumTrackValues;
  //! See \ref ValueHistogramWithBinningMHRWStatsCollectorParams::NumLevels .
  static constexpr int NumLevelsCTime = Params::NumLevels;

  //! See \ref ValueHistogramWithBinningMHRWStatsCollectorParams::Result .
  typedef typename Params::Result Result;

  /** \brief This is the natural ValueHistogramMHRWStatsCollector type on which we're
   *         adding error bars.
   */
  typedef ValueHistogramMHRWStatsCollector<
    ValueCalculator,
    LoggerType,
    BaseHistogramType
    > ValueHistogramMHRWStatsCollectorType;
  
private:
    
  ValueHistogramMHRWStatsCollectorType value_histogram;

  BinningAnalysisType binning_analysis;

  LoggerType & logger;

  Result result;

public:
    
  ValueHistogramWithBinningMHRWStatsCollector(HistogramParams histogram_params,
                                              const ValueCalculator & vcalc,
                                              int num_levels,
                                              LoggerType & logger_)
    : value_histogram(histogram_params, vcalc, logger_),
      binning_analysis(histogram_params.num_bins, num_levels, logger_),
      logger(logger_),
      result(histogram_params, binning_analysis)
  {
    logger.longdebug("ValueHistogramWithBinningMHRWStatsCollector", "constructor()");
  }
  
  //! Get the histogram data collected so far. See \ref BaseHistogramType .
  inline const BaseHistogramType & histogram() const
  {
    return value_histogram.histogram();
  }

  inline const BinningAnalysisType & get_binning_analysis() const
  {
    return binning_analysis;
  }

  /** \brief Get the final histogram data. This method is needed for \ref
   * pageInterfaceResultable compliance.
   *
   * This will only yield a valid value AFTER the all the data has been collected and \ref
   * done() was called.
   */
  inline const Result & getResult() const
  {
    return result;
  }

  // stats collector part

  //! Part of the \ref pageInterfaceMHRWStatsCollector. Initializes the histogram to zeros.
  inline void init()
  {
    value_histogram.init();
  }
  //! Part of the \ref pageInterfaceMHRWStatsCollector. No-op.
  inline void thermalizing_done()
  {
    value_histogram.thermalizing_done();
  }
  //! Finalize the data collection. Part of the \ref pageInterfaceMHRWStatsCollector.
  inline void done()
  {
    logger.longdebug("ValueHistogramWithBinningMHRWStatsCollector::done()", "finishing up ...");

    value_histogram.template done<false>();

    const BaseHistogramType & h = value_histogram.histogram();
    result.hist.params = h.params;
    CountRealAvgType normalization = h.bins.sum() + h.off_chart; // need ALL samples, because that's
								 // what the binning analysis sees
    result.hist.bins = h.bins.template cast<CountRealAvgType>() / normalization;
    result.error_levels = binning_analysis.calc_error_levels(result.hist.bins);
    result.hist.delta = result.error_levels.col(binning_analysis.num_levels()).template cast<CountRealAvgType>();
    result.hist.off_chart = h.off_chart / normalization;

    result.converged_status = binning_analysis.determine_error_convergence(result.error_levels);

    logger.debug("ValueHistogramWithBinningMHRWStatsCollector", [&,this](std::ostream & str) {
        str << "Binning analysis: bin sqmeans at different binning levels are:\n"
            << binning_analysis.get_bin_sqmeans() << "\n"
	    << "\t-> so the error bars at different binning levels are:\n"
	    << result.error_levels << "\n"
	    << "\t-> convergence analysis: \n";
	result.dump_convergence_analysis(str);
	str << "\t... and just for you, here is the final histogram:\n" << result.hist.pretty_print() << "\n";
      });
  }

  //! Part of the \ref pageInterfaceMHRWStatsCollector. No-op.
  template<typename CountIntType2, typename PointType, typename LLHValueType, typename MHRandomWalk>
  inline void raw_move(CountIntType2 k, bool is_thermalizing, bool is_live_iter, bool accepted,
		       double a, const PointType & newpt, LLHValueType newptval,
		       const PointType & curpt, LLHValueType curptval, MHRandomWalk & mh)
  {
    value_histogram.raw_move(k, is_thermalizing, is_live_iter, accepted, a, newpt, newptval, curpt, curptval, mh);
  }

  //! Part of the \ref pageInterfaceMHRWStatsCollector. Records the sample in the histogram.
  template<typename CountIntType2, typename PointType, typename LLHValueType, typename MHRandomWalk>
  inline void process_sample(CountIntType2 k, CountIntType2 n, const PointType & curpt,
			     LLHValueType curptval, MHRandomWalk & mh)
  {
    std::size_t histindex = value_histogram.process_sample(k, n, curpt, curptval, mh);
    binning_analysis.process_new_values(
	Tools::can_basis_vec<Eigen::Array<ValueType,Eigen::Dynamic,1> >(histindex, value_histogram.histogram().num_bins())
	);
  }

};






/** \brief Template, specializable class to get status reports from stats collectors.
 *
 * Specialize this class for your stats collector to be able to provide a short status
 * report. Just provide 2-3 lines with the most important information enough to provide a
 * very basic overview, not a full-length report.
 */
template<typename MHRWStatsCollector_>
struct MHRWStatsCollectorStatus
{
  typedef MHRWStatsCollector_ MHRWStatsCollector;

  static constexpr bool CanProvideStatus = false;

  /** \brief Prepare a string which reports the status of the given stats collector
   *
   * Don't end your string with a newline, it will be added automatically.
   */
  static inline std::string getStatus(const MHRWStatsCollector * /*stats*/)
  {
    return std::string();
  }
};
// static members:
template<typename MHRWStatsCollector_>
constexpr bool MHRWStatsCollectorStatus<MHRWStatsCollector_>::CanProvideStatus;



/** \brief Provide status reporting for a MultipleMHRWStatsCollectors
 *
 */
template<typename... Args>
struct MHRWStatsCollectorStatus<MultipleMHRWStatsCollectors<Args... > >
{
  typedef MultipleMHRWStatsCollectors<Args... > MHRWStatsCollector;

  static constexpr int NumStatColl = MHRWStatsCollector::NumStatColl;
  
  static constexpr bool CanProvideStatus = true;

  template<int I = 0, typename std::enable_if<(I < NumStatColl), bool>::type dummy = true>
  static inline std::string getStatus(const MHRWStatsCollector * stats)
  {
    typedef typename std::tuple_element<I, typename MHRWStatsCollector::MHRWStatsCollectorsTupleType>::type
      ThisStatsCollector;
    return
      (MHRWStatsCollectorStatus<ThisStatsCollector>::CanProvideStatus
       ? (MHRWStatsCollectorStatus<ThisStatsCollector>::getStatus(stats->template getStatsCollector<I>())
	  + ((I < (NumStatColl-1)) ? std::string("\n") : std::string()))
       : std::string())
      + getStatus<I+1>(stats);
  };

  template<int I = 0, typename std::enable_if<(I == NumStatColl), bool>::type dummy = true>
  static inline std::string getStatus(const MHRWStatsCollector * stats)
  {
    (void)stats;
    return std::string();
  }

};
// static members:
template<typename... Args>
constexpr int MHRWStatsCollectorStatus<MultipleMHRWStatsCollectors<Args... > >::NumStatColl;
template<typename... Args>
constexpr bool MHRWStatsCollectorStatus<MultipleMHRWStatsCollectors<Args... > >::CanProvideStatus;



/** \brief Provide status reporting for a ValueHistogramMHRWStatsCollector
 *
 */
template<typename ValueCalculator_,
	 typename LoggerType_,
	 typename HistogramType_
	 >
struct MHRWStatsCollectorStatus<ValueHistogramMHRWStatsCollector<ValueCalculator_, LoggerType_, HistogramType_> >
{
  typedef ValueHistogramMHRWStatsCollector<ValueCalculator_, LoggerType_, HistogramType_> MHRWStatsCollector;
  
  static constexpr bool CanProvideStatus = true;

  static inline std::string getStatus(const MHRWStatsCollector * stats)
  {
    const int maxbarwidth = 50;

    typedef typename MHRWStatsCollector::HistogramType HistogramType;

    return "Histogram: " + histogram_short_bar<HistogramType>(stats->histogram(), true, maxbarwidth);
  }
};
// static members:
template<typename ValueCalculator_,
	 typename LoggerType_,
	 typename HistogramType_
	 >
constexpr bool
MHRWStatsCollectorStatus<ValueHistogramMHRWStatsCollector<ValueCalculator_, LoggerType_, HistogramType_> >::CanProvideStatus;


/** \brief Provide status reporting for a ValueHistogramWithBinningMHRWStatsCollector
 *
 */
template<typename Params_,
	 typename LoggerType_
	 >
struct MHRWStatsCollectorStatus<ValueHistogramWithBinningMHRWStatsCollector<Params_, LoggerType_> >
{
  typedef ValueHistogramWithBinningMHRWStatsCollector<Params_, LoggerType_> MHRWStatsCollector;
  
  static constexpr bool CanProvideStatus = true;

  static inline std::string getStatus(const MHRWStatsCollector * stats)
  {
    const int maxbarwidth = 50;

    typedef typename MHRWStatsCollector::BaseHistogramType BaseHistogramType;
    const BaseHistogramType & histogram = stats->histogram();

    // calculate the error bars at different levels, to determine convergence status.
    typedef typename MHRWStatsCollector::BinningAnalysisType BinningAnalysisType;
    //typedef typename MHRWStatsCollector::CountRealAvgType CountRealAvgType;
    typedef typename BinningAnalysisType::ValueType ValueType;
    const auto& binning_analysis = stats->get_binning_analysis();
    Eigen::Array<ValueType, Eigen::Dynamic, 1> binmeans(histogram.num_bins());
    binmeans = histogram.bins.template cast<ValueType>() /
      (ValueType)(histogram.bins.sum() + histogram.off_chart);

    auto error_levels = binning_analysis.calc_error_levels(binmeans);
    auto conv_status = binning_analysis.determine_error_convergence(error_levels);

    int n_cnvg = 0;
    int n_unknown = 0;
    int n_not_cnvg = 0;
    for (std::size_t k = 0; k < (std::size_t)histogram.num_bins(); ++k) {
      if (conv_status(k) == BinningAnalysisType::CONVERGED) {
	++n_cnvg;
      } else if (conv_status(k) == BinningAnalysisType::NOT_CONVERGED) {
	++n_not_cnvg;
      } else {
	++n_unknown;
      }
    }

    return tomo_internal::histogram_short_bar_fmt<BaseHistogramType>(histogram, "", maxbarwidth)
      + Tools::fmts("   err: (cnvg/?/fail) %d/%d/%d", n_cnvg, n_unknown, n_not_cnvg);
  }
};
// static members:
template<typename Params_,
	 typename LoggerType_
	 >
constexpr bool
MHRWStatsCollectorStatus<ValueHistogramWithBinningMHRWStatsCollector<Params_, LoggerType_> >::CanProvideStatus;


} // namespace Tomographer



#endif
