#include <pthread.h>
#include <assert.h>

#define K 2

/*
The left part of the unfolding:

             r0:mi  ...... r1:mi
           /  |              
w0:ma[0] .. r0:ma[0]        
   |   /   /      \         . 
r0:ma[0] w0:ma[0] r1:mi     
   |               |       
r1:mi             r1:ma[1]
   |             .  |
r1:ma[1].       . w1:ma[1]
   |     .     .
w1:ma[1]  .   . 
           . .
         w1:ma[1]
   \     /     \   /
   r1:ma[1]    r1:ma[1]
 
*/ 
pthread_mutex_t ma[K];
pthread_mutex_t mi;

// The parametric threads
// wa locks on a different mutex
// they are all concurrent with each other
void *wa(void *arg)
{
 unsigned id = (unsigned long) arg;
 pthread_mutex_lock(&ma[id]);
 pthread_mutex_unlock(&ma[id]);
 return 0;
}

// ra locks on a common lock
// and then conflicts with one of the wa's
// To prevent a blow-up in the number
// comment out the unlock mi.
// Not unlocking, will produce an exponential
// number of the SSBs:
/*
               a:lock x  .... b:lock x
                  |              |
                  |              |
                  |              |
 c:lock y ...  a:lock y       b:lock z .... d:lock z

*/
 
// then it locks on one of wa 
void *ra(void *arg)
{
 unsigned id = (unsigned long) arg;
 pthread_mutex_lock (&mi);
  pthread_mutex_lock (&ma[id]);
  pthread_mutex_unlock (&ma[id]);
 pthread_mutex_unlock (&mi);
 return 0;
}

int main()
{
 pthread_t idr[K];
 pthread_t idw[K];
 pthread_mutex_init(&mi, NULL);

 for (int i = 0; i < K; i++)
 {
   pthread_mutex_init(&ma[i], NULL);
   pthread_create(&idw[i], NULL, wa, (void*) (long) i);
   pthread_create(&idr[i], NULL, ra, (void*) (long) i);
 }

 for (int i = 0; i < K; i++)
 {
   pthread_join(idw[i],NULL);
   pthread_join(idr[i],NULL);
 }
}

/*

Exec 1
------
1     2     3     4
w0    r0    w1    r1
===== ===== ===== =====
a0
      lmi
      a0
      umi
            a1          --
                  lmi
                  a1    **
                  umi
c15u: explore: replay seq: 0 5; 1 4; 2 6; 3 1; || 4 3; -1

Exec 2
------
1     2     3     4
w0    r0    w1    r1
===== ===== ===== =====
a0
      lmi               ---
      a0
      umi
                  lmi   **
                  a1
                  umi
            a1
c15u: explore: replay seq: 0 5; 1 4; 2 1; || 4 2; -1

Exec 3
------
1     2     3     4
w0    r0    w1    r1
===== ===== ===== =====
a0
                  lmi
                  a1    --
                  umi
            a1          **
      lmi               
      a0
      umi
c15u: explore: replay seq: 0 5; 1 4; 2 1; 4 2; || 3 2; -1

Exec 4
------
1     2     3     4
w0    r0    w1    r1
===== ===== ===== =====
a0                      --
                  lmi      
            a1          
                  a1    
                  umi
      lmi                  
      a0                **
      umi
c15u: explore: replay seq: 0 5; 1 1; || 2 1; 3 3; 4 5; 2 2; -1

Exec 5
------
1     2     3     4
w0    r0    w1    r1
===== ===== ===== =====
            a1          
                  lmi   --
                  a1    
                  umi
      lmi               **
      a0                
      umi
a0                      
c15u: explore: replay seq: 0 5; 1 1; 2 1; 3 4; 4 1; || 2 1; -1


Exec 6
------
1     2     3     4
w0    r0    w1    r1
===== ===== ===== =====
            a1          
      lmi                  
      a0                
      umi
                  lmi      
                  a1    
                  umi
a0                      
c15u: explore: replay seq: 0 5; 1 1; 2 1; 3 1; || 2 4; 4 3; -1

Exec 7
------
1     2     3     4
w0    r0    w1    r1
===== ===== ===== =====
      lmi               --
      a0                
      umi
                  lmi   **
                  a1    
                  umi
            a1          
a0                      
c15u: explore: replay seq: 0 5; 1 1; 2 1; 3 1; || 4 2; -1

c15u: disset: SSB, count 1, |trail| 14, |D| 3 (2 just, 1 unjust)
c15u: explore: 14e  2j  1u: #0 @0 S C1 C2 C3 C4;  #1 @5 S;  #2 S;  #3 S;  #4 S L00 @10 X01 U00 E

Exec 8 (it should explore it!)
------
1     2     3     4
w0    r0    w1    r1
===== ===== ===== =====
                  lmi      
                  a1    
                  umi
      lmi                  
      a0                
      umi
            a1          
a0                      
*/
