
#ifndef HANDLEBARS_PRIVATE_H
#define HANDLEBARS_PRIVATE_H

#include "handlebars.h"

#ifdef	__cplusplus
extern "C" {
#endif

struct handlebars_context;

#define likely handlebars_likely
#define unlikely handlebars_unlikely

#define CONTEXT context
#define MEMCHK_MSG HANDLEBARS_MEMCHECK_MSG
#define MEMCHKEX(cond, ctx) HANDLEBARS_MEMCHECK(cond, ctx)
#define MEMCHK(cond) MEMCHKEX(cond, CONTEXT)
#define MEMCHKF(ptr) (HBS_TYPEOF(ptr)) handlebars_check(CONTEXT, (void *) (ptr), MEMCHK_MSG)
#define MC(ptr) MEMCHKF(ptr)

#define YY_NO_UNISTD_H 1
#define YYLTYPE handlebars_locinfo

#ifdef	__cplusplus
}
#endif

#endif
