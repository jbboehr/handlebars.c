
#include "handlebars_compiler.h"
#include "handlebars_memory.h"


struct handlebars_compiler * handlebars_compiler_ctor(void)
{
    struct handlebars_compiler * compiler;
    
    compiler = handlebars_talloc_zero(NULL, struct handlebars_compiler);
    
    return compiler;
};

void handlebars_compiler_dtor(struct handlebars_compiler * compiler)
{
    handlebars_talloc_free(compiler);
};

int handlebars_compiler_get_flags(struct handlebars_compiler * compiler)
{
    return compiler->flags;
}

void handlebars_compiler_set_flags(struct handlebars_compiler * compiler, int flags)
{
    // Only allow option flags to be changed
    flags = flags & handlebars_compiler_flag_all;
    compiler->flags = compiler->flags & ~handlebars_compiler_flag_all;
    compiler->flags = compiler->flags | flags;
    
    // Update shortcuts
    compiler->string_params = 1 && (compiler->flags & handlebars_compiler_flag_string_params);
    compiler->track_ids = 1 && (compiler->flags & handlebars_compiler_flag_track_ids);
    compiler->use_depths = 1 && (compiler->flags & handlebars_compiler_flag_use_depths);
}

