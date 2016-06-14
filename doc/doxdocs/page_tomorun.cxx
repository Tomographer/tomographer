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


/** \page pageTomorunNewFigureOfMerit Adding a new figure of merit to the \c tomorun program
 *
 *
 * The \c tomorun executable has several figures of merit built into the program: the
 * trace distance, the purified distance, or the fidelity to any reference state, as well
 * as the expectation value of an observable.  If you wish to produce a histogram of a
 * different figure of merit which can't be cast into one of these, you need to change the
 * source code of the tomorun program.
 *
 * This information page describes how you should change the source code of \c tomorun to
 * include your figure of merit.
 *
 * \note If you perform modifications which may be useful to others, please <b>fork the
 * repository on github</b>, perform your changes, and send me a pull request.  This way
 * your changes will be availble to other users who would like to use the Tomographer
 * project.  Please see <a
 * href="https://github.com/Tomographer/tomographer/blob/master/README.md#contributing">here</a>
 * for information on how to contribute.
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
 * For our example, we can get inspired by the code for, e.g.,
 * \ref Tomographer::TrDistToRefCalculator defined in \ref dmmhrw.h . For example, let's create a
 * file called \c "hs_dist.h" inside the \c "cxx/tomorun/" directory of the
 * tomographer project:
 * \code
 *   #ifndef HS_DIST_H
 *   #define HS_DIST_H
 *
 *   #include <Eigen/Eigen>
 *   #include <tomographer/mhrw.h>
 *
 *   template<typename TomoProblem_, typename DistValueType_ = double>
 *   class HSDistToRefCalculator {
 *   public:
 *     typedef TomoProblem_ TomoProblem;
 *     typedef typename TomoProblem::MatrQ MatrQ;
 *     typedef typename MatrQ::MatrixType MatrixType;
 *   
 *     // For ValueCalculator interface : value type
 *     typedef DistValueType_ ValueType;
 *
 *   private:
 *     MatrixType rho_ref;
 *
 *   public:
 *     // Constructor, the reference state is 'rho_ref'
 *     TrDistToRefCalculator(const TomoProblem & tomo, const MatrixType& rho_ref_)
 *       : rho_ref(tomo.matq.initMatrixType())
 *     {
 *       rho_ref = rho_ref_;
 *     }
 *
 *     // This method actually calculates the given value.  The argument "T" is
 *     // the T-parameterization of the density matrix rho, which is simply a
 *     // square root of rho. (This is indeed the representation used during the
 *     // random walk.)
 *     inline ValueType getValue(const MatrixType & T) const
 *     {
 *       // rho is obtained with T*T.adjoint()
 *       // With Eigen, the HS-norm (Frobenius) of a matrix is given by A.norm()
 *       return (T*T.adjoint() - rho_ref).norm();
 *     }
 *   };
 *
 *   #endif // HS_DIST_H
 * \endcode
 *
 *
 * <h2>2. Declare the new figure of merit as command line option value</h2>
 *
 * The choice of figure of merit is specified as the argument to the command line option
 * \c "--value-type". So you should in fact first decide of a short memo string describing
 * your figure of merit (e.g. for our example \c "HS-dist" would be an appropriate choice).
 *
 * Now, open the file \c "tomorun_opts.h" located inside the \c "cxx/tomorun/" directory.
 * The locations where you should adapt the code are marked by comments saying \c "INSERT
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
 *         // and a callable which creates and returns a ValueCalculator instance:
 *         MyCustomFigureOfMeritValueCalculatorInstance<...>(...),
 *         logger); // and finally the logger instance
 *     return;
 *   }
 * \endcode
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
 *     example \ref Tomographer::Tools::force_pos_semidef() to ensure a matrix is positive
 *     semidefinite.
 *
 * In our example for the Hilbert-Schmidt distance, the reference state is read from the
 * MATLAB data file. It is to be taken by default to be \c "rho_MLE", which is the maximum
 * likelihood estimate state, if \c "--value-type=HS-dist". A custom reference state can
 * be given which is to be read from the MATLAB data file (if \c
 * "--value-type=HS-dist:my_ref_state_dm", where \c "my_ref_state_dm" is the name of the
 * variable inside the MATLAB data file containing the density matrix of the reference
 * state). Here's the code:
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
 *    rho_ref = Tomographer::Tools::force_pos_semidef(rho_ref, 1e-12);
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
 *      [&rho_ref](const OurTomoProblem & tomo) {
 *        return Tomographer::HsDistToRefCalculator<OurTomoProblem>(tomo, rho_ref);
 *      },
 *      // fourth argument: the logger object to emit log messages
 *      logger);
 *  }
 * \endcode
 * 
 */
