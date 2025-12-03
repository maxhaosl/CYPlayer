// CYPlayerDlg.cpp: 实现文件
//
#ifdef DEBUG_MEM_CHECK
#include <vld.h>
#endif

#include "pch.h"
#include "framework.h"
#include "CYPlayer.h"
#include "CYPlayerDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

HWND g_hWnd = nullptr;
// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
    CAboutDlg();

    // 对话框数据
#ifdef AFX_DESIGN_TIME
    enum
    {
        IDD = IDD_ABOUTBOX
    };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

    // 实现
protected:
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// CCYPlayerDlg 对话框

CCYPlayerDlg::CCYPlayerDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_CYPLAYER_DIALOG, pParent)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CCYPlayerDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_LIST_MEDIA, m_lstPlay);
    DDX_Control(pDX, IDC_SLIDER_PROCESS, m_playerPos);
    DDX_Control(pDX, IDC_SLIDER_VOLUME, m_sliderVolume);
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CCYPlayerDlg, CDialogEx)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_BTN_OPEN, &CCYPlayerDlg::OnBnClickedBtnOpen)
    ON_BN_CLICKED(IDC_BTN_PLAY, &CCYPlayerDlg::OnBnClickedBtnPlay)
    ON_BN_CLICKED(IDC_BTN_PAUSE, &CCYPlayerDlg::OnBnClickedBtnPause)
    ON_BN_CLICKED(IDC_BTN_STOP, &CCYPlayerDlg::OnBnClickedBtnStop)
    ON_WM_CLOSE()
    ON_LBN_DBLCLK(IDC_LIST_MEDIA, &CCYPlayerDlg::OnLbnDblclkListMedia)
    ON_LBN_SELCHANGE(IDC_LIST_MEDIA, &CCYPlayerDlg::OnLbnSelchangeListMedia)
    ON_NOTIFY(NM_THEMECHANGED, IDC_LIST_MEDIA, &CCYPlayerDlg::OnNMThemeChangedListMedia)
    ON_MESSAGE(WM_PLAYER_POS, &CCYPlayerDlg::OnPlayerPos)
    ON_WM_HSCROLL()
    ON_BN_CLICKED(IDC_CHK_MUTE, &CCYPlayerDlg::OnBnClickedChkMute)
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_BN_CLICKED(IDC_CHK_LOOP, &CCYPlayerDlg::OnBnClickedChkLoop)
END_MESSAGE_MAP()

// CCYPlayerDlg 消息处理程序
bool bSetFileDuration = false;

void PositionCallBack(int64_t nPos, int64_t nFileDuration)
{
    int64_t* pPos = new int64_t();
    *pPos = nPos;
    int64_t* pFileDuration = new int64_t();
    *pFileDuration = nFileDuration;
    ::PostMessage(g_hWnd, WM_PLAYER_POS, (WPARAM)pPos, (LPARAM)pFileDuration);
}

LRESULT CCYPlayerDlg::OnPlayerPos(WPARAM wParam, LPARAM lParam)
{
    int64_t* pPos = (int64_t*)wParam;
    int64_t* pFileDuration = (int64_t*)lParam;

    int64_t nPosSec = *pPos / 1000;
    int64_t nDurSec = *pFileDuration / 1000;

    if (GetTickCount() - m_dwStartTime > 1000)
    {
        m_bPosDown = false;
    }

    if (!m_bMouseDown && !m_bPosDown)
    {
        m_playerPos.SetRange(0, *pFileDuration);
        m_playerPos.SetPos(*pPos);
    }

    delete pPos;
    delete pFileDuration;

    CString str;
    if (!bSetFileDuration)
    {
        int minute = (int)nDurSec / 60;
        int hour = minute / 60;
        int second = (int)nDurSec % 60;
        str.Format("%02d:%02d:%02d", hour, minute, second);
        GetDlgItem(IDC_STC_ALL_TIME)->SetWindowText(str);
    }

    {
        int minute = (int)nPosSec / 60;
        int hour = minute / 60;
        int second = (int)nPosSec % 60;
        str.Format("%02d:%02d:%02d", hour, minute, second);
        GetDlgItem(IDC_STC_CUR_TIME)->SetWindowText(str);
    }

    return 0;
}

BOOL CCYPlayerDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    g_hWnd = m_hWnd;

    // 将“关于...”菜单项添加到系统菜单中。

    // IDM_ABOUTBOX 必须在系统命令范围内。
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);

    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu != nullptr)
    {
        BOOL bNameValid;
        CString strAboutMenu;
        bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
        ASSERT(bNameValid);
        if (!strAboutMenu.IsEmpty())
        {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }

    // 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
    //  执行此操作
    SetIcon(m_hIcon, TRUE);			// 设置大图标
    SetIcon(m_hIcon, FALSE);		// 设置小图标

    AllocConsole(); // 打开控制台资源

    RECT rcClient;
    GetDlgItem(IDC_STC_VIDEO)->GetClientRect(&rcClient);

    // TODO: 在此添加额外的初始化代码
    //m_objApplicaiton.Init(GetDlgItem(IDC_STC_VIDEO)->m_hWnd, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);
    m_player = CYPLAYER_NAMESPACE::CYPlayerFactory::CreatePlayer();
    m_player->Init(&m_objParam);
    m_player->SetWindow(GetDlgItem(IDC_STC_VIDEO)->m_hWnd);
    m_player->SetPositionCallback(PositionCallBack);

    m_sliderVolume.SetRange(0, 100);
    m_sliderVolume.SetPos(100);
    return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CCYPlayerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX)
    {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    }
    else
    {
        CDialogEx::OnSysCommand(nID, lParam);
    }
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CCYPlayerDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this); // 用于绘制的设备上下文

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // 使图标在工作区矩形中居中
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // 绘制图标
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CDialogEx::OnPaint();
    }
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CCYPlayerDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

void CCYPlayerDlg::OnClose()
{
    if (m_player)
    {
        m_player->Stop();
        m_player->UnInit();
    }
    CYPLAYER_NAMESPACE::CYPlayerFactory::DestroyPlayer(m_player);
    //m_objApplicaiton.UnInit();
    FreeConsole();
    CDialogEx::OnClose();
}

void CCYPlayerDlg::OnBnClickedBtnOpen()
{
    CFileDialog dlg(TRUE);///TRUE为OPEN对话框，FALSE为SAVE AS对话框
    if (dlg.DoModal() == IDOK)
    {
        m_strFilePathName = dlg.GetPathName();

        int nPos = m_strFilePathName.ReverseFind(_T('\\'));
        CString strFileName = m_strFilePathName.Mid(nPos + 1);

        int nIndex = m_lstPlay.AddString(strFileName);
        m_mapList[nIndex] = m_strFilePathName;
        m_lstPlay.SetItemData(nIndex, nIndex);
        m_lstPlay.SetCurSel(nIndex);
    }
}

void CCYPlayerDlg::OnBnClickedBtnPlay()
{
    if (m_strFilePathName.IsEmpty())
    {
        MessageBox("没有选择媒体文件", "提示", MB_OK);
        return;
    }

    bSetFileDuration = false;
    if (m_player)
    {
        CYPLAYER_NAMESPACE::EPlayerMediaParam objParam;
        m_player->Open(m_strFilePathName.GetBuffer(), &objParam);
        m_player->Play();
    }
    m_strFilePathName.ReleaseBuffer();

    //     m_objApplicaiton.stream_open(m_strFilePathName.GetBuffer());
    //     m_strFilePathName.ReleaseBuffer();
}

void CCYPlayerDlg::OnBnClickedBtnPause()
{
    if (m_player)
    {
        bool bPaused = false;
        m_player->Pause(&bPaused);
        if (bPaused)
        {
            GetDlgItem(IDC_BTN_PAUSE)->SetWindowText("继续");
        }
        else
        {
            GetDlgItem(IDC_BTN_PAUSE)->SetWindowText("暂停");
        }
    }
}

void CCYPlayerDlg::OnBnClickedBtnStop()
{
    if (m_player)
    {
        if (m_player->GetState() >= CYPLAYER_NAMESPACE::TYPE_STATUS_PLAYING)
        {
            m_player->Stop();
        }
    }
}

void CCYPlayerDlg::OnLbnDblclkListMedia()
{
    TRACE("1\r\n");
}

void CCYPlayerDlg::OnLbnSelchangeListMedia()
{
    // TODO: 在此添加控件通知处理程序代码
    TRACE("2\r\n");
}

void CCYPlayerDlg::OnNMThemeChangedListMedia(NMHDR* pNMHDR, LRESULT* pResult)
{
    // 该功能要求使用 Windows XP 或更高版本。
    // 符号 _WIN32_WINNT 必须 >= 0x0501。
    // TODO: 在此添加控件通知处理程序代码
    TRACE("3\r\n");
    *pResult = 0;
}

void CCYPlayerDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    if (pScrollBar->GetSafeHwnd() == m_sliderVolume.GetSafeHwnd())
    {
        int nPos = m_sliderVolume.GetPos();
        CString str;
        str.Format(_T("当前位置：%d\r\n"), nPos);
        TRACE(str);

        if (m_player)
        {
            m_player->SetVolume(nPos * 1.0 / 100);
        }
    }
    else if (pScrollBar->GetSafeHwnd() == m_playerPos.GetSafeHwnd())
    {
        switch (nSBCode)
        {
        case TB_THUMBTRACK:
        {
            int pos = m_playerPos.GetPos();
            CString str;
            str.Format(_T("拖动中：%d\r\n"), pos);
            TRACE(str);
            m_bMouseDown = true;
            if (m_player)
            {
                m_player->Seek(pos);
            }
        }
        break;
        case TB_THUMBPOSITION:
        {
            int pos = m_playerPos.GetPos();
            CString str;
            str.Format(_T("拖动结束，位置：%d\r\n"), pos);
            TRACE(str);
            m_bMouseDown = false;
            m_bPosDown = false;
        }
        break;
        case TB_LINEUP:
        case TB_LINEDOWN:
        case TB_PAGEUP:
        case TB_PAGEDOWN:
        {
            m_bPosDown = true;
            m_dwStartTime = GetTickCount();
            int pos = m_playerPos.GetPos();
            CString str;
            str.Format(_T("点击了槽位或箭头，位置：%d\r\n"), pos);
            TRACE(str);
        }
        break;
        }
    }

    CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CCYPlayerDlg::OnBnClickedChkMute()
{
    if (m_player)
    {
        m_player->SetMute(((CButton*)GetDlgItem(IDC_CHK_MUTE))->GetCheck() == 1);
    }
}

void CCYPlayerDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
    // TODO: 在此添加消息处理程序代码和/或调用默认值

    CDialogEx::OnLButtonDown(nFlags, point);
}

void CCYPlayerDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
    // TODO: 在此添加消息处理程序代码和/或调用默认值

    CDialogEx::OnLButtonUp(nFlags, point);
}

void CCYPlayerDlg::OnBnClickedChkLoop()
{
    if (m_player)
    {
        m_player->SetLoop(((CButton*)GetDlgItem(IDC_CHK_LOOP))->GetCheck() == 1);
    }
}
