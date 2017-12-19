﻿#include "file_mgr.h"
#include <string.h>
#include <stdio.h>
#include "pool.h"

#ifdef WIN32
#define R_OK  4  /* Read */
#define W_OK  2  /* Write */
#define X_OK  1  /* Execute */
#define F_OK  0  /* Existence */
#include <io.h>
#include <direct.h>
#include <windows.h>
#else
//#include <strings.h>
#include <unistd.h>
//#include <ctype.h>
#include <time.h>
#include <utime.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#endif

#define WRITEBUFFERSIZE (8192)

namespace Wrap{



	bool CFileMgr::UnZipFile(const char* file, const char* outDir, const char* password)
	{
		unzFile zfile = unzOpen(file);
		if (!zfile)
			return false;

		CreateDir(outDir);

		unz_global_info info;
		if (unzGetGlobalInfo(zfile, &info) != UNZ_OK)
			return false;

		if (unzGoToFirstFile(zfile) != UNZ_OK)
			return false;

		int size_buf = WRITEBUFFERSIZE;
		void* buf = wrap_calloc(size_buf);
		if (!buf)
			return false;

		int numError = 0;

		do
		{
			unz_file_info uzfileInfo = { 0 };
			char filenameInzip[260] = { 0 };
			if (unzGetCurrentFileInfo(zfile, &uzfileInfo, filenameInzip, sizeof(filenameInzip), NULL, 0, NULL, 0) != UNZ_OK)
				return false;
			char* filenameWithoutPath = filenameInzip;
			char* p = filenameWithoutPath;
			while ((*p) != '\0')
			{
				if (((*p) == '/') || ((*p) == '\\'))
					filenameWithoutPath = p + 1;
				p++;
			}

			if ((*filenameWithoutPath) == '\0')
			{
				char temDir[260] = { 0 };
				sprintf(temDir, "%s/%s", outDir, filenameInzip);
				CreateDir(temDir);
			}
			else
			{
				char writeFile[260] = { 0 };
				sprintf(writeFile, "%s/%s", outDir, filenameInzip);

				if (unzOpenCurrentFilePassword(zfile, password) != UNZ_OK)
					return false;

				FILE* fp = fopen(writeFile, "rb");
				if (fp)
				{
					fclose(fp);
					remove(writeFile);
				}

				FILE* fout = fopen(writeFile, "wb");
				if (!fout)
				{
					char pathDir[260] = { 0 };
					strcpy(pathDir, writeFile);
					char* p1 = strrchr(pathDir, '/');
					char* p2 = strrchr(pathDir, '\\');
					char* p = NULL;
					if (p1&&p2)
						p = p1 > p2 ? p1 : p2;
					else if (p1 == NULL)
						p = p2;
					else if (p2 == NULL)
						p = p1;
					if (p)
						*p = 0;
					CreateDir(pathDir);
					//再尝试打开
					fout = fopen(writeFile, "wb");
				}

				if (!fout)
					return false;

				int err;
				do
				{
					err = unzReadCurrentFile(zfile, buf, size_buf);
					if (err < 0)
						break;

					if (err > 0)
					{
						if (fwrite(buf, err, 1, fout) != 1)
						{
							err = UNZ_ERRNO;
							break;
						}
					}
				} while (err > 0);

				fclose(fout);

				if (err == 0)
					ChangeFileDate(writeFile, uzfileInfo.dosDate, uzfileInfo.tmu_date);
				else
					numError++;
			}

			unzCloseCurrentFile(zfile);

		} while (unzGoToNextFile(zfile) == UNZ_OK);

		wrap_free(buf);

		unzClose(zfile);
		return !numError;
	}

	/* changeFileDate : change the date/time of a file
		filename : the filename of the file where date/time must be modified
		dosdate : the new1 date at the MSDos format (4 bytes)
		tmu_date : the new1 SAME date at the tm_unz format */
	void CFileMgr::ChangeFileDate(const char *filename, unsigned long dosdate, tm_unz tmu_date)
	{
#ifdef WIN32
		HANDLE hFile;
		FILETIME ftm, ftLocal, ftCreate, ftLastAcc, ftLastWrite;

		hFile = CreateFileA(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		GetFileTime(hFile, &ftCreate, &ftLastAcc, &ftLastWrite);
		DosDateTimeToFileTime((WORD)(dosdate >> 16), (WORD)dosdate, &ftLocal);
		LocalFileTimeToFileTime(&ftLocal, &ftm);
		SetFileTime(hFile, &ftm, &ftLastAcc, &ftm);
		CloseHandle(hFile);
#else
		struct utimbuf ut;
		struct tm newdate;
		newdate.tm_sec = tmu_date.tm_sec;
		newdate.tm_min=tmu_date.tm_min;
		newdate.tm_hour=tmu_date.tm_hour;
		newdate.tm_mday=tmu_date.tm_mday;
		newdate.tm_mon=tmu_date.tm_mon;
		if (tmu_date.tm_year > 1900)
			newdate.tm_year=tmu_date.tm_year - 1900;
		else
			newdate.tm_year=tmu_date.tm_year ;
		newdate.tm_isdst=-1;

		ut.actime=ut.modtime=mktime(&newdate);
		utime(filename,&ut);
#endif
	}

	void CFileMgr::CreateDir(const char* pDir, bool isDir)
	{
		char dirName[260] = { 0 };
		if (pDir)
			strcpy(dirName, pDir);
		else
			return;

		for (int i = 0; i < (int)strlen(dirName); i++)
		{
			if (dirName[i] == '\\')
				dirName[i] = '/';
		}
		int len = strlen(dirName);
		if (isDir)
		{
			if (dirName[len - 1] != '/')
			{
				dirName[len] = '/';
				len += 1;
			}
		}

		for (int i = 1; i < len; i++)
		{
			if (dirName[i] == '/')
			{
				dirName[i] = 0;
#ifdef WIN32
				if (_access(dirName, F_OK))
				{
					_mkdir(dirName);
				}
#else
				if(  access(dirName,F_OK)!=0   )
				{
					if(mkdir(dirName, 0755)==-1)
						return;
				}
#endif
				dirName[i] = '/';
			}
		}
	}

	void CFileMgr::GetFileListFromDir(const std::string dir, const std::string pattern, std::vector<std::string> &files)
	{
#ifdef WIN32
		WIN32_FIND_DATAA FindFileData;

		HANDLE hFind = FindFirstFileA((dir + "\\*.*").c_str(), &FindFileData);
		if (hFind == INVALID_HANDLE_VALUE)
			return;
		do {
			if (FindFileData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
			{
				if (FindFileData.cFileName[0] != '.')
				{
					char pszTemp[260] = { 0 };
					sprintf(pszTemp, "%s\\%s", dir.c_str(), FindFileData.cFileName);
					GetFileListFromDir(pszTemp, pattern, files);
				}
			}
			else
			{
				if (strstr(FindFileData.cFileName, pattern.c_str()))
				{
					std::string str_file = dir + "\\" + FindFileData.cFileName;
					files.push_back(str_file);
				}
			}

		} while (FindNextFileA(hFind, &FindFileData) != 0);
		FindClose(hFind);
#else
		DIR             *pDir ;  
		dirent		    *ent  ;
		char            childpath[512];

		pDir=opendir(dir.c_str()); 
		if(!pDir)
			return ;
		memset(childpath,0,sizeof(childpath));  

		while((ent=readdir(pDir))!=NULL)  
		{
			if(ent->d_type & DT_DIR)//如果是目录,遍历子目录
			{  
				if(strcmp(ent->d_name,".")==0 || strcmp(ent->d_name,"..")==0)  
					continue;//跳过这两个目录

				sprintf(childpath,"%s\\%s",dir.c_str(),ent->d_name);
				GetFileListFromDir(childpath,pattern,files);
			}  
			else
			{
				if (strstr(ent->d_name,pattern.c_str()))
				{
					std::string str_file = dir + "\\" + ent->d_name;
					files.push_back(str_file);
				}
			}
		}
		closedir(pDir); 
#endif
	}

	bool CFileMgr::IsFileExits(const char* filename)
	{
		if (!filename)
			return false;

		FILE* fp = fopen(filename, "r");
		if (fp)
		{
			fclose(fp);
			return true;
		}
		return false;
	}

	void CFileMgr::MoveFileG(const char* dst, const char* src)
	{
		if (dst&&src)
			rename(dst, src);
	}

	void CFileMgr::RemoveFile(const char* dst)
	{
		if (dst)
			remove(dst);
	}

	bool CFileMgr::RemoveDir(const std::string& dir)
	{
		bool ret = true;
#ifdef WIN32
		WIN32_FIND_DATAA FindFileData;
		HANDLE hFind = FindFirstFileA((dir + "/*.*").c_str(), &FindFileData);
		if (hFind == INVALID_HANDLE_VALUE)
			return ret;
		do {
			if (FindFileData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
			{
				if (strcmp(FindFileData.cFileName, ".") == 0 || strcmp(FindFileData.cFileName, "..") == 0)
					continue;

				char childpath[260] = { 0 };
				sprintf(childpath, "%s/%s", dir.c_str(), FindFileData.cFileName);
				if (CFileMgr::RemoveDir(childpath)){//如果中途尝试某一个文件夹删除失败,那么结束此次所有操作
					ret = false;
					break;
				}
			}
			else
			{
				std::string str_file = dir + "/" + FindFileData.cFileName;
				remove(str_file.c_str());
			}
		} while (FindNextFileA(hFind, &FindFileData) != 0);
		FindClose(hFind);
#else
		DIR *pDir;
		dirent *ent;
		char childpath[512];

		pDir = opendir(dir.c_str());
		if (!pDir)
			return ret;
		memset(childpath, 0, sizeof(childpath));

		while ((ent = readdir(pDir)) != NULL)
		{
			if (ent->d_type & DT_DIR)//如果是目录,遍历子目录
			{
				if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
					continue;//跳过这两个目录

				sprintf(childpath, "%s/%s", dir.c_str(), ent->d_name);
				if (!CFileMgr::RemoveDir(childpath)){//如果中途尝试某一个文件夹删除失败,那么结束此次所有操作
					break;
				}
			}
			else
			{
				std::string str_file = dir + "/" + ent->d_name;
				remove(str_file.c_str());
			}
		}
		closedir(pDir);
#endif
		remove(dir.c_str());
		return ret;
	}
}
