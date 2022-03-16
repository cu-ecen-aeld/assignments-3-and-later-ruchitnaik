#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    int ret;
    
    //Cast thread_param to obtain thread arguments from parameter
    struct thread_data* ThreadParamArg = (struct thread_data *) thread_param;
    
    //Wait for specific time before acquiring lock
    ret = usleep(ThreadParamArg->wait_ms_before_obtain * 1000);
    if(ret != 0){
    	ERROR_LOG("FAILED to sleep for %d before obtaining lock", ThreadParamArg->wait_ms_before_obtain);
    	ThreadParamArg->thread_complete_success = false;
    	return thread_param;
    }
    
    //Obtain mutex after specified wait
    ret = pthread_mutex_lock(ThreadParamArg->pmutex);
    if(ret != 0){
    	ERROR_LOG("FAILED to obtain mutex");
    	ThreadParamArg->thread_complete_success = false;
    	return thread_param;
    }
    
    //Wait for specific time before releasing lock
    ret = usleep(ThreadParamArg->wait_ms_before_release * 1000);
    if(ret != 0){
        ERROR_LOG("FAILED to sleep for %d before releasing lock", ThreadParamArg->wait_ms_before_release);
    	ThreadParamArg->thread_complete_success = false;
    	return thread_param;
    }
    
    //Release mutex after specified wait
    ret = pthread_mutex_unlock(ThreadParamArg->pmutex);
    if(ret != 0){
    	ERROR_LOG("FAILED to release mutex");
    	ThreadParamArg->thread_complete_success = false;
    	return thread_param;
    }
    
    ThreadParamArg->thread_complete_success = true;				//Thread execution completed successfully
    DEBUG_LOG("Thread execution completed successfully");
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     * 
     * See implementation details in threading.h file comment block
     */
    int ret;	
	struct thread_data *pThreadParam;
	
	//Allocate memeory for data structure
	pThreadParam = (struct thread_data *)malloc(sizeof(struct thread_data));
	if(pThreadParam == NULL){
		ERROR_LOG("Not enough memory to be allocated for thread_data");
		return false;
	}
	
	//Setup structure parameters
	pThreadParam->pmutex = mutex;
	pThreadParam->wait_ms_before_obtain = wait_to_obtain_ms;
	pThreadParam->wait_ms_before_release = wait_to_release_ms;

	//Create thread
	//int pthread_create (pthread_t *thread,
	//const pthread_attr_t *attr,
	//void *(*start_routine) (void *),
	//void *arg);
	ret = pthread_create(thread, NULL, threadfunc, (void *)pThreadParam);
	if(ret == 0){
		DEBUG_LOG("Thread created successfully");
		return true;
	}
	
	ERROR_LOG("FAILED while creating the thread");     
    return false;
}

