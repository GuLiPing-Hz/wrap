#ifndef ALLOCATOR_H___
#define ALLOCATOR_H___

#include "config.h"
#include <set>
#include <assert.h>

namespace Wrap{
	//�ڴ���������
	class Allocator{
	private:
		Allocator(){}
		virtual ~Allocator(){
			assert(mPointerSet.size() == 0);//�쳣�Ļ����ڴ�й©�ˡ���
		}

	public:
		/*
		@size  ����������ڴ��С
		*/
		virtual void* alloc(size_t size, const char* file, long line){
			void* pointer = calloc(1, size);
			LOGD("alloc mem pointer = %p,size = %d\n", pointer, size);

			mPointerSet.insert(pointer);
			return pointer;
		}
		virtual void dealloc(void* pointer){
			if (pointer != NULL){
				LOGD("dealloc mem pointer = %p\n", pointer);
				mPointerSet.erase(pointer);
				free(pointer);
			}
		}

		static Allocator* GetInstance();

	private:
		std::set<void*> mPointerSet;
	};
}

#define calloc_(M_o) \
	Wrap::Allocator::GetInstance()->alloc(M_o,__FILE__,__LINE__)

#define free_(M_o) \
	Wrap::Allocator::GetInstance()->dealloc((void*)M_o)

//ͨ�ö���� new_��delete_�����Բ��ü̳�Object
#define new_(M_T,M_o,...) \
	M_T* M_o = (M_T*)Wrap::Allocator::GetInstance()->alloc(sizeof(M_T),__FILE__,__LINE__); \
	M_o = M_o == NULL ? NULL : new (M_o) M_T (__VA_ARGS__)

#define delete_(M_T,M_o) \
	if (M_o != NULL){\
	M_o->~M_T();\
	Wrap::Allocator::GetInstance()->dealloc(M_o);\
	M_o = NULL;\
		}

#endif
