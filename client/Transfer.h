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

#ifndef DCPLUSPLUS_DCPP_TRANSFER_H_
#define DCPLUSPLUS_DCPP_TRANSFER_H_

#include "forward.h"
#include "TimerManager.h"
#include "Segment.h"
#include "UserConnection.h"

class Transfer
#ifdef _DEBUG
	: private boost::noncopyable
#endif
{
	public:
		enum Type
		{
			TYPE_FILE,
			TYPE_FULL_LIST,
			TYPE_PARTIAL_LIST,
			TYPE_TREE,
			TYPE_LAST
		};
		
		static const string g_type_names[TYPE_LAST];
		static const string g_user_list_name;
		static const string g_user_list_name_bz;
		
		explicit Transfer(UserConnection& p_conn, const string& p_path, const TTHValue& p_tth); // [!] IRainman opt.
		virtual ~Transfer() { }
		int64_t getPos() const
		{
			return m_pos; // [3] https://www.box.net/shared/60b6618fc263e636bb4e
		}
		int64_t getStartPos() const
		{
			return getSegment().getStart();
		}
		void resetPos()
		{
			m_pos = 0;    //http://bazaar.launchpad.net/~dcplusplus-team/dcplusplus/trunk/revision/2153
			m_actual = 0;
		}
		void addPos(int64_t aBytes, int64_t aActual)
		{
			m_pos += aBytes;
			m_actual += aActual;
		}
		/** Record a sample for average calculation */
		void tick(uint64_t p_CurrentTick);//[!]IRainman refactoring transfer mechanism
		int64_t getRunningAverage() const
		{
			return m_runningAverage;
		}
		
		int64_t getActual() const
		{
			return m_actual;
		}
		
		int64_t getSize() const
		{
			return getSegment().getSize();
		}
		void setSize(int64_t size)
		{
			m_segment.setSize(size);
		}
		
		bool getOverlapped() const
		{
			return getSegment().getOverlapped();
		}
		void setOverlapped(bool overlap)
		{
			m_segment.setOverlapped(overlap);
		}
		void setStartPos(int64_t aPos)
		{
			m_startPos = aPos;
			m_pos = aPos;
		}
		
		int64_t getSecondsLeft(const bool wholeFile = false) const;
		
		void getParams(const UserConnection& aSource, StringMap& params) const;
		//[+]FlylinkDC
		UserPtr& getUser()
		{
			return getUserConnection().getUser(); // [9] https://www.box.net/shared/4dd3aa9dc4ef80e9e368
		}
		const UserPtr& getUser() const
		{
			return getUserConnection().getUser();
		}
		//[~]FlylinkDC
		const HintedUser& getHintedUser() const // [!] IRainman opt: add const link.
		{
			return getUserConnection().getHintedUser();
		}
		
		const string& getPath() const
		{
			return m_path;
		}
		const TTHValue& getTTH() const
		{
			return m_tth;
		}
		
		UserConnection& getUserConnection()
		{
			return m_userConnection; //TODO - check
		}
		const UserConnection& getUserConnection() const
		{
			return m_userConnection; //TODO - check
		}
		
		GETSET(Segment, m_segment, Segment);
		GETSET(Type, m_type, Type);
		// [!] IRainman refactoring transfer mechanism
		uint64_t getStart() const
		{
			return m_start;
		}
		void setStart(uint64_t tick)
		{
			m_start = tick;
			FastLock l(m_cs);
			setLastTick(tick);
			m_samples.push_back(Sample(m_start, 0));
		}
		// [-] GETSET(uint64_t, start, Start);
		const uint64_t getLastActivity() const
		{
			return getUserConnection().getLastActivity();
		}
		// [~] IRainman refactoring transfer mechanism
		GETSET(uint64_t, m_lastTick, LastTick);
		const bool m_isSecure;
		const bool m_isTrusted;
	private:
		uint64_t m_start;
		
		typedef std::pair<uint64_t, const int64_t> Sample;
		typedef deque<Sample> SampleList;
		
		SampleList m_samples;
		mutable FastCriticalSection m_cs; // [!]IRainman refactoring transfer mechanism
		
		/** The file being transferred */
#ifdef IRAINMAN_USE_NG_TRANSFERS
		const string& m_path; // TODO: maybe it makes sense to back up the line after the original problem will be eliminated. In the Upload to inheritance so not very nice.
#else
		const string m_path;
#endif
		/** TTH of the file being transferred */
#ifdef IRAINMAN_USE_NG_TRANSFERS
		const TTHValue& m_tth;
#else
		const TTHValue m_tth;
#endif
		/** Bytes transferred over socket */
		int64_t m_actual;
		/** Bytes transferred to/from file */
		int64_t m_pos;
		/** Bytes on last avg update */
		//int64_t last; [-] IRainman refactoring transfer mechanism
		/** Starting position */
		int64_t m_startPos;
		/** Actual speed */
		int64_t m_runningAverage;
		UserConnection& m_userConnection;
};

#endif /*TRANSFER_H_*/