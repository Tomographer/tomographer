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


/** \page pageTomorunNewFigureOfMerit Using a custom figure of merit
 *
 * The \c tomorun executable has several figures of merit built into the program: the
 * trace distance, the purified distance, or the fidelity to any reference state, as well
 * as the expectation value of an observable.  If you wish to produce a histogram of a
 * different figure of merit which can't be cast into one of these, you have the following
 * options.
 *
 * I.   Use the Python module, which allows you to specify any custom figure of merit given
 *      as a Python callable;
 *
 * II.  Write a small, special-purpose C++ program which does exactly what you need, in
 *      which you can code your custom figure of merit;
 *
 * III. Modify the source of the \c tomorun program itself, to add your new figure of merit.
 *
 * Option I is the simplest, and should be your default choice; options II and III require
 * a bit more work.
 * 
 * Option II may be easier if you have a very special purpose which might not warrant
 * inclusion into the generic \c tomorun program.  You can simply combine the required
 * tools into a new special-purpose program.  This is not difficult, and there are already
 * examples ready&mdash;see \ref pageCustomTomorunExe.
 *
 * In the following we describe the necessary steps for Option III.
 *
 * \note If you perform modifications which may be useful to others, please <b>fork the
 *       repository on github</b>, perform your changes, and send me a pull request.  This
 *       way your changes will be availble to other users who would like to use the
 *       %Tomographer project.  Please see <a
 *       href="https://github.com/Tomographer/tomographer/blob/master/README.md#contributing">here</a>
 *       for information on how to contribute.
 *
 * We'll illustrate these steps with a simple example: the two-norm distance (aka. the
 * Hilbert-Schmidt distance) to any reference state, defined by \f$
 * d_\mathrm{HS}(\rho,\rho_{\mathrm{Ref}}) = \Vert \rho-\rho_{\mathrm{Ref}}\Vert_2 \f$,
 * with \f$ \Vert A\Vert_2 = \mathrm{tr}\left(A^\dagger A\right) \f$.
 *
 *
 * <h2>1. Code how to calculate your figure of merit</h2>
 *
 * First, you should write the code which calculates the figure of merit, complying to a
 * \ref pageInterfaceValueCalculator type interface.  Your new class should in particular
 * have a method <tt>getValue(const MatrixType & T)</tt> taking as argument an <a
 * href="http://eigen.tuxfamily.org/dox/group__TutorialMatrixClass.html">Eigen matrix
 * type</a> which will be a matrix square root (see \ref pageParamsT) of the quantum state
 * \a rho for which the function should calculate the figure of merit.
 *
 * You should do this in a new header file, that's the easiest.
 *
 * For our example, we can get inspired by the code for, e.g., \ref
 * Tomographer::DenseDM::TSpace::TrDistToRefCalculator defined in \ref tspacefigofmerit.h.
 * For example, let's create a file called \c "hs_dist.h" inside the \c "cxx/tomorun/"
 * directory of the tomographer project:
 * \code
 *   #ifndef HS_DIST_H
 *   #define HS_DIST_H
 *
 *   #include <Eigen/Eigen>
 *   #include <tomographer/tools/needownoperatornew.h>
 *   #include <tomographer/densedm/dmtypes.h>
 *
 *   template<typename DMTypes_, typename DistValueType_ = double>
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
 *     typedef DistValueType_ ValueType;
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
 * Tomographer::Tools::NeedOwnOperatorNew is to make sure that the object, when created,
 * is aligned in memory.  This is needed because the object has a Eigen member (\a
 * rho_ref) which must be aligned in memory for vectorized operations (see Eigen's docs
 * for alignment issues, there's a lot on that).  In the Tomographer project, the \ref
 * Tomographer::Tools::NeedOwnOperatorNew virtual base makes sure that the necessary
 * <code>operator new()</code> is defined so that objects are aligned in memory when
 * allocated.
 *
 *
 * <h2>2. Declare the new figure of merit as command line option value</h2>
 *
 * The choice of figure of merit is specified as the argument to the command line option
 * \c "--value-type". So you should in fact first decide of a short memo string describing
 * your figure of merit (e.g. for our example \c "HS-dist" would be an appropriate choice).
 *
 * Open the file \c "tomorun_opts.h" located inside the \c "cxx/tomorun/" directory.  The
 * locations where you should adapt the code are marked by comments saying \c "INSERT
 * CUSTOM FIGURE OF MERIT HERE".  Adapt the code as follows.
 *
 * The class \c val_type_spec is a type which is used to store the choice of figure of
 * merit.  It stores the choice as an enum value, but it is capable of converting to and
 * from a string. So you should add a new enumeration value, as well as adapt the class
 * methods, so that it understands your new figure of merit.
 * 
 * As with the other figures of merit, the user may also pass a string argument in the
 * form \c "HS-dist:string_argument_goes_here" which can specify for example a reference
 * state.  You don't have to do anything special for that, it's taken care of for you
 * already.
 *
 * Also change the command-line help text to include documentation for your new figure of
 * merit.
 *
 * In our example, we'd change the following enumeration inside the class
 * <tt>val_type_spec</tt> from:
 * \code
 *  enum ValueType {
 *    INVALID = 0,
 *    OBS_VALUE,
 *    TR_DIST,
 *    FIDELITY,
 *    PURIF_DIST
 *  };
 * \endcode
 * to:
 * \code
 *  enum ValueType {
 *    INVALID = 0,
 *    OBS_VALUE,
 *    TR_DIST,
 *    FIDELITY,
 *    PURIF_DIST,
 *    HS_DIST // new figure of merit: HS distance to some reference state
 *  };
 * \endcode
 * Then we would insert the following inside the method \a
 * val_type_spec::set_value_string(), after the similar tests for the other figures of
 * merit:
 * \code
 *   if (valtype_str == "HS-dist") {
 *     valtype = HS_DIST;
 *     ref_obj_name = ref_obj_name_str; // the reference state
 *     return;
 *   }
 * \endcode
 * and similarly, we'd update the function <tt>operator<<(std::ostream & str, const
 * val_type_spec & val)</tt> to include a case for our figure of merit:
 * \code
 *   case val_type_spec::HS_DIST:
 *     str << "HS-dist";
 *     break;
 * \endcode
 *
 * <h2>3. Instruct \c tomorun how to instantiate your figure of merit calculator</h2>
 *
 * The final piece of code which needs to be added is the logic of how your class which
 * calculates the figure of merit (in our example, inside \c "hs_dist.h") should be
 * instantiated.
 *
 * The relevant file to modify is the file named \c tomorun_dispatch.h, located
 * inside the \c "cxx/tomorun/" directory.
 *
 * First, include your \c "hs_dist.h" file near the top: insert the line
 * \code
 *   #include "hs_dist.h"
 * \endcode
 *  near the top of the file, below the other include directives.
 *
 * Then you'll have to code how specifically the program should instantiate your value
 * calculator. Add a conditional in the function <tt>tomorun_dispatch()</tt> which tests
 * whether the user asked for your figure of merit, and instantiate your class
 * appropriately.  The general logic should look like this:
 * \code
 *   if (opt->valtype.valtype == val_type_spec::<MY_CUSTOM_FIGURE_OF_MERIT>) {
 *
 *     ... instantiate ValueCalculator and dispatch to tomorun<...>(...)  ...
 *
 *     // Finally, dispatch the execution to tomorun<>(...):
 *     tomorun<BinningAnalysisErrorBars>(
 *         tomodat,            // the TomoProblem instance
 *         opt,                // the program options
 *         // and our ValueCalculator instance:
 *         MyCustomFigureOfMeritValueCalculator<...>(...),
 *         logger); // and finally the logger instance
 *     return;
 *   }
 * \endcode
 *
 * Starting in Tomographer version 2, the set up is slightly more complicated, because
 * there are two different ways \c tomorun can be compiled.  It can either be compiled by
 * using a \ref Tomographer::MultiplexorValueCalculator, or with static checks and
 * different static code for each figure of merit (as before).  The former approach seems
 * more efficient, and is now the default (this can be changed with CMake options when
 * compiling tomorun). The logic is not that compicated, so you're best off by seeing what
 * the code does for the built-in figures of merit and mimicking that.
 *
 * You're probably best off copying from the built-in examples inside that same function,
 * or the example for the Hilbert-Schmidt distance presented here. Here are some
 * additional tips:
 *
 *   - The object \c "opt->valtype.ref_obj_name" is an std::string of anything the user
 *     specified at the command line or in the config file as second part to the \c
 *     "--value-type" option (e.g., representing the name of the variable in the MATLAB
 *     data file containing the reference state density matrix).
 *
 *   - You can load data from the MATLAB data file via the \c "matf" object, which is a
 *     \ref Tomographer::MAT::File instance.
 *
 *   - You may of course use any tool provided by Eigen and the Tomographer API, for
 *     example \ref Tomographer::Tools::forcePosSemiDef() to ensure a matrix is positive
 *     semidefinite.
 *
 * In our example for the Hilbert-Schmidt distance, the reference state is read from the
 * MATLAB data file. It is to be taken by default to be \c "rho_MLE", which is the maximum
 * likelihood estimate state, if \c "--value-type=HS-dist". A custom reference state can
 * be given which is to be read from the MATLAB data file (if \c
 * "--value-type=HS-dist:my_ref_state_dm", where \c "my_ref_state_dm" is the name of the
 * variable inside the MATLAB data file containing the density matrix of the reference
 * state). Here's the code for the non-multiplexed version of the value-calculator:
 * \code
 *  if (opt->valtype.valtype == val_type_spec::HS_DIST) {
 *    
 *    MatrixType rho_ref(dmt.initMatrixType());
 *
 *    // determine the variable name of the reference state. By default, "rho_MLE".
 *    std::string refname = "rho_MLE";
 *    if (opt->valtype.ref_obj_name.size()) {
 *      // explicit reference state given in the command-line option
 *      refname = opt->valtype.ref_obj_name;
 *    }
 *
 *    // read the reference state from the MATLAB data file into the matrix 'rho_ref'
 *    rho_ref = Tomographer::MAT::value<MatrixType>(matf->var(refname));
 *
 *    // make sure that all eigenvalues of rho_ref are positive.
 *    rho_ref = Tomographer::Tools::forcePosSemiDef(rho_ref, 1e-12);
 *
 *    // emit debug message; this is displayed in verbose mode
 *    logger.debug("tomorun_dispatch()", [&](std::ostream & str) {
 *        str << "Using HS distance figure of merit with rho_ref = \n"
 *            << rho_ref << "\n";
 *      });
 *
 *    // finally, dispatch the execution to the main function tomorun().
 *    tomorun<BinningAnalysisErrorBars>(
 *      // first argument: the tomography problem data and types
 *      tomodat,
 *      // second argument: the command-line options
 *      opt,
 *      // thrid argument: an instance of the figure of merit calculator
 *      HsDistToRefCalculator<DMTypes>(rho_ref),
 *      // fourth argument: the logger object to emit log messages
 *      logger);
 *  }
 * \endcode
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
 * The code in \c "cxx/test/minimal_tomorun.cxx" should be commented and understandable.
 * The basic logic is to use the \ref Tomographer::MHRWTasks::ValueHistogramTools classes,
 * in combination with \ref Tomographer::DenseDM::TSpace::LLHMHWalker, to run random walks
 * over quantum states in T-space (see \ref pageParamsT) and collect statistics about a
 * figure of merit.  If you have any questions, don't hesitate to ask me.
 * 
 */
