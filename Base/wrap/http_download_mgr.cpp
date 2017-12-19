/*
 * HttpDownloadMgr.cpp
 *
 *  Created on: 2014-5-23
 *      Author: glp
 */

#include "http_download_mgr.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"

#define SAVE_BUF_SIZE			1048576/*1024 * 1024*/   // 接收缓冲区大小

namespace Wrap {

CHttpDownloadMgr::CHttpDownloadMgr()
:mThread(NULL)
{
	// TODO Auto-generated constructor stub
	wrap_new_begin;
	for (int i = 0; i < MAX_DOWNLOAD; i++){
		mArraHttpDownload[i] = wrap_new(CHttpDownload, &mReactor, this);//
	}
		
	mThread = ThreadWrapper::CreateThread(&CHttpDownloadMgr::HttpDownload,(void*)this,kLowPriority,"Http download");
}

CHttpDownloadMgr::~CHttpDownloadMgr() {
	// TODO Auto-generated destructor stub
	mReactor.stop();
	if(mThread)
	{
		if(!mThread->Stop())
			mThread->Terminate(0);
		//delete1 mThread;
		wrap_delete(ThreadWrapper, mThread);
	}

	for(int i=0;i<MAX_DOWNLOAD;i++)
	{
		if (mArraHttpDownload[i]){
			//delete mArraHttpDownload[i];
			wrap_delete(CHttpDownload, mArraHttpDownload[i]);
		}
	}
}

bool CHttpDownloadMgr::HttpDownload(ThreadObj pParam)
{
	CHttpDownloadMgr* pMgr = (CHttpDownloadMgr*)pParam;
	if(pMgr)
		return pMgr->mReactor.run();

	return false;
}

int CHttpDownloadMgr::checkIdle()
{
	for(int i=0;i<MAX_DOWNLOAD;i++)
	{
		if (mArraHttpDownload[i] && mArraHttpDownload[i]->isIdle())
			return i;
	}

	return -1;
}

bool CHttpDownloadMgr::initDownload()
{
	if(!mThread)
		return false;
	unsigned int threadID;
	return mThread->Start(threadID);
}

int CHttpDownloadMgr::addTask(TaskDownload& task,bool priority)
{
	if(strlen(task.url) == 0)
		return -3;

	Guard lock(mCS);
	SETURL::const_iterator it = mSetUrl.find(task.url);
	if(it != mSetUrl.end())//已经在下载队列中
		return 0;

	if(!task.info.download)
	{//如果不是下载到文件
		if(!task.info.saveBuf)//如果外部没有分配内存，给一个默认大小的内存
		{
			task.info.saveBuf = (unsigned char*) malloc(SAVE_BUF_SIZE);
			task.info.saveBufLen = SAVE_BUF_SIZE;
		}
		if(!task.info.saveBuf)
			return -1;
		
		task.info.saveTmpBuf = task.info.saveBuf;
	}
    else
    {
        //检查MD5是否一致，一致则不下载
        if(task.info.MD5[0] != '\0')//确保不为空
        {
            //如果不要求强制下载，或者文件校验md5一致的话，则不加入下载队列，认为下载成功
            if(!task.info.mandatory && CHttpDownload::checkMD5(task.info.fileName,task.info.MD5))
            {
                //if(task.onFinish)
                //    task.onFinish(&task,true,task.userData);
                return -2;
            }
        }

		task.info.saveBuf = NULL;
		task.info.saveBufLen = 0;
		task.info.saveTmpBuf = NULL;
    }

	mSetUrl.insert(task.url);
	if(priority)
		mListTask.push_front(task);
	else
		mListTask.push_back(task);

	int nIndex = checkIdle();
	if(nIndex != -1 )
	{
		mArraHttpDownload[nIndex]->clearReDownloadTimes();
		mArraHttpDownload[nIndex]->initDownload(task.url,task.onProgress,&task.info);

		memcpy(&mCurTask,&task,sizeof(task));
		if(!mArraHttpDownload[nIndex]->startDownload())
		{
			LOGE("%s : %s , start download failed",__FUNCTION__,task.url);
			doNext(false,NULL);
			return -1;
		}
		else
			return 0;
	}
	return 0;
}

void CHttpDownloadMgr::doNext(bool success,CHttpDownload* worker)
{
	TaskDownload info;
	{
		Guard lock(mCS);
		if(!mCurTask.info.download && worker && success)
			mCurTask.info.saveBufLen = worker->m_gDownloadInfo.saveBufLen;
		memcpy(&info,&mCurTask,sizeof(info));
		if (worker)
			worker->uninitDownload();
	}
	//结束的回调，以及内存的回收
	if(info.onFinish)
		info.onFinish(&info,success,info.userData);
	if (info.info.saveBuf)//释放内存
		wrap_free(info.info.saveBuf);

	Guard lock(mCS);
	mSetUrl.erase(info.url);
	if(strcmp(mListTask.front().url , info.url) == 0)
		mListTask.pop_front();
	else
	{
		LISTTASK::iterator it = mListTask.begin();
		for (;it!=mListTask.end();it++)
		{
			if(strcmp(it->url , info.url) == 0)
			{
				mListTask.erase(it);
				//由set控制队列中的任务都是不同的URL
				break;
			}
		}
	}

	if(mListTask.empty())
		return ;//已经都处理完毕

	//一般来说，到这一步应该是空闲的，但是如果在OnFinish中添加一个任务的话，这里就不是空闲状态了，所以还是需要判断一下
	int nIndex = checkIdle();

	if(nIndex != -1)
	{
		//取出队列的第一个
		TaskDownload next = mListTask.front();
		mArraHttpDownload[nIndex]->clearReDownloadTimes();
		mArraHttpDownload[nIndex]->initDownload(next.url,next.onProgress,&next.info);
		if(!mArraHttpDownload[nIndex]->startDownload())
		{
			LOGE("%s : %s , start download failed",__FUNCTION__,next.url);
			if(next.onFinish)
				next.onFinish(&next,false,next.userData);
			//继续处理下一个
			doNext(false,NULL);
		}
		else
		{
			Guard lock(mCS);
			memcpy(&mCurTask,&next,sizeof(TaskDownload));
		}
	}
}

} /* namespace Wrap */
