
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "handlebars.h"

const char * handlebars_version_string() {
    return HANDLEBARS_VERSION_STRING;
}

int handlebars_version() {
    return HANDLEBARS_VERSION_PATCH 
        + (HANDLEBARS_VERSION_MINOR * 100)
        + (HANDLEBARS_VERSION_MAJOR * 10000);
}

