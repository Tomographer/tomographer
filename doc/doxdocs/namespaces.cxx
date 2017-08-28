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





/** \namespace Tomographer
 * \brief Base namespace for the \c Tomographer project
 *
 * This namespace contains the public API for the Tomographer project template library.
 */



/** \namespace Tomographer::tomo_internal
 * \internal 
 *
 * \brief Internal namespace for internal definitions.
 *
 * Do not access&mdash;the API might change at anytime!
 *
 */




/** \namespace Tomographer::DenseDM
 *
 * \brief Main implementation for densely represented density matrices
 *
 */

/** \namespace Tomographer::DenseDM::TSpace
 *
 * \brief Implementation of the random walk and figures of merit in T space, for densely
 *        represented density matrices
 *
 */




/** \namespace Tomographer::MHRWTasks
 *
 * \brief Definitions for running multiple Metropolis-Hastings random walks, and
 *        collecting statistics over the visited samples
 *
 * This namespace provides class definitions for running multiple such random walks within
 * a task manager/dispatcher (see \ref pageTaskManagerDispatcher, for example parallel
 * threads \ref MultiProc::OMP::TaskDispatcher).
 *
 * The random walk may be over any space and with any jump function: The random walk is
 * specified by any \ref pageInterfaceMHWalker compliant object.
 *
 * The statistics are collected by a \ref pageInterfaceMHRWStatsCollector.
 *
 * If you are interested in collecting a histogram of a particular function over the
 * explored space, you should consider using the classes in \ref
 * MHRWTasks::ValueHistogramTools, which provides additional helpful tools for averaging
 * histograms, calculating error bars etc.
 *
 */

/** \namespace Tomographer::MHRWTasks::ValueHistogramTools
 *
 * \brief %Tools for running random walk tasks while collecting a histogram of values
 *
 * These tools build on top of \ref Tomographer::MHRWTasks to add specific support for
 * collecting a histogram of values during the random walk, including a binning analysis
 * for error bars, and then aggregating the results of each task together to form an
 * averaged histogram.
 *
 * The \a CData class \ref Tomographer::MHRWTasks::ValueHistogramTools::CDataBase should
 * be used as \ref pageInterfaceMHRandomWalkTaskCData "CData class" to define your random
 * walk task, as it provides additional typedefs as well as automatic construction of the
 * value stats collector.
 *
 * \since Namespace added in %Tomographer 5.0 to replace
 *        Tomographer::MHRWTasks::ValueHistogramTasks.
 */



/** \namespace Tomographer::MultiProc
 *
 * \brief Definitions for multi-processing classes and helpers
 *
 * This namespace encompasses classes and functions allowing to dispatch and process
 * several tasks simultaneously.
 *
 * Currently, only an OpenMP implementation is provided, see \ref MultiProc::OMP and \ref
 * MultiProc::OMP::TaskDispatcher.
 */

/** \namespace Tomographer::MultiProc::OMP
 *
 * \brief Definitions for multitasking using OpenMP multithreading
 *
 */

/** \namespace Tomographer::MultiProc::CxxThreads
 *
 * \brief Definitions for multitasking using C++ Threads
 *
 */

/** \namespace Tomographer::MultiProc::Sequential
 *
 * \brief A Sequential Task dispatcher running tasks one after the other
 *
 */


/** \namespace Tomographer::MultiProc::ThreadCommon
 *
 * \brief Common definitions for thread-based MultiProc implementations
 *
 * \since This namespace was introduced in %Tomographer 5.2
 *
 */


/** \namespace Tomographer::MultiProc::MPI
 *
 * \brief Definitions for multitasking by exploiting an MPI environment (uses Boost.MPI)
 *
 * \since Introduced in %Tomographer 5.3
 */

/** \namespace Tomographer::Tools
 *
 * \brief Various useful tools.
 *
 */


/** \namespace Tomographer::MathTools
 *
 * \brief General math routines
 *
 */


/** \namespace Tomographer::MathTools::SphCoords
 *
 * \brief Utilities for hyperspherical coordinates.
 *
 * See also the corresponding theory page \ref pageParamsSphericalCoords.
 */


/** \namespace Tomographer::MathTools::SolveCLyap
 *
 * \brief Definitions for solving the Complex Lyapunov Equation
 *
 * See \ref SolveCLyap::solve().
 */




/** \namespace Tomographer::MAT
 *
 * \brief Utilities and helpers for reading MATLAB <code>.mat</code> files
 *
 * The class \ref File represents an open MATLAB file from which you can extract
 * data. Variables inside the file are represented with \ref Var objects.
 *
 * The actual data can be extracted to some native C++ representation using \ref
 * Var::value<T>() for some selected C++ types. Example:
 *
 * \code
 *   Tomographer::MAT::File matfile("myfile.mat");
 *
 *   // find the variable in file named `x'
 *   Tomographer::MAT::Var var_x = matfile.var("x");
 *
 *   // get `x' as C++ double. If `x' in the file is not convertible to double, or if
 *   // it is not a scalar, then a VarTypeError is thrown.
 *   double var_x_value = var.value<double>();
 *
 *   // find variable `m' and get it as an Eigen::MatrixXd. Again, if the type is
 *   // incompatible a VarTypeError is thrown.
 *   Tomographer::MAT::Var var = matfile.var("m");
 *   Eigen::MatrixXd matrix = var.value<Eigen::MatrixXd>();
 * \endcode
 *
 * You can extend this mechanism easily to essentially any C++ type. Just specialize the
 * \ref VarValueDecoder template for your C++ type.
 *
 * \note Currently, only numeric types are supported. Neither structures nor cell arrays
 *       nor function handles can be read.
 *
 * \note Also, currently, you can't write data files, you can only read them.
 */





/** \namespace tpy
 *
 * \brief C++ Classes and Utilities for Python Modules
 *
 * These are mostly C++ typedefs for classes which are exposed to Python, as well as some
 * helpers.
 * 
 */
