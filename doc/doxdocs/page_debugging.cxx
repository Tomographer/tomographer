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

/** \page pageDebugging Useful Resources for Debugging
 *
 * <b>CORE Files & GDB/DDD</b>
 *
 *   - Don't forget to compile code with <code>g++ -g</code> to include debugging information
 *
 *   - <code>ulimit -c unlimited</code> &mdash; generate \c core files
 *
 *   - <code>./path/to/program</code> &mdash; if it crashes, generates \c core file
 *
 *   - <code>gdb ./path/to/program -c core</code> &mdash; load core file into GDB
 *
 *   - inside GDB: <code>bt</code> &mdash; display backtrace
 *
 *
 * <b>Using <code>libSegFault.so</code></b>
 *
 * Useful trick:
 *
 * <code>env SEGFAULT_SIGNALS="abrt segv"
 *       LD_PRELOAD=/lib/x86_64-linux-gnu/libSegFault.so ./path/to/program</code>
 * (adapt to location of <code>libSegFault.so</code>): this will print the stack trace
 * if the given signals are caught. You don't get line numbers though.
 *
 */
