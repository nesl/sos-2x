#include <priority.h>
#include <module.h>
#include <systime.h>

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

#ifdef USE_PREEMPTION_PROFILER
void preemption_profile(sos_pid_t id) 
{
  sos_module_t *module = ker_get_module(id);
  uint32_t end = ker_systime32();
  uint32_t total = end - profile_start;
  

  profile_start = end;
  module->average = ((module->average * module->num_runs) + total) / (module->num_runs + 1);
  module->num_runs++;
}
#endif
