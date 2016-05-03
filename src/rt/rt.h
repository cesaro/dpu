
#ifndef _RT_RT_H_
#define _RT_RT_H_

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

// these two are in start.s; host should invoke rt_start
void dpu_rt_start (int argc, const char * const *argv, const char * const *env);
void dpu_rt_end (void);

// called from rt_start, will call main()
int dpu_rt_main (int argc, const char * const *argv, const char * const *env);

// the user's main function, epic :)
int main (int argc, char **argv, char **env);

// instrumentation for loads
void dpu_rt_load8  (uint8_t  *addr, uint8_t  v);
void dpu_rt_load16 (uint16_t *addr, uint16_t v);
void dpu_rt_load32 (uint32_t *addr, uint32_t v);
void dpu_rt_load64 (uint64_t *addr, uint64_t v);

// stores
void dpu_rt_store8  (uint8_t  *addr, uint8_t  v);
void dpu_rt_store16 (uint16_t *addr, uint16_t v);
void dpu_rt_store32 (uint32_t *addr, uint32_t v);
void dpu_rt_store64 (uint64_t *addr, uint64_t v);

// others
void dpu_rt_allo (uint8_t *addr, uint32_t size);
void dpu_rt_mllo (uint8_t *addr, uint64_t size); // malloc & calloc
void dpu_rt_rllo (uint8_t *old, uint8_t *neww, uint64_t size);
void dpu_rt_fre  (uint8_t *addr);
void dpu_rt_call (uint32_t id);
void dpu_rt_ret  (uint32_t id);
void dpu_rt_sig  (int signal);

// memory system
void *dpu_rt_malloc  (size_t size);
void  dpu_rt_free    (void *ptr);
void *dpu_rt_realloc (void *ptr, size_t size);

enum eventtype
{
   NOP = 0,
   // loads
   LD8,
   LD16,
   LD32,
   LD64,
   // stores
   ST8,
   ST16,
   ST32,
   ST64,
   // memory allocation
   ALLO,
   MLLO,
   RLLO,
   FREE,
   CALL,
   RET,
   // threads
   THNEW,
   THJN,
   THSW,
   // locks
   LKNEW,
   LKTAKE,
   LKREL,
   // misc
   EVFULL,
};

struct rt
{
	uint64_t memstart;
	uint64_t memend;
	uint64_t memsize;

	uint64_t stackstart;
	uint64_t stackend;
	uint64_t stacksize;

   struct {
	   uint8_t  *evstart;
	   uint8_t  *evend;
	   uint8_t  *evptr;
      uint64_t *addrstart;
	   uint64_t *addrptr;
      uint16_t *idstart;
      uint16_t *idptr;
      uint64_t *valstart; // optional
	   uint64_t *valptr;
   } trace;

	uint64_t host_rsp;
};

// const for fast address checking without memory access, defined in rt.c
// static const uint64_t memstart;
// static const uint64_t memend;
// static const uint64_t evend;
// static struct rt * const rt;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _RT_RT_H_
