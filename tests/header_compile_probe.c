/**
 * Copyright (c) anno Domini nostri Jesu Christi MMXVI-MMXXIV John Boehr & contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

int handlebars_header_compile_probe;

#ifdef HANDLEBARS_VM_H
void handlebars_vm_cache_clear_compile_probe(struct handlebars_vm * vm)
{
    handlebars_vm_set_cache(vm, NULL);
}
#endif

#ifdef HANDLEBARS_VALUE_H
void handlebars_value_convert_compile_probe(
    struct handlebars_value * value,
    int condition
)
{
    if( condition )
        handlebars_value_convert(value);
    else
        handlebars_value_null(value);
}

struct handlebars_value * handlebars_value_convert_expression_compile_probe(
    struct handlebars_value * value
)
{
    return (handlebars_value_convert(value), value);
}
#endif
