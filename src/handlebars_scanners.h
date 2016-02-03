
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
short handlebars_scanner_next_whitespace(const char * s, short def);

/**
 * @brief Implement the following regexes, def returned if end hit:
 *        /\\r?\\n\\s*?$/
 *        /(^|\\r?\\n)\\s*?$/
 * 
 * @param[in] s The buffer to match
 * @param[in] def The default return value
 * @return zero if not match, otherwise non-zero
 */
short handlebars_scanner_prev_whitespace(const char * s, short def);

#ifdef	__cplusplus
}
#endif

#endif
