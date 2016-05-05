/**
 * Copyright (C) 2016 John Boehr
 *
 * This file is part of handlebars.c.
 *
 * handlebars.c is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * handlebars.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with handlebars.c.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file
 * @brief Scanners
 */

#ifndef HANDLEBARS_SCANNERS_H
#define HANDLEBARS_SCANNERS_H

#include "handlebars.h"

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * @brief Implement the following regexes, def returned if end hit:
 *        /^\\s*?\\r?\\n/
 *        /^\\s*?(\\r?\\n|$)/
 * 
 * @param[in] s The buffer to match
 * @param[in] def The default return value
 * @return zero if not match, otherwise non-zero
 */
bool handlebars_scanner_next_whitespace(const char * s, bool def) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Implement the following regexes, def returned if end hit:
 *        /\\r?\\n\\s*?$/
 *        /(^|\\r?\\n)\\s*?$/
 * 
 * @param[in] s The buffer to match
 * @param[in] def The default return value
 * @return zero if not match, otherwise non-zero
 */
bool handlebars_scanner_prev_whitespace(const char * s, bool def) HBS_ATTR_NONNULL_ALL;

#ifdef	__cplusplus
}
#endif

#endif
