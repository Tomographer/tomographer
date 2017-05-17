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

#ifndef TOMOGRAPHER_TOOLS_STATUSPROVIDER_H
#define TOMOGRAPHER_TOOLS_STATUSPROVIDER_H


#include <tomographer/tools/cxxutil.h>


namespace Tomographer {
namespace Tools {




/** \brief Template, specializable class to provide status reports for different objects
 *
 * Specialize this class for your stats collector to be able to provide a short status
 * report. Just provide 2-3 lines with the most important information enough to provide a
 * very basic overview, not a full-length report.
 *
 * This is used for stats collectors and for mhwalker params adjusters.
 */
template<typename StatusableObject_>
struct TOMOGRAPHER_EXPORT StatusProvider
{
  typedef StatusableObject_ StatusableObject;

  static constexpr bool CanProvideStatusLine = false;

  /** \brief Prepare a short status message which reports the status of the given object
   *
   * Don't end your string with a newline.
   *
   * The message should be one line, or few lines at most.  It should be suited for
   * printing in a terminal in a brief status report.
   */
  static inline std::string getStatusLine(const StatusableObject * /*obj*/)
  {
    return std::string();
  }
};



namespace tomo_internal {
template<typename StObj, typename canprovideline = void>
struct status_can_provide_line {
  static constexpr bool val = false;
};
template<typename StObj>
struct status_can_provide_line<StObj,
                               typename std::enable_if<StatusProvider<StObj>::CanProvideStatusLine,void>::type> {
  static constexpr bool val = true;
};
template<typename StObj, typename canprovidefullmessage = void>
struct status_can_provide_fullmessage {
  static constexpr bool val = false;
};
template<typename StObj>
struct status_can_provide_fullmessage<StObj,
                                      typename std::enable_if<StatusProvider<StObj>::CanProvideStatusFullMessage,void>::type> {
  static constexpr bool val = true;
};
}



/** \brief Query status from different objects
 *
 * These objects may, or may not, provide different status options by specializing \ref
 * StatusProvider.
 *
 */
template<typename StatusableObject_>
struct TOMOGRAPHER_EXPORT StatusQuery
{
  typedef StatusableObject_ StatusableObject;

  static constexpr bool CanProvideStatusLine = tomo_internal::status_can_provide_line<StatusableObject>::val;

  TOMOGRAPHER_ENABLED_IF(CanProvideStatusLine)
  static inline std::string getStatusLine(const StatusableObject * obj)
  {
    return StatusProvider<StatusableObject>::getStatusLine(obj);
  }
  TOMOGRAPHER_ENABLED_IF(!CanProvideStatusLine)
  static inline std::string getStatusLine(const StatusableObject * ) { return std::string(); }

  // other status stuff we could imagine providing in the future

  static constexpr bool CanProvideStatusFullMessage = tomo_internal::status_can_provide_fullmessage<StatusableObject>::val;

  TOMOGRAPHER_ENABLED_IF(CanProvideStatusFullMessage)
  static inline std::string getStatusFullMessage(const StatusableObject * obj)
  {
    return StatusProvider<StatusableObject>::getStatusFullMessage(obj);
  }
  TOMOGRAPHER_ENABLED_IF(!CanProvideStatusFullMessage)
  static inline std::string getStatusFullMessage(const StatusableObject * ) { return std::string(); }

  
};
// static members:
template<typename StatusableObject_>
constexpr bool StatusQuery<StatusableObject_>::CanProvideStatusLine;
template<typename StatusableObject_>
constexpr bool StatusQuery<StatusableObject_>::CanProvideStatusFullMessage;






} // namespace Tools
} // namespace Tomographer





#endif
