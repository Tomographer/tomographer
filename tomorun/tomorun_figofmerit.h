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


#ifndef TOMORUN_FIGOFMERIT_H
#define TOMORUN_FIGOFMERIT_H

#include <string>
#include <tuple>

#include <tomographer/tools/ezmatio.h>
#include <tomographer/tools/cxxutil.h>
#include <tomographer/valuecalculator.h>


// =============================================================================
// Helpers for the tomorun figures of merit to read the reference states etc.
// =============================================================================

// These functions are used by several figures of merit, and they shouldn't need to be
// modified.

template<typename DMTypes>
inline typename DMTypes::MatrixType  get_ref_state_T(Tomographer::MAT::File * matf, std::string ref_obj_name)
{
  using MatrixType = typename DMTypes::MatrixType;
  namespace M = Tomographer::MAT;
  
  // read the reference state given explicitly as, e.g., "fidelity:rho_ref"
  if (!ref_obj_name.size()) {
    ref_obj_name = std::string("rho_ref");
  }
  auto mpsd = matf->var(ref_obj_name).value< M::EigenPosSemidefMatrixWithSqrt<MatrixType> >() ;
  //auto rho_ref = std::move(mpsd.mat);
  auto T_ref = std::move(mpsd.sqrt);
  return T_ref;
}

template<typename DMTypes>
inline typename DMTypes::MatrixType  get_ref_state_rho(Tomographer::MAT::File * matf, std::string ref_obj_name)
{
  using MatrixType = typename DMTypes::MatrixType;
  namespace M = Tomographer::MAT;
  
  // read the reference state given explicitly as, e.g., "fidelity:rho_ref"
  if (!ref_obj_name.size()) {
    ref_obj_name = std::string("rho_ref");
  }
  auto mpsd = matf->var(ref_obj_name).value< M::EigenPosSemidefMatrixWithSqrt<MatrixType> >() ;
  auto rho_ref = std::move(mpsd.mat);
  //auto T_ref = std::move(mpsd.sqrt);
  return rho_ref;
}




// =============================================================================
// DEFINE ALL THE AVAILABLE FIGURES OF MERIT HERE
// =============================================================================



// -----------------------------------------------------------------------------
// Figure of merit: (root) fidelity, cf. Nielsen & Chuang
// -----------------------------------------------------------------------------
struct FidelityFigureOfMerit
{
  // The name of the option: --value-type="fidelity"
  const std::string name{"fidelity"};

  // The ValueCalculator class we need to use, for a given DMTypes
  template<typename DMTypes> using ValueCalculator =
    Tomographer::DenseDM::TSpace::FidelityToRefCalculator<DMTypes,TomorunReal>;

  // The code which instanciates a new ValueCalculator with the appropriate input data
  template<typename DMTypes>
  static inline ValueCalculator<DMTypes> *
  createValueCalculator(DMTypes /*dmt*/, const std::string & ref_obj_name, Tomographer::MAT::File * matf)
  {
    // create the value calculator
    return new ValueCalculator<DMTypes>(get_ref_state_T<DMTypes>(matf, ref_obj_name)) ; 
  }

  // Print some help text to the screen when queried with --help. We can insert footnotes
  // using footnotes.addFootNote(...)
  static void print(std::ostream & stream, Tomographer::Tools::FmtFootnotes & footnotes) {
    stream
      << "The (root) fidelity to a reference state "
      << footnotes.addFootNote(
          "The root fidelity is defined as F(rho,sigma)=|| rho^{1/2} sigma^{1/2} ||_1, "
          "as in Nielsen and Chuang, \"Quantum Computation and Quantum Information\"."
          )
      << ". <RefObject> should be the name of a MATLAB variable present in the MATLAB data file. "
      "This object should be a complex dim x dim matrix, the density matrix of "
      "the reference state. If no <RefObject> is specified, then 'rho_MLE' is "
      "used."
      "\n\n" // new paragraph
      "Note: For the squared fidelity to a pure state (usually preferred in "
      "experimental papers), you should use \"obs-value\" with the observable "
      "being the density matrix of the reference state "
      << footnotes.addFootNote("Indeed, for pure rho_ref, F^2(rho,rho_ref) = tr(rho*rho_ref).")
      << "."
      ;
  }
};


// -----------------------------------------------------------------------------
// Figure of merit: purified distance
// -----------------------------------------------------------------------------
struct PurifiedDistanceFigureOfMerit
{
  // The name of the option: --value-type="purif-dist"
  const std::string name{"purif-dist"};

  // The ValueCalculator class we need to use, for a given DMTypes
  template<typename DMTypes> using ValueCalculator =
    Tomographer::DenseDM::TSpace::PurifDistToRefCalculator<DMTypes,TomorunReal>;

  // The code which instanciates a new ValueCalculator with the appropriate input data
  template<typename DMTypes>
  static inline ValueCalculator<DMTypes> *
  createValueCalculator(DMTypes /*dmt*/, const std::string & ref_obj_name, Tomographer::MAT::File * matf)
  {
    // create the value calculator
    return new ValueCalculator<DMTypes>(get_ref_state_T<DMTypes>(matf, ref_obj_name)) ; 
  }

  // Print some help text to the screen when queried with --help. We can insert footnotes
  // using footnotes.addFootNote(...)
  static void print(std::ostream & stream, Tomographer::Tools::FmtFootnotes & footnotes) {
    stream
      << "The purified distance to a reference state "
      << footnotes.addFootNote(
          "The purified distance, also called \"infidelity\" in the literature, is "
          "defined as P(rho,sigma) = \\sqrt{1 - F^2(rho,sigma)}."
          ) << ". "
      << "<RefObject> should be the name of a MATLAB variable present in the MATLAB "
      "data file. This object should be a complex dim x dim matrix, the density "
      "matrix of the reference state. If no <RefObject> is specified, then "
      "'rho_MLE' is used."
      ;
  }
};

// -----------------------------------------------------------------------------
// Figure of merit: trace distance
// -----------------------------------------------------------------------------
struct TraceDistanceFigureOfMerit
{
  // The name of the option: --value-type="tr-dist"
  const std::string name{"tr-dist"};

  // The ValueCalculator class we need to use, for a given DMTypes
  template<typename DMTypes> using ValueCalculator =
    Tomographer::DenseDM::TSpace::TrDistToRefCalculator<DMTypes,TomorunReal>;

  // The code which instanciates a new ValueCalculator with the appropriate input data
  template<typename DMTypes>
  static inline ValueCalculator<DMTypes> *
  createValueCalculator(DMTypes /*dmt*/, const std::string & ref_obj_name, Tomographer::MAT::File * matf)
  {
    // create the value calculator
    return new ValueCalculator<DMTypes>(get_ref_state_rho<DMTypes>(matf, ref_obj_name)) ; 
  }

  // Print some help text to the screen when queried with --help. We can insert footnotes
  // using footnotes.addFootNote(...)
  static void print(std::ostream & stream, Tomographer::Tools::FmtFootnotes & footnotes) {
    stream <<
      "The trace distance to a reference state "
      << footnotes.addFootNote(
          "The trace distance is computed as d(rho,sigma) = (1/2) ||rho - sigma||_1."
          )
      << ". <RefObject> should "
      "be the name of a MATLAB variable present in the MATLAB data file. This "
      "object should be a complex dim x dim matrix, the density matrix of the "
      "reference state. If no <RefObject> is specified, then 'rho_MLE' is used."
      ;
  }
};

// -----------------------------------------------------------------------------
// Figure of merit: expectation value of an observable
// -----------------------------------------------------------------------------
struct ObsValueFigureOfMerit
{
  const std::string name{"obs-value"};

  template<typename DMTypes> using ValueCalculator =
    Tomographer::DenseDM::TSpace::ObservableValueCalculator<DMTypes>;

  template<typename DMTypes>
  static inline ValueCalculator<DMTypes> *
  createValueCalculator(DMTypes dmt, std::string ref_obj_name, Tomographer::MAT::File * matf)
  {
    using MatrixType = typename DMTypes::MatrixType;
    namespace M = Tomographer::MAT;

    if (!ref_obj_name.size()) {
      ref_obj_name = "Observable";
    }

    auto A = matf->var(ref_obj_name).value<MatrixType>();

    // create the value calculator
    return new ValueCalculator<DMTypes>(dmt, std::move(A)) ; 
  }

  static void print(std::ostream & stream, Tomographer::Tools::FmtFootnotes & /*footnotes*/) {
    stream <<
      "The expectation value of an observable. <RefObject> should "
      "be the name of a MATLAB variable present in the MATLAB data file. This "
      "object should be a complex dim x dim matrix which represents the "
      "observable in question. If no <RefObject> is specified, the variable named "
      "\"Observable\" is looked up in the data file.\n"
      ;
  }
};



// ----------------------------------
// INSERT CUSTOM FIGURE OF MERIT HERE
// ----------------------------------

// ....






//
// List all available figures of merit here.
//
using TomorunFiguresOfMerit = std::tuple<
  // obs-value
  ObsValueFigureOfMerit,
  // tr-dist
  TraceDistanceFigureOfMerit,
  // fidelity
  FidelityFigureOfMerit,
  // purif-dist
  PurifiedDistanceFigureOfMerit
  >;  // INSERT CUSTOM FIGURE OF MERIT HERE -- add to this list













// =============================================================================
// The following are internal helper tools, don't modify.
// =============================================================================


template<typename Getter, typename TupleType>
struct TupleDynGet
{
  template<int I=0, typename std::enable_if<(I<std::tuple_size<TupleType>::value),bool>::type = true>
  static inline typename Getter::ValType get(std::size_t k) {
    tomographer_assert(k < std::tuple_size<TupleType>::value);
    if (I == k) {
      return typename Getter::template Get<typename std::tuple_element<I, TupleType>::type>().value;
    }
    return get<I+1>(k);
  }
  template<int I=0, typename std::enable_if<(I==std::tuple_size<TupleType>::value),bool>::type = true>
  static inline typename Getter::ValType get(std::size_t ) {
    return typename Getter::ValType() ; // assert failure already occurred
  }
};

struct GetStrNameField {
  using ValType = std::string;
  template<typename X_>
  struct Get {
    typedef X_ X;
    inline Get() : value(X().name) { }
    const std::string value;
  };
};

template<typename FiguresOfMeritTuple>
struct PrintFigsOfMerit
{
  template<int I=0, typename std::enable_if<(I<std::tuple_size<FiguresOfMeritTuple>::value),bool>::type = true>
  static inline void print(std::ostream & stream, Tomographer::Tools::FmtFootnotes & f) {
    std::ostringstream oss;
    std::tuple_element<I, FiguresOfMeritTuple>::type::print(oss, f);
    Tomographer::Tools::SimpleWordWrapper wrapper(
        80, // width
        "- \""+std::string( typename std::tuple_element<I, FiguresOfMeritTuple>::type().name )+"\":  ", // init
        6, // indent
        4 // first_indent
        );
    stream << wrapper.wrapped(oss.str()) << "\n";
    print<I+1>(stream, f);
  }
  template<int I=0, typename std::enable_if<(I==std::tuple_size<FiguresOfMeritTuple>::value),bool>::type = true>
  static inline void print(std::ostream &, Tomographer::Tools::FmtFootnotes & ) {
  }
};

template<typename ValueType, typename... ValueCalculators>
Tomographer::MultiplexorValueCalculator<ValueType, ValueCalculators...> * GMplxVCThelper(ValueCalculators* ... )
{
  return NULL;
}

template<typename ValueType, typename FiguresOfMeritTuple>
struct GetMultiplexorValueCalculatorType;
template<typename ValueType, typename... FiguresOfMeritTypes>
struct GetMultiplexorValueCalculatorType<ValueType, std::tuple<FiguresOfMeritTypes...> >
{
  template<typename DMTypes>
  using type =
    typename std::remove_reference<
      decltype(*GMplxVCThelper<ValueType>( (typename FiguresOfMeritTypes::template ValueCalculator<DMTypes>*)NULL... ))
    >::type;
};

template<typename DMTypes>
using TomorunMultiplexorValueCalculatorType =
  GetMultiplexorValueCalculatorType<TomorunReal, TomorunFiguresOfMerit>::type<DMTypes>;

template<typename DMTypes, typename FiguresOfMeritTuple>
struct TrMplxVCInit;
template<typename DMTypes, typename... FiguresOfMeritTypes>
struct TrMplxVCInit<DMTypes, std::tuple<FiguresOfMeritTypes...> >
{
  using FiguresOfMeritTuple = std::tuple<FiguresOfMeritTypes...>;

  template<int N>
  struct InitVC {
    InitVC(DMTypes dmt_, const std::string & ref_obj_name_, Tomographer::MAT::File * matf_)
      : dmt(dmt_), ref_obj_name(ref_obj_name_), matf(matf_)
    {
    }
    DMTypes dmt;
    const std::string & ref_obj_name;
    Tomographer::MAT::File * matf;

    inline typename std::tuple_element<N,FiguresOfMeritTuple>::type::template ValueCalculator<DMTypes> *
    operator()() const
    {
      return std::tuple_element<N,FiguresOfMeritTuple>::type::createValueCalculator(dmt, ref_obj_name, matf);
    }
  };

  template<int... N>
  static inline TomorunMultiplexorValueCalculatorType<DMTypes>
  mk(int i, DMTypes dmt, const std::string & ref_obj_name,
     Tomographer::MAT::File * matf, Tomographer::Tools::Sequence<int,N...>)
  {
    return TomorunMultiplexorValueCalculatorType<DMTypes>(
        i,
        InitVC<N>(dmt, ref_obj_name, matf) ...
        ) ;
  }
  
};

template<typename DMTypes>
inline TomorunMultiplexorValueCalculatorType<DMTypes>
makeTomorunMultiplexorValueCalculatorType(int i, DMTypes dmt, const std::string & ref_obj_name,
                                          Tomographer::MAT::File * matf)
{
  return TrMplxVCInit<DMTypes, TomorunFiguresOfMerit>::mk(
      i, dmt, ref_obj_name, matf,
      Tomographer::Tools::GenerateSequence<int,std::tuple_size<TomorunFiguresOfMerit>::value>::type()
      );
};


// Store a figure of merit
struct figure_of_merit_spec
{
  figure_of_merit_spec(const std::string & str)
    : valtype(-1),
      ref_obj_name()
  {
    set_value_string(str);
  }

  int valtype;
  std::string ref_obj_name;

  inline void set_value_string(const std::string & str)
  {
    std::size_t k = str.find(':');
    std::string valtype_str;
    std::string ref_obj_name_str;
    if (k == std::string::npos) {
      // not found
      valtype_str = str;
    } else {
      valtype_str = str.substr(0, k);
      ref_obj_name_str = str.substr(k+1); // the rest of the string
    }

    valtype = -1;
    for (std::size_t j = 0; j < std::tuple_size<TomorunFiguresOfMerit>::value; ++j) {
      if (valtype_str == TupleDynGet<GetStrNameField,TomorunFiguresOfMerit>::get(j)) {
        valtype = (int)j;
        ref_obj_name = ref_obj_name_str;
      }
    }
    if (valtype == -1) {
      throw std::invalid_argument(std::string("Invalid argument to figure_of_merit_spec: '") + str + std::string("'"));
    }
  }
};

inline std::ostream & operator<<(std::ostream & stream, const figure_of_merit_spec & val)
{
  if (val.valtype < 0 || val.valtype >= (int)std::tuple_size<TomorunFiguresOfMerit>::value) {
    stream << "<invalid figure of merit>";
  }
  stream << TupleDynGet<GetStrNameField,TomorunFiguresOfMerit>::get((std::size_t)val.valtype);

  if (val.ref_obj_name.size()) {
    stream << ":" << val.ref_obj_name;
  }

  return stream;
}




#endif
