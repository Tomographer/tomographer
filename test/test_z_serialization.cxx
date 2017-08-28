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

#include <cmath>

#include <string>
#include <sstream>
#include <random>

#include <boost/math/constants/constants.hpp>

// include before <Eigen/*> !
#include "test_tomographer.h"

#include <tomographer/densedm/indepmeasllh.h>
#include <tomographer/densedm/tspacefigofmerit.h>
#include <tomographer/tools/eigenutil.h>
#include <tomographer/mhrw_valuehist_tools.h>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>


// -----------------------------------------------------------------------------
// fixture(s)

template<typename T>
void save_and_reload(const T & a, T & b)
{
  std::string buf;
  // write
  { std::stringstream sstream;
    boost::archive::text_oarchive oa(sstream);
    oa << a;
    buf = sstream.str();
  }
  BOOST_MESSAGE("DATA: " << buf) ;
  // and read
  { std::istringstream sstream(buf);
    boost::archive::text_iarchive ia(sstream);
    ia >> b;
  }
}


// -----------------------------------------------------------------------------
// test suites


BOOST_AUTO_TEST_SUITE(serializing)

BOOST_AUTO_TEST_CASE(indepmeasllh)
{
  typedef Tomographer::DenseDM::DMTypes<2> DMTypes;
  DMTypes dmt;
  
  typedef Tomographer::DenseDM::IndepMeasLLH<DMTypes> IndepMeasLLH;
  IndepMeasLLH dat(dmt);

  IndepMeasLLH::VectorParamListType Exn(6, dmt.dim2());
  Exn <<
    0.5, 0.5,  1./std::sqrt(2.0),  0,
    0.5, 0.5, -1./std::sqrt(2.0),  0,
    0.5, 0.5,  0,         1./std::sqrt(2.0),
    0.5, 0.5,  0,        -1./std::sqrt(2.0),
    1,   0,    0,         0,
    0,   1,    0,         0
    ;
  IndepMeasLLH::FreqListType Nx(6);
  Nx << 1500, 800, 300, 300, 10, 30;

  dat.setMeas(Exn, Nx);

  IndepMeasLLH * dat2 = NULL;

  save_and_reload(&dat, dat2);

  // compare
  BOOST_CHECK_EQUAL(dat.dmt.dim(), dat2->dmt.dim()) ;
  MY_BOOST_CHECK_EIGEN_EQUAL(dat.Exn(), dat2->Exn(), tol);
  MY_BOOST_CHECK_EIGEN_EQUAL(dat.Nx(), dat2->Nx(), tol);

  delete dat2;
}

BOOST_AUTO_TEST_SUITE(tspacefigofmerit)

BOOST_AUTO_TEST_CASE(fidelitytorefcalculator)
{
  typedef Tomographer::DenseDM::DMTypes<2> DMTypes;
  typedef Tomographer::DenseDM::TSpace::FidelityToRefCalculator<DMTypes> TheType;
  DMTypes::MatrixType T_ref;
  T_ref << std::sqrt(0.2), std::complex<double>(0,std::sqrt(0.1)),
    std::complex<double>(-std::sqrt(0.4), std::sqrt(0.2)), -std::sqrt(0.1) ;
  TheType a(T_ref);

  TheType b;
  save_and_reload(a, b);

  MY_BOOST_CHECK_FLOATS_EQUAL(a.getValue(T_ref), b.getValue(T_ref), tol) ;
}

BOOST_AUTO_TEST_CASE(purifdisttorefcalculator)
{
  typedef Tomographer::DenseDM::DMTypes<2> DMTypes;
  typedef Tomographer::DenseDM::TSpace::PurifDistToRefCalculator<DMTypes> TheType;
  DMTypes::MatrixType T_ref;
  T_ref << std::sqrt(0.2), std::complex<double>(0,std::sqrt(0.1)),
    std::complex<double>(-std::sqrt(0.4), std::sqrt(0.2)), -std::sqrt(0.1) ;
  TheType a(T_ref);

  TheType b;
  save_and_reload(a, b);

  MY_BOOST_CHECK_FLOATS_EQUAL(a.getValue(T_ref), b.getValue(T_ref), tol) ;
}

BOOST_AUTO_TEST_CASE(trdisttorefcalculator)
{
  typedef Tomographer::DenseDM::DMTypes<2> DMTypes;
  typedef Tomographer::DenseDM::TSpace::TrDistToRefCalculator<DMTypes> TheType;
  DMTypes::MatrixType rho_ref;
  rho_ref << 0.8, std::complex<double>(0,0.2),
    std::complex<double>(0,-0.2), 0.2 ;
  TheType a(rho_ref);

  TheType b;
  save_and_reload(a, b);

  DMTypes::MatrixType T_ref;
  T_ref << std::sqrt(0.2), std::complex<double>(0,std::sqrt(0.1)),
    std::complex<double>(-std::sqrt(0.4), std::sqrt(0.2)), -std::sqrt(0.1) ;
  MY_BOOST_CHECK_FLOATS_EQUAL(a.getValue(T_ref), b.getValue(T_ref), tol) ;
}

BOOST_AUTO_TEST_CASE(observablevaluecalculator)
{
  typedef Tomographer::DenseDM::DMTypes<2> DMTypes;
  DMTypes dmt;
  typedef Tomographer::DenseDM::TSpace::ObservableValueCalculator<DMTypes> TheType;
  DMTypes::MatrixType A;
  A << 2, std::complex<double>(0,-1),
    std::complex<double>(0,1), -1 ;
  TheType a(dmt, A);

  TheType b;
  save_and_reload(a, b);

  DMTypes::MatrixType rho_ref;
  rho_ref << 0.8, std::complex<double>(0,0.2),
    std::complex<double>(0,-0.2), 0.2 ;

  DMTypes::MatrixType T_ref;
  T_ref << std::sqrt(0.2), std::complex<double>(0,std::sqrt(0.1)),
    std::complex<double>(-std::sqrt(0.4), std::sqrt(0.2)), -std::sqrt(0.1) ;
  MY_BOOST_CHECK_FLOATS_EQUAL(a.getValue(T_ref), b.getValue(T_ref), tol) ;
}

BOOST_AUTO_TEST_SUITE_END() // tspacefigofmerit




BOOST_AUTO_TEST_CASE(valuehisttools_cdata)
{
  typedef Tomographer::DenseDM::DMTypes<2> DMTypes;
  DMTypes dmt;
  typedef Tomographer::DenseDM::TSpace::ObservableValueCalculator<DMTypes> ValCalc;
  DMTypes::MatrixType A;
  A << 2, std::complex<double>(0,-1),
    std::complex<double>(0,1), -1 ;
  ValCalc valcalc(dmt, A);

  typedef Tomographer::MHRWTasks::ValueHistogramTools::CDataBase<ValCalc>  CDataBaseType;

  CDataBaseType cdata(valcalc,
                      CDataBaseType::HistogramParams(0.0,1.0,100),
                      9,
                      CDataBaseType::MHRWParamsType(0.04, 24, 1024, 32768),
                      123000456u);

  CDataBaseType cdata2;

  save_and_reload(cdata, cdata2);

  DMTypes::MatrixType T_ref;
  T_ref << std::sqrt(0.2), std::complex<double>(0,std::sqrt(0.1)),
    std::complex<double>(-std::sqrt(0.4), std::sqrt(0.2)), -std::sqrt(0.1) ;
  MY_BOOST_CHECK_FLOATS_EQUAL(valcalc.getValue(T_ref), cdata2.valcalc.getValue(T_ref), tol) ;
  MY_BOOST_CHECK_FLOATS_EQUAL(cdata.histogram_params.min, cdata2.histogram_params.min, tol) ;
  MY_BOOST_CHECK_FLOATS_EQUAL(cdata.histogram_params.max, cdata2.histogram_params.max, tol) ;
  BOOST_CHECK_EQUAL(cdata.histogram_params.num_bins, cdata2.histogram_params.num_bins) ;
  BOOST_CHECK_EQUAL(cdata.binningNumLevels.value, cdata2.binningNumLevels.value) ;

  MY_BOOST_CHECK_FLOATS_EQUAL(cdata.mhrw_params.mhwalker_params.step_size,
                              cdata2.mhrw_params.mhwalker_params.step_size, tol) ;
  BOOST_CHECK_EQUAL(cdata.mhrw_params.n_sweep, cdata2.mhrw_params.n_sweep) ;
  BOOST_CHECK_EQUAL(cdata.mhrw_params.n_therm, cdata2.mhrw_params.n_therm) ;
  BOOST_CHECK_EQUAL(cdata.mhrw_params.n_run, cdata2.mhrw_params.n_run) ;
  
  BOOST_CHECK_EQUAL(cdata.base_seed, cdata2.base_seed) ;
}




BOOST_AUTO_TEST_SUITE_END() // serializing
