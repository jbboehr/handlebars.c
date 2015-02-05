
#ifndef HANDLEBARS_TOKEN_H
#define HANDLEBARS_TOKEN_H

#include <stddef.h>

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * @brief Token structure 
 */
struct handlebars_token {
    int token;
    size_t length;
    char * text;
};

/**
 * @brief Construct a token. Returns NULL on failure.
 * 
 * @param[in] token_int Token type
 * @param[in] text Token text
 * @param[in] length Length of the text
 * @param[in] ctx Talloc memory context
 * @return the new token object
 */
struct handlebars_token * handlebars_token_ctor(int token_int, const char * text, size_t length, void * ctx);

/**
 * @brief Destruct a token
 * 
 * @param[in] token Token type
 * @return void
 */
void handlebars_token_dtor(struct handlebars_token * token);

/**
 * @brief Get the token type
 * 
 * @param[in] token Token
 * @return void
 */
int handlebars_token_get_type(struct handlebars_token * token);

/**
 * @brief Get the token text.
 * 
 * @param[in] token Token
 * @return The token text
 */
const char * handlebars_token_get_text(struct handlebars_token * token);

/**
 * @brief Get the token text
 * 
 * @param[in] token Token
 * @param[out] text The token text
 * @param[out] length The text length
 * @return void
 */
void handlebars_token_get_text_ex(struct handlebars_token * token, const char ** text, size_t * length);

/**
 * @brief Get a string for the integral token type
 * 
 * @param[in] type The integral token type
 * @return The string name of the type
 */
const char * handlebars_token_readable_type(int type);

/**
 * @brief Get an integral type for the token name
 * 
 * @param[in] type The token type name
 * @return The integral token type
 */
int handlebars_token_reverse_readable_type(const char * type);

#ifdef	__cplusplus
}
#endif

#endif
