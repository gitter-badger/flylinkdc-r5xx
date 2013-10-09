/*
 * Copyright (C) 2001-2013 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef DCPLUSPLUS_DCPP_FILE_H
#define DCPLUSPLUS_DCPP_FILE_H

#include "Streams.h"

#ifdef _WIN32
//
#else
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <fnmatch.h>
#endif



class File : public IOStream
{
	public:
		enum
		{
			OPEN = 0x01,
			CREATE = 0x02,
			TRUNCATE = 0x04, //-V112
			SHARED = 0x08,
			NO_CACHE_HINT = 0x10
		};
		
#ifdef _WIN32
		enum
		{
			READ = GENERIC_READ,
			WRITE = GENERIC_WRITE,
			RW = READ | WRITE
		};
		
		static uint64_t convertTime(const FILETIME* f);
		
#else // !_WIN32
		
		enum
		{
			READ = 0x01,
			WRITE = 0x02,
			RW = READ | WRITE
		};
		
		// some ftruncate implementations can't extend files like SetEndOfFile,
		// not sure if the client code needs this...
		int extendFile(int64_t len) noexcept;
		
#endif // !_WIN32
		
		File(const tstring& aFileName, int access, int mode, bool isAbsolutePath = true)
		{
			init(aFileName, access, mode, isAbsolutePath); // [1] https://www.box.net/shared/75247d259e1ee4eab670
		}
		File(const string& aFileName, int access, int mode, bool isAbsolutePath = true) // [+] IRainman opt
		{
			init(Text::toT(aFileName), access, mode, isAbsolutePath);
		}
		
		void init(const tstring& aFileName, int access, int mode, bool isAbsolutePath); // [+] IRainman fix
		
		bool isOpen() const noexcept;
		void close() noexcept;
		int64_t getSize() const noexcept;
		void setSize(int64_t newSize);
		
		int64_t getPos() const noexcept;
		void setPos(int64_t pos)  throw(FileException);
		void setEndPos(int64_t pos)  throw(FileException);
		void movePos(int64_t pos)  throw(FileException);
		void setEOF();
		
		size_t read(void* buf, size_t& len);
		size_t write(const void* buf, size_t len);
		size_t flush();
		
		uint64_t getLastWriteTime()const noexcept; //[+]PPA
//		uint32_t getLastModified() const noexcept;

		static void copyFile(const tstring& src, const tstring& target);
		static void copyFile(const string& src, const string& target) // [+] IRainman opt.
		{
			copyFile(Text::toT(src), Text::toT(target));
		}
		static void renameFile(const tstring& source, const tstring& target);
		static void renameFile(const string& source, const string& target) // [+] IRainman opt.
		{
			renameFile(Text::toT(source), Text::toT(target));
		}
		static bool deleteFile(const tstring& aFileName) noexcept
		{
			return ::DeleteFile(formatPath(aFileName).c_str()) != NULL;
		}
		static bool deleteFile(const string& aFileName) noexcept
		{
			return deleteFile(Text::toT(aFileName));
		}
		
#ifndef _CONSOLE
		static size_t bz2CompressFile(const wstring& p_file, const wstring& p_file_bz2);
#endif
		
		static int64_t getSize(const tstring& aFileName) noexcept;
		static int64_t getSize(const string& aFileName) noexcept
		{
			return getSize(Text::toT(aFileName));
		}
		// [+] Greylink
		static uint64_t getTimeStamp(const string& aFileName) throw(FileException);
		static uint64_t getSafeTimeStamp(const string& p_FileName) noexcept
		{
			uint64_t l_time_stamp;
			try
			{
				l_time_stamp = getTimeStamp(p_FileName);
			}
			catch (FileException&)
			{
				l_time_stamp = 12345;
			}
			return l_time_stamp;
		}
		static void setTimeStamp(const string& aFileName, const uint64_t stamp) throw(FileException);
		// [~] Greylink
		
		// [+] IRainman
		static bool isExist(const tstring& aFileName) noexcept;
		static bool isExist(const string& aFileName) noexcept
		{
			return isExist(Text::toT(aFileName));
		}
		// [~] IRainman
		
		//[+]FlylinkDC++ Team
		static bool isExist(const tstring& filename, int64_t& outFileSize, int64_t& outFiletime);
		static bool isExist(const string& filename, int64_t& outFileSize, int64_t& outFiletime)
		{
			return isExist(Text::toT(filename), outFileSize, outFiletime);
		}
		
		static void ensureDirectory(const tstring& aFile) noexcept;
		static void ensureDirectory(const string& aFile) noexcept // [+] IRainman opt.
		{
			dcassert(!aFile.empty());
			ensureDirectory(Text::toT(aFile));
		}
		
		static bool isAbsolute(const string& path) noexcept
		{
			return path.size() > 2 && (path[1] == ':' || path[0] == '/' || path[0] == '\\' || (path[0] == '\\' && path[1] == '\\'));
		}
		
		// [+] IRainman FlylinkDC working with long paths
		// http://msdn.microsoft.com/en-us/library/aa365247(VS.85).aspx#maxpath
		// TODO ������� ����� ������
		// fixed http://code.google.com/p/flylinkdc/issues/detail?id=1019
		static string formatPath(const string& path)
		{
			if (path.size() < MAX_PATH - 1)
				return path;
				
			if (path[1] == '\\' && path[0] == '\\')
				return "\\\\?\\UNC\\" + path.substr(2);
			else
				return "\\\\?\\" + path;
		}
		
		static tstring formatPath(const tstring& path)
		{
			if (path.size() < MAX_PATH - 1)
				return path;
				
			if (path[1] == _T('\\') && path[0] == _T('\\'))
				return _T("\\\\?\\UNC\\") + path.substr(2);
			else
				return _T("\\\\?\\") + path;
		}
		
		virtual ~File() noexcept
		{
			close();
		}
		
		string read(size_t len);
		string read();
		void write(const string& aString)
		{
			write((void*)aString.data(), aString.size());
		}
		static StringList findFiles(const string& path, const string& pattern, bool p_append_path = true);
		static uint64_t currentTime();
		
	protected:
#ifdef _WIN32
		HANDLE h;
#else
		int h;
#endif
};

class FileFindIter
{
	private:
		/** End iterator constructor */
		FileFindIter() : m_handle(INVALID_HANDLE_VALUE)
		{
		}
	public:
		/** Begin iterator constructor, path in utf-8 */
		FileFindIter(const tstring& path) /* [-] IRainman: see init(). [-] : handle(INVALID_HANDLE_VALUE)*/
		{
			init(path);
		}
		FileFindIter(const string& path) /* [-] IRainman: see init(). [-] handle(INVALID_HANDLE_VALUE)*/ // [+] IRainman opt
		{
			init(Text::toT(path));
		}
		
		~FileFindIter();
		
		FileFindIter& operator++();
		bool operator!=(const FileFindIter& rhs) const;
		
		static const FileFindIter end; // [+] IRainman opt.
		
		struct DirData
#ifdef _WIN32
				: public WIN32_FIND_DATA
#endif
		{
			DirData();
			
			string getFileName() const;
			bool isDirectory() const;
			bool isHidden() const;
			bool isLink() const;
			int64_t getSize() const;
			int64_t getLastWriteTime() const;
#ifdef _WIN32 // [+]IRainman
			bool isSystem() const;
			bool isTemporary() const;
			bool isVirtual() const;
#endif  // ~[+]IRainman
			
#ifndef _WIN32
			dirent *m_ent;
			string m_base;
#endif
		};
		
		DirData& operator*()
		{
			return m_data;
		}
		const DirData& operator*() const
		{
			return m_data;
		}
		DirData* operator->()
		{
			return &m_data;
		}
		const DirData* operator->() const
		{
			return &m_data;
		}
		
	private:
#ifdef _WIN32
		HANDLE m_handle;
#else
		DIR* m_dir;
#endif
		void init(const tstring& path);
		DirData m_data;
};

#endif // !defined(FILE_H)

/**
 * @file
 * $Id: File.h 568 2011-07-24 18:28:43Z bigmuscle $
 */