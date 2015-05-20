/**
 *
 * File:	TimerThreadUtil.h
 * Author:	dusteye
 * Brief:   基于互斥锁和条件变量的定时器
 */

#ifndef __TimerThreadUtil__
#define __TimerThreadUtil__


#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
#include<sys/types.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>


template<typename Class>
class TimerThreadUtil{

	struct TimerInfo{
	TimerInfo(TimerThreadUtil<Class> *timer, int repeatTimes, long delay_sec, long interval_sec)
	:_timer(timer)
	,_repeatTimes(repeatTimes)
	,_delay_sec(delay_sec)
	,_interval_sec(interval_sec) {};

		TimerThreadUtil<Class> *_timer;
		int _repeatTimes;
		long _delay_sec;
		long _interval_sec;
	};

public:
typedef bool (Class::*MethodType)(void *);
typedef bool (*FuncType)(void *);

TimerThreadUtil(FuncType func, void *data)
:_object(NULL)
,_method(NULL)
,_data(data)
,_func(func)
,_timerInfo(NULL){}

TimerThreadUtil(Class &obj, MethodType method, void *data)
:_object(&obj)
,_method(method)
,_data(data)
,_func(NULL)
,_timerInfo(NULL){}

void exec(int repeatTimes, long delay_sec, long interval_sec);

private:
	Class* _object;
	MethodType _method;
	FuncType _func;
	TimerInfo *_timerInfo;
	void *_data;
	void *threadFunc(void *);
	static void *threadFuncHelper(void *context);
};

template<typename Class>
void* TimerThreadUtil<Class>::threadFuncHelper(void *context){
	return (*((TimerThreadUtil<Class>**)context))->threadFunc(context);
}


template<typename Class>
void TimerThreadUtil<Class>::exec(int repeatTimes, long delay_sec, long interval_sec){
	pthread_t pid;
	if (_timerInfo == NULL){
		_timerInfo = new TimerInfo(this, repeatTimes, delay_sec, interval_sec);
	} else {
		_timerInfo->_repeatTimes = repeatTimes;
		_timerInfo->_delay_sec = delay_sec;
		_timerInfo->_interval_sec = interval_sec;
	}
	pthread_create(&pid, NULL, &(TimerThreadUtil<Class>::threadFuncHelper), _timerInfo);
}

template<typename Class>
void *TimerThreadUtil<Class>::threadFunc(void *data){
	pthread_cond_t cond;
	pthread_mutex_t mutex;
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);

	struct timespec to;
        TimerInfo *timerInfo = (TimerInfo *)data;
	to.tv_sec = time(NULL) + timerInfo->_delay_sec;
	to.tv_nsec = 0;

	pthread_mutex_lock(&mutex);

	int repeatTimes = timerInfo->_repeatTimes;

	while((repeatTimes < 0 ? 1 : repeatTimes--)){
		int err = pthread_cond_timedwait(&cond, &mutex, &to);
		if (err == ETIMEDOUT){
			to.tv_sec = time(NULL) + timerInfo->_interval_sec;
			to.tv_nsec = 0;
			if (_object != NULL && _method != NULL){
				if ((_object->*_method)(_data)){
					break;
				}
			} else if (_func != NULL){
				if (_func(_data)){
					break;
				}
			}
		}
	}
	pthread_mutex_unlock(&mutex);
	delete (TimerInfo*)data;
	return (void*)0;
}

#endif

