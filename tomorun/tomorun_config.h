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



#ifndef TOMORUN_CONFIG
#define TOMORUN_CONFIG




/* TOMORUN_DO_SLOW_POVM_CONSISTENCY_CHECKS
 *
 * Value: true | false
 *
 * If defined, will make sure that all POVM effects read from the input file are positive
 * semidefinite and nonzero.
 */
#ifndef TOMORUN_DO_SLOW_POVM_CONSISTENCY_CHECKS
#define TOMORUN_DO_SLOW_POVM_CONSISTENCY_CHECKS true
#endif


/* TOMORUN_TIMERCLOCK
 *
 * Value: C++ clock class name (std::chrono::...)
 *
 * The C++ timer class to use.  high_resolution_clock is better (has high resolution), but
 * for some reason (I think for g++ 4.6 compatibility) we shall stick to system_clock. The
 * only effect of this is the precision for timing the run time of the program, and there
 * is no effect on the actual output of the quantum error bars.
 */
//typedef std::chrono::high_resolution_clock TimerClock;
#ifdef TOMORUN_TIMERCLOCK
typedef TOMORUN_TIMERCLOCK TimerClock;
#else
typedef std::chrono::system_clock TimerClock;
#endif


/* TOMORUN_INT
 *
 * Value: a integer type
 *
 * The main integer type. Used to count a number of iterations, the sweep size, etc.
 */
#ifdef TOMORUN_INT
typedef TOMORUN_INT TomorunInt;
#else
typedef int TomorunInt;
#endif


/* TOMORUN_REAL
 *
 * The main floating-point type. Used for everything, from the matrix elements of the
 * quantum state to the step size of the random walk.  You may try to go to 'long double'
 * if you have trouble with precision --- but beware, I haven't tested this and there
 * might still be some hard-coded values at some places (1e-8 epsilons etc.). Beware and
 * please report issues to me!
 */
#ifdef TOMORUN_REAL
typedef TOMORUN_REAL TomorunReal;
#else
typedef double TomorunReal;
#endif


/* TOMORUN_CUSTOM_FIXED_DIM, TOMORUN_CUSTOM_FIXED_MAX_DIM, TOMORUN_CUSTOM_MAX_POVM_EFFECTS
 *
 * You may define these to fixed values to specialize the tomorun problem to a specific
 * fixed dimension and a specific maximum number of POVM effects.  Leave either to
 * "Eigen::Dynamic" to allow any value at run-time.
 *
 * TOMORUN_CUSTOM_FIXED_DIM fixes the dimension of the system to a compile-time fixed
 * value which cannot be changed at run-time.  Leave the default common cases or use
 * Eigen::Dynamic if you want tomorun to work with different system sizes.
 * TOMORUN_CUSTOM_FIXED_MAX_DIM specifies a maximum dimension for the dimension of the
 * quantum system; the latter may at run-time take any value up to this limit.
 *
 * If these are not defined (the default), then some common cases are provided with a
 * fallback to all-dynamic specified at runtime. (See bottom of tomorun.cxx)
 */
//#define TOMORUN_CUSTOM_FIXED_DIM         Eigen::Dynamic
//#define TOMORUN_CUSTOM_FIXED_MAX_DIM     Eigen::Dynamic
//#define TOMORUN_CUSTOM_MAX_POVM_EFFECTS  Eigen::Dynamic


/* TOMORUN_USE_MULTIPLEXORVALUECALCULATOR
 *
 * Value: 1 | 0
 *
 * Use the class MultiplexorValueCalculator and dynamically choose the figure of merit to
 * calculate, rather than using a fully templated tomorun<> function specialized to a
 * single figure of merit and replicated for each figure of merit. I'm not sure which is
 * faster, tests needed.
 */
#ifndef TOMORUN_USE_MULTIPLEXORVALUECALCULATOR
#define TOMORUN_USE_MULTIPLEXORVALUECALCULATOR true
#endif




#endif