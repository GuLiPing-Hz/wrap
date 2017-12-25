#ifndef ALLOCATOR_H___
#define ALLOCATOR_H___

#include "config.h"
#include "funcs.h"
#include <map>
#include <assert.h>
#include "mutex.h"

namespace Wrap{
	//内存分配管理器
	class Allocator{
	public:
		struct LeakPosition{
			char file[250];
			long line;
			size_t size;
		};

	private:
		Allocator(){}
		virtual ~Allocator(){
			Guard lock(mMutex);
			assert(mPointerMap.size() == 0);//异常的话，内存泄漏了。。

			std::map<void*, LeakPosition>::iterator it = mPointerMap.begin();
			for (it; it != mPointerMap.end(); it++){
				LOGE("Mem Leak for %s %ld", it->second.file, it->second.line);
			}
		}

	public:

		/*
		@size  期望分配的内存大小
		*/
		virtual void* alloc(size_t size, const char* file, long line){
			Guard lock(mMutex);
			void* pointer = calloc(1, size);
			//LOGD("alloc mem pointer = %p,size = %d", pointer, size);

			LeakPosition lp;
			STR_CPY(lp.file, file);
			lp.line = line;
			lp.size = size;
			mPointerMap.insert(std::make_pair(pointer, lp));
			return pointer;
		}
		virtual void dealloc(void* pointer){
			Guard lock(mMutex);
			if (pointer != NULL){
				//LOGD("dealloc mem pointer = %p", pointer);
				mPointerMap.erase(pointer);
				free(pointer);
			}
		}

		static Allocator* GetInstance(){
			if (sIns == NULL){
				sIns = new Allocator();
			}
			return sIns;
		}
		static void ReleaseIns(){
			delete sIns;
			sIns = NULL;
		}

	private:
		static Allocator* sIns;
		std::map<void*,LeakPosition> mPointerMap;
		Mutex mMutex;
	};
}

#define wrap_calloc(M_o) \
	Wrap::Allocator::GetInstance()->alloc(M_o,__FILE__,__LINE__)

#define wrap_free(M_o) \
	Wrap::Allocator::GetInstance()->dealloc((void*)M_o);\
	M_o = NULL

//通用对象的 new_和delete_，可以不用继承Object,一个闭包只需申明一次即可
#define wrap_new_begin void* M_temp = NULL
//在使用new之前，请申明new_begin;
#define wrap_new(M_T,...) \
	(M_temp = Wrap::Allocator::GetInstance()->alloc(sizeof(M_T), __FILE__, __LINE__)) == NULL ? NULL : new (M_temp)M_T(__VA_ARGS__)

//释放对象,如果有namespace，那么传递M_T的时候可以去掉
//还有就是最好把析构函数写成虚析构,否则子类的析构函数将无法执行
#define wrap_delete(M_T,M_o) \
	if (M_o != NULL){\
	M_o->~M_T();\
	Wrap::Allocator::GetInstance()->dealloc(M_o);\
	M_o = NULL;\
}

#endif
