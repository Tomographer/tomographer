
// =============================================================================
// Type Interfaces -- Title Page
// =============================================================================

/** \page pageTypeInterfaces Type Interfaces
 *
 * These pages document <em>type interfaces</em>: i.e. types which may appear as template
 * parameters, and which must conform to some standard in order to complete their tasks.
 *
 * Documented Type Interfaces in the %Tomographer framwork are:
 *
 *  - \subpage pageInterfaceRandomWalk
 *  - \subpage pageInterfaceMHWalker
 *  - \subpage pageInterfaceMHRWStatsCollector
 *  - \subpage pageInterfaceTomoValueCalculator
 *  - \subpage pageTaskManagerDispatcher
 *
 */


// =============================================================================
// MHRWStatsCollector
// =============================================================================

/** \page pageInterfaceMHRWStatsCollector MHRWStatsCollector Interface
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
 * \par void process_sample(CountIntType, const PointType & pt, FnValueType fnval, MHRandomWalk & rw)
 *     is called at the end of each sweep, after the thermalization sweeps have finished. 
 *     This function is meant to actually take live samples.
 *
 * \par void raw_move(CountIntType k, bool is_thermalizing, bool is_live_iter, bool accepted, double a, const PointType & newpt, FnValueType newptval, const PointType & curpt, FnValueType curptval, MHRandomWalk & rw)
 *     is called after each move during the random walk. Note that if you want to take
 *     real samples, use \c process_sample() instead.
 *
 * \par
 *     \c k is the iteration number, \c is_thermalizing is \c true during the first part
 *     of the random walk during the thermalizing runs, \c is_live_iter is set to \c true
 *     only if a sample is taken at this point, i.e. if not thermalizing and after a full
 *     sweep. \c accepted indicates whether this Metropolis-Hastings move was accepted or
 *     not and \c a gives the ratio of the function which was tested for the move. (Note
 *     that \c a might not be calculated and left to 1 if known to be greater than 1.) \c
 *     newpt and \c newptval are the new proposal jump point and the function value at
 *     that new point. The function value is either the actual value of the function, or
 *     its logarithm, or a dummy value, depending on \c MHWalker::UseFnSyntaxType. 
 *     Similarly \c curpt and \c curptval are the current point and function value. The
 *     object \c rw is a reference to the random walk object instance.
 * 
 *
 */




// =============================================================================
// MHWalker
// =============================================================================

/** \page pageInterfaceMHWalker MHWalker Interface
 *
 * A \a MHWalker compliant type describes a particular Metropolis-Hastings walk on some
 * state space. It takes care for example of providing candidate new points (jump
 * function), and calculating the probability ratio for the jump.
 *
 * \a MHWalker types are used in particular by:
 *  - \refitem Tomographer::MHRandomWalk.
 *
 * A type implementing the \a MHWalker interface must provide the following types:
 *
 * \par typedef ... PointType
 *     The type needed to represent a point in state space in which we are performing a
 *     random walk. An object of such type is never default-constructed, but always
 *     copy-constructed from another \a PointType. One should also be able to assign a \a
 *     PointType to another \c PointType (e.g. <code>curpt = other_point</code>).
 *
 * \par typedef ... RealScalar
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
 * \par void init()
 *
 * \par void thermalizing_done()
 *
 * \par void done()
 *
 * \par PointType jump_fn(const PointType & curpt, RealScalar step_size)
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
 * .........................
 *
 */




// =============================================================================
// RandomWalk
// =============================================================================



/** \page pageInterfaceRandomWalk RandomWalk Interface
 *
 * The \a RandomWalk type is responsible for actually implementing the random walk. It
 * should keep the current state of the random walk in memory and update it when the \c
 * move() function is called.
 *
 * \a RandomWalk types are used in particular by:
 *  - \refitem Tomographer::RandomWalkBase.
 *
 * The \a RandomWalk type should provide the following typedef member:
 *
 * \par typedef ... CountIntType
 *          This (usually an <code>unsigned int</code> or <code>unsigned long</code>)
 *          should specify which type to use to count the iterations. This is also the
 *          return type of \c n_sweep() etc.
 *
 * \a RandomWalk needs to provide the following members, which are called at the
 * appropriate times:
 *
 * \par CountIntType RandomWalk::n_sweep()
 *          Number of iterations that compose a "sweep".
 *
 * \par CountIntType RandomWalk::n_therm()
 *          Number of thermalizing sweeps to perform.
 *
 * \par CountIntType RandomWalk::n_run()
 *          Number of live sweeps to perform.
 *
 * \par void RandomWalk::move(CountIntType k, bool is_thermalizing, bool is_live_iter)
 *          Is called to perform a new random walk iteration. The random walk object is
 *          responsible for keeping the current state of the random walk in memory,
 *          and for processing a jump function. This method should update the internal
 *          state of the random walk. This function does not return anything. \c k is the
 *          raw iteration count, starting at zero (and which is reset to zero after the
 *          thermalizing sweeps). \c is_thermalizing is \c true during the thermalizing
 *          runs, \c false otherwise. \c is_live_iter is \c true when a live sample is
 *          taken, only once every sweep after the thermalization runs.
 *
 * \par void RandomWalk::process_sample(CountIntType k)
 *          Is called for each "live" point for which a sample should be taken. The point
 *          in question is the current state of the random walk. This only happens after
 *          thermalization, and at the last iteration of a sweep.
 *
 * \par void RandomWalk::init()
 *          Is called before starting the random walk. The RandomWalk may perform custom
 *          last-minute initializations if required.
 *
 * \par void RandomWalk::thermalizing_done()
 *          Is called after the thermalizing runs and before starting the live runs.
 *
 * \par void RandomWalk::done()
 *          Is called after the random walk is finished. This happens after the given
 *          number of iterations were reached.
 *
 */


/** \page pageInterfaceTomoValueCalculator TomoValueCalculator Interface
 *
 * \todo ............ doc. ..................
 *
 * NOTE: must be copy-constructible, and different threads must be able to operate safely
 * on different copies.
 */
