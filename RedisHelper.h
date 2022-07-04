/*
 * HiRedis.h
 *
 *  Created on: Sep 12, 2017
 *      Author: root
 */

#ifndef HIREDIS_H_
#define HIREDIS_H_
#include <string>
#include <string.h>
#include <vector>
#include <map>
#include "hiredis.h"
#include <pthread.h>
#include <list>
#include <alloca.h>
using namespace std;



#define SingleRedisHelp RedisHelper::GetInstance()

class RedisHelper
{
public:
    static RedisHelper *GetInstance();

public:
    bool connect(const char* host = "127.0.0.1",unsigned short u_port = 6379,string strPasswd = "");

    bool subscribe(string channel, ...);

    bool set(string key, string value);

    bool publish(string channel, string message);

    bool publish(string channel, char* message, int length);

    bool lpush(string key, string message);

    int llen(string key);

    bool brpop(string key, string &message);

    bool getMessage(string& message);

    bool getMessage(string& message, string& channel);

    bool disConnect();
private:
    static RedisHelper * redisHelper_;

    RedisHelper();
    virtual ~RedisHelper();
private:
	redisContext * m_pContextSub;
	redisContext * m_pContextPub;

	pthread_mutex_t  m_dataMutex; //线程锁

	bool m_bRunFlag;
};

#endif /* HIREDIS_H_ */
