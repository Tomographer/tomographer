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



#ifndef TOMORUN_CONFIG
#define TOMORUN_CONFIG


/*
 * To set these options, compile with the corresponding "-D" flags. With cmake, use the
 * syntax:
 *
 *  tomographer-X.Y/build> cmake .. -DCMAKE_CXX_FLAGS="-DTOMORUN_REAL='long double' \
 *                                                     -DTOMORUN_RNG_CLASS=std::mt19937_64  [...] "
 */




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
 */
#ifdef TOMORUN_TIMERCLOCK
typedef TOMORUN_TIMERCLOCK TimerClock;
#else
typedef std::chrono::high_resolution_clock TimerClock;
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
 * fixed dimension and a specific maximum number of POVM effects.  You may set to
 * "Eigen::Dynamic" to always use dynamic size matrices which may allow any size at
 * run-time.  Leave these symbols undefined to have a selection of common values of
 * fixed-size matrices with a fallback to dynamic-size.
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
 * faster, benchmarks needed.
 */
#ifndef TOMORUN_USE_MULTIPLEXORVALUECALCULATOR
#define TOMORUN_USE_MULTIPLEXORVALUECALCULATOR true
#endif





/* TOMORUN_RNG_CLASS
 *
 * Value: C++ pseudo random number generator name (e.g. std::mt19937)
 *
 */
#ifdef TOMORUN_RNG_CLASS
typedef TOMORUN_RNG_CLASS TomorunBaseRngType;
#else
typedef std::mt19937 TomorunBaseRngType;
#endif


/* TOMORUN_RANDOM_DEVICE
 *
 * The name of the physical random device (see C++ std::random_device constructor) to use
 * to seed the pseudo-RNG.
 *
 * You may leave an empty macro definition to use the default constructor and use the
 * default device.
 *
 */
#ifndef TOMORUN_RANDOM_DEVICE
#define TOMORUN_RANDOM_DEVICE "/dev/random"
#endif


/* TOMORUN_USE_DEVICE_SEED
 *
 * Set to a nonzero value to seed the pseudo-random number generators with a seed taken
 * from a physical random device (TOMORUN_RANDOM_DEVICE).
 *
 * Set to zero to not attempt to access any random device (the code won't even refer to
 * std::random_device).  The pseudo-rng's will be seeded using consecutive seeds starting
 * from a base seed derived from the current time.
 *
 */
#ifndef TOMORUN_USE_DEVICE_SEED
#define TOMORUN_USE_DEVICE_SEED 0
#endif




#endif
