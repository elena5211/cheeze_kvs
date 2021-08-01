#include "my_kvs.h"
#include <string.h>
#define XXH_INLINE_ALL
#include "xxhash.h"
#include <stdbool.h>
#include <stdio.h>
struct node *root_ptr;
int count = 0;

void FreeNode(struct node *node)
{
	free(node -> key);
	free(node -> value);
	free(node);
	return;
}

void SetNode(struct node *pos, char *key, char *value, kvs_key_t klen, kvs_value_t vlen, uint64_t hash, bool left)
{
	struct node *new_node = (struct node *)malloc(sizeof(struct node));
	new_node->key = (char *)malloc(sizeof(char)*klen);
	memcpy(new_node->key, key, klen);
	new_node->value = (char *)malloc(sizeof(char)*vlen);
	memcpy(new_node->value, value, vlen);
	new_node->klen = klen;
	new_node->vlen = vlen;
	new_node->right = NULL;
	new_node->left = NULL;
	new_node->hash = hash;
	if(left == true)
		pos->left = new_node;
	else
		pos->right = new_node;
	return;
}

void SearchInsert(struct node *pos, char *key, char *value, kvs_key_t klen, kvs_value_t vlen, uint64_t hash)
{
	//이미 존재하는 값
	if (pos->hash == hash)
	{
		if(strlen(pos->value)<strlen(value))
		{
			free(pos->value);
			pos->value = (char *)malloc(sizeof(char) * vlen);
			memcpy(pos->value, value, vlen);
		}
		else
			memcpy(pos->value, value, vlen);
	}
	//왼쪽으로
	else if (hash < pos->hash)
	{
		if((pos->left) == NULL)
		{
			SetNode(pos, key, value, klen, vlen, hash, true);
			return;
		}
		else
			return SearchInsert(pos->left, key, value, klen, vlen, hash);
	}
	//오른쪽으로
	else if (hash > pos->hash)
	{
		if(pos->right == NULL)
		{
			SetNode(pos, key, value, klen, vlen, hash, false);
			return;
		}
		else
			return SearchInsert(pos->right, key, value, klen, vlen, hash);
	}
}

struct node* findMinNode(struct node* root)
{
    struct node* tmp = root;
    while(tmp->left!= NULL)
        tmp = tmp->left;
    return tmp;
}

struct node* SearchDelete(struct node *node, uint64_t hash, bool *success, bool pass = false)
{
	//기본 작동 원리: 1-1. 지울 Node를 삭제하거나 1-2. 대체 key, value, hash를 가리키게 하고 2. 호출한 node의 오른쪽(혹은 왼쪽)에 위치하게 한다.
	struct node* tNode = NULL;
    if(node == NULL)
        return NULL;

    if(node->hash > hash)
        node->left = SearchDelete(node->left, hash, success, pass);
    else if(node->hash < hash)
        node->right = SearchDelete(node->right, hash, success, pass);
    else
    {
        // 자식 노드가 둘 다 있을 경우
        if(node->right != NULL && node->left != NULL)
        {
			//value 바꿀 node 선택
            tNode = findMinNode(node->right);
            node->hash = tNode->hash;
			//값 바꾸기
			free(node->key);
			free(node->value);
			node->key = tNode->key;
			node->value = tNode->value;
			//bool true가 되면 node->right의 key value를 node가 참조하고 있다는 것을 의미, node만 free한다.
			node->right = SearchDelete(node->right, tNode->hash, success, true);
        }
        else
        {
            tNode = (node->left == NULL) ? node->right : node->left;
			if (pass)
				free(node);
			//false인 경우 자식 두개에서 호출된 경우가 아님, node key value를 다 지워버린다.
			else
				FreeNode(node);
			
			*success = true;
            return tNode;
        }
    }

    return node;
}

void Searchget(struct node *pos, char *value, kvs_key_t klen, uint64_t hash)
{
	if (pos->hash == hash)
	{
		memcpy(value, pos->value, pos->vlen);
	}

	else if (hash < pos->hash)
	{
		if((pos->left) == NULL)
		{
			return;
		}
		else
			return Searchget(pos->left, value, klen, hash);
	}
	else if (hash > pos->hash)
	{
		if((pos->right) == NULL)
		{
			return;
		}
		else
			return Searchget(pos->right, value, klen, hash);
	}
}

void my_kvs_set_env (struct my_kvs *my_kvs, KVS_SET my_kvs_set, KVS_GET my_kvs_get, KVS_DEL my_kvs_del) {
	my_kvs->my_kvs = my_kvs;
	my_kvs->set = my_kvs_set;
	my_kvs->get = my_kvs_get;
	my_kvs->del = my_kvs_del;
	return;
}

int my_kvs_init (struct my_kvs **my_kvs) {
	*my_kvs = (struct my_kvs *)calloc(1, sizeof (struct my_kvs));
	root_ptr = (struct node*)malloc(sizeof(struct node));
	return 0;
}

int my_kvs_destroy (struct my_kvs *my_kvs) {
	free(my_kvs);
	return 0;
}

int my_kvs_set (struct my_kvs *my_kvs, struct kvs_key *key, struct kvs_value *value, struct kvs_context *ctx) {
	if(count == 0)
	{
		root_ptr->value = (char *)malloc(value->vlen * sizeof(char));
		//strcpy(root_ptr->value, value->value);
		memcpy(root_ptr->value, value->value, value->vlen);
		root_ptr->key = (char *)malloc(key->klen * sizeof(char));
		//strcpy(root_ptr->key, key->key);
		memcpy(root_ptr->key, key->key, key->klen);
		root_ptr->klen = key->klen;
		root_ptr->vlen = value->vlen;
		root_ptr->left = NULL;
		root_ptr->right = NULL;
		root_ptr->hash = XXH64(root_ptr->key, key->klen, 0);
		count++;
		return 0;
	}

	uint64_t hash;
	hash = XXH64(key->key, key->klen, 0);
	SearchInsert(root_ptr, key->key, value->value, key->klen, value->vlen, hash);
	return 0;
}

int my_kvs_get (struct my_kvs *my_kvs, struct kvs_key *key, struct kvs_value *value, struct kvs_context *ctx) {
	uint64_t hash;
	hash = XXH64(key->key, key->klen, 0);
	Searchget(root_ptr, value->value, key->klen, hash);
	
	return 0;
}

int my_kvs_del (struct my_kvs *my_kvs, struct kvs_key *key, struct kvs_context *ctx) {
	uint64_t hash;
	bool success = false;
	hash = XXH64(key->key, key->klen, 0);
	root_ptr = SearchDelete(root_ptr, hash, &success);

	return success ? 0 : -1;
}
