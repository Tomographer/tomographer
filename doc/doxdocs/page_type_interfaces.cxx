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
 * \par void thermalizing_done()
 * is called after the thermalizing runs, before starting the "live" runs.
 * 
 * \par void done()
 * is called after the random walk is finished.
 *
 * \par void process_sample(CountIntType k, CountIntType n, const PointType & pt, FnValueType fnval, MHRandomWalk & rw)
 *     is called at the end of each sweep, after the thermalization sweeps have finished.
 *     This function is meant to actually take live samples. \a k is the raw iteration
 *     number, \a n is the sample number (= number of live samples already taken), \a pt
 *     the current point of the walk, \a fnval the value of the function at this point
 *     (this may be the value of the MH jump function, its logarithm, or a dummy value,
 *     depending on the random walk's MHWalker::UseFnSyntaxType, see \ref
 *     pageInterfaceMHWalker).
 *
 * \par void raw_move(CountIntType k, bool is_thermalizing, bool is_live_iter, bool accepted, double a, const PointType & newpt, FnValueType newptval, const PointType & curpt, FnValueType curptval, MHRandomWalk & rw)
 *     is called after each move during the random walk. Note that if you want to take
 *     real samples, use \c process_sample() instead.
 *
 * \par
 *     \c k is the iteration number (which is reset to zero after the thermalizing
 *     sweeps), \c is_thermalizing is \c true during the first part of the random walk
 *     during the thermalizing runs, \c is_live_iter is set to \c true only if a sample is
 *     taken at this point, i.e. if not thermalizing and after a full sweep. \c accepted
 *     indicates whether this Metropolis-Hastings move was accepted or not and \c a gives
 *     the ratio of the function which was tested for the move. (Note that \c a might not
 *     be calculated and left to 1 if known to be greater than 1.) \c newpt and \c
 *     newptval are the new proposal jump point and the function value at that new
 *     point. The function value is either the actual value of the function, or its
 *     logarithm, or a dummy value, depending on \c MHWalker::UseFnSyntaxType.  Similarly
 *     \c curpt and \c curptval are the current point and function value. The object \c rw
 *     is a reference to the random walk object instance.
 * 
 *
 *
 * \todo API CHANGE: change names -> camel case (?)
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
 * \par typedef .. Result
 *    The type that the result has
 *
 * \par Result getResult()
 *    Obtain the said result. The return type must be anything that may be assigned to a
 *    \a Result type, or a value that the \a Result accepts in a constructor.
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
 * \a MHWalker types are used in particular by:
 *   - \ref Tomographer::MHRandomWalk.
 *
 * A type implementing the \a MHWalker interface must provide the following types:
 *
 * \par typedef ... PointType
 *     The type needed to represent a point in state space in which we are performing a
 *     random walk. An object of such type is never default-constructed, but always
 *     copy-constructed from another \a PointType. One should also be able to assign a \a
 *     PointType to another \c PointType (e.g. <code>curpt = other_point</code>).
 *
 * \par typedef ... StepRealType
 *     The type needed to represent a step size. This will most likely be a \c double
 *     or some floating-point type.
 *
 * \par typedef ... FnValueType &mdash; required only if UseFnSyntaxType != MHUseFnRelativeValue
 *     The return value type of the function evaluated at each point during the
 *     Metropolis-Hastings random walk. Usually this is \c double or some floating-point
 *     type. You do not need to provide this typedef if \c UseFnSyntaxType is set to \c
 *     MHUseFnRelativeValue.
 *
 * A \a MHWalker must provide the following constant enumeration values:
 *
 * \par enum { UseFnSyntaxType = ... }
 *     Specifies how we calculate the function probability ratio of two points in the
 *     random walk. \c UseFnSyntaxType should be set to one of either \ref
 *     Tomographer::MHUseFnValue, \ref Tomographer::MHUseFnLogValue, or \ref
 *     Tomographer::MHUseFnRelativeValue. See \ref
 *     labelMHWalkerUseFnSyntaxType "Role of UseFnSyntaxType".
 *
 *
 * And must provide the following members:
 *
 * \par MHWalker(MHWalker&& other)
 *     A MHWalker type must have a move constructor. (Of course, replace \a "MHWalker" by
 *     the name of your class).
 *
 * \par void init()
 *
 * \par PointType startpoint()
 *
 * \par void thermalizing_done()
 *
 * \par void done()
 *
 * \par PointType jump_fn(const PointType & curpt, StepRealType step_size)
 *
 * \par FnValueType fnval(const PointType & curpt) &mdash; required only if UseFnSyntaxType == MHUseFnValue
 *
 * \par FnValueType fnlogval(const PointType & curpt) &mdash; required only if UseFnSyntaxType == MHUseFnLogValue
 *
 * \par double fnrelval(const PointType & newpt, const PointType & curpt) &mdash; required only if UseFnSyntaxType == MHUseFnRelativeValue
 *
 * <br><br>
 * 
 * \anchor labelMHWalkerUseFnSyntaxType
 * \par Role of \c UseFnSyntaxType:
 *
 *  - MHUseFnValue --> use MHWalker::fnval(newpt)
 *  - MHUseFnLogValue --> use MHWalker::fnlogval(newpt)
 *  - MHUseFnRelativeValue --> use MHWalker::fnrelval(newpt, curpt)
 *
 *
 * \todo WRITE DOC .........................
 *
 *
 * \todo API CHANGE: change names -> camel case (?)
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
 * \par MHRWStatsCollectorType createStatsCollector() const
 *     Create an \a MHRWStatsCollector -type instance to use. This must be a type which
 *     compiles both with the \ref pageInterfaceMHRWStatsCollector and the \ref
 *     pageInterfaceResultable. It must have as its \a Result the type given as \a
 *     MHRWStatsColelctorResultType.
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
 * \par std::size_t num_bins() const
 *      The number of bins in this histogram.
 *
 * \par CountType count(std::size_t i) const
 *      Number of counts in the bin number i
 *
 * \par CountType errorbar(std::size_t i) const
 *      <em>(Only if <code>HasErrorBars = true</code>)</em> Error bar (standard deviation)
 *      associated to the bin number i.
 * 
 *
 * \todo API CHANGE: change name \c num_bins() -> camel case
 */
