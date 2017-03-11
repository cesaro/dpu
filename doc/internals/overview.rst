
- Describe the mapping from steroids threads to DPU threads, and the limintation
  on the number of threads that that artificially imposes.

- how to profile with valgrind

- do a README, and move current contents of the README here ;)
  - describe dependencies, how to install, and how to run on a simple program


Communication Steroids - DPU
----------------------------

- I suspect that there is a problem due to the manner in which we handle tids in
  steroids and pids in DPU. I think each of them should send to the other a tid
  naming consistent with its own naming model and the opposite peer should do
  the translation when necessary. This would require enriching the data
  structure for the replay, to send structures {action, count} where action is
  "context switch" or "thread create", and count is an event count or a new tid.

Invariants expected by DPU on the incoming stream of actions:

- when the stream contains an action THCREAT(x), the tid x satisfies
  0 <= x <= N,
  where N is the number of times that an action THCREAT happened in the stream
  before this THCREAT, and x is *not* the the tid of a thread that is currently
  alive.

