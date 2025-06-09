#include "../../include/utils/yaml_parser.h"
#include <yaml.h>

/*
This will pass in valid key-value pairs to the allocator for it to handle.

These key value pairs are just as is and don't take into account the YAML
hierarchy. This means the values passed to the allocated will be at the end of
the nesting. The allocator is expected to take into account this, and manage
where to pass the data.

Way that the allocator can handle the where the resource will go.
example_tile.yaml:
  grass:
    walkable: true
    swimmable: false
    breakable: false
    intractable: false
    visuals:
      asset_path: "assets/"

The dest struct may have a len counter to tell it where to put the data.
This handles the tile type i.e., "grass" or any other one.

Then the nested data inside "grass", like asset_path may not be treated
differently than non-nested properties like breakable.

With this knowledge an example allocator will look like:

void tile_allocator(void *dest, const char *key, const char *val) {
  TileProps *props = (TileProps *)dest;

  if (!strcmp(key, "walkable") && !strcmp(val, "true")) {
    props->walkable[props->len] = true;
  } else if (!strcmp(key, "swimmable") && !strcmp(val, "true")) {
    props->swimmable[props->len] = true;!
  } else if (!strcmp(key, "breakable") && !strcmp(val, "true")) {
    props->breakable[props->len] = true;
  } else if (!strcmp(key, "intractable") && !strcmp(val, "true")) {
    props->intractable[props->len] = true;
  } else if (!strcmp(key, "asset_path")) {
    strcpy(props->asset_path[props->len], val);
    props->len++; // Assuming that asset_path is always the last field.
  } else {
    return false;
  }

  return true;
}

Author Note: Since this game is designed using data oriented design, I, in my
above example imply data oriented design. I highly recommend to my future self
and others to not go into nesting yaml. A top level definition like "grass" is
enough and no other further nesting shall be needed.
You may add nesting to please the humans into thinking this stuff is organized,
but at the end of the day, the parser will just not consider it.
*/
StatusCode yaml_ParserParse(const char *yaml_file,
                            void (*allocator)(void *dest, String key,
                                              String val, String id),
                            void *dest) {
  assert(dest);
  FILE *fh = fopen(yaml_file, "rb");
  if (!fh) {
    fprintf(stderr, "Unable to open the file: %s\n", yaml_file);
    return FAILURE;
  }

  yaml_parser_t parser;
  yaml_event_t event;

  if (!yaml_parser_initialize(&parser)) {
    fprintf(stderr, "Failed to initialize parser to file %s\n", yaml_file);
    fclose(fh);
    return FAILURE;
  }

  yaml_parser_set_input_file(&parser, fh);

  bool expecting_value = false, in_sequence = false;
  String last_key = {0}, id = {0};

  while (true) {
    if (!yaml_parser_parse(&parser, &event)) {
      fprintf(stderr, "Parser error %d in file %s\n", parser.error, yaml_file);
      break;
    }

    if (event.type == YAML_STREAM_END_EVENT) {
      yaml_event_delete(&event);
      break;
    } else if (event.type == YAML_SEQUENCE_START_EVENT) {
      in_sequence = true;
    } else if (event.type == YAML_SEQUENCE_END_EVENT) {
      in_sequence = false;
      /*
      Now after the ending of sequence, we expect a key to be found not a
      values.
      */
      expecting_value = false;
    } else if (event.type == YAML_MAPPING_START_EVENT) {
      /*
      This accounts for nested yaml, only allowing for valid key-val pairs to
      be parsed.
      */
      expecting_value = false;
      /*
      As the defined yaml structure for this game, when mapping starts, the last
      key is always the ID.
      */
      snprintf(id, sizeof(id), "%s", last_key);
    } else if (event.type == YAML_MAPPING_END_EVENT) {
      /*
      This accounts for nested yaml, only allowing for valid key-val pairs to
      be parsed.
      */
      expecting_value = false;
    } else if (event.type == YAML_SCALAR_EVENT) {
      char *val = (char *)event.data.scalar.value;
      /*
      Current design decision to ignore if the allocator returns false. Because
      it can also mean that the required data it needs is currently not
      provided.
      */
      if (in_sequence) {
        /*
        No need to change any flags, continue providing new vals with same key
        and id.
        */
        allocator(dest, last_key, val, id);
      } else if (expecting_value) {
        allocator(dest, last_key, val, id);
        /*
        Normal value received, now next val should be a key
        */
        expecting_value = false;
      } else {
        /*
        If we are not expecting val, current val must be a key and we will
        expect value after it.
        */
        snprintf(last_key, sizeof(last_key), "%s", val);
        expecting_value = true;
      }
    }
    yaml_event_delete(&event);
  }

  yaml_parser_delete(&parser);
  fclose(fh);

  return SUCCESS;
}
