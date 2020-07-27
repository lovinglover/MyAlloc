#include <iostream>
using std::cerr;
using std::endl;
using std::cout;

static const size_t _BLOCK = 8;	// 每个小块均为8的倍数，最小为8
static const size_t _MAX_BYTES = 128;		// 最多可以处理128字节大小的对象的内存分配管理
static const size_t _NUMBERS_OF_FREELISTS = _MAX_BYTES / _BLOCK;	// 自由链表free-list共16个

class MyAlloc {
private:
	struct obj
	{
		struct obj* next;	// 指向下一个位置的指针
	};

	static size_t ROUND_UP(size_t bytes) {
		// 向上调整为8的倍数
		return (bytes + _BLOCK - 1) &~(_BLOCK - 1);
	}

	static size_t GET_INDEX(size_t bytes) {
		// 返回在自由链表中的索引
		return ((bytes + _BLOCK - 1) / _BLOCK - 1);
	}

	static void* fill(size_t);	// 填满某一个链表下的空间，并返回一个该对象的地址，
	static char* chunk_alloc(size_t, int&);		// 申请分配n个size大小的空间

	static obj* free_list[_NUMBERS_OF_FREELISTS];
	static char* pool_start;	// 指向内存池的开始位置
	static char* pool_end;		// 指向内存池的结束位置
	static size_t heap_size_total;	// 记录一共分配了多少内存


public:
	static void* allocate(size_t);
	static void deallocate(void*, size_t);
};

char* MyAlloc::chunk_alloc(size_t size, int& objs)
{
	// 一次向操作系统要较大一块内存，防止频繁的调用malloc
	// objs为需要分配的个数，但可变，依据实际情况分配可以分配的个数
	char* result;
	size_t bytes_need = size *objs;
	size_t bytes_left = pool_end - pool_start;

	if (bytes_left >= bytes_need) {
		// 剩余内存可以满足，直接移动指向内存池开始位置的指针
		result = pool_start;
		pool_start += bytes_need;
		return result;
	}
	else if (bytes_left >= size) {
		// 剩余内存至少可用满足一个对象的大小
		objs = bytes_left / size;
		bytes_need = size *objs;
		result = pool_start;
		pool_start += bytes_need;
		return result;
	}
	else {
		// 剩余内存无法满足
		size_t bytes_to_get = 2 * bytes_need + ROUND_UP(heap_size_total >> 4);
		if (bytes_left > 0) {
			// 将这些剩余内存挂载到相应的自由链表上
			obj** my_free_list = free_list + GET_INDEX(bytes_left);
			((obj*)pool_start)->next = *my_free_list;
			*my_free_list = (obj*)pool_start;
		}
		pool_start = (char*)malloc(bytes_to_get);
		if (nullptr == pool_start) {
			// 分配失败，向右寻找，从较大端的剩余空间取一块满足当前的需求
			obj** my_free_list;
			obj* p;
			for (int i = size; i <= _MAX_BYTES; i += _BLOCK) {
				my_free_list = free_list + GET_INDEX(i);
				p = *my_free_list;
				if (nullptr != p) {
					// 把借用的内存放到正确的链表位置
					*my_free_list = p->next;
					pool_start = (char*)p;
					pool_end = pool_start + i;
					return chunk_alloc(size, objs);
				}
			}
			// 走遍循环说明没有可用内存，终止程序
			cerr << "cannot get more memory..." << endl;
			abort();
		}
		heap_size_total += bytes_to_get;
		pool_end = pool_start + bytes_to_get;
		return chunk_alloc(size, objs);
	}
}

void* MyAlloc::allocate(size_t size)
{
	obj** my_free_list;
	obj* result;

	if (size > _MAX_BYTES) {
		// 对象字节超过128，不处理
		cerr << "cannot deal with this..." << endl;
		abort();
	}

	my_free_list = free_list + GET_INDEX(size);
	result = *my_free_list;

	if (result == nullptr) {
		// 说明该条链表下没有数据或者没有可用内存，需要填充
		void* r = fill(ROUND_UP(size));
		return r;
	}
	// 否则说明该链表下有可用内存，可以直接返回内存地址
	*my_free_list = result->next;
	return result;
}

void MyAlloc::deallocate(void* p, size_t size)
{
	obj* q = (obj*)p;
	obj** my_free_list;
	
	if (size > _MAX_BYTES) {
		// 对象字节超过128，不处理
		cerr << "cannot deal with this..." << endl;
		abort();
	}
	// 将该块内存回收到自由链表中，但并没有归还给操作系统
	my_free_list = free_list + GET_INDEX(size);
	q->next = *my_free_list;
	*my_free_list = q;
}

void* MyAlloc::fill(size_t size)
{
	int objs = 20;		// 默认申请20个对象大小的空间
	char* chunk = chunk_alloc(size, objs);
	obj** my_free_list = nullptr;
	obj* result = nullptr;
	obj* curr_obj = nullptr;
	obj* next_obj = nullptr;

	if (1 == objs) {
		// 只能分配一个，直接返回
		return chunk;
	}

	result = (obj*)chunk;	// 将得到的第一块size大小的内存作为结果返回

	my_free_list = free_list + GET_INDEX(size);
	*my_free_list = next_obj = (obj*)(chunk + size);	// 自由链表开始指向整个chunk第二个size大小开始
	for (int i = 1; i != (objs-1) ; i++) {
		curr_obj = next_obj;
		next_obj = (obj*)((char*)next_obj + size);
		curr_obj->next = next_obj;
	}
	curr_obj->next = nullptr;
	return result;
}

MyAlloc::obj* MyAlloc::free_list[_NUMBERS_OF_FREELISTS] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
char* MyAlloc::pool_start = nullptr;
char* MyAlloc::pool_end = nullptr;
size_t MyAlloc::heap_size_total = 0;
