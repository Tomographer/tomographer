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


#ifndef TOMOPY_COMMON_P_H
#define TOMOPY_COMMON_P_H

#include <memory>

#include <tomographerpy/common.h>
#include <tomographerpy/exc.h> // TomographerCxxError
#include <tomographerpy/pylogger.h>


namespace tpy
{

// Python/C++ logger
//
// The logger object instance is defined in tomographerpy.cxx
extern PyLogger * logger;

// the main exception object
extern py::object TomographerCxxErrorObj;

}




using namespace pybind11::literals; // _a for arg



// http://stackoverflow.com/a/34930421/1694896
namespace tpy { namespace internal {

template <std::size_t... I> class index_sequence {};
template <std::size_t N, std::size_t ...I> struct make_index_sequence : make_index_sequence<N-1, N-1,I...> {};
template <std::size_t ...I> struct make_index_sequence<0,I...> : index_sequence<I...> {};

template<typename Kl, typename ArgPack, std::size_t... I>
void inplaceconstruct_from_tuple_args(Kl & p, py::tuple t, index_sequence<I...>)
{
  new (&p) Kl( t[I].cast<typename std::tuple_element<I, ArgPack>::type>()... ) ;
}

template<typename Kl, typename... Args>
void unpack_tuple_and_construct(Kl & p, py::tuple t)
{
  if (t.size() != sizeof...(Args)) {
    throw TomographerCxxError(streamstr("Invalid pickle state: expected "<<(sizeof...(Args))<<", got "
                                        <<t.size()));
  }
  // Invoke the in-place constructor. Note that this is needed even when the object just
  // has a trivial default constructor
  inplaceconstruct_from_tuple_args<Kl, std::tuple<Args...> >(p, t, make_index_sequence<sizeof...(Args)>()) ;
}

} // internal
} // tpy


#endif
