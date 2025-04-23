// CYPlayerDlg.h: 头文件
//

#pragma once


#include "CYPlayer/CYPlayerFactory.hpp"
#include <map>

#define WM_PLAYER_POS  WM_USER+100

// CCYPlayerDlg 对话框
class CCYPlayerDlg : public CDialogEx
{
    // 构造
public:
    CCYPlayerDlg(CWnd* pParent = nullptr);	// 标准构造函数

    // 对话框数据
#ifdef AFX_DESIGN_TIME
    enum
    {
        IDD = IDD_CYPLAYER_DIALOG
    };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

    // 实现
protected:
    HICON m_hIcon;

    // 生成的消息映射函数
    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg void OnClose();
    afx_msg HCURSOR OnQueryDragIcon();
    DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedBtnOpen();
    afx_msg void OnBnClickedBtnPlay();
    afx_msg void OnBnClickedBtnPause();
    afx_msg void OnBnClickedBtnStop();
    afx_msg LRESULT OnPlayerPos(WPARAM wParam, LPARAM lParam);
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
private:
    std::map<int, CString> m_mapList;
    CListBox m_lstPlay;
    CString m_strFilePathName;

public:
    afx_msg void OnLbnDblclkListMedia();
    afx_msg void OnLbnSelchangeListMedia();
    afx_msg void OnNMThemeChangedListMedia(NMHDR* pNMHDR, LRESULT* pResult);

private:
    CYPLAYER_NAMESPACE::EPlayerParam m_objParam;
    CYPLAYER_NAMESPACE::ICYPlayer* m_player = nullptr;
public:
    bool m_bMouseDown = false;
    bool m_bPosDown = false;
    DWORD m_dwStartTime = 0;
    CSliderCtrl m_playerPos;
    CSliderCtrl m_sliderVolume;
    afx_msg void OnBnClickedChkMute();
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnBnClickedChkLoop();
};
