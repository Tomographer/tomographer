
#ifndef SPHCOORDS_H
#define SPHCOORDS_H

#include <Eigen/Eigen>
#include <cmath>

/** \file sphcoords.h
 *
 * \brief spherical coordinates conversion routines, with jacobian etc.
 *
 * conventions:
 *
 * cart: cartesian coordinates x, y, z...
 *
 * rtheta: [r, theta_1, theta_2, ..., theta_{ds}]  with   [ds=N-1=dimension of sphere]
 *         theta_{ds} = 0..2pi
 *         theta_{i} = 0..pi   [ for 1 <= i <= ds-1 ]
 *
 *
 * NOTE: In the special case of the 2-sphere, this does not map back to usuall 3-D
 * spherical coordinates, with theta=0 at (X=0,Y=0,Z=1), theta=pi at (X=0,Y=0,Z=-1), and
 * theta=pi/2,phi=0 for (X=1,Y=0,Z=0)
 *
 * In fact, it is mapped to:
 *
 *    X = r Cos[theta1]                   -- what we normally call Z
 *    Y = r Sin[theta1] * Cos[Theta2]     -- what we normally call X
 *    Z = r Sin[theta1] * Sin[Theta1]     -- what we normally call Y
 *
 * So effectively, the angles count from (X=+1,Y=0,Z=0) and theta1 increases to
 * (X=-1,Y=0,Z=0); then theta2 wraps around, with theta2=0 corresponding to the direction
 * in which Y=+1.
 *
 */



namespace Tomographer
{
namespace Tools
{


/** \brief Convert Cartesian coordinates to spherical coordinates in N dimensions
 *
 * The transformation is as the one given in
 * http://en.wikipedia.org/wiki/N-sphere#Spherical_coordinates .
 *
 * See \ref sphcoords.h for information about the conventions.
 *
 * \param rtheta (output), vector of \i N entries, will store the R and Theta values
 *        corresponding to the spherical coordinates representation of \c cart. 
 *        `rtheta(0)` is the R coordinate and `rtheta(1),..,rtheta(N-1)` are the angle
 *        coordinates; all but the last range from `0..pi`, the last ranges from -pi to
 *        pi.
 *
 * \param cart is a vector of \i N entries, corresponding to the cartesian coordinates of
 *        the point to transform.
 */
template<typename Der1, typename Der2>
void cart_to_sph(Eigen::MatrixBase<Der2>& rtheta, const Eigen::MatrixBase<Der1>& cart)
{
  typedef typename Eigen::MatrixBase<Der1>::Scalar Scalar;

  eigen_assert(cart.cols() == 1 && rtheta.cols() == 1);
  eigen_assert(cart.rows() == rtheta.rows());

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

  // all except last
  rtheta.block(1,0,ds-1,1).setZero();

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


/** \brief Convert spherical coordinates to Cartesian coordinates in N dimensions
 *
 * The transformation is as the one given in
 * http://en.wikipedia.org/wiki/N-sphere#Spherical_coordinates .
 *
 * See \ref sphcoords.h for information about the conventions.
 *
 * \param cart (output) is a vector of \i N entries, which will be set to the cartesian
 *        coordinates of the point represented by \c rtheta.
 *
 * \param rtheta vector of \i N entries which specifies the R and Theta spherical
 *        coordinates to transform. `rtheta(0)` is the R coordinate and
 *        `rtheta(1),..,rtheta(N-1)` are the angle coordinates.
 *
 */
template<typename Der1, typename Der2>
inline void sphsurf_to_cart(Eigen::MatrixBase<Der2>& cart, const Eigen::MatrixBase<Der1>& theta,
                            const typename Eigen::MatrixBase<Der1>::Scalar R = 1.0)
{
  // same as sph_to_cart, except that only the angles are given, and R is given
  // separately, defaulting to 1.

  //  typedef typename Eigen::MatrixBase<Der1>::Scalar Scalar;

  eigen_assert(cart.cols() == 1 && theta.cols() == 1);
  eigen_assert(cart.rows() == theta.rows() + 1);

  const size_t ds = theta.rows(); // dimension of the sphere

  // see http://people.sc.fsu.edu/~jburkardt/cpp_src/hypersphere_properties/hypersphere_properties.cpp

  cart.setConstant(R); // R coordinate
  
  size_t i;
  for (i = 0; i < ds; ++i) {
    cart(i) *= cos(theta(i));
    cart.block(i+1, 0, ds+1 - (i+1), 1) *= sin(theta(i));
  }
}

template<typename Der1, typename Der2>
void sph_to_cart(Eigen::MatrixBase<Der2>& cart, const Eigen::MatrixBase<Der1>& rtheta)
{
  //  typedef typename Eigen::MatrixBase<Der1>::Scalar Scalar;

  eigen_assert(cart.cols() == 1 && rtheta.cols() == 1);
  eigen_assert(cart.rows() == rtheta.rows());

  const size_t ds = rtheta.rows()-1; // dimension of the sphere

  // see http://people.sc.fsu.edu/~jburkardt/cpp_src/hypersphere_properties/hypersphere_properties.cpp

  sphsurf_to_cart(cart, rtheta.block(1,0,ds,1), rtheta(0));
}



// volume element of hypersphere
template<typename Der1>
typename Eigen::MatrixBase<Der1>::Scalar cart_to_sph_jacobian(const Eigen::MatrixBase<Der1>& rtheta)
{
  const size_t ds = rtheta.rows()-1; // dimension of the sphere
  typename Eigen::MatrixBase<Der1>::Scalar jac = pow(rtheta(0), (int)ds); // r^{n-1}

  size_t i;
  for (i = 0; i < ds-1; ++i) {
    jac *= pow(sin(rtheta(1+i)), (int)(ds-1-i));
  }

  return jac;
}

// surface element of sphere; (think like the volume element but with r integrated out
// with a delta function at r=1)
//
// Note that here only the vector of theta's is given, no R
//
template<typename Der1>
typename Eigen::MatrixBase<Der1>::Scalar surf_sph_jacobian(const Eigen::MatrixBase<Der1>& theta)
{
  const size_t ds = theta.rows();

  typename Eigen::MatrixBase<Der1>::Scalar jac = 1;

  size_t i;
  for (i = 0; i < ds-1; ++i) {
    jac *= pow(sin(theta(i)), (int)(ds-1-i));
  }

  return jac;
}



//
// The differential of the coordinate change theta -> x for the sphere of radius R = 1
//
// dxdtheta(k,i) is the expression \frac{\partial x_k}{\partial \theta_i}
//
// For this & the 2nd derivatives, see [Drafts&Calculations Vol. V 13.02.2015 (~60%)]
//
template<typename Der1, typename Der2>
void sphsurf_diffjac(Eigen::ArrayBase<Der1> & dxdtheta, const Eigen::MatrixBase<Der2>& theta)
{
  typedef typename Eigen::ArrayBase<Der1>::Scalar Scalar;

  // FIXME: this is highly ineffective, we calculate the same product of sines all the time...

  const size_t ds = theta.rows();
  const size_t n = ds + 1;

  eigen_assert(theta.cols() == 1);
  eigen_assert(dxdtheta.rows() == (int)n);
  eigen_assert(dxdtheta.cols() == (int)ds);

  size_t i, k, mm;
  for (i = 0; i < ds; ++i) {
    for (k = 0; k < n; ++k) {
      //cout << "k,i = "<< k<< ", "<< i << "\n";
      Scalar val = 0.0;
      // pick the right case
      if (i > k) {
        //cout << "[situation i>k]\n";
        val = 0.0;
      } else if (i == k && k < n - 1) {
        //cout << "[situation i == k && k < n-1]\n";
        val = -1;
        for (mm = 0; mm <= i; ++mm) {
          val *= sin(theta(mm));
        }
      } else if (i == k && k == n-1) {
        //cout << "[situation i == k && k == n-1]\n";
        val = cos(theta(i));
        for (mm = 0; mm < i; ++mm) {
          val *= sin(theta(mm));
        }
      } else if (i < k && k < n-1) {
        //cout << "[situation i < k && k < n-1]\n";
        val = cos(theta(i)) * cos(theta(k));
        for (mm = 0; mm < k; ++mm) {
          if (mm == i) {
            continue;
          }
          val *= sin(theta(mm));
        }
      } else { // i < k && k == n-1
        //cout << "[else: {situation i < k && k == n-1}]\n";
        val = cos(theta(i));
        for (mm = 0; mm < n-1; ++mm) {
          if (mm == i) {
            continue;
          }
          val *= sin(theta(mm));
        }
      }
      dxdtheta(k,i) = val;
      //cout << "dxdtheta(k="<<k<<",i="<<i<<") = "<< val << "\n";
    }
  }
}

//
// The second differential of the coordinate change theta -> x for the sphere of radius R = 1
//
// n = number of cartesian dimensions = number of spherical dimensions + 1
// ds = number of spherical dimensions = n - 1
// (i.e. k = 0..(n-1),  i = 0..(n-2) = 0..(ds-1) )
//
// ddxddtheta(k,i+ds*j) is the expression \frac{\partial^2 x_k}{\partial \theta_i \partial \theta_j}
//
template<typename Der1, typename Der2>
void sphsurf_diffjac2(Eigen::ArrayBase<Der1> & ddxddtheta, const Eigen::MatrixBase<Der2>& theta)
{
  typedef typename Eigen::ArrayBase<Der1>::Scalar Scalar;

  // FIXME: this is highly ineffective, we calculate the same product of sines all the time...

  const size_t ds = theta.rows();
  const size_t n = ds + 1;

  eigen_assert(theta.cols() == 1);
  eigen_assert(ddxddtheta.rows() == (int)n);
  eigen_assert(ddxddtheta.cols() == (int)(ds*ds));

  size_t i, j, k, mm;
  for (k = 0; k < n; ++k) {
    for (i = 0; i < ds; ++i) {
      for (j = 0; j <= i; ++j) {
        //cout << "k,i,j = "<< k<< ", "<< i << ", " << j << "\n";
        Scalar val = 0.0;
        if (i > k) {
          val = 0.0;
        } else if (i == k && k < n-1) {
          val = -cos(theta(j));
          for (mm = 0; mm <= i; ++mm) {
            if (mm == j) {
              continue;
            }
            val *= sin(theta(mm));
          }
        } else if (i == k && k == n-1) {
          if (j == i) {
            val = -1.0;
            for (mm = 0; mm <= i; ++mm) {
              val *= sin(theta(mm));
            }
          } else { // j < i
            val = cos(theta(i))*cos(theta(j));
            for (mm = 0; mm < i; ++mm) { // i not included
              if (mm == j) {
                continue;
              }
              val *= sin(theta(mm));
            }
          }
        } else if (i < k && k < n-1) {
          if (j == i) {
            val = -cos(theta(k));
            for (mm = 0; mm < k; ++mm) {
              val *= sin(theta(mm));
            }
          } else { // j < i
            val = cos(theta(j))*cos(theta(i))*cos(theta(k));
            for (mm = 0; mm < k; ++mm) {
              if (mm == j || mm == i) {
                continue;
              }
              val *= sin(theta(mm));
            }
          }
        } else { // i < k && k == n-1
          if (j == i) {
            val = -1.0;
            for (mm = 0; mm < n-1; ++mm) {
              val *= sin(theta(mm));
            }
          } else { // j < i
            val = cos(theta(i))*cos(theta(j));
            for (mm = 0; mm < n-1; ++mm) {
              if (mm == i || mm == j) {
                continue;
              }
              val *= sin(theta(mm));
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


}
}


#endif
