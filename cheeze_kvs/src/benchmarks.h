#ifndef __BENCHMARKS_H__
#define __BENCHMARKS_H__

#include "datalab_store.h"

struct thread_args {
	int id;
	kvs_key_t klen;
	uint32_t vlen;
	int count;
	int op_type;
	struct datalab_kvs_wrapper *wrapper;
};

#endif
