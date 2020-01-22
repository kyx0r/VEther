/**
	Copyright (c) 2009 James Wynn (james@jameswynn.com)

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
	THE SOFTWARE.
*/

#include "FileWatcherWin32.h"

#if FILEWATCHER_PLATFORM == FILEWATCHER_PLATFORM_WIN32

//#define _WIN32_WINNT 0x0550
#include <windows.h>
#include <sstream>
#include "../../src/startup.h"
#include "../../src/flog.h"
#include "../../src/zone.h"

typedef std::basic_string<TCHAR> String;
String GetErrorMessage(DWORD dwErrorCode)
{
    LPTSTR psz = NULL;
    const DWORD cchMsg = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM
                                         | FORMAT_MESSAGE_IGNORE_INSERTS
                                         | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                       NULL, // (not used with FORMAT_MESSAGE_FROM_SYSTEM)
                                       dwErrorCode,
                                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                       reinterpret_cast<LPTSTR>(&psz),
                                       0,
                                       NULL);
    if (cchMsg > 0)
    {
        // Assign buffer to smart pointer with custom deleter so that memory gets released
        // in case String's c'tor throws an exception.
        auto deleter = [](void* p) { ::HeapFree(::GetProcessHeap(), 0, p); };
        std::unique_ptr<TCHAR, decltype(deleter)> ptrBuffer(psz, deleter);
        return String(ptrBuffer.get(), cchMsg);
    }
    error("Failed to retrieve error message string.");
    return String("");
}

//Replace all strings of type in std::string.
//@@1 target string @@2 pattern string @@3 what to replace pattern with.
std::string replaceAll( const std::string& s, const std::string& f, const std::string& r )
{
	if ( s.empty() || f.empty() || f == r || s.find(f) == std::string::npos )
	{
		return s;
	}
	std::ostringstream build_it;
	size_t i = 0;
	for ( size_t pos; ( pos = s.find( f, i ) ) != std::string::npos; )
	{
		build_it.write( &s[i], pos - i );
		build_it << r;
		i = pos + f.size();
	}
	if ( i != s.size() )
	{
		build_it.write( &s[i], s.size() - i );
	}
	return build_it.str();
}

namespace FW
{
/// Internal watch data
struct WatchStruct
{
	_OVERLAPPED mOverlapped;
	HANDLE mDirHandle;
	BYTE mBuffer[4 * 1024];
	LPARAM lParam;
	DWORD mNotifyFilter;
	bool mStopNow;
	FileWatcherImpl* mFileWatcher;
	FileWatchListener* mFileWatchListener;
	char* mDirName;
	WatchID mWatchid;
	bool mIsRecursive;
};

// forward decl
bool RefreshWatch(WatchStruct* pWatch, bool _clear = false);

/// Unpacks events and passes them to a user defined callback.
void CALLBACK WatchCallback(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
{
	PFILE_NOTIFY_INFORMATION pNotify;
	WatchStruct* pWatch = (WatchStruct*) lpOverlapped;
	if(dwNumberOfBytesTransfered == 0)
		return;

	if (dwErrorCode == ERROR_SUCCESS)
	{
		size_t offset = 0;
			pNotify = (PFILE_NOTIFY_INFORMATION) &pWatch->mBuffer[offset];
			offset += pNotify->NextEntryOffset;

			int requiredSize = WideCharToMultiByte(CP_ACP, 0, pNotify->FileName,
			                                       pNotify->FileNameLength / sizeof(WCHAR), NULL, 0, NULL, NULL);
			if (!requiredSize)
			{
				goto skip;
			}
			PCHAR szFile = new CHAR[requiredSize + 1];
			int count = WideCharToMultiByte(CP_ACP, 0, pNotify->FileName,
			                                pNotify->FileNameLength / sizeof(WCHAR),
			                                szFile, requiredSize, NULL, NULL);
			if (!count)
			{
				delete[] szFile;
				goto skip;
			}
			szFile[requiredSize] = 0;

			pWatch->mFileWatcher->handleAction(pWatch, szFile, pNotify->Action);
			if (szFile != NULL)
			{
				delete[] szFile;
			}
	}
skip:;
	if (!pWatch->mStopNow)
	{
		RefreshWatch(pWatch);
	}
}

/*
ReadDirectoryChangesW fails with ERROR_INVALID_PARAMETER when the buffer length 
is greater than 64 KB and the application is monitoring a directory over the network. 
This is due to a packet size limitation with the underlying file sharing protocols.
*/

/// Refreshes the directory monitoring.
bool RefreshWatch(WatchStruct* pWatch, bool _clear)
{
	return ReadDirectoryChangesW(
	           pWatch->mDirHandle, pWatch->mBuffer, sizeof(pWatch->mBuffer), pWatch->mIsRecursive,
	           pWatch->mNotifyFilter, NULL, &pWatch->mOverlapped, _clear ? NULL : WatchCallback);
}

/// Stops monitoring a directory.
void DestroyWatch(WatchStruct* pWatch)
{
	if (pWatch)
	{
		pWatch->mStopNow = TRUE;

		CancelIo(pWatch->mDirHandle);

		RefreshWatch(pWatch, true);

		if (!HasOverlappedIoCompleted(&pWatch->mOverlapped))
		{
			SleepEx(5, TRUE);
		}

		CloseHandle(pWatch->mOverlapped.hEvent);
		CloseHandle(pWatch->mDirHandle);
		delete pWatch->mDirName;
		zone::Z_Free(pWatch);
	}
}

/// Starts monitoring a directory.
WatchStruct* CreateWatch(LPCSTR szDirectory, bool recursive, DWORD mNotifyFilter)
{
	WatchStruct* pWatch = (WatchStruct*) zone::Z_Malloc(sizeof(WatchStruct));

	pWatch->mDirHandle = CreateFileA(szDirectory, FILE_LIST_DIRECTORY,
	                                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
	                                 OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);

	if (pWatch->mDirHandle != INVALID_HANDLE_VALUE)
	{
		pWatch->mOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		pWatch->mNotifyFilter = mNotifyFilter;
		pWatch->mIsRecursive = recursive;
		if (RefreshWatch(pWatch))
		{
			return pWatch;
		}
		else
		{
			CloseHandle(pWatch->mOverlapped.hEvent);
			CloseHandle(pWatch->mDirHandle);
			error("%s", GetErrorMessage(GetLastError()).c_str());
			zone::Z_Free(pWatch);
			return NULL;
		}
	}

	error("File not found: %s", szDirectory);
	zone::Z_Free(pWatch);
	return NULL;
}

//--------
FileWatcherWin32::FileWatcherWin32()
	: mLastWatchID(0)
{
}

//--------
FileWatcherWin32::~FileWatcherWin32()
{
	WatchMap::iterator iter = mWatches.begin();
	WatchMap::iterator end = mWatches.end();
	for(; iter != end; ++iter)
	{
		DestroyWatch(iter->second);
	}
	mWatches.clear();
}

//--------
WatchID FileWatcherWin32::addWatch(const String& directory, FileWatchListener* watcher, bool recursive)
{
	std::string dir;
	dir = directory.substr(0, directory.find_last_of("\\/"));
	
	WatchMap::iterator iter = mWatches.begin();
	WatchMap::iterator end = mWatches.end();
	for(; iter != end; ++iter)
	{
		if(dir == iter->second->mDirName)
		{
			delete watcher;
			return iter->second->mWatchid; //prevent addition of same dir
		}
	}
	
	WatchID watchid = ++mLastWatchID;

	WatchStruct* watch = CreateWatch(dir.c_str(), recursive,
	                                 FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_FILE_NAME);
									 
	if(watch)
	{		
		watch->mWatchid = watchid;
		watch->mFileWatcher = this;
		watch->mFileWatchListener = watcher;
		watch->mDirName = new char[dir.length()+1];
		strcpy(watch->mDirName, dir.c_str());

		mWatches.insert(std::make_pair(watchid, watch));
	}
	return watchid;
}

//--------
void FileWatcherWin32::removeWatch(const String& directory)
{
	WatchMap::iterator iter = mWatches.begin();
	WatchMap::iterator end = mWatches.end();
	for(; iter != end; ++iter)
	{
		if(directory == iter->second->mDirName)
		{
			removeWatch(iter->first);
			return;
		}
	}
}

//--------
void FileWatcherWin32::removeWatch(WatchID watchid)
{
	WatchMap::iterator iter = mWatches.find(watchid);

	if(iter == mWatches.end())
		return;

	WatchStruct* watch = iter->second;
	mWatches.erase(iter);

	DestroyWatch(watch);
}

//--------
void FileWatcherWin32::update()
{
	MsgWaitForMultipleObjectsEx(0, NULL, 0, QS_ALLINPUT, MWMO_ALERTABLE);
}

//--------
void FileWatcherWin32::handleAction(WatchStruct* watch, const String& filename, unsigned long action)
{
	Action fwAction = {};

	switch(action)
	{
	case FILE_ACTION_RENAMED_NEW_NAME:
	case FILE_ACTION_ADDED:
		fwAction = Actions::Add;
		break;
	case FILE_ACTION_RENAMED_OLD_NAME:
	case FILE_ACTION_REMOVED:
		fwAction = Actions::Delete;
		break;
	case FILE_ACTION_MODIFIED:
		fwAction = Actions::Modified;
		break;
	}

	watch->mFileWatchListener->handleFileAction(watch->mWatchid, watch->mDirName, filename, fwAction);
}

};//namespace FW

#endif//_WIN32
