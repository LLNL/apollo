#if defined(VERTEST_OMPT_TR7) || defined(VERTEST_OMPT_5_0)
#include "omp-tools.h"
#endif /* VERTEST_OMPT_TR7 || VERTEST_OMPT_5_0 */
#if defined(VERTEST_OMPT_TR6)
#include "ompt.h"
#endif /* VERTEST_OMPT_TR6 */
#include "omp.h"
#include "stdio.h"

#if !defined(VERTEST_OMPT_5_0) && !defined(VERTEST_OMPT_TR7) && !defined(VERTEST_OMPT_TR6)
#error "No OMPT Version was chosen"
#endif

void on_ompt_callback_mutex_acquire(
#if defined(VERTEST_OMPT_5_0) || defined(VERTEST_OMPT_TR7)
ompt_mutex_t kind,
#endif /* VERTEST_OMPT_5_0)|| VERTEST_OMPT_5_0 */
#if defined(VERTEST_OMPT_TR6)
ompt_mutex_kind_t kind,
#endif /* VERTEST_OMPT_TR6 */
unsigned int hint,
unsigned int impl,
#if defined(VERTEST_OMPT_5_0)
ompt_wait_id_t wait_id,
#endif /* VERTEST_OMPT_5_0 */
#if defined(VERTEST_OMPT_TR7) || defined(VERTEST_OMPT_TR6)
omp_wait_id_t wait_id,
#endif /* VERTEST_OMPT_TR7 || VERTEST_OMPT_TR6 */
const void *codeptr_ra)
{
   printf("Useless Callback\n");
}

int ompt_initialize(
ompt_function_lookup_t lookup,
#if defined(VERTEST_OMPT_5_0)
int initial_device_num,
#endif /* VERTEST_OMPT_5_0 */
ompt_data_t* tool_data)
{
   ompt_set_callback_t ompt_set_callback = (ompt_set_callback_t) lookup("ompt_set_callback");
   int ret = ompt_set_callback(ompt_callback_mutex_acquire, (ompt_callback_t)on_ompt_callback_mutex_acquire);
   return 1;
}

void ompt_finalize(ompt_data_t* tool_data)
{
   return;
}

/* This function should probably check the version returned by omp_version - although clang 8.0 is returning 201611... */
ompt_start_tool_result_t * ompt_start_tool(
unsigned int omp_version,
const char *runtime_version)
{
   static ompt_start_tool_result_t result;
   result.initialize = &ompt_initialize;
   result.finalize = &ompt_finalize;
   result.tool_data.value = 0L;
   result.tool_data.ptr = NULL;
   return &result;
}

int main (int argc, char * argv[])
{
   #if defined(VERTEST_OMPT_5_0)
   printf("Testing Version 5.0\n");
   #endif /* VERTEST_OMPT_5_0 */
   #if defined(VERTEST_OMPT_TR7)
   printf("Testing Version TR7\n");
   #endif /* VERTEST_OMPT_TR7 */
   #if defined(VERTEST_OMPT_TR6)
   printf("Testing Version TR6\n");
   #endif /* VERTEST_OMPT_TR6 */
   #pragma omp parallel
   {
       printf ("Hi from thread %d\n", omp_get_thread_num());
   }
}
