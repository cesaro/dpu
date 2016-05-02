
#include <stdio.h>
#include <inttypes.h>

#include "fe3/rt.h"

void dpu_rt_load8 (uint8_t *addr, uint8_t v)
{
   printf ("ld8  %16p %20"PRIu8"\n", addr, v);
}
void dpu_rt_load16 (uint16_t *addr, uint16_t v)
{
   printf ("ld16 %16p %20"PRIu16"\n", addr, v);
}
void dpu_rt_load32 (uint32_t *addr, uint32_t v)
{
   printf ("ld32 %16p %20"PRIu32"\n", addr, v);
}
void dpu_rt_load64 (uint64_t *addr, uint64_t v)
{
   printf ("ld64 %16p %20"PRIu64"\n", addr, v);
}

void dpu_rt_store8 (uint8_t *addr, uint8_t v)
{
   printf ("st8  %16p %20"PRIu8"\n", addr, v);
}
void dpu_rt_store16 (uint16_t *addr, uint16_t v)
{
   printf ("st16 %16p %20"PRIu16"\n", addr, v);
}
void dpu_rt_store32 (uint32_t *addr, uint32_t v)
{
   printf ("st32 %16p %20"PRIu32"\n", addr, v);
}
void dpu_rt_store64 (uint64_t *addr, uint64_t v)
{
   printf ("st64 %16p %20"PRIu64"\n", addr, v);
}

