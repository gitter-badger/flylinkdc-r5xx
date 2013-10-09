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

#include "stdinc.h"
#include "DCPlusPlus.h"
#include "../windows/resource.h"
#include "UserInfo.h"
#include "CFlylinkDBManager.h"

int UserInfo::compareItems(const UserInfo* a, const UserInfo* b, int col)
{
	//PROFILE_THREAD_SCOPED()
	if (a == nullptr || b == nullptr)
		return 0;
	if (col == COLUMN_NICK)
	{
		PROFILE_THREAD_SCOPED_DESC("COLUMN_NICK")
		const bool a_isOp = a->isOP(),
		           b_isOp = b->isOP();
		if (a_isOp && !b_isOp)
			return -1;
		if (!a_isOp && b_isOp)
			return 1;
		if (BOOLSETTING(SORT_FAVUSERS_FIRST))
		{
			const bool a_isFav = FavoriteManager::getInstance()->isFavoriteUser(a->getUser());
			const bool b_isFav = FavoriteManager::getInstance()->isFavoriteUser(b->getUser());
			if (a_isFav && !b_isFav)
				return -1;
			if (!a_isFav && b_isFav)
				return 1;
		}
		// workaround for faster hub loading
		// lstrcmpiA(a->m_identity.getNick().c_str(), b->m_identity.getNick().c_str());
	}
	switch (col)
	{
		case COLUMN_SHARED:
		case COLUMN_EXACT_SHARED:
		{
			PROFILE_THREAD_SCOPED_DESC("COLUMN_SHARED")
			return compare(a->getIdentity().getBytesShared(), b->getIdentity().getBytesShared()); // https://www.box.net/shared/043aea731a61c46047fe
		}
		case COLUMN_SLOTS:
		{
			PROFILE_THREAD_SCOPED_DESC("COLUMN_SLOTS")
			return compare(a->getIdentity().getSlots(), b->getIdentity().getSlots());
		}
		case COLUMN_HUBS:
		{
			PROFILE_THREAD_SCOPED_DESC("COLUMN_HUBS")
			return compare(a->getIdentity().getHubNormalRegOper(), b->getIdentity().getHubNormalRegOper());
		}
#ifdef PPA_INCLUDE_LASTIP_AND_USER_RATIO
		case COLUMN_UPLOAD:
		{
			PROFILE_THREAD_SCOPED_DESC("COLUMN_UPLOAD")
			return compare(a->getUser()->getBytesUploadRAW(), b->getUser()->getBytesUploadRAW());
		}
		case COLUMN_DOWNLOAD:
		{
			PROFILE_THREAD_SCOPED_DESC("COLUMN_DOWNLOAD")
			return compare(a->getUser()->getBytesDownloadRAW(), b->getUser()->getBytesDownloadRAW());
		}
#endif
		case COLUMN_IP:
		{
			PROFILE_THREAD_SCOPED_DESC("COLUMN_IP")
			return compare(Socket::convertIP4(a->getIdentity().getIp()), Socket::convertIP4(b->getIdentity().getIp()));
		}
	}
	{
		PROFILE_THREAD_SCOPED_DESC("COLUMN_DEFAULT")
		return Util::DefaultSort(a->getText(col).c_str(), b->getText(col).c_str());
	}
}

//[+]IRainman merge, moved from class OnlineUsers
tstring UserInfo::getText(int p_col) const
{
	//PROFILE_THREAD_SCOPED();
#ifdef IRAINMAN_USE_HIDDEN_USERS
	// dcassert(isHidden() == false);
#endif
	switch (p_col)
	{
		case COLUMN_NICK:
		{
			return getIdentity().getNickT();
		}
		case COLUMN_SHARED:
		{
			return Util::formatBytesW(getIdentity().getBytesShared());
		}
		case COLUMN_EXACT_SHARED:
		{
			return Util::formatExactSize(getIdentity().getBytesShared());
		}
		case COLUMN_DESCRIPTION:
		{
			return Text::toT(getIdentity().getDescription());
		}
		case COLUMN_APPLICATION:
		{
			return Text::toT(getIdentity().getApplication());
		}
#ifdef IRAINMAN_INCLUDE_FULL_USER_INFORMATION_ON_HUB
		case COLUMN_CONNECTION:
		{
			return getDownloadSpeed();
		}
#endif
#ifdef PPA_INCLUDE_LASTIP_AND_USER_RATIO
		case COLUMN_UPLOAD:
		{
			return getUser()->getUpload();
		}
		case COLUMN_DOWNLOAD:
		{
			return getUser()->getDownload();
		}
#endif // PPA_INCLUDE_LASTIP_AND_USER_RATIO
		case COLUMN_IP:
		{
			return Text::toT(getIdentity().getIp());
		}
		case COLUMN_GEO_LOCATION:
		{
			return getLocation().getDescription();
		}
		case COLUMN_EMAIL:
		{
			return Text::toT(getIdentity().getEmail());
		}
#ifdef IRAINMAN_INCLUDE_FULL_USER_INFORMATION_ON_HUB
# ifdef IRAINMAN_INCLUDE_DETECTION_MANAGER
		case COLUMN_VERSION:
		{
			const auto l_cl = getIdentity().getStringParam("CL")
			                  return Text::toT(l_cl.empty() ? getIdentity().getStringParam("VE") : l_cl);
		}
# endif
		case COLUMN_MODE:
		{
			return (getIdentity().isTcpActive(getClient())) ? _T("A") : _T("P");
		}
#endif
		case COLUMN_HUBS:
		{
			return getHubs();
		}
		case COLUMN_SLOTS:
		{
			return Util::toStringW(getIdentity().getSlots());
		}
#ifdef IRAINMAN_INCLUDE_FULL_USER_INFORMATION_ON_HUB
		case COLUMN_CID:
		{
			return Text::toT("CID:" + getIdentity().getUser()->getCID().toBase32());
		}
#endif
		default:
		{
			dcassert(0);
			return Util::emptyStringT;
		}
	}
}


tstring UserInfo::formatSpeedLimit(const uint32_t limit)
{
	return limit ? Util::formatBytesW(limit) + _T('/') + TSTRING(S) : Util::emptyStringT;
}

tstring UserInfo::getLimit() const
{
	return formatSpeedLimit(getIdentity().getLimit());
}

tstring UserInfo::getDownloadSpeed() const
{
	return formatSpeedLimit(getIdentity().getDownloadSpeed());
}