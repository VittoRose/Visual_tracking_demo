#include    <stdio.h>
#include    <math.h>
#include    <semaphore.h>
#include    <pthread.h>
#include    <sched.h>
#include    <time.h>
#include    <allegro.h>
#define     ACT 1

extern void time_add_ms(struct timespec *t, int ms);
extern int task_create(void* (*task)(void*), int ind, int period, int deadline, int prio, int aflag);
extern void wait_task_end(int index);
extern int get_index(void* arg);
extern int get_period(int i);
extern void wait_for_period(int i);
extern void task_activate(int i);
extern void ptask_init(int policy);
extern void time_copy(struct timespec *td, struct timespec ts);
extern int deadline_miss(int i);
extern void wait_for_activation(int i);
extern int time_cmp(struct timespec t1, struct timespec t2);
extern int get_dmiss(int i);
