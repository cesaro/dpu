
.text

# exported symbols
.globl dpu_rt_start   # void dpu_rt_start (int argc, char **argv)
.globl dpu_rt_end	    # void dpu_rt_end (void)

dpu_rt_start :
	# 1. save all registers
	# 2. save rsp into rt.host_rsp
	# 3. rsp = rt.stackend
	# 4. jump into C, call dpu_rt_main, with the right arguments!

	jmp dpu_rt_main # FIXME for the time being

	# 5. execute the exit prologue
	jmp dpu_rt_end

dpu_rt_end :
	# 1. restore host stack: rsp = rt.host_rsp
	# 2. restore all host registers from the stack
	# 3. return to the host code
	ret

