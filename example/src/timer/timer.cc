#include "timer.h"
#include <sys/time.h>

// retorna a função em milisegundos
tTime now(void) {
   struct timeval tv;
   if(gettimeofday(&tv, (__timezone_ptr_t)NULL) != 0) return 0;
   return (tTime)((tv.tv_sec * 1000ul) + (tv.tv_usec / 1000ul));
}
