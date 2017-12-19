#include "buffer_value.h"
#include "pool.h"

namespace Wrap{
	void ReleaseBufferValue(BufferValue* &value){
		if (value){
			if (value->list.empty()){
				//PoolMgr::GetIns()->addToPool(bj);
				//delete value;
				value = nullptr;
				wrap_delete(BufferValue, value);
			}
			else{
				BufferValue::VECTORBJ::iterator it = value->list.begin();
				for (it; it != value->list.end(); it++){
					BufferValue* item = *it;
					ReleaseBufferValue(item);
				}
			}
		}
	}
}
