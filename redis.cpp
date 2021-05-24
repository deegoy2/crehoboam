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

static bool test_set(acl::redis& cmd, int n)
{
    std::cout<< "deeapk1" << std::endl;
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
    std::cout<< "deepak" << std::endl;
	int n = 1, conn_timeout = 10, rw_timeout = 10;
	acl::string addr("127.0.0.1:6379"), command;
	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	acl::redis_client client(addr.c_str(), conn_timeout, rw_timeout);

	acl::redis cmd(&client);
	test_set(cmd, n);
	test_get(cmd, n);
	test_exists(cmd, n);

    DB* db;
    Options options;
    options.IncreaseParallelism();
    options.OptimizeLevelStyleCompaction();
    options.create_if_missing = true;

    // open DB
    Status s = DB::Open(options, kDBPath, &db);
    assert(s.ok());

    // Put key-value
    s = db->Put(WriteOptions(), "key1", "value");
    assert(s.ok());
    std::string value;
    // get value
    s = db->Get(ReadOptions(), "key1", &value);
    assert(s.ok());
    std::cout<< "deepak" << value << std::endl;
    assert(value == "value");

	return 0;
}
