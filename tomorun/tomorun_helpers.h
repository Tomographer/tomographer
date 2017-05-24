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

#include <tomographer/tools/cxxutil.h>

#ifndef TOMORUN_HELPERS
#define TOMORUN_HELPERS


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
