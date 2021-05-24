#include "stdafx.h"
#include <iostream>
#include <cstdio>
#include <string>

static acl::string __key = "stream_key";

std::string gen_random(const int len) {
    
    std::string tmp_s;
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    
    srand( (unsigned) time(NULL) * getpid());

    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i) 
        tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
    
    
    return tmp_s;
}

static void xadd(acl::redis_stream& redis, int n)
{
	acl::string name, value, result;
	std::map<acl::string, acl::string> fields;

	int i;
	for (i = 0; i < n; i++) {
		name.format("deepak-%d-1", i);
		value = gen_random(10);
		fields[name] = value;

		name.format("jyotika-%d-2", i);
		value = gen_random(10);
		fields[name] = value;

		name.format("name-%d-3", i);
		value = gen_random(10);
		fields[name] = value;

		if (redis.xadd(__key, fields, result) == false) {
			printf("xadd error=%s\r\n", redis.result_error());
			break;
		}

		if (i <= 10) {
			printf("xadd ok, key=%s, id=%s\r\n",
				__key.c_str(), result.c_str());
		}

		fields.clear();
		redis.clear();
	}

	printf("xadd ok, key=%s, n=%d\r\n", __key.c_str(), i);
}

int main()
{
	int conn_timeout = 10, rw_timeout = 10;
	acl::string addr("127.0.0.1:6379"), command;
	acl::acl_cpp_init();
	acl::log::stdout_open(true);
	acl::redis_client client(addr.c_str(), conn_timeout, rw_timeout);
    std::cout<< "Connected to redis server!!" << std::endl;
	acl::redis cmd(&client);
    xadd(cmd,10);
	return 0;
}
