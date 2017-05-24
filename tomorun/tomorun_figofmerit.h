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
#include <tomographer/tools/fmt.h>
#include <tomographer/valuecalculator.h>
#include <tomographer/densedm/tspacefigofmerit.h>


// =============================================================================
// Some helpers for the figures of merit: Read the reference state, etc.
// =============================================================================

// These functions are helpers used by several figures of merit, and they shouldn't need
// to be modified. You are welcome to use them to construct your figure of merit below.
// You can also add your helpers here if you think they might be useful for yet other
// figures of merit.


/** read_ref_state_T -- reads a density matrix from the MATLAB file and returns
 *                      it in T-parameterization
 *
 * From the MATLAB file pointed to by matf, reads the variable with the name
 * given by ref_obj_name (or "rho_ref" if ref_obj_name is empty), which is
 * expected to be a square complex matrix which is positive semidefinite.  Its
 * matrix square root is calculated and returned.
 */
template<typename DMTypes>
inline typename DMTypes::MatrixType  read_ref_state_T(Tomographer::MAT::File * matf, std::string ref_obj_name)
{
  using MatrixType = typename DMTypes::MatrixType;
  namespace M = Tomographer::MAT;
  
  // The reference state may be given explicitly as, e.g., "fidelity:rho_ref", but in case
  // it isn't provided we default to the name "rho_ref":
  if (!ref_obj_name.size()) {
    ref_obj_name = std::string("rho_ref");
  }

  // read this variable from the MATLAB data file, and extract a positive semidefinite matrix
  auto m_psd = matf->var(ref_obj_name).value< M::EigenPosSemidefMatrixWithSqrt<MatrixType> >() ;

  //auto rho_ref = std::move(m_psd.mat);
  auto T_ref = std::move(m_psd.sqrt);

  return T_ref;
}

/** read_ref_state_rho -- reads a density matrix from the MATLAB file and
 *                        returns it as is
 *
 * From the MATLAB file pointed to by matf, reads the variable with the name
 * given by ref_obj_name (or "rho_ref" if ref_obj_name is empty), which is
 * expected to be a square complex matrix which is positive semidefinite.  It is
 * then returned.
 */
template<typename DMTypes>
inline typename DMTypes::MatrixType  read_ref_state_rho(Tomographer::MAT::File * matf, std::string ref_obj_name)
{
  using MatrixType = typename DMTypes::MatrixType;
  namespace M = Tomographer::MAT;
  
  // The reference state may be given explicitly as, e.g., "tr-dist:rho_ref", but in case
  // it isn't provided we default to the name "rho_ref":
  if (!ref_obj_name.size()) {
    ref_obj_name = std::string("rho_ref");
  }

  // read this variable from the MATLAB data file, and extract a positive semidefinite matrix
  auto m_psd = matf->var(ref_obj_name).value< M::EigenPosSemidefMatrixWithSqrt<MatrixType> >() ;

  auto rho_ref = std::move(m_psd.mat);
  //auto T_ref = std::move(m_psd.sqrt);

  return rho_ref;
}

// =============================================================================






// =============================================================================
// DEFINE ALL THE AVAILABLE FIGURES OF MERIT HERE
// =============================================================================



// -----------------------------------------------------------------------------
// Figure of merit: (root) fidelity, cf. Nielsen & Chuang
// -----------------------------------------------------------------------------
struct FidelityFigureOfMerit
{
  // The name of the figure merit for the --value-type option: here, --value-type="fidelity"
  const std::string name{"fidelity"};

  // The ValueCalculator class we need to use, for the given DMTypes
  template<typename DMTypes> using ValueCalculator =
    Tomographer::DenseDM::TSpace::FidelityToRefCalculator<DMTypes,TomorunReal>;

  // The code which instanciates a new ValueCalculator with the appropriate input data
  template<typename DMTypes>
  static inline ValueCalculator<DMTypes> *
  createValueCalculator(DMTypes /*dmt*/, const std::string & ref_obj_name, Tomographer::MAT::File * matf)
  {
    // create the value calculator
    return new ValueCalculator<DMTypes>(read_ref_state_T<DMTypes>(matf, ref_obj_name)) ; 
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
      "the reference state. If no <RefObject> is specified, then 'rho_ref' is "
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
  // The name of the figure merit for the --value-type option: here, --value-type="purif-dist"
  const std::string name{"purif-dist"};

  // The ValueCalculator class we need to use, for the given DMTypes
  template<typename DMTypes> using ValueCalculator =
    Tomographer::DenseDM::TSpace::PurifDistToRefCalculator<DMTypes,TomorunReal>;

  // The code which instanciates a new ValueCalculator with the appropriate input data
  template<typename DMTypes>
  static inline ValueCalculator<DMTypes> *
  createValueCalculator(DMTypes /*dmt*/, const std::string & ref_obj_name, Tomographer::MAT::File * matf)
  {
    // create the value calculator
    return new ValueCalculator<DMTypes>(read_ref_state_T<DMTypes>(matf, ref_obj_name)) ; 
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
      "'rho_ref' is used."
      ;
  }
};

// -----------------------------------------------------------------------------
// Figure of merit: trace distance
// -----------------------------------------------------------------------------
struct TraceDistanceFigureOfMerit
{
  // The name of the figure merit for the --value-type option: here, --value-type="tr-dist"
  const std::string name{"tr-dist"};

  // The ValueCalculator class we need to use, for the given DMTypes
  template<typename DMTypes> using ValueCalculator =
    Tomographer::DenseDM::TSpace::TrDistToRefCalculator<DMTypes,TomorunReal>;

  // The code which instanciates a new ValueCalculator with the appropriate input data
  template<typename DMTypes>
  static inline ValueCalculator<DMTypes> *
  createValueCalculator(DMTypes /*dmt*/, const std::string & ref_obj_name, Tomographer::MAT::File * matf)
  {
    // create the value calculator
    return new ValueCalculator<DMTypes>(read_ref_state_rho<DMTypes>(matf, ref_obj_name)) ; 
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
      "reference state. If no <RefObject> is specified, then 'rho_ref' is used."
      ;
  }
};

// -----------------------------------------------------------------------------
// Figure of merit: expectation value of an observable
// -----------------------------------------------------------------------------
struct ObsValueFigureOfMerit
{
  // The name of the figure merit for the --value-type option: here, --value-type="obs-value"
  const std::string name{"obs-value"};

  // The ValueCalculator class we need to use, for the given DMTypes
  template<typename DMTypes> using ValueCalculator =
    Tomographer::DenseDM::TSpace::ObservableValueCalculator<DMTypes>;

  // The code which instanciates a new ValueCalculator with the appropriate input data
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

  // Print some help text to the screen when queried with --help. We can insert footnotes
  // using footnotes.addFootNote(...)
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

// struct MyNiceCustomFigureOfMerit
// {
//   // The name of the figure merit for the --value-type option: here, --value-type="obs-value"
//   const std::string name{"nice-custom-figure-of-merit"};
//
//   // The ValueCalculator class we need to use, for the given DMTypes
//   template<typename DMTypes> using ValueCalculator =
//     XXXXXXXXXXXXXXXXXXXXXXXValueCalculator<DMTypes>;
//
//   // The code which instanciates a new ValueCalculator with the appropriate input data
//   template<typename DMTypes>
//   static inline ValueCalculator<DMTypes> *
//   createValueCalculator(DMTypes dmt, std::string ref_obj_name, Tomographer::MAT::File * matf)
//   {
//     using MatrixType = typename DMTypes::MatrixType;
//     namespace M = Tomographer::MAT;
//
//     ..... read data from the MATLAB file, if necessary .....
//
//     // ... and create the value calculator, with appropriate arguments if necessary
//     return new ValueCalculator<DMTypes>(dmt, ...) ; 
//   }
//
//   // Print some help text to the screen when queried with --help. We can insert footnotes
//   // using footnotes.addFootNote(...)
//   static void print(std::ostream & stream, Tomographer::Tools::FmtFootnotes & footnotes) {
//     stream <<
//       "Calculate the hypercomplex-geometric-tensorial reduced affinity of the quantum state, using the "
//       "conventions of Ref. " << footnotes.addFootNote(
//             "Meisfurst et al., Journal of Irrelevant Complicated Stuff 123:12, p.9331 (2017)"
//             ) << ".";
//       ;
//   }
// };






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
  PurifiedDistanceFigureOfMerit //,
  // INSERT CUSTOM FIGURE OF MERIT HERE -- add to this list
  //MyNiceCustomFigureOfMerit
  >;





#endif
