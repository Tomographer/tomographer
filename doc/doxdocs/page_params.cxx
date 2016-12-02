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
// Parameterizations -- Title Page
// =============================================================================


/** \page pageParams Some Useful Parameterizations
 *
 * Just some useful parameterizations for points, matrices and so on.
 *
 *  - \subpage pageParamsSphericalCoords
 *  - \subpage pageParamsX
 *  - \subpage pageParamsT
 *  - \subpage pageParamsA
 */



// =============================================================================
// Hyperspherical Coordinates
// =============================================================================

/** \page pageParamsSphericalCoords Hyperspherical Coordinates
 *
 * In \a N-dimensional Euclidian real space, we represent points with their cartesian
 * coordinates \a x_i.
 *
 * The <a href="http://en.wikipedia.org/wiki/N-sphere#Spherical_coordinates"
 * target="_blank">Hyperspherical Coordinates</a> is another representation for these
 * points, with the new coordinates \f$ (r,\theta_1,\theta_2,\ldots,\theta_{N-1}) \f$ :
 * \f{align*}{
 *   r &\in [0,\infty[ \ ; \\
 *   \theta_i &\in [0,\pi] \qquad\text{ for } i=1,\ldots,N-2 \ ; \\
 *   \theta_{N-1} &\in [-\pi,\pi[ \ .
 * \f}
 *
 * Routines that allow to compute the coordinate transforamtions, as well as corresponding
 * jacobian and "differential" jacobian, are defined in \ref tomographer/mathtools/sphcoords.h.
 *
 * The transformation to cartesian coordinates is given by:
 * \f{align*}{
 *    x_1 &= r\,\cos\left(\theta_1\right) \ ;\\
 *    x_2 &= r\,\sin\left(\theta_1\right)\cos\left(\theta_2\right) \ ;\\
 *    & \ldots \\
 *    x_{N-1} &= r\,\sin\left(\theta_1\right)\ldots\sin\left(\theta_{N-2}\right)\cos\left(\theta_{N-1}\right) \ ;\\
 *    x_{N} &=   r\,\sin\left(\theta_1\right)\ldots\sin\left(\theta_{N-2}\right)\sin\left(\theta_{N-1}\right) \ .
 * \f}
 *
 * \note In the special case of the 2-sphere, this does not map back to usuall 3-D <a
 * href="http://en.wikipedia.org/wiki/Spherical_coordinate_system"
 * target="_blank">spherical coordinates</a> (i.e., with \f$\theta=0\f$ at
 * <em>(X=0,Y=0,Z=1)</em>, \f$\theta=\pi\f$ at <em>(X=0,Y=0,Z=-1)</em>, and
 * \f$(\theta=\pi/2, \phi=0)\f$ for <em>(X=1,Y=0,Z=0)</em>). In fact, the mapping for
 * 3-dimensional Euclidean space is:
 * \f{align*}{
 *    x_1 &= r \cos\left(\theta_1\right)
 *        & &\text{ -- what we normally call Z;}  \\
 *    x_2 &= r \sin\left(\theta_1\right) \cos\left(\theta_2\right)
 *        & &\text{ -- what we normally call X;}  \\
 *    x_3 &= r \sin\left(\theta_1\right) \sin\left(\theta_2\right)
 *        & &\text{ -- what we normally call Y.}
 * \f}
 * So effectively, the angles count from <em>(X=+1,Y=0,Z=0)</em> and \f$\theta_1\f$
 * increases to <em>(X=-1,Y=0,Z=0)</em>; then \f$\theta_2\f$ wraps around, with
 * \f$\theta_2=0\f$ corresponding to the direction in which <em>Y=+1</em>.
 *
 */



/** \page pageParamsX \a X Parameterization
 *
 * Parameterization of a \f$ d\times d\f$ (complex) hermitian matrix \f$ A \f$ into a real
 * vector \f$ (x_i)\f$ of \f$ d^2 \f$ elements. The parameterization is linear, and
 * preserves inner products: \f$ \mathrm{tr}(A\,A') = \sum_i x_i x_i' \f$.
 *
 * The parameterization is defined as follows: the first \f$ d\f$ entries of \f$ (x_i)\f$
 * are the diagonal entries of \f$ A\f$. The following \f$ d(d-1)/2\f$ entries are the
 * real parts of the off-diagonal entries, and the yet followoing \f$ d(d-1)/2\f$ entries
 * are the imaginary parts of the off-diagonal entries. All off-diagonal entries are
 * normalized by a factor \f$1/\sqrt{2}\f$ to preserve inner products. The off-diagonals
 * are listed in the lower triangular part and row-wise. More precisely, we have (define
 * as shorthand \f$ d'= d(d-1)/2\f$):
 *
 * \f{align*}{
 * A = \begin{pmatrix}
 *    x_{1}  &   \ast   &   \ast      &  \ldots   &  \ast \\
 *    (x_{d+1} + i x_{d+d'+1})/\sqrt2  &
 *             x_{2}    &   \ast      &  \ldots    &  \ast \\
 *    (x_{d+2} + i x_{d+d'+2})/\sqrt2  &  (x_{d+3} + i x_{d+d'+3})/\sqrt2 &
 *                          x_{3}     &            &  \ast \\
 *    \vdots &          &             &  \ddots    &  \ast \\
 *    (x_{d'} + i x_{2d'})/\sqrt2 &  \ldots    &             &
 *    (x_{d+d'} + i x_{2d'+d})/\sqrt2   &   x_{d}
 * \end{pmatrix}.
 * \f}
 * The upper triangular off-diagonals are set of course such that \f$ A\f$ is hermitian.
 *
 * See \ref Tomographer::DenseDM::ParamX.
 */

/** \page pageParamsT \a T Parameterization
 *
 * Parameterization of a density operator \f$ \rho\f$ by a complex matrix \f$ T\f$ such
 * that \f$ \rho = T T^\dagger \f$ and with \f$ T\f$ satisfying \f$
 * \mathrm{tr}(TT^\dagger)=1 \f$.
 *
 * The matrix \f$ T\f$ is obviously not unique but has a
 * unitary freedom: \f$ T' = TU\f$ is also possible parameterization for any unitary \f$
 * U\f$. You can choose a gauge to fix this unitary freedom. Two are common:
 *
 * - Force \f$ T\f$ to be positive semidefinite. Then \f$ T = \rho^{1/2} \f$.
 *
 * - Force \f$ T\f$ to be a lower triangular matrix. You can obtain \f$ T\f$ by
 *   performing a Cholesky (or LLT or LDLT) decomposition.
 *
 * Throughout the project, if we refer to a &lsquo;\a T parameterization,&rsquo; we do not
 * imply any particular gauge.  For example, the class \ref
 * Tomographer::DenseDM::TSpace::LLHMHWalker does not fix the gauge and performs the
 * random walk on all valid T matrices.
 *
 * See, for example, \ref Tomographer::DenseDM::fidelityT().
 */


/** \page pageParamsA \a A Parameterization
 *
 * Parameterize a traceless hermitian matrix \f$ A\f$ in an orthonormal basis of \f$
 * su(d)\f$.  The (complex) traceless hermitian matrix \f$ A\f$ is written as
 * \f[
 *   A = \sum_{j=1}^{d^2-1} a_j A_j\ ,
 * \f]
 * where the \f$ A_j\f$ are the normalized version of the generalized Gell-Mann matrices,
 * i.e. \f$ A_j = \lambda_j/\sqrt2\f$ where \f$\lambda_j\f$ are defined as in Refs. [1-3].
 *
 * Whenever we talk about the \a A parameterization of a matrix which is not traceless, we
 * imply the \a A parameterization of its traceless part, i.e. \f$ A -
 * \mathrm{tr}(A)\mathbb{I}/d\f$.
 *
 * 1. <a href="http://mathworld.wolfram.com/GeneralizedGell-MannMatrix.html">Wolfram
 *       MathWorld: Generalized Gell-Mann Matrix</a>;
 *
 * 2. Br&uuml;ning et al., &ldquo;Parametrizations of density matrices,&rdquo; Journal of
 *    Modern Optics 59:1 1 (2012), <a
 *    href="http://dx.doi.org/10.1080/09500340.2011.632097">doi:10.1080/09500340.2011.632097</a>,
 *    <a href="http://arxiv.org/abs/1103.4542">arXiv:1103.4542</a>;
 *
 * 3. Bertlmann &amp; Krammer, &ldquo;Bloch vectors for qudits,&rdquo; Journal of Physics
 *    A 41:23 235303 (2008) <a
 *    href="http://dx.doi.org/10.1088/1751-8113/41/23/235303">doi:10.1088/1751-8113/41/23/235303</a>,
 *    <a href="http://arxiv.org/abs/0806.1174">arXiv:0806.1174</a>.
 *
 */
