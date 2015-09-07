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



// =============================================================================
// Theory for specific components -- Title Page
// =============================================================================

/** \page pageTheory Theory for specific %Tomographer components
 *
 * These pages document the theory specific to some components of the %Tomographer
 * framework.
 *
 * Specific Topics:
 *  - \subpage pageTheoryAveragedHistogram
 *  - \subpage pageTheoryBinningAnalysis
 *
 */


// =============================================================================
// Theory for specific components -- Averaged Histogram
// =============================================================================

/** \page pageTheoryAveragedHistogram Averaged Histogram
 *
 * <h3>Averaging Raw Histograms</h3>
 *
 * Given raw histogram counts from independent experiments, we can combine them into one
 * histogram with error bars as follows. Let \f$ x_k^{(i)} \f$ be the raw histogram counts
 * in bin \f$ k\f$ of experiment \f$ i\f$ of \f$ n\f$. Then the full histogram counts
 * \f$ y_k \f$, with corresponding error bars \f$ \Delta_k \f$, is
 * \f{align*}{
 *   y_k &= \frac1n \sum_i x_k^{(i)} \ ; \\
 *   \Delta_k &= \sqrt{\frac1{n-1}\left(\langle (x_k^{(i)})^2 \rangle - \langle x_k^{(i)} \rangle^2\right)}\ ,
 * \f}
 * where \f$ \langle\cdot\rangle = \frac1n \sum_i \cdot \f$ is the average over the
 * different experiments.
 *
 *
 * <h3>Averaging Histograms Which Already Have Error Bars</h3>
 *
 * Let \f$ x_k^{(i)} \f$ raw histogram counts, and suppose that we already have error bars
 * \f$ \delta_k^{(i)} \f$ on these counts (e.g., from binning analysis).
 *
 * The combined histogram \f$ y_k \f$, with final corresponding error bars \f$ \Delta_k
 * \f$, is
 * \f{align*}{
 *   y_k = \frac1n \sum_i x_k^{(i)} \ ; \\
 *   \Delta_k = \frac1n\,\sqrt{\sum_i \left(\delta_k^{(i)}\right)^2} \ .
 * \f}
 * (propagation of errors in error analysis in physics: \f$ \Delta f = \sqrt{
 * \left(\frac{\partial f}{\partial x}\right)^2 \Delta x^2 + \cdots } \f$ ).
 *
 */


// =============================================================================
// Theory for specific components -- Binning Analysis
// =============================================================================

/** \page pageTheoryBinningAnalysis Binning Analysis
 *
 * Reference: [Ambegaokar, Troyer, Am. J. Phys. (2010), http://dx.doi.org/10.1119/1.3247985
 *             http://arxiv.org/abs/0906.0943]
 *
 * The Binning Analysis provides a powerful way of determining error bars for integrals
 * calculated using the Metropolis-Hastings algorithm.
 *
 * Suppose we have a set of \f$ N \f$ correlated samples \f$ \{ x_i \} \f$ which we have
 * obtained using a Metropolis-Hastings random walk over a probability measure \f$ P(x)dx
 * \f$. We may have already taken one sample in every \f$ N_\mathrm{sweep} \f$ in order to
 * decorrelate the samples a bit already.
 *
 * Suppose our goal is to approximate the integral
 * \f[
 *    \langle f\rangle = \int f\left(x\right)\, P\left(x\right)\,dx \ .
 * \f]
 * We'll do so by calculating the average of the function evaluated at all our samples,
 * \f[
 *    f_\mathrm{MH} = \frac1N\sum_i f\left(x_i\right)\ .
 * \f]
 * The question now is, what is the error bar on this approximation? The naive value,
 * valid for independent samples, is (writing \f$ f_i := f(x_i) \f$)
 * \f[
 *    \Delta_\mathrm{naive} = \sqrt{ \frac{ \frac1N\sum_i f_i^2 - \left(\frac1N\sum_i f_i\right)^2 }{N-1} }
 * \f]
 * (see [Ambegaokar/Troyer, Eq. (10)]).
 *
 * However this formula is wrong if the samples are correlated. For this, we introduce the
 * <em>binning analysis</em>.
 *
 * Take the sequence of samples \f$ f_i =: f_i^{(0)} \f$, and combine them pairwise to
 * take the average of their two values, to generate a new sequence \f$ f_i^{(1)} \f$ of
 * half the initial length; this can be repeated to generate new pairwise averaged samples
 * of each previous binning level:
 * \f[
 *     f_i^{(n)} := \frac12\left( f_{2i}^{n-1} + f_{2i+1}^{n-1} \right)\ .
 * \f]
 * (We assume \f$ i \f$ counts starting at \f$ 0 \f$.)
 *
 * Intuitively, the samples at each binning level are less and less correlated, and the
 * naive estimate as used in \f$ \Delta_\mathrm{naive} \f$ gets better and better. So
 * calculate the "naive" error bars at each level:
 * \f[
 *     \Delta^{(n)} := \sqrt{ \frac{ \frac1{N^{(n)}}\sum_i (f_i^{(n)})^2 - \left(\frac1{N^{(n)}}\sum_i f_i^{(n)}\right)^2 }{N^{(n)}-1} }
 * \f]
 * (with \f$ N^{(n)} = N / 2^{n} \f$).
 *
 * These errors should converge as the binning level increases (See [Ambegaokar/Troyer,
 * Fig. 5]). If this is the case, the naive errors at each level converge to the true
 * error. If not, there are not enough samples and the error bar hasn't converged.
 *
 * Make sure that at the last binning level you still have enough samples to get a
 * reliable estimate of \f$ \Delta^{(n)} \f$ from those samples.
 *
 * This analysis is done using the class \ref Tomographer::BinningAnalysis.
 */
