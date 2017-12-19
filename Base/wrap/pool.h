#ifndef POOL_H__
#define POOL_H__

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <list>
#include "allocator.h"
#include "funcs.h"

namespace Wrap{
	class PoolMgr;
	class PoolObj{
		friend class PoolMgr;
	public:
		PoolObj(const char* n = NULL) :ref(1){
			LOGI("PoolObj");
			if (n == NULL){
				name[0] = 0;
			}
			else{
				StrLCpy(name, n, MIN(49, strlen(n) + 1));
				name[49] = 0;
			}
		}
		virtual ~PoolObj(){
			LOGI("~PoolObj");
		}

	public:
		int retain(){ return ++ref; }
		int release(){
			--ref;

			assert(ref >= 0);
			if (ref == 0){
				//delete this;
				PoolObj* temp = this;
				wrap_delete(PoolObj, temp);

				static int ret;
				ret = 0;
				return ret;
			}
			else{
				return ref;
			}
		}

		const char* getName(){
			return name;
		}

		virtual void reuse(){
			retain();
		}
		virtual void unuse(){
			release();
		}

	protected:
		int ref;
		char name[50];
	};

	typedef std::list<PoolObj*> LISTPOOLO;
	class PoolMgr{
	protected:
		static PoolMgr* sIns;

		virtual ~PoolMgr(){
			clearPool();
		}
	public:
		static PoolMgr* GetIns(){
			if (sIns == nullptr){
				wrap_new_begin;
				sIns = wrap_new(PoolMgr);// new1 PoolMgr();
			}
			return sIns;
		}
		static void ReleaseIns(){
			// 			if (sIns)
			// 				delete sIns;
			wrap_delete(PoolMgr, sIns);
		}

		template<class T>
		T* getFromPool(const char* name){
			LISTPOOLO::iterator it = lst.begin();
			for (it; it != lst.end(); it++){
				PoolObj* obj = *it;
				if (obj && strcmp(obj->getName(), name) == 0){
					lst.erase(it);
					obj->reuse();
					obj->release();
					return (T*)obj;
				}
			}

			//return new1 T();
			wrap_new_begin;
			return wrap_new(T);
		}
		void addToPool(PoolObj* obj){
			if (obj){
				obj->retain();
				lst.push_back(obj);
				obj->unuse();
			}
		}

	private:
		void clearPool(){
			LISTPOOLO::iterator it = lst.begin();
			for (it; it != lst.end(); it++){
				PoolObj* obj = *it;
				if (obj){
					int ret = obj->release();
					if (ret){
						LOGE("PoolObj 内存泄漏啦 obj=%p", obj);
					}
				}
			}
			lst.clear();
		}

	protected:
		LISTPOOLO lst;
	};

	template<typename T>
	class ClsGuard{
	public:
		ClsGuard(T *p) : m_p(p){}
		virtual ~ClsGuard(){ wrap_delete(T, m_p); }
	private:
		T *m_p;
	};

	class VoidGuard
	{
	public:
		VoidGuard(void *p) : m_p(p){}
		virtual ~VoidGuard(){ wrap_free(m_p); }
	private:
		void *m_p;
	};
}

#endif // POOL_H__
