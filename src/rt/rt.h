
#ifndef _RT_RT_H_
#define _RT_RT_H_

#include <inttypes.h>

// these two are in start.s; host should invoke rt_start
void dpu_rt_start (int argc, char **argv, char **env);
void dpu_rt_end (void);

// called from rt_start, will call main()
int dpu_rt_main (int argc, char **argv, char **env);

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
void dpu_rt_store8  (uint8_t *addr, uint8_t  v);
void dpu_rt_alloca  (uint8_t *addr, uint32_t size);
void dpu_rt_malloc  (uint8_t *addr, uint64_t size);
void dpu_rt_realloc (uint8_t *old, uint8_t *neww, uint64_t size);
void dpu_rt_free    (uint8_t *addr);
void dpu_rt_call    (uint32_t id);
void dpu_rt_ret     (uint32_t id);
void dpu_rt_sig     (int signal);

struct rt
{
	uint64_t memstart;
	uint64_t memend;
	uint64_t memsize;

	uint64_t stackstart;
	uint64_t stackend;
	uint64_t stacksize;

	void *   tracestart;
	void *   traceend;
	void *   traceptr;

	uint64_t host_rsp;
};

// const for fast address checking without memory access
extern const uint64_t memstart;
extern const uint64_t memend;
extern const uint64_t memsize;

// what contains the state of the runtime, including logtrace, heap, etc.
extern struct rt rt;

#endif
