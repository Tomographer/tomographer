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

#ifndef SPHCOORDS_H
#define SPHCOORDS_H

#include <cmath>

#include <Eigen/Eigen>

#include <tomographer/tools/cxxutil.h> // tomographer_assert()


/** \file sphcoords.h
 *
 * \brief spherical coordinates conversion routines, with jacobian etc. See \ref
 * pageParamsSphericalCoords.
 *
 */


namespace Tomographer {
namespace MathTools {
namespace SphCoords {


/** \brief Convert Cartesian coordinates to spherical coordinates in N dimensions
 *
 * See \ref pageParamsSphericalCoords for information about the conventions and ranges of
 * the coordinates.
 *
 * \param rtheta (output), vector of \a N entries, will store the \f$ r\f$ and
 *        \f$\theta_i\f$ values corresponding to the spherical coordinates representation
 *        of the point represented by \a cart. The value <em>rtheta(0)</em> is the \f$r\f$
 *        coordinate and <em>rtheta(1),..,rtheta(N-1)</em> are the angle coordinates,
 *        corresponding respectively to \f$\theta_1\ldots\theta_{N-1}\f$. All \f$ \theta_i
 *        \f$ 's but the last range from \f$0..\pi\f$, while \f$\theta_{N-1}\f$ ranges
 *        from \f$-\pi\f$ to \f$+\pi\f$.
 *
 * \param cart is a vector of \a N entries, corresponding to the cartesian coordinates of
 *        the point to transform.
 */
template<typename Der1, typename Der2>
inline void cart_to_sph(Eigen::MatrixBase<Der2>& rtheta, const Eigen::MatrixBase<Der1>& cart)
{
  { using namespace Eigen; EIGEN_STATIC_ASSERT_LVALUE(Der2); }
  typedef typename Eigen::MatrixBase<Der1>::Scalar Scalar;

  tomographer_assert(cart.cols() == 1 && rtheta.cols() == 1);
  tomographer_assert(cart.rows() == rtheta.rows());

  const size_t ds = cart.rows()-1; // dimension of the sphere

  // see http://people.sc.fsu.edu/~jburkardt/cpp_src/hypersphere_properties/hypersphere_properties.cpp
  // and http://en.wikipedia.org/wiki/N-sphere#Spherical_coordinates
  //
  // Remember that artcot(x/y) for us can be replaced by atan2(y, x)

  //
  // R coordinate -- rtheta(0)
  //
  rtheta(0) = sqrt(cart.transpose()*cart);

  //
  // theta coordinates -- rtheta(1..ds)
  //

  // REMEMBER: .block() arguments are (OFFSET, SIZE) and not (STARTOFFSET, ENDOFFSET)
  //           .segment() arguments are (STARTPOS, LEN) and not (STARTPOS, ENDPOS)

  // all except last
  rtheta.segment(1,ds-1).setZero();

  size_t i;

  Scalar val = pow(cart(ds),2);
  for (i = ds-1; i >= 1; --i) {
    val = val + pow(cart(i),2);
    rtheta(i) = val;
  }

  for (i = 1; i < ds; i++) {
    rtheta(i) = atan2(sqrt(rtheta(i)), cart(i-1));
  }

  // last angle, theta(ds-1) == rtheta(ds) :
  val = sqrt(pow(cart(ds), 2) + pow(cart(ds-1), 2)) + cart(ds-1);

  rtheta(ds) = 2.0 * atan2(cart(ds), val);
}


/** \brief Convert spherical angles to Cartesian coordinates in N dimensions
 *
 * See \ref pageParamsSphericalCoords for information about the conventions and ranges of
 * the coordinates.
 *
 * This function behaves exactly like \ref sph_to_cart(), but takes the \a theta arguments
 * separately from the \a R argument. This is useful if you only have angle coordinates
 * that parameterize a fixed-radius hypersphere.
 *
 * \param cart (output) is a vector of \a N entries, which will be set to the cartesian
 *        coordinates of the point represented by the spherical coordinates (\a R, \a
 *        theta).
 *
 * \param theta vector of <em>N-1</em> entries which specifies the \f$\theta_i\f$
 *        spherical coordinates to transform. The value <em>theta(0)</em> is
 *        \f$\theta_1\f$, and so on.
 *
 * \param R the radial coordinate part of the hyperspherical coordinates of the point to
 *        transform.
 *
 */
template<typename Der1, typename Der2>
inline void sphsurf_to_cart(Eigen::MatrixBase<Der2>& cart, const Eigen::MatrixBase<Der1>& theta,
                            const typename Eigen::MatrixBase<Der1>::Scalar R = 1.0)
{
  { using namespace Eigen; EIGEN_STATIC_ASSERT_LVALUE(Der2); }
  // same as sph_to_cart, except that only the angles are given, and R is given
  // separately, defaulting to 1.

  //  typedef typename Eigen::MatrixBase<Der1>::Scalar Scalar;

  tomographer_assert(cart.cols() == 1 && theta.cols() == 1);
  tomographer_assert(cart.rows() == theta.rows() + 1);

  const size_t ds = theta.rows(); // dimension of the sphere

  // see http://people.sc.fsu.edu/~jburkardt/cpp_src/hypersphere_properties/hypersphere_properties.cpp

  cart.setConstant(R); // R coordinate
  
  size_t i;
  for (i = 0; i < ds; ++i) {
    cart(i) *= cos(theta(i));
    cart.segment(i+1, ds+1 - (i+1)) *= sin(theta(i));
  }
}

/** \brief Convert spherical coordinates to Cartesian coordinates in N dimensions
 *
 * See \ref pageParamsSphericalCoords for information about the conventions and ranges of
 * the coordinates.
 *
 * \param cart (output) is a vector of \a N entries, which will be set to the cartesian
 *        coordinates of the point represented by \a rtheta.
 *
 * \param rtheta vector of \a N entries which specifies the \f$r\f$ and \f$\theta_i\f$
 *        spherical coordinates to transform. <em>rtheta(0)</em> is the \f$r\f$ coordinate
 *        and <em>rtheta(1),..,rtheta(N-1)</em> are the angle coordinates, corresponding
 *        respectively to \f$\theta_1\ldots\theta_{N-1}\f$.
 *
 */
template<typename Der1, typename Der2>
inline void sph_to_cart(Eigen::MatrixBase<Der2>& cart, const Eigen::MatrixBase<Der1>& rtheta)
{
  { using namespace Eigen; EIGEN_STATIC_ASSERT_LVALUE(Der2); }
  //  typedef typename Eigen::MatrixBase<Der1>::Scalar Scalar;

  tomographer_assert(cart.cols() == 1 && rtheta.cols() == 1);
  tomographer_assert(cart.rows() == rtheta.rows());

  const size_t ds = rtheta.rows()-1; // dimension of the sphere

  // see http://people.sc.fsu.edu/~jburkardt/cpp_src/hypersphere_properties/hypersphere_properties.cpp

  sphsurf_to_cart(cart, rtheta.segment(1,ds), rtheta(0));
}



/** \brief Volume element of the hypersphere
 *
 * Calculates the <a
 * href="http://en.wikipedia.org/wiki/N-sphere\#Spherical_volume_element"
 * target="_blank">volume element</a>, or Jacobian, of the conversion from cartesian
 * coordinates to spherical coordinates. More precisely, this function computes
 * \f[
 *   J = \left\vert\det \frac{\partial(x_i)}{\partial(r,\theta_j)}\right\vert
 *     = r^{N-1} \sin^{N-2}\left(\theta_1\right)\sin^{N-3}\left(\theta_2\right)
 *       \ldots\sin\left(\theta_{N-2}\right)\ ,
 * \f]
 * where \a N is the dimension of the Euclidean space in which the <em>N-1</em>-sphere is
 * embedded.
 *
 * See \ref pageParamsSphericalCoords for more info and conventions.
 * 
 */
template<typename Der1>
inline typename Eigen::MatrixBase<Der1>::Scalar cart_to_sph_jacobian(const Eigen::MatrixBase<Der1>& rtheta)
{
  const size_t ds = rtheta.rows()-1; // dimension of the sphere
  typename Eigen::MatrixBase<Der1>::Scalar jac = pow(rtheta(0), (int)ds); // r^{n-1}

  size_t i;
  for (i = 0; i < ds-1; ++i) {
    jac *= pow(sin(rtheta(1+i)), (int)(ds-1-i));
  }

  return jac;
}

/** \brief Surface element of the hypersphere
 *
 * Calculates the <a
 * href="http://en.wikipedia.org/wiki/N-sphere\#Spherical_volume_element"
 * target="_blank">volume element</a>, or Jacobian, of the conversion from cartesian
 * coordinates to spherical coordinates <em>on the surface of the hypersphere of fixed
 * radius R=1</em>. More precisely, this function computes
 * \f[
 *   \left\vert J\right\vert_{r=1}
 *     = \left\vert\det \frac{\partial(x_i)}{\partial(r,\theta_j)}\right\vert_{r=1}
 *     = \sin^{N-2}\left(\theta_1\right)\sin^{N-3}\left(\theta_2\right)
 *       \ldots\sin\left(\theta_{N-2}\right)\ ,
 * \f]
 * where \a N is the dimension of the Euclidean space in which the <em>N-1</em>-sphere is
 * embedded.
 *
 * See \ref pageParamsSphericalCoords for more info and conventions.
 * 
 * \param theta is the vector of the <em>N-1</em> spherical angles,
 *        \f$\theta_1\ldots\theta_{N-1}\f$.
 */
template<typename Der1>
inline typename Eigen::MatrixBase<Der1>::Scalar surf_sph_jacobian(const Eigen::MatrixBase<Der1>& theta)
{
  const size_t ds = theta.rows();

  typename Eigen::MatrixBase<Der1>::Scalar jac = 1;

  size_t i;
  for (i = 0; i < ds-1; ++i) {
    jac *= pow(sin(theta(i)), (int)(ds-1-i));
  }

  return jac;
}


/** \brief The differential of passing from spherical to cartesian coordinates on the
 * sphere of unit radius.
 *
 * The input parameter \a theta is a list of spherical angles, with <em>theta(i-1)</em>
 * (C/C++ offset) corresponding to \f$ \theta_i \f$. The coordinate \f$ r\f$ is set equal
 * to one. The dimension \a N of the Euclidian space is given by the length of the \a
 * theta vector plus one.
 *
 * After this function returns, <em>dxdtheta(k-1,i-1)</em> (C/C++ offsets) is set to the
 * value of
 * \f[
 *   \frac{\partial x_k}{\partial \theta_i} ,
 * \f]
 * with <em>k=1,...,N</em>, and <em>i=1,..,N-1</em>.
 *
 */
template<typename Der1, typename Der2>
inline void sphsurf_diffjac(Eigen::ArrayBase<Der1> & dxdtheta, const Eigen::MatrixBase<Der2>& theta)
{
  //
  // For this calculation & the 2nd derivatives, see [Drafts&Calculations, Vol. V,
  // 13.02.2015 (~60%)]
  //

  { using namespace Eigen; EIGEN_STATIC_ASSERT_LVALUE(Der1); }
  typedef typename Eigen::ArrayBase<Der1>::Scalar Scalar;

  const size_t ds = theta.rows();
  const size_t n = ds + 1;

  tomographer_assert(theta.cols() == 1);
  tomographer_assert(dxdtheta.rows() == (int)n);
  tomographer_assert(dxdtheta.cols() == (int)ds);

  Eigen::Array<Scalar, Der2::RowsAtCompileTime, 1> sintheta(theta.rows());
  sintheta = theta.array().sin();
  Eigen::Array<Scalar, Der2::RowsAtCompileTime, 1> costheta(theta.rows());
  costheta = theta.array().cos();

  size_t i, k, mm;
  for (i = 0; i < ds; ++i) {
    for (k = 0; k < n; ++k) {
      //std::cout << "k,i = "<< k<< ", "<< i << "\n";
      Scalar val = 0.0;
      // pick the right case
      if (i > k) {
        //std::cout << "[situation i>k]\n";
        val = 0.0;
      } else if (i == k && k < n - 1) {
        //std::cout << "[situation i == k && k < n-1]\n";
        val = -1;
        for (mm = 0; mm <= i; ++mm) {
          val *= sintheta(mm);
        }
      } else if (i == k && k == n-1) {
        //std::cout << "[situation i == k && k == n-1]\n";
        val = costheta(i);
        for (mm = 0; mm < i; ++mm) {
          val *= sintheta(mm);
        }
      } else if (i < k && k < n-1) {
        //std::cout << "[situation i < k && k < n-1]\n";
        val = costheta(i) * costheta(k);
        for (mm = 0; mm < k; ++mm) {
          if (mm == i) {
            continue;
          }
          val *= sintheta(mm);
        }
      } else { // i < k && k == n-1
        //std::cout << "[else: {situation i < k && k == n-1}]\n";
        val = costheta(i);
        for (mm = 0; mm < n-1; ++mm) {
          if (mm == i) {
            continue;
          }
          val *= sintheta(mm);
        }
      }
      dxdtheta(k,i) = val;
      //std::cout << "dxdtheta(k="<<k<<",i="<<i<<") = "<< val << "\n";
    }
  }
}

/** \brief The second order differential of passing from spherical to cartesian
 * coordinates on the sphere of unit radius.
 *
 * The input parameter \a theta is a list of spherical angles, with <em>theta(i-1)</em>
 * (C/C++ offset) corresponding to \f$ \theta_i \f$. The coordinate \f$ r\f$ is set equal
 * to one. The dimension \a N of the Euclidian space is given by the length of the \a
 * theta vector plus one.
 *
 * After this function returns, <em>ddxddtheta(k-1,(i-1)+(N-1)*(j-1))</em> (C/C++ offsets)
 * is set to the value of
 * \f[
 *   \frac{\partial^2 x_k}{\partial \theta_i \partial \theta_j}\ ,
 * \f]
 * with <em>k = 1,...,N</em>, and <em>i,j = 1,..,N-1</em>.
 */
template<typename Der1, typename Der2>
inline void sphsurf_diffjac2(Eigen::ArrayBase<Der1> & ddxddtheta, const Eigen::MatrixBase<Der2>& theta)
{
  //
  // For this calculation & the 2nd derivatives, see [Drafts&Calculations, Vol. V,
  // 13.02.2015 (~60%)]
  //

  { using namespace Eigen; EIGEN_STATIC_ASSERT_LVALUE(Der1); }
  typedef typename Eigen::ArrayBase<Der1>::Scalar Scalar;

  const size_t ds = theta.rows();
  const size_t n = ds + 1;

  tomographer_assert(theta.cols() == 1);
  tomographer_assert(ddxddtheta.rows() == (int)n);
  tomographer_assert(ddxddtheta.cols() == (int)(ds*ds));

  Eigen::Array<Scalar, Der2::RowsAtCompileTime, 1> sintheta(theta.rows());
  sintheta = theta.array().sin();
  Eigen::Array<Scalar, Der2::RowsAtCompileTime, 1> costheta(theta.rows());
  costheta = theta.array().cos();

  size_t i, j, k, mm;
  for (k = 0; k < n; ++k) {
    for (i = 0; i < ds; ++i) {
      for (j = 0; j <= i; ++j) {
        //std::cout << "k,i,j = "<< k<< ", "<< i << ", " << j << "\n";
        Scalar val = 0.0;
        if (i > k) {
          val = 0.0;
        } else if (i == k && k < n-1) {
          val = -costheta(j);
          for (mm = 0; mm <= i; ++mm) {
            if (mm == j) {
              continue;
            }
            val *= sintheta(mm);
          }
        } else if (i == k && k == n-1) {
          if (j == i) {
            val = -1.0;
            for (mm = 0; mm <= i; ++mm) {
              val *= sintheta(mm);
            }
          } else { // j < i
            val = costheta(i)*costheta(j);
            for (mm = 0; mm < i; ++mm) { // i not included
              if (mm == j) {
                continue;
              }
              val *= sintheta(mm);
            }
          }
        } else if (i < k && k < n-1) {
          if (j == i) {
            val = -costheta(k);
            for (mm = 0; mm < k; ++mm) {
              val *= sintheta(mm);
            }
          } else { // j < i
            val = costheta(j)*costheta(i)*costheta(k);
            for (mm = 0; mm < k; ++mm) {
              if (mm == j || mm == i) {
                continue;
              }
              val *= sintheta(mm);
            }
          }
        } else { // i < k && k == n-1
          if (j == i) {
            val = -1.0;
            for (mm = 0; mm < n-1; ++mm) {
              val *= sintheta(mm);
            }
          } else { // j < i
            val = costheta(i)*costheta(j);
            for (mm = 0; mm < n-1; ++mm) {
              if (mm == i || mm == j) {
                continue;
              }
              val *= sintheta(mm);
            }
          }
        }
        // set the calculated value
        ddxddtheta(k, i+ds*j) = val;
	ddxddtheta(k, j+ds*i) = val; // symmetric
      }
    }
  }
}


} // namespace SphCoords
} // namespace MathTools
} // namespace Tomographer


#endif
