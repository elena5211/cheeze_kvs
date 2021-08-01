#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <random>
#include <functional>
#include <pthread.h>
#include <unistd.h>
#include "datalab_store.h"
#include "benchmarks.h"
#include "my_kvs.h"

using namespace std;

#define SUCCESS 0
#define FAILED 1
#define WRITE_OP  1
#define READ_OP   2
#define DELETE_OP 3

bool verify;

void usage (char *program)
{
	printf("==============\n");
	printf("usage: %s [-c] [-n num_ios] [-o op_type] [-k klen] [-v vlen] [-t threads]\n", program);
	printf("-c      check        :  check if insertions were done correctly\n");
	printf("-n      num_ios      :  total number of ios (ignore this for iterator)\n");
	printf("-o      op_type      :  1: write; 2: read; 3: delete\n");
	printf("-k      klen         :  key length\n");
	printf("-v      vlen         :  value length\n");
	printf("-t      threads      :  number of threads\n");
	printf("==============\n");
}

int perform_read (int id, struct my_kvs *my_kvs, int count, kvs_key_t klen, uint32_t vlen) {
	int ret;
	char key[4096];
	char value[4096];

	int start_key = id * count;
	for (int i = start_key; i < start_key + count; i++) {
		memset(value, 0, vlen);
		sprintf(key, "%0*d", klen - 1, i);

		struct kvs_key  kvskey = {key, klen};
		struct kvs_value kvsvalue = {value, vlen};

		ret = my_kvs->get(my_kvs, &kvskey, &kvsvalue, NULL);
		if(ret != KVS_SUCCESS) {
			fprintf(stderr, "get tuple %s failed with error 0x%x\n", key, ret);
			//exit(1);
		} else {
			//fprintf(stdout, "retrieve tuple %s with value = %s, vlen = %d, actual vlen = %d \n", key, value, kvsvalue.length, kvsvalue.actual_value_size);
		}
	}

	return SUCCESS;
}


int perform_insertion (int id, struct my_kvs *my_kvs, int count, kvs_key_t klen, uint32_t vlen) {
	int ret;
	char key[4096];
	char value[4096];

	int start_key = id * count;

	mt19937 engine(0); // Seed
	uniform_int_distribution<int> distribution('0', 'z');
	auto generator = bind(distribution, engine);

	for(int i = start_key; i < start_key + count; i++) {
		sprintf(key, "%0*d", klen - 1, i);
		//sprintf(value, "%0*d", klen - 1, i + 10);

		for (uint32_t j = 0; j < vlen - 1; j++)
			value[j] = generator();
		value[vlen - 1] = '\0';

		struct kvs_key  kvskey = {key, klen};
		struct kvs_value kvsvalue = {value, vlen};

		if (verify)
			printf("<INSERTION> key:\"%s\", value:\"%s\"\n", key, value);

		ret = my_kvs->set(my_kvs, &kvskey, &kvsvalue, NULL);
		if (ret != KVS_SUCCESS) {
			fprintf(stderr, "set tuple failed with error 0x%x\n", ret);
			return FAILED;
		} else {
			//fprintf(stdout, "thread %d store key %s with value %s done \n", id, key, value);
		}

		if (verify) {
			char test[4096];
			struct kvs_value testvalue = {test, vlen};
			ret = my_kvs->get(my_kvs, &kvskey, &testvalue, NULL);
			if(ret != KVS_SUCCESS) {
				fprintf(stderr, "get tuple %s failed with error 0x%x\n", key, ret);
				//exit(1);
			} else {
				//fprintf(stdout, "retrieve tuple %s with value = %s, vlen = %d, actual vlen = %d \n", key, value, kvsvalue.length, kvsvalue.actual_value_size);
			}
			if (strcmp(value, test)) {
				fprintf(stderr, "<ERROR> Expected: \"%s\", but KVS returned: \"%s\"\n", value, test);
			}
		}

		if(i % 100 == 0)
			fprintf(stdout, "%d\r", i);
	}

	return SUCCESS;
}

int perform_delete (int id, struct my_kvs *my_kvs, int count, kvs_key_t klen, uint32_t vlen) {
	int ret;
	char key[4096];

	int start_key = id * count;  
	for (int i = start_key; i < start_key + count; i++) {
		sprintf(key, "%0*d", klen - 1, i);

		kvs_key  kvskey = {key, klen};
		ret = my_kvs->del(my_kvs, &kvskey, NULL);
		if(ret != KVS_SUCCESS) {
			fprintf(stderr, "del tuple failed with error 0x%x\n", ret);
			return FAILED;
		} else {
			//fprintf(stderr, "del key %s done \n", key);
		}

		// 2nd verify
		ret = my_kvs->del(my_kvs, &kvskey, NULL);
		if (ret == KVS_SUCCESS) {
			fprintf(stderr, "Key %s still exists!\n", key);
			return FAILED;
		}
	}

	return SUCCESS;
}

void do_io (int id, struct my_kvs *my_kvs, int count, kvs_key_t klen, uint32_t vlen, int op_type) {

	switch(op_type) {
		case WRITE_OP:
			perform_insertion(id, my_kvs, count, klen, vlen);
			break;
		case READ_OP:
			//perform_insertion(id, cont_hd, count, klen, vlen);
			perform_read(id, my_kvs, count, klen, vlen);
			break;
		case DELETE_OP:
			//perform_insertion(id, cont_hd, count, klen, vlen);
			perform_delete(id, my_kvs, count, klen, vlen);
			break;
		default:
			fprintf(stderr, "Please specify a correct op_type for testing\n");
			return;
	}
	return;
}

void *iothread (void *args)
{
	thread_args *targs = (thread_args *)args;
	do_io(targs->id, targs->wrapper->my_kvs, targs->count, targs->klen, targs->vlen, targs->op_type);
	return 0;
}

void  datalab_kvs_wrapper_set_env (struct datalab_kvs_wrapper *wrapper, struct my_kvs *my_kvs) {
	wrapper->wrapper = wrapper;
	wrapper->my_kvs = my_kvs;
	return;
}

int main (int argc, char *argv[]) {
	int num_ios = 10;
	int op_type = 1;
	kvs_key_t klen = 16;
	uint32_t vlen = 4096;
	int ret, c, t = 1;

	while ((c = getopt(argc, argv, "cn:o:k:v:t:h")) != -1) {
		switch(c) {
			case 'c':
				verify = true;
				break;
			case 'n':
				num_ios = atoi(optarg);
				break;
			case 'o':
				op_type = atoi(optarg);
				break;
			case 'k':
				klen = atoi(optarg);
				break;
			case 'v':
				vlen = atoi(optarg);
				break;
			case 't':
				t = atoi(optarg);
				break;
			case 'h':
				usage(argv[0]);
				return SUCCESS;
			default:
				usage(argv[0]);
				return SUCCESS;
		}
	}

	struct datalab_kvs_wrapper *wrapper;
	datalab_kvs_wrapper_init(&wrapper);

	struct my_kvs *my_kvs;
	my_kvs_init(&my_kvs);

	wrapper->set_env = datalab_kvs_wrapper_set_env;
	wrapper->set_env(wrapper, my_kvs);

	my_kvs->set_env = my_kvs_set_env;
	my_kvs->set_env(my_kvs, my_kvs_set, my_kvs_get, my_kvs_del);

	thread_args args[t];
	pthread_t tid[t];

	if (op_type != WRITE_OP) { // if op_type == READ or DELETE, preloading occurs.
		printf("[Preloading] 채우자 채워~\n");
		for(int i = 0; i < t; i++){
			args[i].id = i;
			args[i].klen = klen;
			args[i].vlen = vlen;
			args[i].count = num_ios;
			args[i].wrapper = wrapper;
			args[i].op_type = WRITE_OP;
			pthread_attr_t *attr = (pthread_attr_t *)malloc(sizeof(pthread_attr_t));
			cpu_set_t cpus;
			pthread_attr_init(attr);
			CPU_ZERO(&cpus);
			CPU_SET(0, &cpus); // CPU 0
			pthread_attr_setaffinity_np(attr, sizeof(cpu_set_t), &cpus);

			ret = pthread_create(&tid[i], attr, iothread, &args[i]);
			if (ret != 0) { 
				fprintf(stderr, "thread exit\n");
				free(attr);
				return FAILED;
			}
			pthread_attr_destroy(attr);
			free(attr);
		}

		for(int i = 0; i < t; i++) {
			pthread_join(tid[i], 0);
		}
		printf("[Preloading] Done!\n");
	}



	struct timespec t1, t2;
	clock_gettime(CLOCK_REALTIME, &t1);

	for(int i = 0; i < t; i++){
		args[i].id = i;
		args[i].klen = klen;
		args[i].vlen = vlen;
		args[i].count = num_ios;
		args[i].wrapper = wrapper;
		args[i].op_type = op_type;
		pthread_attr_t *attr = (pthread_attr_t *)malloc(sizeof(pthread_attr_t));
		cpu_set_t cpus;
		pthread_attr_init(attr);
		CPU_ZERO(&cpus);
		CPU_SET(0, &cpus); // CPU 0
		pthread_attr_setaffinity_np(attr, sizeof(cpu_set_t), &cpus);

		ret = pthread_create(&tid[i], attr, iothread, &args[i]);
		if (ret != 0) { 
			fprintf(stderr, "thread exit\n");
			free(attr);
			return FAILED;
		}
		pthread_attr_destroy(attr);
		free(attr);
	}

	for(int i = 0; i < t; i++) {
		pthread_join(tid[i], 0);
	}

	clock_gettime(CLOCK_REALTIME, &t2);
	unsigned long long start, end;
	start = t1.tv_sec * 1000000000L + t1.tv_nsec;
	end = t2.tv_sec * 1000000000L + t2.tv_nsec;
	double sec = (double)(end - start) / 1000000000L;
	fprintf(stdout, "Total time %.2f sec; Throughput %.2f ops/sec\n", sec, (double) num_ios * t /sec );

	my_kvs_destroy(wrapper->my_kvs);
	datalab_kvs_wrapper_destroy(wrapper);

	return SUCCESS;
}
