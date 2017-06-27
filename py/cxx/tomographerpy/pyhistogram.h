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

#ifndef PY_HISTOGRAM_H
#define PY_HISTOGRAM_H


#include <tomographerpy/common.h>

#include <tomographer/histogram.h>
#include <tomographer/tools/cxxutil.h>


// namespace Tomographer { namespace Tools {
// template<>
// inline bool isFinite<py::float_>(py::object o)
// {
//   auto numpy = py::module::import("numpy");
//   return numpy.attr("isfinite")(o).cast<bool>();
// }
// } } // Tomographer::Tools


namespace tpy {

//! Histogram Params. See \ref Tomographer::HistogramParams
typedef Tomographer::HistogramParams<RealType> HistogramParams;



//! Histogram class like \ref Tomographer::Histogram, but with NumPy arrays storage
class Histogram
{
public:
  Histogram(HistogramParams params_)
    : params(params_),
      bins(py::cast(Eigen::VectorXd::Zero(params_.num_bins))),
      off_chart(py::cast(0.0))
  {
  }

  Histogram(tpy::RealType min, tpy::RealType max, Eigen::Index num_bins)
    : params(min, max, num_bins),
      bins(py::cast(Eigen::VectorXd::Zero(num_bins))),
      off_chart(py::cast(0.0))
  {
  }

  template<typename Scalar, typename CountType>
  Histogram(const Tomographer::Histogram<Scalar,CountType> & h)
    : params(h.params),
      bins(py::cast(h.bins)),
      off_chart(py::cast(h.off_chart))
  {
  }

  //  ### Apparently doesn't work, need to be more brutal:
  //  template<typename Scalar, typename CountType>
  //  inline operator Tomographer::Histogram<Scalar,CountType> ()
  template<typename X>
  inline operator X () const
  {
    typedef typename X::Scalar Scalar;
    typedef typename X::CountType CountType;
    return toCxxHistogram<Scalar,CountType>();
  }

  template<typename Scalar, typename CountType>
  inline Tomographer::Histogram<Scalar,CountType> toCxxHistogram() const
  {
    Tomographer::Histogram<Scalar,CountType> h(params);
    h.bins = bins.cast<Eigen::Matrix<CountType,Eigen::Dynamic,1> >();
    h.off_chart = off_chart.cast<CountType>();
    return h;
  }

  inline void set_bins(py::object newbins)
  {
    if (py::len(newbins.attr("shape")) != 1) {
      throw py::type_error("Expected 1-D NumPy array for assignment to Histogram.bins");
    }
    if ((Eigen::Index)py::len(newbins) != params.num_bins) {
      throw py::type_error(streamstr("Expected "<<params.num_bins<<" elements for assignment to Histogram.bins,"
                                     " got "<<py::len(newbins)));
    }
    bins = newbins;
  }
  inline void set_off_chart(py::object o)
  {
    auto np = py::module::import("numpy");
    if ( ! np.attr("isscalar")(o).cast<bool>() ) {
      throw py::type_error("Expected scalar for assignment to Histogram.off_chart");
    }
    off_chart = o;
  }

  inline void load(py::object x, py::object o)
  {
    set_bins(x);
    set_off_chart(o);
  }

  inline py::object normalization() const {
    auto np = py::module::import("numpy");
    // off_chart + binResolution() * sum(bins)
    return np.attr("add")(off_chart, np.attr("multiply")(py::cast(params.binResolution()), bins.attr("sum")()));
  }

  inline py::object totalCounts() const {
    auto np = py::module::import("numpy");
    // off_chart + sum(bins)
    return np.attr("add")(off_chart, bins.attr("sum")());
  }

  HistogramParams params;
  py::object bins;
  py::object off_chart;

  enum { HasErrorBars = 0 };
};


//! A Histogram with real counts and error bars. See \ref Tomographer::HistogramWithErrorBars
class HistogramWithErrorBars : public Histogram
{
public:
  HistogramWithErrorBars(HistogramParams params_)
    : Histogram(params_),
      delta(py::cast(Eigen::VectorXd::Zero(params_.num_bins)))
  {
  }

  HistogramWithErrorBars(tpy::RealType min, tpy::RealType max, Eigen::Index num_bins)
    : Histogram(min, max, num_bins),
      delta(py::cast(Eigen::VectorXd::Zero(num_bins)))
  {
  }

  template<typename Scalar, typename CountType>
  HistogramWithErrorBars(const Tomographer::HistogramWithErrorBars<Scalar,CountType> & h)
    : Histogram(h),
      delta(py::cast(h.delta))
  {
  }

  //  ### Apparently doesn't work, need to be more brutal:
  //  template<typename Scalar, typename CountType>
  //  inline operator Tomographer::HistogramWithErrorBars<Scalar,CountType> ()
  template<typename X>
  inline operator X () const
  {
    typedef typename X::Scalar Scalar;
    typedef typename X::CountType CountType;
    return toCxxHistogram<Scalar,CountType>();
  }

  template<typename Scalar, typename CountType>
  inline Tomographer::HistogramWithErrorBars<Scalar,CountType> toCxxHistogram() const
  {
    Tomographer::HistogramWithErrorBars<Scalar,CountType> h(params);
    h.bins = bins.cast<Eigen::Matrix<CountType,Eigen::Dynamic,1> >();
    h.delta = delta.cast<Eigen::Matrix<CountType,Eigen::Dynamic,1> >();
    h.off_chart = off_chart.cast<CountType>();
    return h;
  }

  inline void set_delta(py::object newdelta)
  {
    if (py::len(newdelta.attr("shape")) != 1) {
      throw py::type_error("Expected 1-D NumPy array for assignment to HistogramWithErrorBars.delta");
    }
    if ((Eigen::Index)py::len(newdelta) != params.num_bins) {
      throw py::type_error(streamstr("Expected "<<params.num_bins<<" elements for assignment to HistogramWithErrorBars.delta,"
                                     " got "<<py::len(newdelta)));
    }
    delta = newdelta;
  }

  inline void load(py::object x, py::object err, py::object o)
  {
    set_bins(x);
    set_delta(err);
    set_off_chart(o);
  }

  py::object delta;

  enum { HasErrorBars = 1 };
};




} // namespace Py


#endif
