#include    <stdio.h>
#include    <time.h>
#include    <pthread.h>
#include    <semaphore.h>
#include    <stdlib.h>

#define MAXTASK 10
#define ACT 1

pthread_t thread_id[MAXTASK];
int ptask_policy;
struct timespec ptask_t0;

struct task_param{
    //variables and parameters necessary to run the task

    int index;
    int period;
    int deadline;
    int priority;
    int dmiss;
    struct timespec at;
    struct timespec dl;
    pthread_t tid;
    sem_t tsem;
}task_var[MAXTASK];

void ptask_init(int policy){
    int i;

    ptask_policy = policy;
    clock_gettime(CLOCK_MONOTONIC, &ptask_t0);

    for(i=0; i<MAXTASK; i++){
        sem_init(&task_var[i].tsem, 0,1);
    }
}

void time_add_ms(struct timespec *t, int ms){
    t->tv_sec += ms/1000;
    t->tv_nsec += (ms%1000)*1000000;

    if(t->tv_nsec > 1000000000){
        t->tv_nsec -= 1000000000;
        t->tv_sec +=1;
    }
}

void time_copy(struct timespec *td, struct timespec ts){
    td->tv_sec = ts.tv_sec;
    td->tv_nsec = ts.tv_nsec;
}

int time_cmp(struct timespec t1, struct timespec t2){
    if(t1.tv_sec > t2.tv_sec) return 1;
    if(t1.tv_sec < t2.tv_sec) return -1;
    if(t1.tv_nsec > t2.tv_nsec) return 1;
    if(t1.tv_nsec < t2.tv_nsec) return -1;
    return 0;
}

void task_activate(int i){
    sem_post(&task_var[i].tsem);
}

int task_create(void* (*task)(void*), int ind, int period, int deadline, int prio, int aflag){
    int check_error;
    pthread_attr_t myatt;
    struct sched_param mypar;

    if (ind >= MAXTASK) exit(1);
    
    task_var[ind].index = ind;
    task_var[ind].period = period;
    task_var[ind].priority = prio;
    task_var[ind].dmiss = 0;
        
    pthread_attr_init(&myatt);
    pthread_attr_setinheritsched(&myatt, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&myatt, ptask_policy);
    mypar.sched_priority = task_var[ind].priority;
    pthread_attr_setschedparam(&myatt, &mypar);
    
    check_error = pthread_create(&thread_id[ind], &myatt, task, (void*)&task_var[ind]);

    if(aflag == ACT) task_activate(ind);
    
    return check_error;
}

int get_index(void* arg){
    struct task_param *param_pointer;
    param_pointer = (struct task_param*)arg;
    return param_pointer->index;
}

int get_period(int i){
    return task_var[i].period;
}

void wait_task_end(int index){
    pthread_join(thread_id[index], NULL);
}

void wait_for_period(int i){

    sem_wait(&task_var[i].tsem);
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &(task_var[i].at), NULL);
    time_copy(&(task_var[i].dl), task_var[i].at);

    time_add_ms(&(task_var[i].at), task_var[i].period);
    time_add_ms(&(task_var[i].dl), task_var[i].period);
    sem_post(&task_var[i].tsem);
}

void wait_for_activation(int i){
    struct timespec t;

    sem_wait(&task_var[i].tsem);
    clock_gettime(CLOCK_MONOTONIC, &t);
    time_copy(&(task_var[i].at), t);
    time_copy(&(task_var[i].dl), t);
    time_add_ms(&(task_var[i].at),task_var[i].period);
    time_add_ms(&(task_var[i].dl), task_var[i].deadline);
}

int deadline_miss(int i){

    struct timespec now;
    
    clock_gettime(CLOCK_MONOTONIC, &now);
    
    if(time_cmp(now, task_var[i].dl) > 0){
        task_var[i].dmiss++;
        return 1;
    }
    return 0;
}

int get_dmiss(int i){
    return task_var[i].dmiss;
}