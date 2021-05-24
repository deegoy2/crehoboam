#include "stdafx.h"
#include <iostream>
#include <cstdio>
#include <string>
#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/options.h"
using namespace ROCKSDB_NAMESPACE;

std::string kDBPath = "/Users/d0g00kj/elabs/temp";
static acl::string __keypre("test_key");
static acl::string __key = "stream_key";
static DB* db;

static void get(std::string key){
    std::string value;
    db->Get(ReadOptions(), key, &value);
    std::cout << key << "\t" << value << std::endl;
}
static void put(std::string key, std::string value){
    db->Put(WriteOptions(), key, value);
    // std::cout << key << "\t" << value << std::endl;
}

static void show_message(const acl::redis_stream_message& message)
{
	printf("\tid=%s\r\n", message.id.c_str());
	for (std::vector<acl::redis_stream_field>::const_iterator cit= message.fields.begin(); cit != message.fields.end(); ++cit) {
		printf("\t\tname=%s, value=%s\r\n", (*cit).name.c_str(),(*cit).value.c_str());
        std::cout << "before: " ;
        get((*cit).name.c_str());
        put((*cit).name.c_str(),(*cit).value.c_str());
        std::cout << "after : " ;
        get((*cit).name.c_str());
	}
}

static std::string show_messages(const acl::redis_stream_messages& messages)
{
	size_t i = 0;
	printf("key=%s\r\n", messages.key.c_str());
    std::string last_seen = "";
	for (std::vector<acl::redis_stream_message>::const_iterator
		it = messages.messages.begin(); it != messages.messages.end();
		++it) {

		if (++i <= 10) {
			show_message(*it);
            last_seen = it->id.c_str();
		}
	}
	printf("total messages count=%lu\r\n", (unsigned long) messages.size());
    return last_seen;
}
static std::string xread(acl::redis_stream& redis, size_t count, std::string pre_last_seen)
{
    std::string last_seen = "0-0";
	acl::redis_stream_messages messages;
	std::map<acl::string, acl::string> streams;
	streams[__key] = pre_last_seen;
    
    std::cout << "pre_last_seen:=" << pre_last_seen.c_str() << std::endl;
    // sleep(1);
    
	if (redis.xread(messages, streams, count) == false) {
        printf("xread error=%s\n", redis.result_error());
		// printf("xread error=%s, key=%s\r\n", redis.result_error(), __key.c_str());
		// const acl::string* req = redis.request_buf();
		// printf("request=[%s]\r\n", req ? req->c_str() : "NULL");
		return pre_last_seen;
	}

	printf("xread ok, key=%s\r\n", __key.c_str());

	if (messages.empty()) {
		printf("no messages\r\n");
        sleep(3);
        return pre_last_seen;
	} else {
		last_seen = show_messages(messages);
	}
    return last_seen;
}

static bool test_set(acl::redis& cmd, int n)
{
	acl::string key, val;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		val.format("val_%s", key.c_str());
		cmd.clear();
		if (cmd.set(key, val) == false)
		{
			printf("set key: %s error: %s\r\n", key.c_str(),
				cmd.result_error());
			return false;
		}
		else if (i < 10)
			printf("set key: %s ok\r\n", key.c_str());
	}

	return true;
}

static bool test_get(acl::redis& cmd, int n)
{
	acl::string key, val;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		cmd.clear();
		val.clear();
		if (cmd.get(key, val) == false)
		{
			printf("get key: %s error: %s\r\n", key.c_str(),
				cmd.result_error());
			return false;
		}
		else if (i < 10)
			printf("get key: %s ok, val: %s\r\n", key.c_str(),
				val.c_str());
	}

	return true;
}

static bool test_exists(acl::redis& cmd, int n)
{
	acl::string key;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		cmd.clear();
		if (cmd.exists(key.c_str()) == false)
		{
			if (i < 10)
				printf("no exists key: %s\r\n", key.c_str());
		}
		else if (i < 10)
			printf("exists key: %s\r\n", key.c_str());
	}

	return true;
}

int main()
{
    
    Options options;
    options.IncreaseParallelism();
    options.OptimizeLevelStyleCompaction();
    options.create_if_missing = true;
    Status s = DB::Open(options, kDBPath, &db);
    assert(s.ok());
    std::cout<< "RocksDb started!!" << std::endl;
    // Put key-value
    s = db->Put(WriteOptions(), "key1", "value");
    assert(s.ok());
    std::string value;
    // get value
    s = db->Get(ReadOptions(), "key1", &value);
    std::cout<< "deepak" << value << std::endl;
    assert(s.ok());
    assert(value == "value");


	int n = 1, conn_timeout = 10, rw_timeout = 10;
	acl::string addr("127.0.0.1:6379"), command;
	acl::acl_cpp_init();
	acl::log::stdout_open(true);
	acl::redis_client client(addr.c_str(), conn_timeout, rw_timeout);
    std::cout<< "Connected to redis server!!" << std::endl;
	acl::redis cmd(&client);
	test_set(cmd, n);
	test_get(cmd, n);
	test_exists(cmd, n);

    std::string pre_last_seen = "0-0";
    while(true){
        std::string new_last_seen = xread(cmd,1000, pre_last_seen);
        std::cout << "new_last_seen:=" << new_last_seen.c_str() << std::endl;
        pre_last_seen = new_last_seen;
    }
	return 0;
}
