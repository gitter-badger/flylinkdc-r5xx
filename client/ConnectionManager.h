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

#ifndef DCPLUSPLUS_DCPP_CONNECTION_MANAGER_H
#define DCPLUSPLUS_DCPP_CONNECTION_MANAGER_H

#include <boost/asio/ip/address_v4.hpp>
#include "TimerManager.h"

#include "UserConnection.h"
#include "Singleton.h"
#include "ConnectionManagerListener.h"
#include "HintedUser.h"

class SocketException;

class ConnectionQueueItem
#ifdef _DEBUG
	: boost::noncopyable
#endif
{
	public:
		typedef ConnectionQueueItem* Ptr;
		typedef FlyLinkVector<Ptr> List;
		typedef List::const_iterator Iter;
		
		enum State
		{
			CONNECTING,                 // Recently sent request to connect
			WAITING,                    // Waiting to send request to connect
			NO_DOWNLOAD_SLOTS,          // Not needed right now
			ACTIVE                      // In one up/downmanager
		};
		
		ConnectionQueueItem(const HintedUser& aUser, bool aDownload) : token(Util::toString(Util::rand())),
			lastAttempt(0), errors(0), state(WAITING), download(aDownload), m_user(aUser), hubUrl(aUser.hint) { }
			
		const string& getToken() const
		{
			return token;
		}
		GETSET(uint64_t, lastAttempt, LastAttempt);
		GETSET(int, errors, Errors); // Number of connection errors, or -1 after a protocol error
		GETSET(State, state, State);
		GETSET(string, hubUrl, HubUrl); // TODO - ���� �� ������� �������� � �� ����� �������������
		bool getDownload() const
		{
			return download;
		}
		
		UserPtr& getUser()
		{
			return m_user;
		}
		const UserPtr& getUser() const
		{
			return m_user;
		}
	private:
		const string token;
		UserPtr m_user;
		const bool download;
};

class ExpectedMap
{
	public:
#ifdef RIP_USE_CONNECTION_AUTODETECT
		enum DefinedExpectedReason {REASON_DEFAULT, REASON_DETECT_CONNECTION};
#endif
		
		/** Nick -> myNick, hubUrl for expected NMDC incoming connections */
		struct NickHubPair
		{
			const string m_Nick;
			const string m_HubUrl;
#ifdef RIP_USE_CONNECTION_AUTODETECT
			const DefinedExpectedReason reason;
#endif
			
			NickHubPair(const string& p_Nick, const string& p_HubUrl
#ifdef RIP_USE_CONNECTION_AUTODETECT
			            , DefinedExpectedReason p_reason = REASON_DEFAULT
#endif
			           ):
				m_Nick(p_Nick),
				m_HubUrl(p_HubUrl)
#ifdef RIP_USE_CONNECTION_AUTODETECT
				, reason(p_reason)
#endif
			{
			}
		};
		//[~]FlylinkDC
		void add(const string& aNick, const string& aMyNick, const string& aHubUrl
#ifdef RIP_USE_CONNECTION_AUTODETECT
		         , ExpectedMap::DefinedExpectedReason reason = ExpectedMap::REASON_DEFAULT
#endif
		        )
		{
			FastLock l(cs);
			m_expectedConnections.insert(make_pair(aNick, NickHubPair(aMyNick, aHubUrl
#ifdef RIP_USE_CONNECTION_AUTODETECT
			                                                          , reason
#endif
			                                                         )));
		}
		
		NickHubPair remove(const string& aNick)
		{
			FastLock l(cs);
			const auto& i = m_expectedConnections.find(aNick);
			if (i == m_expectedConnections.end())
				return NickHubPair(Util::emptyString, Util::emptyString
#ifdef RIP_USE_CONNECTION_AUTODETECT
				                   , REASON_DEFAULT
#endif
				                  );
				                  
			const NickHubPair tmp = i->second;
			m_expectedConnections.erase(i);
			return tmp;
		}
		
	private:
		/** Nick -> myNick, hubUrl for expected NMDC incoming connections */
		typedef boost::unordered_map<string, NickHubPair> ExpectMap;
		ExpectMap m_expectedConnections;
		
		FastCriticalSection cs; // [!] IRainman opt: use spin lock here.
};

// Comparing with a user...
inline bool operator==(const ConnectionQueueItem::Ptr ptr, const UserPtr& aUser)
{
	return ptr->getUser() == aUser;
}

class ConnectionManager : public Speaker<ConnectionManagerListener>,
	public UserConnectionListener, TimerManagerListener,
	public Singleton<ConnectionManager>
{
	public:
		void nmdcExpect(const string& aNick, const string& aMyNick, const string& aHubUrl
#ifdef RIP_USE_CONNECTION_AUTODETECT
		                , ExpectedMap::DefinedExpectedReason reason = ExpectedMap::REASON_DEFAULT
#endif
		               )
		{
			m_expectedConnections.add(aNick, aMyNick, aHubUrl
#ifdef RIP_USE_CONNECTION_AUTODETECT
			                          , reason
#endif
			                         );
		}
		
		void nmdcConnect(const string& aIPServer, uint16_t aPort, const string& aMyNick, const string& hubUrl, const string& encoding,
#ifdef IRAINMAN_ENABLE_STEALTH_MODE
		                 bool stealth,
#endif
		                 bool secure);
		void nmdcConnect(const string& aIPServer, uint16_t aPort, uint16_t localPort, BufferedSocket::NatRoles natRole, const string& aNick, const string& hubUrl, const string& encoding,
#ifdef IRAINMAN_ENABLE_STEALTH_MODE
		                 bool stealth,
#endif
		                 bool secure);
		void adcConnect(const OnlineUser& aUser, uint16_t aPort, const string& aToken, bool secure);
		void adcConnect(const OnlineUser& aUser, uint16_t aPort, uint16_t localPort, BufferedSocket::NatRoles natRole, const string& aToken, bool secure);
		
		void getDownloadConnection(const UserPtr& aUser);
		void force(const UserPtr& aUser);
		void setUploadLimit(const UserPtr& aUser, int lim);
		
		void disconnect(const UserPtr& aUser);
		void disconnect(const UserPtr& aUser, bool isDownload); // [!] IRainman fix.
		
		void shutdown();
		bool isShuttingDown() const
		{
			return shuttingDown;
		}
		
		/** Find a suitable port to listen on, and start doing it */
		void listen();
		void disconnect() noexcept;
		
		uint16_t getPort() const
		{
			return server ? static_cast<uint16_t>(server->getPort()) : 0;
		}
		uint16_t getSecurePort() const
		{
			return secureServer ? static_cast<uint16_t>(secureServer->getPort()) : 0;
		}
		
		static uint16_t iConnToMeCount;
		// [-] SSA private:
		
		class Server : public BASE_THREAD
		{
			public:
				Server(bool p_secure, uint16_t p_port, const string& p_ip = "0.0.0.0");
				uint16_t getPort() const
				{
					return m_port;
				}
				~Server()
				{
					m_die = true;
					join();
				}
			private:
				int run() noexcept;
				
				Socket m_sock;
				uint16_t m_port;
				string m_ip;
				bool m_secure;
				volatile bool m_die; // [!] IRainman fix: this variable is volatile.
		};
		
		friend class Server;
		// [+] SSA private:
	private:
		static std::unique_ptr<webrtc::RWLockWrapper> g_csConnection;
		static std::unique_ptr<webrtc::RWLockWrapper> g_csDownloads;
		static std::unique_ptr<webrtc::RWLockWrapper> g_csUploads;
		static std::unique_ptr<webrtc::RWLockWrapper> g_csDdosCheck;
		static std::unique_ptr<webrtc::RWLockWrapper> g_csTTHFilter;
		
		/** All ConnectionQueueItems */
		static ConnectionQueueItem::List g_downloads;
		static ConnectionQueueItem::List g_uploads;
		
		/** All active connections */
		static boost::unordered_set<UserConnection*> g_userConnections;
		typedef std::pair<boost::asio::ip::address_v4, boost::asio::ip::address_v4> CFlyDDOSkey; // uint32_t boost::asio::ip::address_v4 �  ����� �����
		class CFlyTickDetect
		{
			public:
				uint64_t m_first_tick;
				uint64_t m_last_tick;
				uint16_t m_count_connect;
				uint16_t m_block_id;
				CFlyTickDetect(): m_first_tick(0), m_last_tick(0), m_count_connect(0), m_block_id(0)
				{
				}
		};
		class CFlyTTHTick : public CFlyTickDetect
		{
		};
		class CFlyDDoSTick : public CFlyTickDetect
		{
			public:
				std::string m_type_block;
				boost::unordered_set<uint16_t> m_ports;
				std::vector<std::string> m_original_query_for_debug;
				CFlyDDoSTick()
				{
				}
				string getPorts() const
				{
					string l_ports;
					string l_sep;
					for (auto i = m_ports.cbegin(); i != m_ports.cend(); ++i)
					{
						l_ports += l_sep;
						l_ports += Util::toString(*i);
						l_sep = ",";
					}
					return " Port: " + l_ports;
				}
		};
		std::map<CFlyDDOSkey, CFlyDDoSTick> m_ddos_map;
		
		boost::unordered_map<string, CFlyTTHTick> m_tth_duplicate_search;
		
#define USING_IDLERS_IN_CONNECTION_MANAGER // [!] IRainman fix: don't disable this.
#ifdef USING_IDLERS_IN_CONNECTION_MANAGER
		UserList m_checkIdle;
#endif
		
		StringList nmdcFeatures;
		StringList adcFeatures;
		
		ExpectedMap m_expectedConnections;
		
		uint64_t m_floodCounter;
		
		Server* server;
		Server* secureServer;
		
		bool shuttingDown;
		
		friend class Singleton<ConnectionManager>;
		ConnectionManager();
		
		~ConnectionManager();
		
		static void setIP(UserConnection* p_uc, ConnectionQueueItem* p_qi); // [+]PPA
		UserConnection* getConnection(bool aNmdc, bool secure) noexcept;
		void putConnection(UserConnection* aConn);
		
		void addUploadConnection(UserConnection* uc);
		void addDownloadConnection(UserConnection* uc);
		
		ConnectionQueueItem* getCQI_L(const HintedUser& aHintedUser, bool download);
		void putCQI_L(ConnectionQueueItem* cqi);
		
		void accept(const Socket& sock, bool secure) noexcept;
		
		bool checkKeyprint(UserConnection *aSource);
		
		void failed(UserConnection* aSource, const string& aError, bool protocolError);
		
	public:
		static bool getCipherNameAndIP(UserConnection* p_conn, string& p_chiper_name, string& p_ip);
		
		bool checkIpFlood(const string& aIPServer, uint16_t aPort, const boost::asio::ip::address_v4& p_ip_hub, const string& userInfo, const string& p_HubInfo);
		bool checkTTHDuplicateSearch(const string& p_search_command);
	private:
	
		void cleanupTTHDuplicateSearch(const uint64_t p_tick);
		void cleanupIpFlood(const uint64_t p_tick);
		
		// UserConnectionListener
		void on(Connected, UserConnection*) noexcept;
		void on(Failed, UserConnection*, const string&) noexcept;
		void on(ProtocolError, UserConnection*, const string&) noexcept;
		void on(CLock, UserConnection*, const string&
#ifdef IRAINMAN_INCLUDE_PK_LOCK_IN_IDENTITY
		        , const string&
#endif
		       ) noexcept;
		void on(Key, UserConnection*, const string&) noexcept;
		void on(Direction, UserConnection*, const string&, const string&) noexcept;
		void on(MyNick, UserConnection*, const string&) noexcept;
		void on(Supports, UserConnection*, StringList &) noexcept;
		
		void on(AdcCommand::SUP, UserConnection*, const AdcCommand&) noexcept;
		void on(AdcCommand::INF, UserConnection*, const AdcCommand&) noexcept;
		void on(AdcCommand::STA, UserConnection*, const AdcCommand&) noexcept;
		
		// TimerManagerListener
		void on(TimerManagerListener::Second, uint64_t aTick) noexcept;
		void on(TimerManagerListener::Minute, uint64_t aTick) noexcept;
		
};

#endif // !defined(CONNECTION_MANAGER_H)

/**
 * @file
 * $Id: ConnectionManager.h 568 2011-07-24 18:28:43Z bigmuscle $
 */
