FAMILIES = \
	cleanup_pop           \
	mutex_spin-lock       \
	pthread_attr_init     \
	schedule              \
	sigpause mutex_unlock \
	pthread_detach        \
	semaphores mutex_lock \
	pi_test               \
	pthread_join          \
	sem_getvalue

all:
	for p in  $(FAMILIES); \
		do \
		echo $$p ; \
		cd $$p;\
		$(MAKE) ; \
		cd .. ; \
	done

clean:
	for p in  $(FAMILIES); \
	do \
	echo $$p ; \
	cd $$p; \
        $(MAKE) clean ; \
	cd .. ; \
	done

run:
	for p in  $(FAMILIES); \
	do \
	echo $$p ; \
	cd $$p; \
        $(MAKE) run ; \
	cd .. ; \
	done

.PHONY: clean run
