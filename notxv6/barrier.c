#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

static int nthread = 1;
static int round = 0;
//add lab6-barrier
static __thread int thread_flag = 0;//lacal sense(标志) 每个线程单独的内存地址

struct barrier
{
  pthread_mutex_t barrier_mutex;
  pthread_cond_t barrier_cond;
  int nthread;      // Number of threads that have reached this round of the barrier
  int round;     // Barrier round
  //add lab6-barrier
  int flag;//帮助识别是哪个轮次的

} bstate;

static void
barrier_init(void)
{
  assert(pthread_mutex_init(&bstate.barrier_mutex, NULL) == 0);
  assert(pthread_cond_init(&bstate.barrier_cond, NULL) == 0);
  bstate.nthread = 0;
  //add lab6-barrier
  bstate.flag = 0;
}

static void 
barrier()
{
  // YOUR CODE HERE
  //
  // Block until all threads have called barrier() and
  // then increment bstate.round.
  //
  pthread_mutex_lock(&bstate.barrier_mutex);
  thread_flag = !thread_flag;
  pthread_mutex_unlock(&bstate.barrier_mutex);

  /*
    这个while循环的目的是为了将不是第一轮的线程全部卡在这个while循环中，直到第一轮的最后一个线程到达，反转bstate.flag后，
    最后一个线程会使用broadcast唤醒所有的睡眠线程，包括不是第一轮的线程，这些不是第一轮的线程被唤醒后，会继续while判断，
    由于bstate.flag已经被反转，所以不符合条件，则第二轮的开始往下执行
    虽然第三轮和第一轮的thread_flag会相同，但根据逻辑想一想，应该不可能有已经执行第三轮了，还有第一轮的线程会要barrier
  */
  while(thread_flag == bstate.flag){
    pthread_cond_wait(&bstate.barrier_cond, &bstate.barrier_mutex);
  }
  
  //在合适的位置多几次加锁解锁，性能更好
  pthread_mutex_lock(&bstate.barrier_mutex);
  int arrived = ++bstate.nthread;
  if (arrived == nthread){
    bstate.round++;
    bstate.flag = !bstate.flag;
    bstate.nthread = 0;
    pthread_cond_broadcast(&bstate.barrier_cond);
  }else{
    pthread_cond_wait(&bstate.barrier_cond, &bstate.barrier_mutex);
  }
  pthread_mutex_unlock(&bstate.barrier_mutex);
}

static void *
thread(void *xa)
{
  long n = (long) xa;
  long delay;
  int i;

  for (i = 0; i < 20000; i++) {
    int t = bstate.round;
    assert (i == t);
    barrier();
    usleep(random() % 100);
  }

  return 0;
}

int
main(int argc, char *argv[])
{
  pthread_t *tha;
  void *value;
  long i;
  double t1, t0;

  if (argc < 2) {
    fprintf(stderr, "%s: %s nthread\n", argv[0], argv[0]);
    exit(-1);
  }
  nthread = atoi(argv[1]);
  tha = malloc(sizeof(pthread_t) * nthread);
  srandom(0);

  barrier_init();

  for(i = 0; i < nthread; i++) {
    assert(pthread_create(&tha[i], NULL, thread, (void *) i) == 0);
  }
  for(i = 0; i < nthread; i++) {
    assert(pthread_join(tha[i], &value) == 0);
  }
  printf("OK; passed\n");
}
