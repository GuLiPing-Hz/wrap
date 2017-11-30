#ifndef JSON_VALUE_H___
#define JSON_VALUE_H___

#include <vector>

namespace Wrap{

	class BufferValue{
	public:
		enum eDataType {
			type_unknow = 0,
			type_char = 1,//[16+1][���鳤��][data]
			type_short = 2,
			type_int = 3,
			type_int64 = 4,
			type_float = 5,//[]
			//17��Ҫ���������
			type_str = 17,

			//����������Ҫ��BufferUnitArraʹ��
			type_custom = 6,//[16+6][���鳤��][{�ṹ����,�ṹ��}]...
			type_array = 16,//
		};
		eDataType type;

		struct Data{
			union BaseData {//�������ݽṹ
				char c;
				short s;
				int i;
				long long ll;
				float f;
			};
			BaseData base;
			//�ַ�����Ҫ������
			std::string str;
		};
		Data data;

		typedef std::vector<BufferValue*> VECTORBJ;
		VECTORBJ list;

		//BufferJson() :PoolObj("BufferJson"){}

		// 	virtual void reuse(){
		// 		PoolObj::reuse();
		// 		clear();
		// 	}

		void clear(){
			type = type_unknow;
			list.clear();
		}
	};

	void ReleaseBufferValue(BufferValue* &value);

}
#endif // !JSON_VALUE_H___

