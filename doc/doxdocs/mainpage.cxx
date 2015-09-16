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

/** \mainpage %Tomographer C++ Framework: API Documentation
 *
 * The <a href="https://github.com/Tomographer/tomographer" target="_blank">%Tomographer
 * C++ Framework</a> groups a set of classes and functions which allow to reliably analyze
 * data from quantum experiments. These serve in particular as components for the tomorun
 * executable program.
 *
 * <h2>%Tomographer's Components</h2>
 *
 * The classes and routines of the project belong to several categories:
 *
 * <h3>Tools</h3>
 *
 * These are basic tools and utilities:
 *
 * - most tools, e.g. C++ language utilities and matrix utilities, are located in the
 *   namespace \ref Tomographer::Tools;
 *
 * - \ref Tomographer::MAT provides a set of routines to read data from MATLAB files
 *   (based on the <a href="http://matio.sourceforge.net/">MatIO</a> library);
 *
 * - %Tomographer provides a lightweight mechanism for logging messages. See \ref
 *   pageLoggers;
 *
 * - Some more utilities are provided in \ref Tomographer::SolveCLyap and \ref
 *   Tomographer::SphCoords.
 *
 * <h3>Specifying the Quantum Tomography Problem</h3>
 *
 * A quantum tomography setting with measurement results is specified as following in our
 * generic framework:
 *
 * - A type complying with the \ref pageInterfaceMatrQ specifies which C++ types to use to
 *   store a density matrix and parameterizations of it. For the moment this should be a
 *   \ref Tomographer::MatrQ instance.
 *
 * - A type implementing the \ref pageInterfaceTomoProblem stores the experimental data
 *   and is responsible for calculating the loglikelihood function. For the moment, you
 *   should use \ref Tomographer::IndepMeasTomoProblem.
 *
 * \note These type interfaces above might change in the future.
 * 
 * <h3>Running the Metropolis-Hastings Random Walk</h3>
 *
 * - The \ref Tomographer::MHRandomWalk takes care of running a Metropolis-Hastings random
 *   walk. You need to give it a specification of the random walk parameters, including
 *   what the state space is, the jump function, starting point, step size, etc. in the
 *   form of a type implementing the \ref pageInterfaceMHWalker.
 *
 * - The \ref Tomographer::DMStateSpaceLLHMHWalker is a type implementing the \ref
 *   pageInterfaceMHWalker, which is capable of running a random walk on the space of
 *   quantum states represented by dense C++ Eigen matrices, and using the distribution
 *   proportional to the loglikelihood function on the Hilbert-Schmidt measure.
 *
 * - While running the random walk, you'll want to collect some form of statistics. This
 *   is done with objects which comply with the \ref pageInterfaceMHRWStatsCollector. For
 *   example, see \ref Tomographer::ValueHistogramMHRWStatsCollector and \ref
 *   Tomographer::ValueHistogramWithBinningMHRWStatsCollector.
 *
 * <h3>Multiprocessing: Running Tasks in Parallel</h3>
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
 * - The \ref Tomographer::MHRWTasks namespace groups a set of classes which may be used
 *   to specify tasks to run which consist of abstract Metropolis-Hastings random
 *   walks.
 *
 *
 * <h2>Documentation Pages</h2>
 *
 * <h3>Important Namespaces</h3>
 *
 *   - \ref Tomographer — base %Tomographer namespace
 *   - \ref Tomographer::Tools — namespace for various tools
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
 */
