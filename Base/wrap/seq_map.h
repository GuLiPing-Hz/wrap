#ifndef SEQMAP__H__
#define SEQMAP__H__
/*
	注释添加以及修改于 2014-4-3

	封装一个模板类map和一个线程安全的模板类map
	*/
#include <map>
#include <assert.h>
#include "mutex.h"

namespace Wrap
{
	template <class T>
	class SeqMap : public std::map<int, T>
	{
	public:

		typedef SeqMap<T> _Myt;
		typedef std::map<int, T> _Mybase;
		typedef typename _Mybase::iterator iterator;

		SeqMap(){}
		virtual ~SeqMap(){}
		void put(int id, T t) { _Mybase::insert(std::make_pair(id, t)); }
		void cover(int id, T t) { (*this)[id] = t; }
		T* get(int id)
		{
			iterator it;
			it = this->find(id);
			if (it == this->end())
				return NULL;
			return &(it->second);
		}
		unsigned int del(int id) {
			return _Mybase::erase(id);
		}
		iterator del(iterator i){
			return _Mybase::erase(i);
		}
		unsigned int size() { return _Mybase::size(); }
		void clear() { _Mybase::clear(); }
		_Mybase* getMap() { return this; }
	};

	template <class T>
	class SeqMap_ThreadSafe : public SeqMap<T>
	{
	public:

		typedef SeqMap_ThreadSafe<T> _Myt;
		typedef SeqMap<T> _Mybase;
		typedef typename _Mybase::iterator iterator;

		SeqMap_ThreadSafe()
		{
		}
		virtual ~SeqMap_ThreadSafe(){ }

		void lock(){
			mCS.lock();
		}
		void unlock(){
			mCS.unlock();
		}
		void put(int id, T t)
		{
			//加入临界区的保护，使之具有线程安全。
			Guard g(mCS);
			_Mybase::put(id, t);
		}
		void cover(int id, T t)
		{
			Guard g(mCS);
			_Mybase::cover(id, t);
		}
		T* get(int id)
		{
			Guard g(mCS);
			return _Mybase::get(id);
		}
		unsigned int del(int id)
		{
			Guard g(mCS);
			return _Mybase::del(id);
		}
		/*
		@注意：想要返回值的时候必须在外面增加安全锁，否则返回值将不可靠
		*/
		iterator del(iterator i){
			Guard g(mCS);
			return this->erase(i);
		}
		unsigned int size()
		{
			Guard g(mCS);
			return _Mybase::size();
		}
		void clear()
		{
			Guard g(mCS);
			_Mybase::clear();
		}
	protected:
		Mutex mCS;
	};
};

#endif//SEQMAP__H__
