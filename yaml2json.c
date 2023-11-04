/*
 * Example libyaml parser.
 *
 * This is a simple libyaml parser example which scans and prints
 * the libyaml parser events.
 *
 */
#include <yaml.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h>
#include <assert.h>

#define MAX_DEPTH 128

#ifdef DEBUG
#define INDENT "  "
#define STRVAL(x) ((x) ? (char*)(x) : "")
static void indent(int level)
{
    for (int i = 0; i < level; i++) fprintf(stderr, "%s", INDENT);
}
#define print_debug(level, template, event_type) { indent(level); fprintf(stderr, template, event_type); }
#else
#define print_debug(level, template, event_type)
#endif

json_t *yaml_to_json(yaml_parser_t *parser) {
#ifdef DEBUG
    int level = 0;
#endif

    yaml_event_t event;
    yaml_event_type_t event_type;

    json_t *json[MAX_DEPTH];
    *json = NULL;
    int top = 0;

    json_t *current_key = NULL, *value = NULL;

    do {
        if (!yaml_parser_parse(parser, &event)) goto error;
        event_type = event.type;
        
        switch (event_type) {
        case YAML_NO_EVENT: print_debug(level, "no-event (%d)\n", event_type); break;
        case YAML_STREAM_START_EVENT: print_debug(level++, "stream-start-event (%d)\n", event_type); break;
        case YAML_STREAM_END_EVENT: print_debug(--level, "stream-end-event (%d)\n", event_type); break;
        case YAML_DOCUMENT_START_EVENT: print_debug(++level, "document-start-event (%d)\n", event_type); break;
        case YAML_DOCUMENT_END_EVENT: print_debug(--level, "document-end-event (%d)\n", event_type); break;
        case YAML_ALIAS_EVENT: print_debug(level, "alias-event (%d)\n", event_type); break;
        case YAML_SCALAR_EVENT:
            print_debug(level, "scalar-event (%d)", event_type);
#ifdef DEBUG
            fprintf(stderr, " = {value=\"%s\", length=%d}\n", STRVAL(event.data.scalar.value), (int)event.data.scalar.length);
#endif
            value = json_string((char *)event.data.scalar.value);
            if (top > 0) {
                json_t *last = json[top-1];
                if (json_is_object(last) && !current_key) {
                    current_key = value;
                } else {
                    if (json_is_array(last)) {
                        json_array_append_new(last, value);
                    } else if (json_is_object(last)) {
                        json_object_set_new_nocheck(last, json_string_value(current_key), value);
                        json_decref(current_key);
                        current_key = NULL;
                    } else {
                        json[top++] = value;
                    }
                }
            } else { 
                json[top++] = value;
            }
            break;
        case YAML_SEQUENCE_START_EVENT: print_debug(level++, "sequence-start-event (%d)\n", event_type);
            value = json_array();
            if (top > 1) {
                json_t *last = json[top-1];
                if (json_is_array(last)) {
                    json_array_append_new(last, value);
                } else if (json_is_object(last)) {
                    json_object_set_new_nocheck(last, json_string_value(current_key), value);
                    json_decref(current_key);
                    current_key = NULL;
                }
            }
            json[top++] = value;
            break;
        case YAML_SEQUENCE_END_EVENT: print_debug(--level, "sequence-end-event (%d)\n", event_type); json[--top]; break;
        case YAML_MAPPING_START_EVENT: print_debug(level++, "mapping-start-event (%d)\n", event_type);
            value = json_object();
            if (top > 0) {
                json_t *last = json[top-1];
                if (json_is_array(last)) {
                    json_array_append_new(last, value);
                } else if (json_is_object(last)) {
                    json_object_set_new_nocheck(last, json_string_value(current_key), value);
                    json_decref(current_key);
                    current_key = NULL;
                }
            } else {
                current_key = NULL;
            }
            json[top++] = value;
            break;
        case YAML_MAPPING_END_EVENT: print_debug(--level, "mapping-end-event (%d)\n", event_type); json[--top]; break;
        }
#ifdef DEBUG
        if (level < 0) {
            fprintf(stderr, "indentation underflow!\n");
            level = 0;
        }
#endif        
        yaml_event_delete(&event);
    } while (event_type != YAML_STREAM_END_EVENT);
    
error:
    yaml_parser_delete(parser);
    return *json; 
}

int main(int argc, char *argv[])
{
    yaml_parser_t parser;
    FILE *input_file;
    json_t *result_json;
    char *output_json_str;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_yaml_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    input_file = fopen(argv[1], "r");
    if (!input_file) {
        perror("Failed to open the input file");
        return EXIT_FAILURE;
    }

    yaml_parser_initialize(&parser);
    yaml_parser_set_input_file(&parser, input_file);

    result_json = yaml_to_json(&parser);
    if (result_json) {
        output_json_str = json_dumps(result_json, JSON_INDENT(4) | JSON_ENCODE_ANY);
        if (output_json_str) {
            printf("%s\n", output_json_str);
            free(output_json_str);
        } else {
            fprintf(stderr, "Failed to dump json.");
        }
        json_decref(result_json);
    } else goto error;

    fclose(input_file);
    yaml_parser_delete(&parser);
    return EXIT_SUCCESS;

error:
    fprintf(stderr, "Failed to parse: %s\n", parser.problem);
    yaml_parser_delete(&parser);
    return EXIT_FAILURE;
}
