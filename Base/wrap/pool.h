#ifndef POOL_H__
#define POOL_H__

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <list>
#include "allocator.h"

namespace Wrap{
	class PoolMgr;
	class PoolObj{
		friend class PoolMgr;
	public:
		PoolObj(const char* n = NULL) :ref(1){
			if (n == NULL){
				name[0] = 0;
			}
			else{
				strncpy(name, n, MIN(49, strlen(n) + 1));
				name[49] = 0;
			}
		}
		virtual ~PoolObj(){}

	public:
		int retain(){ return ++ref; }
		int release(){
			--ref;

			assert(ref >= 0);
			if (ref == 0){
				delete this;

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
				//sIns = new PoolMgr();
				new_(PoolMgr, sIns);
			}
			return sIns;
		}
		static void ReleaseIns(){
// 			if (sIns)
// 				delete sIns;
			delete_(PoolMgr, sIns);
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

			//return new T();
			new_(T, ret);
			return ret;
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
						LOGE("PoolObj 内存泄漏啦 obj=%p\n", obj);
					}
				}
			}
			lst.clear();
		}

	protected:
		LISTPOOLO lst;
	};

	class VoidGuard
	{
	public:
		VoidGuard(void *p) : m_p(p){}
		virtual ~VoidGuard(){ free_(m_p); }
	private:
		void *m_p;
	};
}

#endif // POOL_H__
