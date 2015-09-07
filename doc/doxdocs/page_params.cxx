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
 * jacobian and "differential" jacobian, are defined in \ref tomographer/tools/sphcoords.h.
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



/** \page pageParamsX \a X parameterization
 *
 * See e.g. \ref Tomographer::param_herm_to_x()
 *
 * \todo DOC............
 */

/** \page pageParamsT \a T parameterization
 *
 * Parameterization such that \f$ \rho = T T^\dagger \f$.
 *
 * Often, \a T is chosen to be positive semidefinite. But this is not always the case, and
 * you should double-check.
 * 
 * \todo DOC.............................
 */


/** \page pageParamsA \a A parameterization
 *
 * \todo DOC...............................
 */
