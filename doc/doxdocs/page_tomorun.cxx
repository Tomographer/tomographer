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


/** \page pageTomorun Tomorun &mdash; Standalone Executable
 *
 * The \c tomorun program is simply a straightforward piecing together of the
 * different components available in the %Tomographer C++ framework.
 *
 * It can be used as a standalone program, giving it the measurement data as
 * input (via a MATLAB data file), and retrieving the final histogram of the
 * figure of merit as output.
 *
 * For help on using \a tomorun, refer to <a
 * href="https://tomographer.github.io/tomographer/get-started"
 * target="_blank">this "getting started" page</a>, or query its help text:
 *
 * \code
 *   > tomorun --help
 * \endcode
 *
 * The \c tomorun code itself is not included in this API documentation.  The
 * code is located under \c "tomorun/".  If you wish to change functionality in
 * \c tomorun, or if you wish to implement a very particular calculation, you
 * might like to have a look at the test example \c "test/minimal_tomorun.cxx",
 * which provides a very minimal implementation of tomorun for a specific
 * example&mdash;it may be more convenient for you to modify that program.
 *
 *   - \subpage pageTomorunConfigBuild
 * 
 *   - \subpage pageTomorunNewFigureOfMerit
 *
 *   - \subpage pageCustomTomorunExe
 */




/** \page pageTomorunConfigBuild Tomorun optimized custom build configuration
 *
 * You may compile a custom version of \a tomorun, which will better suit your
 * needs and be better adapted your specific computer architecture compared to
 * the generic binary releases of Tomographer/Tomorun.
 *
 * Proceed <a
 * href="https://tomographer.github.io/tomographer/download/#downloading--installing-tomorun-from-source"
 * target="_blank">as described here</a>.  When you arrive at the step where you
 * run \c cmake, make sure you specify the options which are relevant to you
 * using the \c -DTOMORUN_CXX_FLAGS=... option.
 *
 * For instance, if you will only run \a tomorun on two-qubit systems:
 * \code
 *     build> cmake .. -DTOMORUN_CXX_FLAGS="-DTOMORUN_CUSTOM_FIXED_DIM=4 -DTOMORUN_CUSTOM_FIXED_MAX_DIM=4 -DTOMORUN_CUSTOM_MAX_POVM_EFFECTS=Eigen::Dynamic"
 * \endcode
 *
 * Or, for instance, if you're sure you will not need very verbose log messages
 * (LONGDEBUG messages will be unconditionally discarded, with a slight runtime
 * speed-up):
 * \code
 *     build> cmake .. -DTOMORUN_CXX_FLAGS="-DTOMORUN_MAX_LOG_LEVEL=DEBUG"
 * \endcode
 *
 * Or, for instance, if you would like to compile \a tomorun using types with
 * less precision, in order to gain speed (at the loss of precision though!)
 * \code
 *     build> cmake .. -DTOMORUN_CXX_FLAGS="-DTOMORUN_INT=int -DTOMORUN_REAL=float"
 * \endcode
 *
 *
 * The possible compile-time options you can set are documented here:
 *
 * - \c TOMORUN_INT
 *
 *   Value: a integer type
 *
 *   The main integer type. Used to count a number of iterations, the sweep size, etc.
 *
 * - \c TOMORUN_REAL
 *
 *   Value: a floating-point type (e.g., \c float, \c double, <code>long double</code>)
 *
 *   The main floating-point type. Used for everything, from the matrix elements
 *   of the quantum state to the step size of the random walk.  You may try to
 *   go to <code>long double</code> if you have trouble with precision --- but
 *   beware, I haven't tested this and there might still be some hard-coded
 *   values at some places (1e-8 epsilons etc.). Beware and please report issues
 *   to me!
 *
 *   \note Using \c float here apparently significantly reduces the precision of the
 *         resulting histogram.  Make sure you know what you are doing.
 *
 * - \c TOMORUN_CUSTOM_FIXED_DIM, \c TOMORUN_CUSTOM_FIXED_MAX_DIM, \c
 *   TOMORUN_CUSTOM_MAX_POVM_EFFECTS
 *
 *   You may define these to fixed values to specialize the tomorun problem to a
 *   specific fixed dimension and a specific maximum number of POVM effects.
 *   You may set to \c Eigen::Dynamic to always use dynamic size matrices which
 *   may allow any size at run-time.  Leave these symbols undefined to have a
 *   selection of common values of fixed-size matrices with a fallback to
 *   dynamic-size.
 *
 *   \warning You need to define all three macros, not just one or two of them.
 *
 *   \c TOMORUN_CUSTOM_FIXED_DIM fixes the dimension of the system to a
 *   compile-time fixed value which cannot be changed at run-time.  Use
 *   \c Eigen::Dynamic if you want tomorun to work with different system sizes.
 *   \c TOMORUN_CUSTOM_FIXED_MAX_DIM specifies a maximum dimension for the
 *   dimension of the quantum system; the latter may at run-time take any value
 *   up to this limit; use \c Eigen::Dynamic for no limit.
 *
 *   If these are not defined (the default), then some common cases are provided
 *   with a fallback to all-dynamic specified at runtime. (See bottom of
 *   tomorun.cxx)
 *
 * - \c TOMORUN_MAX_LOG_LEVEL
 *
 *   Value: one of \c LONGDEBUG, \c DEBUG, \c INFO, \c WARNING, \c ERROR.
 *
 *   If defined, will compile out all log messages of level strictly less severe
 *   than the level provided here.
 *   
 *
 * - \c TOMORUN_RNG_CLASS
 *
 *   Value: C++ pseudo random number generator name (e.g. \c std::mt19937)
 *
 *   The random number generator algorithm to use for the random walk, given as
 *   a C++ class name (see C++11's \c <random>).
 *
 * - \c TOMORUN_USE_DEVICE_SEED
 *
 *   Set to a nonzero value to seed the pseudo-random number generators with a
 *   seed taken from a physical random device (see also \c TOMORUN_RANDOM_DEVICE).
 *
 *   Set to zero to not attempt to access any random device (the code won't even
 *   refer to \c std::random_device).  The pseudo-rng's will be seeded using
 *   consecutive seeds starting from a base seed derived from the current time.
 *   
 *
 * - \c TOMORUN_RANDOM_DEVICE
 *
 *   The name of the physical random device (see C++ \c std::random_device
 *   constructor) to use to seed the pseudo-RNG.
 *
 *   This setting doesn't have any effect unless \c TOMORUN_USE_DEVICE_SEED is
 *   set.
 *
 *   You may leave an empty macro definition to use the default constructor and
 *   use the default device.
 *
 *
 *
 *
 * Less important options which probably shouldn't be specified:
 *
 * - \c TOMORUN_DO_SLOW_POVM_CONSISTENCY_CHECKS
 *
 *   Value: \c true | \c false
 *
 *   If defined, will make sure that all POVM effects read from the input file
 *   are positive semidefinite and nonzero.  This doesn't affect the random walk
 *   at all, it's only a constant overhead at start-up.
 *
 *   This option is on by default.
 *
 * - \c TOMORUN_TIMERCLOCK
 *
 *   Value: C++ clock class name (std::chrono::...) [default: \c
 *          std::chrono::high_resolution_clock ]
 *
 *   C++ clock type to use when timing the duration of the computation.  This
 *   option is really historical and was meant to allow compilation with older
 *   compilers such as G++ 4.6 which didn't yet support the new C++ clock types.
 *
 */






/** \page pageTomorunNewFigureOfMerit Tomorun and custom figures of merit
 *
 * The \c tomorun executable has several figures of merit built into the
 * program: the trace distance, the purified distance, or the fidelity to any
 * reference state, as well as the expectation value of an observable.  If you
 * wish to produce a histogram of a different figure of merit which can't be
 * cast into one of these, you have the following options.
 *
 * I.   Use the Python module, which allows you to specify any custom figure of
 *      merit given as a Python callable;
 *
 * II.  Write a small, special-purpose C++ program which does exactly what you
 *      need, in which you can code your custom figure of merit;
 *
 * III. Modify the source of the \c tomorun program itself, to add your new
 *      figure of merit.
 *
 * Option I is the simplest, and should be your default choice; options II and
 * III require a bit more work.
 * 
 * Option II may be easier if you have a very special purpose which might not
 * warrant inclusion into the generic \c tomorun program.  You can simply
 * combine the required tools into a new special-purpose program.  This is not
 * difficult, and there are already examples ready&mdash;see \ref
 * pageCustomTomorunExe.
 *
 * In the following we describe the necessary steps for Option III.
 *
 * \note If you perform modifications which may be useful to others, please
 *       <b>fork the repository on github</b>, perform your changes, and send me
 *       a pull request.  This way your changes will be availble to other users
 *       who would like to use the %Tomographer project.  Please see <a
 *       href="https://github.com/Tomographer/tomographer/blob/master/README.md#contributing">here</a>
 *       for information on how to contribute.
 *
 * We'll illustrate these steps with a simple example: the two-norm distance
 * (aka. the Hilbert-Schmidt distance) to any reference state, defined by \f$
 * d_\mathrm{HS}(\rho,\rho_{\mathrm{Ref}}) = \Vert
 * \rho-\rho_{\mathrm{Ref}}\Vert_2 \f$, with \f$ \Vert A\Vert_2 =
 * \mathrm{tr}\left(A^\dagger A\right) \f$.
 *
 *
 * <h2>1. Code how to calculate your figure of merit</h2>
 *
 * First, you should write the code which calculates the figure of merit,
 * complying to a \ref pageInterfaceValueCalculator type interface.  Your new
 * class should in particular have a method <tt>getValue(const MatrixType &
 * T)</tt> taking as argument an <a
 * href="http://eigen.tuxfamily.org/dox/group__TutorialMatrixClass.html">Eigen
 * matrix type</a> which will be a matrix square root (see \ref pageParamsT) of
 * the quantum state \a rho for which the function should calculate the figure
 * of merit.
 *
 * You should do this in a new header file, that's the easiest.
 *
 * For our example, we can get inspired by the code for, e.g., \ref
 * Tomographer::DenseDM::TSpace::TrDistToRefCalculator defined in \ref
 * tspacefigofmerit.h.  For example, let's create a file called \c "hs_dist.h"
 * inside the \c "tomorun/" directory of the tomographer project:
 *
 * \code
 *   #ifndef HS_DIST_H
 *   #define HS_DIST_H
 *
 *   #include <Eigen/Eigen>
 *   #include <tomographer/tools/needownoperatornew.h>
 *   #include <tomographer/densedm/dmtypes.h>
 *
 *   template<typename DMTypes_, typename ValueType_ = double>
 *   class HSDistToRefCalculator
 *     : public virtual Tomographer::Tools::NeedOwnOperatorNew<typename DMTypes_::MatrixType>::ProviderType
 *   {
 *   public:
 *     typedef DMTypes_ DMTypes;
 *     // this is the type to use to store a dense (dim x dim) matrix:
 *     typedef typename DMTypes::MatrixType MatrixType;
 *     typedef typename DMTypes::MatrixTypeConstRef MatrixTypeConstRef;
 *   
 *     // For the ValueCalculator interface: the value type
 *     typedef ValueType_ ValueType;
 *
 *   private:
 *     const MatrixType rho_ref;
 *
 *   public:
 *     // Constructor, the reference state is 'rho_ref'
 *     TrDistToRefCalculator(MatrixTypeConstRef rho_ref_)
 *       : rho_ref(rho_ref_)
 *     {
 *     }
 *
 *     // This method actually calculates the given value.  The argument "T" is
 *     // the T-parameterization of the density matrix rho, which is simply a
 *     // square root of rho. (This is indeed the representation used during the
 *     // random walk.)
 *     inline ValueType getValue(MatrixTypeConstRef T) const
 *     {
 *       // rho is obtained with T*T.adjoint().  With Eigen, the HS-norm (Frobenius) of a
 *       // matrix is given by A.norm().
 *       return (T*T.adjoint() - rho_ref).norm();
 *     }
 *   };
 *
 *   #endif // HS_DIST_H
 * \endcode
 *
 * The part about \ref needownoperatornew.h and \ref
 * Tomographer::Tools::NeedOwnOperatorNew is to make sure that the object, when
 * created, is aligned in memory.  This is needed because the object has a Eigen
 * member (\a rho_ref) which must be aligned in memory for vectorized operations
 * (see Eigen's docs for alignment issues, there's a lot on that).  In the
 * Tomographer project, the \ref Tomographer::Tools::NeedOwnOperatorNew virtual
 * base makes sure that the necessary <code>operator new()</code> is defined so
 * that objects are aligned in memory when allocated.
 *
 *
 * <h2>2. Integrate the new figure of merit into the \c tomorun program</h2>
 *
 * In order to integrate a new figure of merit into the \c tomorun program, one
 * simply has to declare it in the \c "tomorun/tomorun_figofmerit.h" header
 * file.
 *
 * All you have to do is (1) Write a class which describes how to create the
 * value calculator for this figure of merit, what help text to display, and the
 * option name; and (2) add this class to the list of known figures of merit (at
 * the bottom of the file).
 *
 * For (1), in our example, we might define the class \a HsDistFigureOfMerit as
 * follows. (Also, we need to add <code>\#include "hs_dist.h"</code> at the top
 * of the file.)
 *
 * \code
 *  struct HsDistFigureOfMerit
 *  {
 *    // The name of the figure merit for the --value-type option: here, --value-type="HS-dist"
 *    const std::string name{"HS-dist"};
 *  
 *    // The ValueCalculator class we need to use, for the given DMTypes
 *    template<typename DMTypes> using ValueCalculator = HSDistToRefCalculator<DMTypes,TomorunReal>;
 *  
 *    // The code which instanciates a new ValueCalculator with the appropriate input data
 *    template<typename DMTypes>
 *    static inline ValueCalculator<DMTypes> *
 *    createValueCalculator(DMTypes dmt, const std::string & ref_obj_name, Tomographer::MAT::File * matf)
 *    {
 *      // create the value calculator
 *      return new ValueCalculator<DMTypes>(read_ref_state_rho<DMTypes>(matf, ref_obj_name)) ; 
 *    }
 *  
 *    // Print some help text to the screen when queried with --help. We can insert footnotes
 *    // using footnotes.addFootNote(...)
 *    static void print(std::ostream & stream, Tomographer::Tools::FmtFootnotes & footnotes) {
 *      stream <<
 *        "The Hilbert-Schmidt distance to a reference state "
 *        << footnotes.addFootNote(
 *            "The Hilbert-Schmidt distance is computed as d_{HS}(rho,sigma) = "
 *            "tr( (rho - sigma)^\\dagger (rho - sigma) )."
 *            )
 *        << ". <RefObject> should be the name of a MATLAB variable present in the "
 *        "MATLAB data file. This object should be a complex dim x dim matrix, the "
 *        "density matrix of the reference state. If no <RefObject> is specified, "
 *        "then 'rho_ref' is used."
 *        ;
 *    }
 *  };
 * \endcode
 *
 *
 * Finally, for (2), you have to insert your class name (here \a
 * HsDistFigureOfMerit) into the \a TomorunFiguresOfMerit tuple at the bottom of
 * the file, in order to make the program aware of this new figure of merit.
 *
 * Recompile \a tomorun (and fix any build issues etc.). You may now use your
 * custom figure of merit with the option \c "--value-type=HS-dist" or \c
 * "--value-type=HS-dist:varname".  Notice that it also appears in the help text
 * when you run \c "tomorun --help"
 * 
 */





// =============================================================================





/** \page pageCustomTomorunExe Creating a custom tomorun-like program
 *
 * The \c tomorun executable has several features and has a relatively rigid structure.
 * If it doesn't quite suit your needs, for example, if you have to integrate over a
 * different state space or using another parameterization, then you are probably better
 * off combining the correct C++ classes into a new, special-purpose C++ program (instead
 * of attempting to modify \c tomorun to include your use case).
 *
 * A very minimal version of a &quot;<code>tomorun</code>-like&quot; program is provided
 * in the test suite as \c "test/minimal_tomorun.cxx".
 *
 * A couple other examples in the same vein are provided, \c
 * "test/minimal_tomorun_controlled.cxx" and \c "test/minimal_single_random_walk.cxx".
 *
 * You may want to copy one of those examples into a new source file, change the way you
 * specify your inputs (use the \ref Tomographer::MAT classes to read inputs from a \c
 * matlab file, for example), and generally speaking adjust any other aspect of the
 * program you may want.
 *
 * The code in \c "test/minimal_tomorun.cxx" should be commented and understandable.
 * The basic logic is to use the \ref Tomographer::MHRWTasks::ValueHistogramTools classes,
 * in combination with \ref Tomographer::DenseDM::TSpace::LLHMHWalker, to run random walks
 * over quantum states in T-space (see \ref pageParamsT) and collect statistics about a
 * figure of merit.  If you have any questions, don't hesitate to ask me.
 * 
 */
