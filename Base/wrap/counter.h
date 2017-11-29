#ifndef COUNTER__H___
#define COUNTER__H___
/*
	注释添加以及修改于 2014-4-2 

	提供一个计数器，每次获取一次计数的时候，自加1，不断循环
*/
namespace Wrap{
	class Counter
	{
	public:
		Counter(int min = 1, int max = 20000) //兼顾到seq使用short类型的协议
			: m_count(min), m_min(min), m_max(max){}
		inline int get(){
			if (m_count >= m_max) m_count = m_min;
			return ++m_count;
		}
	private:
		int m_count;
		int m_min;
		int m_max;
	};
}

#endif//COUNTER__H___
