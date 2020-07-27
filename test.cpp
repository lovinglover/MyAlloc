#include "myAlloc.hpp"



int main()
{
	void* p1 = MyAlloc::allocate(40);
	void* p2 = MyAlloc::allocate(60);	// 非8的倍数，调整到64
	void* p3 = MyAlloc::allocate(80);

	void* q1 = MyAlloc::allocate(40);
	void* p4 = MyAlloc::allocate(100);
	void* q2 = MyAlloc::allocate(60);
	void* q3 = MyAlloc::allocate(80);

	cout << p1 << " " << p2 << " " << p3 << " " << p4 << " " << endl;
	cout << q1 << " " << q2 << " " << q3 << endl;
	MyAlloc::deallocate(p1, 40);
	MyAlloc::deallocate(p2, 60);
	MyAlloc::deallocate(p3, 80);
	MyAlloc::deallocate(q1, 40);
	MyAlloc::deallocate(p4, 100);
	MyAlloc::deallocate(q2, 60);
	MyAlloc::deallocate(q3, 80);
	return 0;
}
