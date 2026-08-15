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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <string.h>
#include <time.h>

#define HANDLEBARS_COMPILER_PRIVATE
#define HANDLEBARS_OPCODES_PRIVATE
#define HANDLEBARS_OPCODE_SERIALIZER_PRIVATE

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"

#include "handlebars_compiler.h"
#include "handlebars_module_printer.h"
#include "handlebars_opcodes.h"
#include "handlebars_opcode_printer.h"
#include "handlebars_opcode_serializer.h"
#include "handlebars_string.h"



struct handlebars_string * handlebars_module_print(
    struct handlebars_context * ctx,
    struct handlebars_module * module
) {
    struct handlebars_string * buffer = handlebars_string_init(ctx, 1024);
    const char * timestamp = ctime(&module->ts);
    if( timestamp == NULL ) {
        timestamp = "(invalid)\n";
    }
    buffer = handlebars_string_asprintf_append(
        ctx,
        buffer,
        "HASH: %llu\n"
        "VERSION: %d\n"
        "ADDR: %p\n"
        "SIZE: %zu\n"
        "DATA OFFSET: %zu\n"
        "TS: %s" // "\n"
        "FLAGS: %lu\n"
        "PROGRAMS: %zu\n"
        "OPCODES: %zu\n"
        "\n",
        (long long unsigned) module->hash,
        module->version,
        module->addr,
        module->size,
        module->data_offset,
        timestamp,
        module->flags,
        module->program_count,
        module->opcode_count
    );
    for ( size_t i = 0; i < module->program_count; i++ ) {
        buffer = handlebars_string_asprintf_append(
            ctx,
            buffer,
            "PROGRAM: %zu\n"
            "OPCODE_COUNT: %zu\n"
            "OPCODE_OFFSET: %zu\n"
            "\n",
            module->programs[i].guid,
            module->programs[i].opcode_count,
            module->programs[i].opcode_offset
        );
    }
    for( size_t i = 0; i < module->opcode_count; ) {
        struct handlebars_module_table_entry * entry = NULL;

        for( size_t j = 0; j < module->program_count; j++ ) {
            if( module->programs[j].opcode_offset == i ) {
                entry = &module->programs[j];
                break;
            }
        }

        assert(entry != NULL);
        if( entry == NULL
                || entry->opcode_count == 0
                || entry->opcode_count > module->opcode_count - i ) {
            break;
        }

        buffer = handlebars_string_asprintf_append(
            ctx,
            buffer,
            "%sPROGRAM: %zu\n",
            i == 0 ? "" : "\n",
            entry->guid
        );
        for( size_t local_opcode_id = 0;
                local_opcode_id < entry->opcode_count;
                local_opcode_id++, i++ ) {
            buffer = handlebars_string_asprintf_append(ctx, buffer, "OP[%03zu,%03zu]: ", local_opcode_id, i);
            buffer = handlebars_opcode_print_append(ctx, buffer, &module->opcodes[i], 0);
            buffer = handlebars_string_asprintf_append(ctx, buffer, "\n");
        }
    }
    return buffer;
}
