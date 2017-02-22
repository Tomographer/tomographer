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



/** \mainpage %Tomographer C++ Framework: API Documentation
 *
 * The <a href="https://github.com/Tomographer/tomographer" target="_blank">%Tomographer
 * C++ Framework</a> groups a set of classes and functions which allow to reliably analyze
 * data from quantum experiments. These serve in particular as components for the \c
 * tomorun executable program.
 *
 * <a href="py/index.html">The API documentation for the Python interface to Tomographer
 * is available here.</a>
 *
 * <h2>%Tomographer's Components</h2>
 *
 * The classes and routines of the project belong to several categories.
 *
 * <h3>Generic Tools</h3>
 *
 * These are basic tools and utilities:
 *
 * - Most tools, e.g. C++ language utilities and other tools, are defined in the
 *   namespace \ref Tomographer::Tools;
 *
 * - \ref Tomographer::MAT provides a set of routines to read data from MATLAB files
 *   (based on the <a href="http://matio.sourceforge.net/">MatIO</a> library);
 *
 * - %Tomographer provides a lightweight mechanism for logging messages. See \ref
 *   pageLoggers;
 *
 * - Some more utilities are provided in the namespace \ref Tomographer::MathTools
 *   (generate a Haar-random unitary, manipulate spherical coordinates, etc.).
 *
 * <h3>Engine for Running a Metropolis-Hastings Random Walk</h3>
 *
 * - The \ref Tomographer::MHRandomWalk takes care of running a Metropolis-Hastings random
 *   walk. You need to give it a specification of the random walk parameters, including
 *   what the state space is, the jump function, starting point, step size, etc. in the
 *   form of a type implementing the \ref pageInterfaceMHWalker.
 *
 * - While running the random walk, you'll want to collect some form of statistics. This
 *   is done with objects which comply with the \ref pageInterfaceMHRWStatsCollector. For
 *   example, see \ref Tomographer::ValueHistogramMHRWStatsCollector and \ref
 *   Tomographer::ValueHistogramWithBinningMHRWStatsCollector.
 *
 * - For example a specific implementation: The \ref
 *   Tomographer::DenseDM::TSpace::LLHMHWalker is a type implementing the \ref
 *   pageInterfaceMHWalker, which is capable of running a random walk on the space of
 *   quantum states represented by dense C++ Eigen matrices, and using the distribution
 *   proportional to the loglikelihood function on the Hilbert-Schmidt measure.
 *
 * <h3>Engine for Multiprocessing: Running Tasks in Parallel</h3>
 *
 * - An abstract multiprocessing framework is specified using a set of interfaces, see
 *   \ref pageTaskManagerDispatcher. This requires on one hand an implementation of a
 *   multiprocessing environment, and on the other hand a specification of which tasks are
 *   to be run.
 *
 * - Classes in the \ref Tomographer::MultiProc::OMP namespace are an implementation of a
 *   multiprocessing environment using <a href="https://en.wikipedia.org/wiki/OpenMP"
 *   target="_blank">OpenMP</a>. This dispatches the tasks over several threads (on a
 *   same machine).
 *
 * <h3>Both Together: Running Metropolis-Hastings Random Walks in Parallel</h3>
 *
 * - The \ref Tomographer::MHRWTasks namespace groups a set of classes which may be used
 *   to specify a series of Metropolis-Hastings random walks to be run in parallel, which
 *   can be executed by a \ref pageTaskManagerDispatcher such as \ref
 *   Tomographer::MultiProc::OMP.
 *
 * - On top of this the classes in \ref Tomographer::MHRWTasks::ValueHistogramTasks
 *   provide more specific definitions for collecting a histogram about a value
 *   (e.g. figure of merit) during a Metropolis-Hastings random walk.
 *
 * <h3>Specific Implementation for Quantum States Specified as Densely Stored Matrices</h3>
 *
 * These classes provide the types and specification of how to perform the random walk,
 * how to calculate the loglikelihood function as well as the figures of merit for quantum
 * states, with the quantum states and POVM effects stored explicitly as matrices (either
 * directly, or via a \ref pageParamsX or \ref pageParamsT).  Classes relating to this
 * implementation are located in the \ref Tomographer::DenseDM namespace.
 *
 * Currently, this is the only concrete implementation of our tomography method.  In the
 * future, one could imagine extensions to other implementations, such as directly
 * performing the random walk in the X-parameterization space.
 *
 * - The \ref Tomographer::DenseDM::DMTypes class defines some canonical types for this
 *   implementation, such as the type used to store a matrix, the type used to store a
 *   X-parameterziation vector, etc.
 *
 * - A class implementing the \ref pageInterfaceDenseLLH is capable of calculating the
 *   loglikelihood function for a particular experiment.  Currently, we only support
 *   observed POVM effects which can be written as a product of POVM effects (though this
 *   is not necessarily a product POVM!), in the class \ref
 *   Tomographer::DenseDM::IndepMeasLLH.
 *
 * - The class \ref Tomographer::DenseDM::TSpace::LLHMHWalker specifies the random walk in
 *   T-space on the basis of given types (in DMTypes) and a way to calculate the LLH
 *   function (via a \ref pageInterfaceDenseLLH compliant object).
 *
 * - Some predefined figures of merit for the random walk in T space are defined as \ref
 *   Tomographer::DenseDM::TSpace::FidelityToRefCalculator "FidelityToRefCalculator", \ref
 *   Tomographer::DenseDM::TSpace::PurifDistToRefCalculator "PurifDistToRefCalculator", \ref
 *   Tomographer::DenseDM::TSpace::TrDistToRefCalculator "TrDistToRefCalculator", and \ref
 *   Tomographer::DenseDM::TSpace::ObservableValueCalculator "ObservableValueCalculator".
 * 
 *
 * <h2>Documentation Pages</h2>
 *
 * <h3>Important Namespaces</h3>
 *
 *   - \ref Tomographer — base %Tomographer namespace
 *   - \ref Tomographer::Tools — various C++ language-related tools
 *   - \ref Tomographer::MathTools — various mathematical tools
 *   - Additional tools are available in selected namespaces. See the \htmlonly <a href="namespaces.html">List of Namespaces</a>.\endhtmlonly \latexonly List of Namespaces.\endlatexonly
 *
 * <h3>Specific Topics</h3>
 *
 *   - \subpage pageTypeInterfaces
 *   - \subpage pageTheory
 *   - \subpage pageParams
 *   - \subpage pageLoggers
 *
 * <h3>Other Specific Resources</h3>
 *
 *   - \subpage pageDebugging
 *
 * <h3>Tomorun-Executable Related Topics</h3>
 *
 * The \c tomorun program is simply a straightforward piecing together of the different
 * components detailed above.  Most of the work is already done by the classes in \ref
 * Tomographer::MHRWTasks::ValueHistogramTasks.
 *
 * The \c tomorun code is not included in this API documentation.  The code is located
 * under \c "cxx/tomorun/".  If you wish to change functionality in \c tomorun, or if you
 * wish to implement a very particular calculation, you might like to have a look at the
 * test example \c "cxx/test/minimal_tomorun.cxx", which provides a very minimal
 * implementation of tomorun for a specific example&mdash;it may be more convenient for
 * you to modify that program.
 *
 * Topics:
 *
 *   - \subpage pageTomorunNewFigureOfMerit
 *   - \subpage pageCustomTomorunExe
 *
 * <h3>Known Bugs & To Do List</h3>
 *
 *   - \subpage bugs "Bug List"
 *   - \subpage todo "To Do List"
 *
 */
