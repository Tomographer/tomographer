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



/** \page pageDenseDMTypeInterfaces DenseDM Type Interfaces
 *
 * The following type interfaces (see \ref pageTypeInterfaces) are meant in the context of
 * DenseDM stuff, where the quantum state is represented as a dense object (all
 * coefficients individually stored as a dense matrix or array, see Eigen's dense
 * objects).  The quantum state is typically stored either as the density matrix, or one
 * of the parameterizations \ref pageParamsX or \ref pageParamsT.
 *
 * Type interfaces:
 *   - \subpage pageInterfaceDenseLLH
 *
 */



// =============================================================================
// DenseLLH
// =============================================================================

/** \page pageInterfaceDenseLLH DenseLLH Interface
 *
 * <em>This is a &lsquo;type interface.&rsquo; See \ref pageTypeInterfaces
 * for more info on what that is.</em>
 *
 * A \a DenseLLH compliant type is one which is capable of calculating the loglikelihood
 * function for a particular realization of a quantum tomography experiment.
 *
 * The log-likelihood function is defined as the logarithm of the likelihood function:
 * \f[
 *    \texttt{llh}(\rho) = \ln \mathrm{tr}\left(B^n \rho^{\otimes n}\right)\ ,
 * \f]
 * where \f$ B^n \f$ is the joint POVM effect observed on the \f$ n \f$ systems (in the
 * most general scenario of [Christandl & Renner, PRL (2012)]), and where \f$ \rho \f$ is
 * the quantum state at which to evaluate the log-likelihood function.
 *
 * \note Here, the log-likelihood function is defined WITHOUT any \f$ -2 \f$ factor which
 *       is sometimes conventionally implied.
 *
 * Currently, the only implementation is \ref Tomographer::DenseDM::IndepMeasLLH, which
 * stores the individual POVM effects along with frequencies, while assuming that the
 * global observed POVM effect (in the general scenario) can be written as a product of
 * effects (though this does not imply that the POVM itself is a product POVM).
 *
 * A \a DenseLLH compliant type should expose the following members:
 *
 * \par typedef ... DMTypes
 *   The \ref Tomographer::DenseDM::DMTypes DMTypes type to use to store quantum states
 *   and POVM effects as dense objects.
 * 
 * \par typedef ... LLHValueType
 *   The type used to store the value of the loglikelihood function. Typically the boring
 *   old \a double is suitable.
 *
 * \par const DMTypes dmt;
 *   A public member which is an instance of the corresponding \a DMTypes, which can be
 *   used to construct dense objects to store quantum states and POVM effects.
 *
 * \par enum { LLHCalcType = ... }
 *   Specifies how this object can calculate the loglikelihood function.  The value must
 *   be one of \ref Tomographer::DenseDM::LLHCalcTypeX "LLHCalcTypeX" or \ref
 *   Tomographer::DenseDM::LLHCalcTypeRho "LLHCalcTypeRho".  (In the future, we may add
 *   more values to this enum to support further parameterizations.)
 *
 * \par LLHValueType logLikelihoodX(VectorParamTypeConstRef x)
 *   <em>(Required only if <code>LLHCalcType = LLHCalcTypeX</code>)</em> Calculate the
 *   value of the loglikelihood function for the point \a x, given in \ref pageParamsX.
 *   The argument type \a VectorParamTypeConstRef matches the one declared in \a DMTypes.
 *
 * \par LLHValueType logLikelihoodRho(MatrixTypeConstRef rho)
 *   <em>(Required only if <code>LLHCalcType = LLHCalcTypeRho</code>)</em> Calculate the
 *   value of the loglikelihood function for the point \a rho, given as a density matrix.
 *   The argument type \a MatrixTypeConstRef matches the one declared in \a DMTypes.
 *
 */

