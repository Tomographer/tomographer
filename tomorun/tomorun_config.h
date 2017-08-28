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
 * These options are documented at
 * https://tomographer.github.io/tomographer/api-doc/current/html/page_tomorun_config_build.html
 *
 */



//
// TOMORUN_INT
//
#ifdef TOMORUN_INT
typedef TOMORUN_INT TomorunInt;
#else
typedef int TomorunInt;
#endif


//
// TOMORUN_REAL
//
#ifdef TOMORUN_REAL
typedef TOMORUN_REAL TomorunReal;
#else
typedef double TomorunReal;
#endif


//
// TOMORUN_CUSTOM_FIXED_DIM, TOMORUN_CUSTOM_FIXED_MAX_DIM, TOMORUN_CUSTOM_MAX_POVM_EFFECTS
//
//#define TOMORUN_CUSTOM_FIXED_DIM         Eigen::Dynamic
//#define TOMORUN_CUSTOM_FIXED_MAX_DIM     Eigen::Dynamic
//#define TOMORUN_CUSTOM_MAX_POVM_EFFECTS  Eigen::Dynamic


//
// TOMORUN_MAX_LOG_LEVEL
//
//#define TOMORUN_MAX_LOG_LEVEL   DEBUG




//
// TOMORUN_RNG_CLASS
//
#ifdef TOMORUN_RNG_CLASS
typedef TOMORUN_RNG_CLASS TomorunRng;//TomorunBaseRngType;
#else
typedef std::mt19937 TomorunRng;//TomorunBaseRngType;
#endif

//
// TOMORUN_USE_DEVICE_SEED
//
#ifndef TOMORUN_USE_DEVICE_SEED
#define TOMORUN_USE_DEVICE_SEED 0
#endif

//
// TOMORUN_RANDOM_DEVICE
//
#ifndef TOMORUN_RANDOM_DEVICE
#define TOMORUN_RANDOM_DEVICE "/dev/random"
#endif






//
// TOMORUN_DO_SLOW_POVM_CONSISTENCY_CHECKS
//
#ifndef TOMORUN_DO_SLOW_POVM_CONSISTENCY_CHECKS
#define TOMORUN_DO_SLOW_POVM_CONSISTENCY_CHECKS true
#endif

//
// TOMORUN_TIMERCLOCK
//
#ifdef TOMORUN_TIMERCLOCK
typedef TOMORUN_TIMERCLOCK TimerClock;
#else
typedef std::chrono::high_resolution_clock TimerClock;
#endif










// DEPRECATED OPTIONS, NO LONGER HAVE EFFECT
// (would bloat the code too much with the other options:)

// /* TOMORUN_USE_MULTIPLEXORVALUECALCULATOR
//  *
//  * Value: 1 | 0
//  *
//  * Use the class MultiplexorValueCalculator and dynamically choose the figure of merit to
//  * calculate, rather than using a fully templated tomorun<> function specialized to a
//  * single figure of merit and replicated for each figure of merit. I'm not sure which is
//  * faster, benchmarks needed.
//  */
// #ifndef TOMORUN_USE_MULTIPLEXORVALUECALCULATOR
// #define TOMORUN_USE_MULTIPLEXORVALUECALCULATOR true
// #endif
#ifdef TOMORUN_USE_MULTIPLEXORVALUECALCULATOR
#warning "TOMORUN_USE_MULTIPLEXORVALUECALCULATOR is now deprecated. The multiplexor is always used."
#endif



#endif
