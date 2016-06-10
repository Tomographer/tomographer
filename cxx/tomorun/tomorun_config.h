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




/* DO_SLOW_POVM_CONSISTENCY_CHECKS
 *
 * If defined, will make sure that all POVM effects read from the input file are positive
 * semidefinite and nonzero.
 */
#define DO_SLOW_POVM_CONSISTENCY_CHECKS


/* TimerClock
 *
 * The C++ timer class to use.  high_resolution_clock is better (has high resolution), but
 * for some reason (I think for g++ 4.6 compatibility) we shall stick to system_clock. The
 * only effect of this is the precision for timing the run time of the program, and there
 * is no effect on the actual output of the quantum error bars.
 */
//typedef std::chrono::high_resolution_clock TimerClock;
typedef std::chrono::system_clock TimerClock;


/* TomorunInt
 *
 * The main integer type. Used to count a number of iterations, the sweep size, etc.
 */
typedef int TomorunInt;


/* TomorunReal
 *
 * The main floating-point type. Used for everything, from the matrix elements of the
 * quantum state to the step size of the random walk.  You may try to go to 'long double'
 * if you have trouble with precision --- but beware, I haven't tested this and there
 * might still be some hard-coded values at some places (1e-8 epsilons etc.). Beware and
 * please report issues to me!
 */
typedef double TomorunReal;


/* TOMORUN_CUSTOM_FIXED_DIM, TOMORUN_CUSTOM_MAX_POVM_EFFECTS
 *
 * You may define these to fixed values to specialize the tomorun problem to a specific
 * fixed dimension and a specific maximum number of POVM effects.  Leave either to
 * "Eigen::Dynamic" to allow any value at run-time.
 *
 * If these are not defined (the default), then some common cases are provided with a
 * fallback to all-dynamic specified at runtime. (See bottom of tomorun.cxx)
 */
//#define TOMORUN_CUSTOM_FIXED_DIM         Eigen::Dynamic
//#define TOMORUN_CUSTOM_MAX_POVM_EFFECTS  Eigen::Dynamic






#endif
