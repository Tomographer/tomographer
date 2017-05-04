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


// =============================================================================
// Type Interfaces -- Title Page
// =============================================================================

/** \page pageTypeInterfaces Type Interfaces
 *
 * These pages document <em>type interfaces</em>. These specify signatures and members
 * which a type must provide in order to fulfil a particular task. This is what the C++
 * standard calls &lsquo;concepts.&rsquo; Typically, classes might expect their template
 * parameters to conform to a particular type interface.
 *
 * For example, the class \ref Tomographer::ValueHistogramMHRWStatsCollector calculates
 * the value of a function specified by a template parameter. In order to use it, it must
 * be made clear which methods the class is allowed to call on this template
 * parameter. This specification corresponds to a <em>type interface</em>; in this case
 * the class expects as template parameter any type which complies with the \ref
 * pageInterfaceValueCalculator.
 *
 * Documented Type Interfaces in the %Tomographer framwork are:
 *
 *  - \subpage pageInterfaceMHWalker
 *  - \subpage pageInterfaceMHRWStatsCollector
 *  - \subpage pageInterfaceResultable
 *  - \subpage pageInterfaceValueCalculator
 *  - \subpage pageInterfaceMHRandomWalkTaskCData
 *  - \subpage pageTaskManagerDispatcher
 *  - \subpage pageInterfaceHistogram
 *  - \subpage pageDenseDMTypeInterfaces
 */

// no longer: *  - \subpage pageInterfaceRandomWalk


// =============================================================================
// MHRWStatsCollector
// =============================================================================

/** \page pageInterfaceMHRWStatsCollector MHRWStatsCollector Interface
 *
 * <em>This is a &lsquo;type interface.&rsquo; See \ref pageTypeInterfaces
 * for more info on what that is.</em>
 *
 * A type implementing a \a MHRWStatsCollector interface is responsible for collecting
 * statistics from samples during a Metropolis-Hastings random walk (see \ref
 * Tomographer::MHRandomWalk).
 *
 * This type must provide the following members. The members, or the class itself, must be
 * templates with type parameters \c CountIntType, \c PointType, \c FnValueType, and \c
 * MHRandomWalk. (It is usually most convenient to template the methods themselves, so
 * that you don't have to specify the parameters when instantiating the object.)
 *
 * \par void init()
 * is called before starting the random walk
 *
 * \par void thermalizingDone()
 * is called after the thermalizing runs, before starting the "live" runs.
 * 
 * \par void done()
 * is called after the random walk is finished.
 *
 * \par void processSample(CountIntType k, CountIntType n, const PointType & pt, FnValueType fnval, MHRandomWalk & rw)
 *     is called at the end of each sweep, after the thermalization sweeps have finished.
 *     This function is meant to actually take live samples. \a k is the raw iteration
 *     number, \a n is the sample number (= number of live samples already taken), \a pt
 *     the current point of the walk, \a fnval the value of the function at this point
 *     (this may be the value of the MH jump function, its logarithm, or a dummy value,
 *     depending on the random walk's MHWalker::UseFnSyntaxType, see \ref
 *     pageInterfaceMHWalker).
 *
 * \par void rawMove(CountIntType k, bool is_thermalizing, bool is_live_iter, bool accepted, double a, const PointType & newpt, FnValueType newptval, const PointType & curpt, FnValueType curptval, MHRandomWalk & rw)
 *     is called after each move during the random walk. Note that if you want to take
 *     real samples, use \c process_sample() instead.
 *
 * \par
 *     \c k is the iteration number (which is reset to zero after the thermalizing
 *     sweeps), \c is_thermalizing is \c true during the first part of the random walk
 *     during the thermalizing runs, \c is_live_iter is set to \c true only if a sample is
 *     taken at this point, i.e. if not thermalizing and after each full sweep. \c accepted
 *     indicates whether this Metropolis-Hastings move was accepted or not and \c a gives
 *     the ratio of the function which was tested for the move. (Note that \c a might not
 *     be calculated and left to 1 if known to be greater than 1.) \c newpt and \c
 *     newptval are the new proposal jump point and the function value at that new
 *     point. The function value is either the actual value of the function, or its
 *     logarithm, or a dummy value, depending on \c MHWalker::UseFnSyntaxType.  Similarly
 *     \c curpt and \c curptval are the current point and function value. The object \c rw
 *     is a reference to the random walk object instance.
 * 
 */


// =============================================================================
// Resultable
// =============================================================================

/** \page pageInterfaceResultable Resultable Interface
 *
 * <em>This is a &lsquo;type interface.&rsquo; See \ref pageTypeInterfaces
 * for more info on what that is.</em>
 *
 * This abstract type interface describes a type which results in some output. This might
 * be for example some forms of \ref pageInterfaceMHRWStatsCollector 's such as \ref
 * Tomographer::ValueHistogramMHRWStatsCollector which results in a histogram.
 *
 * \par typedef .. ResultType
 *    The type that the result has
 *
 * \par ResultType getResult()
 *    Obtain the said result. The return type must be anything that may be assigned to a
 *    \a ResultType type, or a value that the \a ResultType accepts in a constructor.
 */


// =============================================================================
// MHWalker
// =============================================================================

/** \page pageInterfaceMHWalker MHWalker Interface
 *
 * <em>This is a &lsquo;type interface.&rsquo; See \ref pageTypeInterfaces
 * for more info on what that is.</em>
 *
 * A \a MHWalker compliant type describes a particular Metropolis-Hastings walk on some
 * state space. It takes care for example of providing candidate new points (jump
 * function), and calculating the probability ratio for the jump.
 *
 * In the following documentation, \f$ P(x) \f$ designates the positive function which
 * drives the Metropolis-Hastings random walk.  The collected samples will
 * (asymptotically) be distributed according to \f$ P(x) / \int P(x)\,dx \f$.
 *
 * For instance, the \ref Tomographer::MHRandomWalk class needs to be provided a \a
 * MHWalker compliant type in order to carry out the random walk.
 *
 * A type implementing the \a MHWalker interface must provide the following types:
 *
 * \since Changed in %Tomographer 5.0: Added the \a WalkerParams member type and obsoleted \a StepRealType.
 *
 * \par typedef ... PointType
 *     The type needed to represent a point in state space in which we are performing a
 *     random walk. An object of such type is never default-constructed, but always
 *     copy-constructed from another \a PointType. One should also be able to assign a \a
 *     PointType to another \c PointType (e.g. <code>curpt = other_point</code>).
 *
 * \par typedef ... WalkerParams
 *     A user type describing parameters of the random walk jump process, such as the step
 *     size.  This is likely to be a \c double or some floating-point type to store just
 *     the step size.  This type should be streamable using C++ streams, this is used for
 *     logging & debugging.
 *
 * \par typedef ... FnValueType &mdash; required only if UseFnSyntaxType != MHUseFnRelativeValue
 *     The return value type of the function evaluated at each point during the
 *     Metropolis-Hastings random walk. Usually this is \c double or some floating-point
 *     type. You do not need to provide this typedef if \c UseFnSyntaxType is set to \c
 *     MHUseFnRelativeValue.
 *
 * A \a MHWalker must provide the following constant enumeration values:
 *
 * \par static constexpr int UseFnSyntaxType = ...
 *     Specifies how we calculate the function probability ratio of two points in the
 *     random walk. \c UseFnSyntaxType should be set to one of either \ref
 *     Tomographer::MHUseFnValue (this class calculates the function value at each point),
 *     \ref Tomographer::MHUseFnLogValue (this class calculates the natural logarithm of
 *     the function at each point), or \ref Tomographer::MHUseFnRelativeValue (this class
 *     calculates the ratio of the values at two points).  See below, "Role of
 *     UseFnSyntaxType".
 *
 * And must provide the following members:
 *
 * \par MHWalker(MHWalker&& other)
 *     A MHWalker type must have a move constructor.
 *
 * \par PointType startPoint()
 *     Should return the starting point for the random walk.  This function will be called
 *     before \a init().
 *
 * \par void init()
 *     Will be called when beginning the random walk, i.e. just before the first
 *     thermalization iteration.
 *
 * \par void thermalizingDone()
 *     This method is called after all the thermalization sweeps have finished, and before
 *     starting with the live iterations.  Typically this function shouldn't do anything,
 *     it's just provided for convenience.
 *
 * \par void done()
 *     Called after the random walk has been completed and all samples collected.
 *
 * \par PointType jumpFn(const PointType & curpt, const WalkerParams& walker_params)
 *     Provide the next point where the random walk should consider jumping to. This
 *     function should return a new point depending on the current point \a curpt,
 *     according to some symmetric proposal distribution.
 *
 * \par
 *     This jump function should honor the specified \a walker_params, which is the value
 *     passed to the constructor of the \ref Tomographer::MHRWParams class.  In the
 *     future, we may be able to dynamically adjust the parameters, so \a MHWalker
 *     implementations should typically not assume that the parameters won't change from
 *     one call of jumpFn() to another.
 *
 * \par FnValueType fnVal(const PointType & curpt)
 *     <em>[Required only if UseFnSyntaxType == MHUseFnValue.]</em>
 *     If <em>UseFnSyntaxType==MHUseFnValue</em>, this function should return the value of
 *     the function \f$ P(x) \f$ defining the random walk, evaluated at the point \a
 *     curpt.  See below ("Role of UseFnSyntaxType").
 *
 * \par FnValueType fnLogVal(const PointType & curpt)
 *     <em>[Required only if UseFnSyntaxType == MHUseFnLogValue.]</em>
 *     If <em>UseFnSyntaxType==MHUseFnLogValue</em>, this function should return the value
 *     of the function \f$ \ln P(x) \f$ defining the random walk, evaluated at the point
 *     \a curpt.  See below ("Role of UseFnSyntaxType").
 *
 * \par double fnRelVal(const PointType & newpt, const PointType & curpt)
 *     <em>[Required only if UseFnSyntaxType == MHUseFnRelativeValue.]</em>
 *     If <em>UseFnSyntaxType==MHUseFnRelativeValue</em>, this function should return the
 *     ratio \f$ P(\mathrm{newpt})/P(\mathrm{curpt}) \f$.  See below ("Role of
 *     UseFnSyntaxType").
 *
 * <br>
 * 
 * \anchor labelMHWalkerUseFnSyntaxType
 * \par Role of \c UseFnSyntaxType:
 *     In a Metropolis-Hastings random walk, the probability according to which one jumps
 *     to the next proposed point is given by the ratio of the values of the function \f$
 *     P(x) \f$.  There are three ways this class can provide this probability ratio.
 *
 * \par
 *     1. You may provide the value \f$ P(x) \f$ itself.  In this case, set
 *     <em>UseFnSyntaxType = Tomographer::MHUseFnValue</em>.  The class must define the
 *     member function \a fnVal() as described above.  It doesn't have to provide the
 *     member functions \a fnLogVal() or \a fnRelVal().
 *
 * \par
 *     2. You may provide the natural logarithm of the function, \f$ \ln P(x) \f$.  Choose
 *     this option if it is more natural to calculate \f$ \ln P(x) \f$ instead of \f$ P(x)
 *     \f$ (for instance, if P(x) is a product of many terms).  The random walk class
 *     (\ref Tomographer::MHRandomWalk) will not calculate the exponential of the value
 *     you give, but rather the exponential of the difference of two values of \f$ \ln
 *     P(x) \f$ at two points in order to directly optain the probability ratio.
 *     In this case, set set <em>UseFnSyntaxType = Tomographer::MHUseFnLogValue</em>.  The
 *     class must define the member function \a fnLogVal() as described above.  It doesn't
 *     have to provide the member functions \a fnVal() or \a fnRelVal().
 *
 * \par
 *    3. You may directly provide the ratio of values for two points \f$ P(x')/P(x) \f$.
 *    in this case, set <em>UseFnSyntaxType = Tomographer::MHUseRelativeValue</em>.  The
 *    class must define the member function \a fnRelVal() as described above.  It does not
 *    have to provide the member functions \a fnVal() or \a fnLogVal(), nor does it have to
 *    provide the type \a FnValueType.
 *
 */



// =============================================================================
// MHWalkerParamsAdjuster Interface
// =============================================================================

/** \page pageInterfaceMHWalkerParamsAdjuster MHWalkerParamsAdjuster Interface
 *
 * This type is responsible for dynamically adjusting the parameters of a
 * Metropolis-Hastings random walk carried out by a \ref Tomographer::MHRandomWalk
 * instance.
 *
 * You can use \ref Tomographer::MHWalkerParamsNoAdjuster if you don't need to dynamically
 * adjust the parameters of the random walk.
 *
 * A \a MHWalkerParamsAdjuster compliant type should provide the following constant public
 * member value (e.g., as an enum):
 *
 * \par enum { AdjustmentStrategy = ... }
 *    Specify how often the parameters of the random walk should be adjusted.  The value
 *    should be a value in the \ref Tomographer::MHWalkerParamsAdjustmentStrategy enum.
 *
 * One should also provide the following member functions:
 *
 * \par void initParams(MHRWParamsType & params, const MHWalker & mhwalker, const MHRandomWalkType & mhrw)
 *    Called before starting the random walk. The \a params may be modified if desired.
 *    References to the \ref pageInterfaceMHWalker (\a mhwalker) and to the \ref
 *    Tomographer::MHRandomWalk instance (\a mhrw) are provided.
 *
 * \par void thermalizingDone(MHRWParamsType & params, const MHWalker & mhwalker, const MHRandomWalkType & mhrw)
 *    Called after the thermalization has finished. The \a params may be modified if
 *    desired.  References to the \ref pageInterfaceMHWalker (\a mhwalker) and to the \ref
 *    Tomographer::MHRandomWalk instance (\a mhrw) are provided.
 *
 * \par template<bool IsThermalizing, bool IsAfterSample> inline void adjustParams(MHRWParamsType & params, const MHWalker & mhwalker, CountIntType iter_k, const MHRandomWalkType & mhrw)
 *    This function is responsible for adjusting the random walk paramters (see \ref
 *    pageInterfaceMHWalker) stored in \a params (it should update the params in place).
 *    As convenience a reference to the \ref pageInterfaceMHWalker (\a mhwalker) and to
 *    the \ref Tomographer::MHRandomWalk instance (\a mhrw), as well as the iteration
 *    number \a iter_k, are provided.
 *
 * \par
 *    The template parameter \a IsThermalizing is set to \a true during the thermalization
 *    sweeps.  The parameter \a IsAfterSample is set to \a true if this function is called
 *    upon processing a live sample.  If <em>IsAfterSample==true</em>, then necessarily
 *    <em>IsThermalizing==false</em>.
 *
 * \par
 *    If the \a AdjustmentStrategy includes both the \a
 *    MHWalkerParamsAdjustEveryIteration and \a MHWalkerParamsAdjustEverySample bits, then
 *    while running, the callback \a adjustParams(...) will be called twice for the points
 *    that correspond to live samples: once after the iteration move, and once after
 *    processing the sample.
 *
 * \par
 *    More involved informaton (accept events, current point, etc.) are not provided here.
 *    If the parameters are to be adjusted based on some statistics taken on the random
 *    walk (which is usually the case), you should use a MHRWStatsCollector and point your
 *    adjuster to that stats collector to get its information.  For example, look at \ref
 *    Tomographer::MHRWMovingAverageAcceptanceRatioStatsCollector and \ref
 *    Tomographer::MHRWStepSizeAdjuster.
 *
 */


// =============================================================================
// ValueCalculator Interface
// =============================================================================

/** \page pageInterfaceValueCalculator ValueCalculator Interface
 *
 * <em>This is a &lsquo;type interface.&rsquo; See \ref pageTypeInterfaces
 * for more info on what that is.</em>
 *
 * A \a ValueCalculator is responsible for calculating a particular value at a particular
 * point. This might be, for example, calculating the value of a figure of merit at
 * various point samples during a Metropolis-Hastings random walk.
 *
 * The type represented by \a PointType depends on the use of the \a ValueCalculator, and
 * is should be documented by whoever uses it. For example, a \ref
 * Tomographer::ValueHistogramMHRWStatsCollector will call the \a ValueCalculator using
 * the \a PointType of the random walk (see \ref pageInterfaceMHWalker).
 *
 * A \a ValueCalculator must be <em>copy-constructible</em>, and different threads must be
 * able to operate safely on different copies.
 *
 * \par typedef .. ValueType
 *     The type of the returned value.
 * 
 * \par ValueType getValue(PointType pt) const
 *     Get the value corresponding to a particular point \a pt.
 *
 */


// =============================================================================
// MHRandomWalkTaskCData
// =============================================================================

/** \page pageInterfaceMHRandomWalkTaskCData MHRandomWalkTaskCData Interface
 *
 * <em>This is a &lsquo;type interface.&rsquo; See \ref pageTypeInterfaces
 * for more info on what that is.</em>
 *
 * A \a MHRandomWalkTaskCData is an object which provides data about how to conduct a
 * repetition of random walks, while collecting statistics. It may store constant global
 * data.
 *
 * A \a MHRandomWalkTaskCData must inherit \ref
 * Tomographer::MHRWTasks::CDataBase<CountIntType,StepRealType>, in order to expose some basic
 * types and functions.
 *
 * A \a MHRandomWalkTaskCData must be copy-constructible, and different threads must be
 * able to operate safely on different copies.
 *
 *
 * \par typedef .. MHRWStatsCollectorResultType
 *
 * \par MHRWStatsCollectorType createStatsCollector(LoggerType & logger) const
 *     Create an \a MHRWStatsCollector -type instance to use. This must be a type which
 *     compiles both with the \ref pageInterfaceMHRWStatsCollector and the \ref
 *     pageInterfaceResultable. It must have as its \a ResultType the type given as \a
 *     MHRWStatsColelctorResultType. 
 *
 * \par
 *     The logger may be used to log messages, and may be passed on to the stats collector
 *     for the same purpose.  Use a template parameter for \a LoggerType.
 *
 * \par MHWalker createMHWalker(Rng & rng, LoggerType & logger) const
 *     Create an \a MHWalker -type instance. This may be any \ref pageInterfaceMHWalker
 *     -compliant type. The \a Rng parameter is the same type as provided to the
 *     MHRWTasks::MHRandomWalkTask template parameter, use a template argument for this
 *     function in case. Use a template parameter for \a LoggerType.
 *
 */


// =============================================================================
// Histogram
// =============================================================================

/** \page pageInterfaceHistogram Histogram Interface
 *
 * <em>This is a &lsquo;type interface.&rsquo; See \ref pageTypeInterfaces
 * for more info on what that is.</em>
 *
 * \par typedef .. Scalar
 *      Type used to quantify the quantity which is binned into separate bins
 * 
 * \par typedef .. CountType
 *      Type used to count the number of hits in each bin
 *
 * \par static constexpr bool HasErrorBars = ..
 *      Whether this Histogram type can provide error bars (e.g. obtained e.g. through
 *      binning analysis, or by averaging several histograms)
 *
 * In the following, we use \a std::size_t as indexing type, but it can be replaced by any
 * other integral type. You should use \a std::size_t if you store your histogram as a
 * dense object (that's the type which can hold the size of the largest possible object
 * which can be stored in memory).
 *
 * \par std::size_t numBins() const
 *      The number of bins in this histogram.
 *
 * \par CountType count(std::size_t i) const
 *      Number of counts in the bin number i
 *
 * \par CountType errorBar(std::size_t i) const
 *      <em>(Only if <code>HasErrorBars = true</code>)</em> Error bar (standard deviation)
 *      associated to the bin number i.
 * 
 */


// =============================================================================
// OperatorNewProviderType
// =============================================================================

// ----need to clarify all this: TODO DOC --------
/* * \page pageInterfaceOperatorNewProviderType OperatorNewProviderType Interface
 *
 * <em>This is a &lsquo;type interface.&rsquo; See \ref pageTypeInterfaces
 * for more info on what that is.</em>
 *
 * \par operator new( ... )
 *   Define any required or desired implementations of <em>operator new</em>, as may be
 *   required for particular types (see \ref Tomographer::Tools::NeedOwnOperatorNew)
 *
 * \par typedef <own type> OperatorNewProviderType;
 *   A member type, typedef-ing the provider type itself as \a OperatorNewProviderType .
 *
 * \par template<typename T> struct OperatorNewAllocatorType { typedef .. Type; }
 *   A template struct member, capable of specifying which allocator type to use for
 *   std::vector's and other STL types of contained value \a T.
 *
 * For an example, see \ref Tomographer::Tools::EigenAlignedOperatorNewProvider.
 *
 */
