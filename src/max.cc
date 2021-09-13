#include "max.h"
#include "conky.h"
#include "logging.h"
#include "core.h"
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_MAX_STR_LEN 128

void print_max(struct text_object *obj, char *p, unsigned int p_max_size) {
  std::vector<char> buf(p_max_size);
  generate_text_internal(&(buf[0]), p_max_size, *obj->sub);

  char* stored_string = (char*)(obj->data.opaque);

  double reading;
  sscanf(&(buf[0]), "%f", &reading);

  if(!strlen(stored_string))
    strncpy(stored_string, &(buf[0]), MAX_MAX_STR_LEN);
  
  double old_reading;
  sscanf(stored_string, "%f", &old_reading);

  if(reading > old_reading)
    strncpy(stored_string, &(buf[0]), MAX_MAX_STR_LEN);

  snprintf(p, p_max_size, "%s", stored_string);
}

void max_parse_arg(struct text_object * obj, const char * arg){
  obj->sub = static_cast<text_object *>(malloc(sizeof(struct text_object)));
  extract_variable_text_internal(obj->sub, arg);

  obj->data.opaque = malloc(sizeof(char)*MAX_MAX_STR_LEN);
  ((char*)(obj->data.opaque))[0] = '\0';
}

void max_free_obj_info(struct text_object *obj) {
  free_and_zero(obj->data.opaque);
}
