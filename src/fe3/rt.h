
#ifndef _FE3_RT_H_
#define _FE3_RT_H_

void dpu_rt_load8  (uint8_t  *addr, uint8_t  v);
void dpu_rt_load16 (uint16_t *addr, uint16_t v);
void dpu_rt_load32 (uint32_t *addr, uint32_t v);
void dpu_rt_load64 (uint64_t *addr, uint64_t v);

void dpu_rt_store8  (uint8_t  *addr, uint8_t  v);
void dpu_rt_store16 (uint16_t *addr, uint16_t v);
void dpu_rt_store32 (uint32_t *addr, uint32_t v);
void dpu_rt_store64 (uint64_t *addr, uint64_t v);

#endif
