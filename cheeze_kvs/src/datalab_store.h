#ifndef __CHEEZE_STORE_H__
#define __CHEEZE_STORE_H__

#include <cstdint>
#include <cstdbool>
#include <cstdlib>

#define kvs_key_t uint32_t
#define kvs_value_t uint32_t
#define KVS_SUCCESS 0
#define KVS_FAIL 1

struct kvs_key {
	char *key;
	kvs_key_t klen;
};

struct kvs_value {
	char *value;
	kvs_value_t vlen;
};

struct kvs_context {
	bool io_async;
	void *extra;
};

typedef int (*KVS_SET) (struct my_kvs *my_kvs, struct kvs_key *key, struct kvs_value *value, struct kvs_context *ctx);
typedef int (*KVS_GET) (struct my_kvs *my_kvs, struct kvs_key *key, struct kvs_value *value, struct kvs_context *ctx);
typedef int (*KVS_DEL) (struct my_kvs *my_kvs, struct kvs_key *key, struct kvs_context *ctx);

struct datalab_kvs_wrapper {
	struct datalab_kvs_wrapper *wrapper;
	struct my_kvs *my_kvs;
	void (*set_env) (struct datalab_kvs_wrapper *wrapper, struct my_kvs *mykvs); 
};

static inline int datalab_kvs_wrapper_init (struct datalab_kvs_wrapper **wrapper) {
	*wrapper = (struct datalab_kvs_wrapper *)calloc(1, sizeof (struct datalab_kvs_wrapper));
	return 0;
}

static inline int datalab_kvs_wrapper_destroy (struct datalab_kvs_wrapper *wrapper) {
	free(wrapper);
	return 0;
}

#endif
