
#include "stdafx.h"

#ifdef FLYLINKDC_USE_SQL_EXPLORER

#include "Resource.h"

#include "../client/QueueManager.h"
#include "../client/ClientManager.h"
#include "FlySQLExplorer.h"
#include "LineDlg.h"
#include "PrivateFrame.h"
#include "dclstGenDlg.h"
#include "MainFrm.h"
#include "../FlyFeatures/flyServer.h"

namespace FlyWindow
{

ItemInfo::ItemInfo()
{
}

ItemInfo::~ItemInfo()
{
}

int ItemInfo::getImageIndex() const
{
	return FileImage::DIR_ICON;
}

FlySQLExplorer* FlySQLExplorer::m_flySQLExplorer = NULL;

FlySQLExplorer::FlySQLExplorer()
	: m_statusContainer(STATUSCLASSNAME, this, m_statusMessageMap)
	, m_treeContainer(WC_TREEVIEW, this, m_controlMessageMap)
	, m_listContainer(WC_LISTVIEW, this, m_controlMessageMap)
{
}

FlySQLExplorer::~FlySQLExplorer()
{
	m_flySQLExplorer = NULL;
}

FlySQLExplorer *FlySQLExplorer::Instance()
{
	if (m_flySQLExplorer == NULL)
	{
		m_flySQLExplorer = new FlySQLExplorer();
		HWND aHWND = m_flySQLExplorer->CreateEx(WinUtil::mdiClient);
		if (aHWND == 0)
		{
			delete m_flySQLExplorer;
		}
	}
	return m_flySQLExplorer;
}

void FlySQLExplorer::setWindowTitle()
{
	SetWindowText(L"FlySQLExplorer");
}

LRESULT FlySQLExplorer::OnCreate(UINT, WPARAM, LPARAM, BOOL& bHandled)
{
	// ������������� ��������� �������
	setWindowTitle();
	
	// ������� ���������
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	m_ctrlStatus.Attach(m_hWndStatusBar);
	m_ctrlStatus.SetFont(Fonts::systemFont);
	m_statusContainer.SubclassWindow(m_ctrlStatus.m_hWnd);
	
	// ������� ������ � ����� ����� ������
	m_ctrlTree.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_HASLINES | TVS_SHOWSELALWAYS | TVS_DISABLEDRAGDROP, WS_EX_CLIENTEDGE, IDC_DIRECTORIES);
	m_ctrlTree.SetBkColor(Colors::bgColor);
	m_ctrlTree.SetTextColor(Colors::textColor);
	if (BOOLSETTING(USE_EXPLORER_THEME)
#ifdef FLYLINKDC_SUPPORT_WIN_2000
	        && CompatibilityManager::IsXPPlus())
#endif
	{
		SetWindowTheme(m_ctrlTree.m_hWnd, L"explorer", NULL);
	}
	m_treeContainer.SubclassWindow(m_ctrlTree);
	
	
	// ������� ������� � ������ ����� ������
	m_ctrlList.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_FILES);
	m_listContainer.SubclassWindow(m_ctrlList);
	SET_EXTENDENT_LIST_VIEW_STYLE(m_ctrlList);
	SET_LIST_COLOR(m_ctrlList);
	
	
	// ������������� ��������� ��� ���������
	SetSplitterExtendedStyle(SPLIT_PROPORTIONAL); // ��� ��������� �������� ����-���������� ������� ����������� �������� �������� ���������������.
	m_nProportionalPos = 2500;
	
	bHandled = FALSE;
	return 1;
}


LRESULT FlySQLExplorer::onClose(UINT, WPARAM, LPARAM, BOOL& bHandled)
{
	DestroyWindow();
	return 0;
}

LRESULT FlySQLExplorer::onSpeaker(UINT, WPARAM wParam, LPARAM, BOOL& bHandled)
{
	switch (wParam)
	{
		case FINISHED:
		{
			m_ctrlTree.EnableWindow(TRUE);
			break;
		}
		case ABORTED:
		{
			PostMessage(WM_CLOSE, 0, 0);
			break;
		}
		default:
		{
			dcassert(0);
			break;
		}
	}
	return 0;
}

void FlySQLExplorer::UpdateLayout(BOOL bResizeBars /* = TRUE */)
{
	RECT rect;
	GetClientRect(&rect);
	UpdateBarsPosition(rect, bResizeBars);
	
	// ������������� ������������ ���������
	SetSplitterRect(&rect);
}

void FlySQLExplorer::runUserCommand(UserCommand& uc)
{
}
}

#endif // FLYLINKDC_USE_SQL_EXPLORER