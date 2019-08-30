//
//  LauncherDlg.h
//
//  Created by Luis Cuenca on 6/5/2019.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

// CLauncherDlg dialog
class CLauncherDlg : public CDialog
{
    // Construction
public:
    enum DrawStep {
        DrawLogo = 0,
        DrawLoginLogin,
        DrawLoginErrorOrg,
        DrawLoginErrorCred,
        DrawChoose,
        DrawProcessSetup,
        DrawProcessUpdate,
        DrawProcessFinishHq,
        DrawProcessFinishUpdate,
        DrawProcessUninstall,
        DrawError
    };

    struct TextFormat {
        int size;
        COLORREF color;
        bool isButton;
        bool isBold;
        bool underlined;
    };

    CLauncherDlg(CWnd* pParent = nullptr);
    ~CLauncherDlg();
    virtual BOOL PreTranslateMessage(MSG* pMsg);

    void setDrawDialog(DrawStep step, BOOL isUpdate = FALSE);

// Dialog Data
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_LAUNCHER_DIALOG };
#endif

// Implementation
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    void startProcess();
    void setCustomDialog();
    void setVerticalElement(CWnd* element, int verticalOffset, int heightOffset = 0, bool fromMainWindowBottom = true);    
    BOOL getHQInfo(const CString& orgname);
    DrawStep _drawStep { DrawStep::DrawLogo };
    BOOL getTextFormat(int ResID, TextFormat& formatOut);
    void showWindows(std::vector<CStatic*> windows, bool show);
    POINT getMouseCoords(MSG* pMsg);
    void MarkWindowAsUnpinnable();


    bool _isConsoleRunning { false };
    bool _isInstalling { false };
    bool _isFirstDraw { false };
    bool _showSplash { true };
    
    bool _draggingWindow { false };
    POINT _dragOffset;

    int _splashStep { 0 };
    float _logoRotation { 0.0f };

    HICON m_hIcon;
    CButton m_btnNext;
    CButton m_trouble_link;
    CButton m_terms_link;
    
    CStatic* m_message_label;
    CStatic* m_action_label;
    CStatic* m_message2_label;
    CStatic* m_action2_label;
    CStatic* m_terms;
    CStatic* m_trouble;
    CStatic* m_voxel;
    CStatic* m_progress;

    CEdit m_orgname;
    CEdit m_username;
    CEdit m_password;

    CStatic* m_orgname_banner;
    CStatic* m_username_banner;
    CStatic* m_password_banner;

    CStatic* m_version;

    HWND _applicationWND { 0 };

    void drawBackground(CHwndRenderTarget* pRenderTarget);
    void drawLogo(CHwndRenderTarget* pRenderTarget);
    void drawSmallLogo(CHwndRenderTarget* pRenderTarget);
    void drawVoxel(CHwndRenderTarget* pRenderTarget);
    void drawProgress(CHwndRenderTarget* pRenderTarget, float progress, const D2D1::ColorF& color);

    void prepareLogin(DrawStep step);
    void prepareProcess(DrawStep step);
    void prepareChoose();

    void redrawBanner(const CEdit& edit, CStatic* banner);

    // Generated message map functions
    virtual BOOL OnInitDialog();
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnNextClicked();
    afx_msg void OnTroubleClicked();
    afx_msg void OnTermsClicked();
    afx_msg void OnOrgEditChangeFocus();
    afx_msg void OnUserEditChangeFocus();
    afx_msg void OnPassEditChangeFocus();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    DECLARE_MESSAGE_MAP()

public:
    afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
};
