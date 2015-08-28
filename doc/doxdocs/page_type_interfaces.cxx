
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
 *  - \subpage pageInterfaceMatrQ
 *  - \subpage pageInterfaceTomoProblem
 *  - \subpage pageInterfaceRandomWalk
 *  - \subpage pageInterfaceMHWalker
 *  - \subpage pageInterfaceMHRWStatsCollector
 *  - \subpage pageInterfaceResultable
 *  - \subpage pageInterfaceValueCalculator
 *  - \subpage pageInterfaceMHRandomWalkTaskCData
 *  - \subpage pageTaskManagerDispatcher
 *  - \subpage pageInterfaceHistogram
 *
 */


// =============================================================================
// MatrQ
// =============================================================================

/** \page pageInterfaceMatrQ MatrQ Interface
 *
 * Declares types for dealing with quantum states and POVMs of a fixed dimension which
 * we'll call here \a dim. See also \ref Tomographer::MatrQ.
 *
 * \par typedef ... RealScalar
 * The real scalar type we're working with.
 *
 * \par typedef ... ComplexScalar
 * The complex scalar type we're working with.
 *
 * \par typedef ... IntFreqType
 * The integral type to use for measurement counts. Usually \c int is enough, except if in
 * your experiment you take >1e9 measurements
 *
 * \par typedef ... MatrixType
 * The type needed to represent a density matrix. This is usually expected to be a Eigen
 * type.
 *
 * \par MatrixType initMatrixType() const
 * Returns an expression which can be assigned to a \a MatrixType such that the matrix is
 * initialized to a square \a dim x \a dim zero matrix.
 *
 * \par typedef ... VectorParamType
 * The type needed to represent a X-parameterization of a density matrix. This is usually
 * expected to be a Eigen type.
 *
 * \par MatrixType initVectorParamType() const
 * Returns an expression which can be assigned to a \a VectorParamType such that the
 * vector is initialized to a zero column vector with <em>dim*dim</em> entries.
 *
 * \par typedef ... VectorParamNdofType
 * The type needed to represent some parameterization of a density matrix with
 * <em>dim*dim-1</em> parameters. This is usually expected to be a Eigen type.
 *
 * \par MatrixType initVectorParamNdofType() const
 * Returns an expression which can be assigned to a \a VectorParamNdofType such that the
 * vector is initialized to a zero column vector with <em>dim*dim-1</em> entries.
 *
 * \par typedef ... VectorParamListType
 * The type needed to represent a list of X-parameterizations of e.g. POVM effects, each
 * with <em>dim*dim</em> parameters. This is usually expected to be a Eigen type.
 *
 * \par MatrixType initVectorParamListType(std::size_t length) const
 * Returns an expression which can be assigned to a \a VectorParamListType such that the
 * list is initialized to a list with \a length copies of a zero column vector with
 * <em>dim*dim</em> entries.
 *
 * \par typedef ... FreqListType
 * The type needed to represent frequency counts of measurements. This is typically an
 * \ref Eigen::Array integral type. 
 *
 * \par MatrixType initFreqListType(std::size_t len) const
 * Returns an expression which can be assigned to a \a FreqListType such that the vector
 * is initialized to \a len items with zero counts each.
 *
 *
 */


// =============================================================================
// TomoProblem
// =============================================================================

/** \page pageInterfaceTomoProblem TomoProblem Interface
 *
 * Stores the data relevant for a tomography problem. Includes:
 *
 * - a \ref pageInterfaceMatrQ object, which stores the dimension and is able to construct
 *   requested types;
 *
 * - cached values of \a dim, <em>dim2=dim*dim</em>, and <em>Ndof=dim*dim-1</em>
 *
 * - a factor by which to artificially amplify/reduce the number of frequency counts
 *
 * - the list of POVM effects and their frequency counts (measurement data)
 *
 * - the maximum likelihood estimate, along with cached values of various of its
 *   parameterizations
 *
 * - a method which can calculate the loglikelihood function.
 *
 * See also: \ref Tomographer::IndepMeasTomoProblem<MatrQ_,LLHValueType_,UseCLoopInstead>
 *
 *
 * \par const MatrQ matrq;
 * A \ref pageInterfaceMatrQ object which we may use to create matrices etc.
 *
 * \par const IntegralType dim;
 * Cached dimension.
 *
 * \par const IntegralType dim2;
 * Cached value of <em>dim*dim</em>
 *
 * \par const IntegralType Ndof;
 * Cached value of <em>dim*dim-1</em>
 *
 * \par typedef .. LLHValueType;
 * The type to use to store the value of the loglikelihood function calculated by \a
 * calc_llh().
 *
 * \par LLHValueType NMeasAmplifyFactor;
 * Factor by which to multiply all measurement frequencies when calculating the
 * loglikelihood function
 * 
 * \par typename MatrQ::VectorParamListType Exn;
 * The POVM Entries, parameterized with \ref pageParamsX
 *
 * \par typename MatrQ::FreqListType Nx;
 * The frequency list, i.e. number of times each POVM effect was observed
 *
 * \par typename MatrQ::MatrixType rho_MLE;
 * Maximum likelihood estimate as density matrix
 *
 * \par typename MatrQ::MatrixType T_MLE;
 * Maximum likelihood estimate as \ref pageParamsT of the density matrix
 *
 * \par typename MatrQ::VectorParamType x_MLE;
 * Maximum likelihood estimate as \ref pageParamsX
 *
 * \par LLHValueType calc_llh(const typename MatrQ::VectorParamType & x) const;
 * Calculate the loglikelihood function, defined as
 * \f[
 *   \lambda\left(\rho\right) = -2\,\ln\,\mathrm{tr}\left[B^n\,\rho^{\otimes n}\right]\ .
 * \f]
 * The argument \a x is the \ref pageParamsX of the density matrix at which the
 * loglikelihood function should be evaluated.
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
 * \par void process_sample(CountIntType k, CountIntType n, const PointType & pt, FnValueType fnval, MHRandomWalk & rw)
 *     is called at the end of each sweep, after the thermalization sweeps have finished.
 *     This function is meant to actually take live samples. \a k is the raw iteration
 *     number, \a is the sample number (= number of live samples already taken), \a pt the
 *     current point of the walk, \a fnval the value of the function at this point (this
 *     may be the value of the MH jump function, its logarithm, or a dummy value,
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
 */


// =============================================================================
// Resultable
// =============================================================================

/** \page pageInterfaceResultable Resultable Interface
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
 * \par PointType startpoint()
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
 *   - \ref Tomographer::RandomWalkBase.
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


/** \page pageInterfaceValueCalculator ValueCalculator Interface
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


/** \page pageInterfaceMHRandomWalkTaskCData MHRandomWalkTaskCData Interface
 *
 * A \a MHRandomWalkTaskCData is an object which provides data about how to conduct a
 * repetition of random walks, while collecting statistics. It may store constant global
 * data.
 *
 * A \a MHRandomWalkTaskCData must inherit \ref
 * Tomographer::MHRWTasks::CDataBase<CountIntType,RealType>, in order to expose some basic
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


/** \page pageInterfaceHistogram Histogram Interface
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
 */
