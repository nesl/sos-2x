#include <priority.h>
#include <module.h>

pri_t get_module_priority(sos_pid_t id) 
{
  sos_module_t *module = ker_get_module(id);
  if (module != NULL) {
    return module->priority;
  }
  else {
    return 0;
  }
}

void set_module_priority(sos_pid_t id, pri_t priority)
{
  sos_module_t *module = ker_get_module(id);
  if (module != NULL) {
    module->priority = priority;
  }
}
