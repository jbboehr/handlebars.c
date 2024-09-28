/**
 * Copyright (c) anno Domini nostri Jesu Christi MMXVI-MMXXIV John Boehr & contributors
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file
 * @brief Scanners
 */

#ifndef HANDLEBARS_SCANNERS_H
#define HANDLEBARS_SCANNERS_H

#include "handlebars.h"

HBS_EXTERN_C_START

/**
 * @brief Implement the following regexes, def returned if end hit:
 *        /^\\s*?\\r?\\n/
 *        /^\\s*?(\\r?\\n|$)/
 *
 * @param[in] s The buffer to match
 * @param[in] def The default return value
 * @return zero if not match, otherwise non-zero
 */
bool handlebars_scanner_next_whitespace(
    const char * s,
    bool def
) HBS_TEST_PUBLIC HBS_ATTR_NONNULL_ALL HBS_ATTR_PURE;

/**
 * @brief Implement the following regexes, def returned if end hit:
 *        /\\r?\\n\\s*?$/
 *        /(^|\\r?\\n)\\s*?$/
 *
 * @param[in] s The buffer to match
 * @param[in] def The default return value
 * @return zero if not match, otherwise non-zero
 */
bool handlebars_scanner_prev_whitespace(
    const char * s,
    bool def
) HBS_TEST_PUBLIC HBS_ATTR_NONNULL_ALL HBS_ATTR_PURE;

HBS_EXTERN_C_END

#endif /* HANDLEBARS_SCANNERS_H */
