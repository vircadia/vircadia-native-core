//
//  LauncherDlg.cpp
//
//  Created by Luis Cuenca on 6/5/2019.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "stdafx.h"
#include <regex>
#include "LauncherApp.h"
#include "LauncherDlg.h"

#include <propsys.h>
#include <propkey.h>
#include <d2d1.h>
#pragma comment(lib, "d2d1")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static int ACTION_FONT_SIZE = 40;
static int MESSAGE_FONT_SIZE = 19;
static int FIELDS_FONT_SIZE = 24;
static int BUTTON_FONT_SIZE = 25;
static int TERMS_FONT_SIZE = 17;
static int TROUBLE_FONT_SIZE = 14;

static COLORREF COLOR_GREY = RGB(120, 120, 120);
static COLORREF COLOR_BLACK= RGB(0, 0, 0);
static COLORREF COLOR_WHITE = RGB(255, 255, 255);
static COLORREF COLOR_LIGHTER_GREY = RGB(230, 230, 230);
static COLORREF COLOR_LIGHT_GREY = RGB(200, 200, 200);
static COLORREF COLOR_BLUE = RGB(50, 160, 200);

static CString GRAPHIK_REGULAR = _T("Graphik-Regular");
static CString GRAPHIK_SEMIBOLD = _T("Graphik-Semibold");

static CString TROUBLE_URL = _T("https://www.highfidelity.com/hq-support");
static CString TERMS_URL = _T("https://www.highfidelity.com/termsofservice");

static int SPLASH_DURATION = 100;


CLauncherDlg::CLauncherDlg(CWnd* pParent)
    : CDialog(IDD_LAUNCHER_DIALOG, pParent)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    EnableD2DSupport();
}

CLauncherDlg::~CLauncherDlg() {
    theApp._manager.closeLog();
}

void CLauncherDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_BUTTON_NEXT, m_btnNext);
    DDX_Control(pDX, IDC_TROUBLE_LINK, m_trouble_link);
    DDX_Control(pDX, IDC_TERMS_LINK, m_terms_link);
    DDX_Control(pDX, IDC_ORGNAME, m_orgname);
    DDX_Control(pDX, IDC_USERNAME, m_username);
    DDX_Control(pDX, IDC_PASSWORD, m_password);
    CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CLauncherDlg, CDialog)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_WM_TIMER()
    ON_EN_SETFOCUS(IDC_ORGNAME, &CLauncherDlg::OnOrgEditChangeFocus)
    ON_EN_SETFOCUS(IDC_USERNAME, &CLauncherDlg::OnUserEditChangeFocus)
    ON_EN_SETFOCUS(IDC_PASSWORD, &CLauncherDlg::OnPassEditChangeFocus)
    ON_BN_CLICKED(IDC_BUTTON_NEXT, &CLauncherDlg::OnNextClicked)
    ON_BN_CLICKED(IDC_TROUBLE_LINK, &CLauncherDlg::OnTroubleClicked)
    ON_BN_CLICKED(IDC_TERMS_LINK, &CLauncherDlg::OnTermsClicked)
    ON_WM_CTLCOLOR()
    ON_WM_DRAWITEM()
    ON_WM_SETCURSOR()
END_MESSAGE_MAP()

// CLauncherDlg message handlers

BOOL CLauncherDlg::OnInitDialog() {
    CDialog::OnInitDialog();
    MarkWindowAsUnpinnable();
    SetIcon(m_hIcon, TRUE);         // Set big icon
    SetIcon(m_hIcon, FALSE);        // Set small icon

    CFont editFont;
    if (LauncherUtils::getFont(GRAPHIK_REGULAR, FIELDS_FONT_SIZE, true, editFont)) {
        m_orgname.SetFont(&editFont);
        m_username.SetFont(&editFont);
        m_password.SetFont(&editFont);
    }
    CFont buttonFont;
    if (LauncherUtils::getFont(_T("Graphik-Bold"), BUTTON_FONT_SIZE, true, buttonFont)) {
        m_btnNext.SetFont(&editFont);
    }

    m_message_label = (CStatic *)GetDlgItem(IDC_MESSAGE_LABEL);
    m_action_label = (CStatic *)GetDlgItem(IDC_ACTION_LABEL);
    m_message2_label = (CStatic *)GetDlgItem(IDC_MESSAGE2_LABEL);
    m_action2_label = (CStatic *)GetDlgItem(IDC_ACTION2_LABEL);

    m_orgname_banner = (CStatic *)GetDlgItem(IDC_ORGNAME_BANNER);
    m_username_banner = (CStatic *)GetDlgItem(IDC_USERNAME_BANNER);
    m_password_banner = (CStatic *)GetDlgItem(IDC_PASSWORD_BANNER);

    m_terms = (CStatic *)GetDlgItem(IDC_TERMS);
    m_trouble = (CStatic *)GetDlgItem(IDC_TROUBLE);

    m_voxel = (CStatic *)GetDlgItem(IDC_VOXEL);
    m_progress = (CStatic *)GetDlgItem(IDC_PROGRESS);

    m_version = (CStatic *)GetDlgItem(IDC_VERSION);
    CString version;
    version.Format(_T("V.%s"), theApp._manager.getLauncherVersion());
    m_version->SetWindowTextW(version);

    m_voxel->EnableD2DSupport();
    m_progress->EnableD2DSupport();

    m_pRenderTarget = GetRenderTarget();

    SetTimer(1, 2, NULL);
    
    return TRUE;
}

void CLauncherDlg::MarkWindowAsUnpinnable() {
    HWND hwnd = AfxGetMainWnd()->m_hWnd;
    IPropertyStore* pps;
    HRESULT hr = SHGetPropertyStoreForWindow(hwnd, IID_PPV_ARGS(&pps));
    if (SUCCEEDED(hr)) {
        PROPVARIANT var;
        var.vt = VT_BOOL;
        var.boolVal = VARIANT_TRUE;
        hr = pps->SetValue(PKEY_AppUserModel_PreventPinning, var);
        pps->Release();
    }
}

POINT CLauncherDlg::getMouseCoords(MSG* pMsg) {
    POINT pos;
    pos.x = (int)(short)LOWORD(pMsg->lParam);
    pos.y = (int)(short)HIWORD(pMsg->lParam);
    return pos;
}

BOOL CLauncherDlg::PreTranslateMessage(MSG* pMsg) {
    switch (pMsg->message) {
    case WM_KEYDOWN:
        if (pMsg->wParam == 'A' && GetKeyState(VK_CONTROL) < 0) {
            CWnd* wnd = GetFocus();
            CWnd* myWnd = this->GetDlgItem(IDC_ORGNAME);
            if (wnd && (wnd == this->GetDlgItem(IDC_ORGNAME) ||
                wnd == this->GetDlgItem(IDC_USERNAME) ||
                wnd == this->GetDlgItem(IDC_PASSWORD))) {
                ((CEdit*)wnd)->SetSel(0, -1);
            }
            return TRUE;
        } else if (pMsg->wParam == VK_RETURN) {
            OnNextClicked();
            return TRUE;
        } else if (pMsg->wParam == VK_ESCAPE) {
            theApp._manager.onCancel();
        }
        break;
    case WM_LBUTTONDOWN:
        if (pMsg->hwnd == GetSafeHwnd()) {
            _draggingWindow = true;
            _dragOffset = getMouseCoords(pMsg);
            SetCapture();
        }
        break;
    case WM_LBUTTONUP:
        if (_draggingWindow) {
            ReleaseCapture();
            _draggingWindow = false;
        }
        break;
    case WM_MOUSEMOVE:
        if (_draggingWindow) {
            POINT pos = getMouseCoords(pMsg);
            RECT windowRect;
            GetWindowRect(&windowRect);
            int width = windowRect.right - windowRect.left;
            int height = windowRect.bottom - windowRect.top;
            ClientToScreen(&pos);
            MoveWindow(pos.x - _dragOffset.x, pos.y - _dragOffset.y, width, height, FALSE);
        }
        break;
    default:
        break;
    }
    return CDialog::PreTranslateMessage(pMsg);
}

void CLauncherDlg::setCustomDialog() {
    
    LONG lStyle = GetWindowLong(GetSafeHwnd(), GWL_STYLE);
    lStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
    SetWindowLong(GetSafeHwnd(), GWL_STYLE, lStyle);

    LONG lExStyle = GetWindowLong(GetSafeHwnd(), GWL_EXSTYLE);
    lExStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
    SetWindowLong(GetSafeHwnd(), GWL_EXSTYLE, lExStyle);

    SetWindowPos(NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
}

void CLauncherDlg::OnPaint()
{
    CPaintDC dc(this);
    setCustomDialog();
    CDialog::OnPaint();
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CLauncherDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

void CLauncherDlg::startProcess() {
    if (theApp._manager.needsUpdate()) {
        theApp._manager.addToLog(_T("Starting Process Update"));
        setDrawDialog(DrawStep::DrawProcessUpdate);
    } else {
        theApp._manager.addToLog(_T("Starting Process Setup"));
        setDrawDialog(DrawStep::DrawProcessSetup);
    }
    theApp._manager.addToLog(_T("Deleting download directory"));
    CString downloadDir;
    theApp._manager.getAndCreatePaths(LauncherManager::PathType::Download_Directory, downloadDir);
    LauncherUtils::deleteDirectoryOnThread(downloadDir, [&](bool error) {
        if (!error) {
            if (!theApp._manager.isLoggedIn()) {
                theApp._manager.addToLog(_T("Downloading Content"));
                theApp._manager.downloadContent();
            } else {
                theApp._manager.addToLog(_T("Downloading App"));
                theApp._manager.downloadApplication();
            }
        } else {
            theApp._manager.addToLog(_T("Error deleting download directory."));
            theApp._manager.setFailed(true);
        }
    });
}

BOOL CLauncherDlg::getHQInfo(const CString& orgname) {
    CString hash;
    CString lowerOrgName = orgname;
    lowerOrgName.MakeLower();
    LauncherUtils::hMac256(lowerOrgName, LAUNCHER_HMAC_SECRET, hash);
    CString msg;
    msg.Format(_T("Calculated hash: \"%s\" => \"%s\""), lowerOrgName, hash);
    theApp._manager.addToLog(msg);
    return theApp._manager.readOrganizationJSON(hash) == LauncherUtils::ResponseError::NoError;
}

afx_msg void CLauncherDlg::OnTroubleClicked() {
    LauncherUtils::executeOnForeground(TROUBLE_URL, _T(""));
}

afx_msg void CLauncherDlg::OnTermsClicked() {
    LauncherUtils::executeOnForeground(TERMS_URL, _T(""));
}

afx_msg void CLauncherDlg::OnNextClicked() {
    if (_drawStep == DrawStep::DrawChoose) {
        CString displayName;
        m_username.GetWindowTextW(displayName);
        theApp._manager.setDisplayName(displayName);
        theApp._manager.addToLog(_T("Setting display name: " + displayName));
        startProcess();
    } else if (_drawStep == DrawStep::DrawError) {
        theApp._manager.restartLauncher();
    } else if (_drawStep == DrawStep::DrawLoginLogin || 
               _drawStep == DrawStep::DrawLoginErrorCred || 
               _drawStep == DrawStep::DrawLoginErrorOrg) {
        CString token;
        CString username, password, orgname;
        m_orgname.GetWindowTextW(orgname);
        m_username.GetWindowTextW(username);
        m_password.GetWindowTextW(password);
        // trim spaces
        orgname = CString(std::regex_replace(LauncherUtils::cStringToStd(orgname), std::regex("^ +| +$|( ) +"), "$1").c_str());
        username = CString(std::regex_replace(LauncherUtils::cStringToStd(username), std::regex("^ +| +$|( ) +"), "$1").c_str());
        // encode
        username = LauncherUtils::urlEncodeString(username);
        password = LauncherUtils::urlEncodeString(password);
        LauncherUtils::ResponseError error;
        if (orgname.GetLength() > 0 && username.GetLength() > 0 && password.GetLength() > 0) {
            theApp._manager.addToLog(_T("Trying to get organization data"));
            if (getHQInfo(orgname)) {
                theApp._manager.addToLog(_T("Organization data received."));
                theApp._manager.addToLog(_T("Trying to log in with credentials"));
                error = theApp._manager.getAccessTokenForCredentials(username, password);
                if (error == LauncherUtils::ResponseError::NoError) {
                    theApp._manager.addToLog(_T("Logged in correctly."));
                    setDrawDialog(DrawStep::DrawChoose);
                } else if (error == LauncherUtils::ResponseError::BadCredentials) {
                    theApp._manager.addToLog(_T("Bad credentials. Try again"));
                    setDrawDialog(DrawStep::DrawLoginErrorCred);
                } else {
                    theApp._manager.addToLog(_T("Error Reading or retrieving response."));
                    MessageBox(L"Error Reading or retrieving response.", L"Network Error", MB_OK | MB_ICONERROR);
                }
            } else {
                theApp._manager.addToLog(_T("Organization name does not exist."));
                setDrawDialog(DrawStep::DrawLoginErrorOrg);
            }
        }
    }
}

void CLauncherDlg::drawBackground(CHwndRenderTarget* pRenderTarget) {
    CD2DBitmap m_pBitmamBackground(pRenderTarget, IDB_PNG1, _T("PNG"));
    auto size = GetRenderTarget()->GetSize();
    CD2DRectF backRec(0.0f, 0.0f, size.width, size.height);
    GetRenderTarget()->DrawBitmap(&m_pBitmamBackground, backRec);
    pRenderTarget->DrawBitmap(&m_pBitmamBackground, backRec);
}

void CLauncherDlg::drawLogo(CHwndRenderTarget* pRenderTarget) {
    CD2DBitmap m_pBitmamLogo(pRenderTarget, IDB_PNG2, _T("PNG"));
    auto size = pRenderTarget->GetSize();
    int logoWidth = 231;
    int logoHeight = 173;
    float logoPosX = 0.5f * (size.width - logoWidth);
    float logoPosY = 0.5f * (size.height - logoHeight);
    CD2DRectF logoRec(logoPosX, logoPosY, logoPosX + logoWidth, logoPosY + logoHeight);
    pRenderTarget->DrawBitmap(&m_pBitmamLogo, logoRec);
}

void CLauncherDlg::drawSmallLogo(CHwndRenderTarget* pRenderTarget) {
    CD2DBitmap m_pBitmamLogo(pRenderTarget, IDB_PNG5, _T("PNG"));
    auto size = pRenderTarget->GetSize();
    int xPadding = 6;
    int yPadding = 22;
    int logoWidth = 100;
    int logoHeight = 18;
    float logoPosX = size.width - logoWidth - xPadding;
    float logoPosY = size.height - logoHeight - yPadding;
    CD2DRectF logoRec(logoPosX, logoPosY, logoPosX + logoWidth, logoPosY + logoHeight);
    pRenderTarget->DrawBitmap(&m_pBitmamLogo, logoRec);
}

void CLauncherDlg::drawVoxel(CHwndRenderTarget* pRenderTarget) {
    CD2DBitmap m_pBitmamVoxel(pRenderTarget, IDB_PNG4, _T("PNG"));
    auto size = pRenderTarget->GetSize();
    int logoWidth = 132;
    int logoHeight = 134;
    float voxelPosX = 0.5f * (size.width - logoWidth);
    float voxelPosY = 0.5f * (size.height - logoHeight);
    CD2DRectF voxelRec(voxelPosX, voxelPosY, voxelPosX + logoWidth, voxelPosY + logoHeight);
    auto midPoint = D2D1::Point2F(0.5f * size.width, 0.5f * size.height);
    _logoRotation += 2.0f;
    CD2DSolidColorBrush brush(pRenderTarget, D2D1::ColorF(0.0f, 0.0f, 0.0f));
    pRenderTarget->SetTransform(D2D1::Matrix3x2F::Rotation(_logoRotation - 2.0f, midPoint));
    pRenderTarget->FillRectangle(voxelRec, &brush);
    pRenderTarget->SetTransform(D2D1::Matrix3x2F::Rotation(_logoRotation, midPoint));
    pRenderTarget->DrawBitmap(&m_pBitmamVoxel, voxelRec);
    pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
}

void CLauncherDlg::drawProgress(CHwndRenderTarget* pRenderTarget, float progress, const D2D1::ColorF& color) {
    auto size = pRenderTarget->GetSize();
    if (progress == 0.0f) {
        return;
    } else {
        progress = min(1.0f, progress);
    }    
    CRect winRec;
    float fullHeight = (float)size.height;
    float halfHeight = 0.5f * (float)size.height;
    CD2DRectF bkCircleRect1 = CD2DRectF(0.0f, 0.0f, fullHeight, fullHeight);
    float progPos = halfHeight + (float)(size.width - size.height) * progress;
    CD2DRectF bkCircleRect2 = CD2DRectF(progPos - halfHeight, 0.0f, progPos + halfHeight, fullHeight);
    CD2DRectF bkRect = CD2DRectF(halfHeight, 0.0f, progPos, fullHeight);
    CD2DEllipse bkCircle1 = CD2DEllipse(bkCircleRect1);
    CD2DEllipse bkCircle2 = CD2DEllipse(bkCircleRect2);
    CD2DSolidColorBrush brush(pRenderTarget, color);
    pRenderTarget->FillEllipse(bkCircle1, &brush);
    pRenderTarget->FillEllipse(bkCircle2, &brush);
    pRenderTarget->FillRectangle(bkRect, &brush);
}

void CLauncherDlg::showWindows(std::vector<CStatic*> windows, bool show) {
    for (auto window : windows) {
        window->ShowWindow(show ? SW_SHOW : SW_HIDE);
    }
}

void CLauncherDlg::prepareLogin(DrawStep step) {
    m_voxel->ShowWindow(SW_HIDE);
    m_progress->ShowWindow(SW_HIDE);
    m_orgname_banner->SetWindowTextW(_T("Organization Name"));
    m_username_banner->SetWindowTextW(_T("Username"));
    m_password_banner->SetWindowTextW(_T("Password"));
    CString editText;
    m_orgname.GetWindowTextW(editText);
    m_orgname_banner->ShowWindow(editText.GetLength() == 0 ? SW_SHOW : SW_HIDE);
    m_username.GetWindowTextW(editText);
    m_username_banner->ShowWindow(editText.GetLength() == 0 ? SW_SHOW : SW_HIDE);
    m_password.GetWindowTextW(editText);
    m_password_banner->ShowWindow(editText.GetLength() == 0 ? SW_SHOW : SW_HIDE);
    m_orgname.ShowWindow(SW_SHOW);
    m_username.ShowWindow(SW_SHOW);
    m_password.ShowWindow(SW_SHOW);
    CString actionText = step == DrawStep::DrawLoginLogin ? _T("Please log in") : _T("Uh-oh, we have a problem");
    CString messageText = step == DrawStep::DrawLoginLogin ? _T("Be sure you've uploaded your Avatar before signing in.") :
        step == DrawStep::DrawLoginErrorCred ? _T("There is a problem with your credentials.\n Please try again.") : _T("There is a problem with your Organization name.\n Please try again.");
    m_action_label->SetWindowTextW(actionText);
    m_message_label->SetWindowTextW(messageText);
    m_action_label->ShowWindow(SW_SHOW);
    m_message_label->ShowWindow(SW_SHOW);
    m_btnNext.ShowWindow(SW_SHOW);
    m_trouble->SetWindowTextW(_T("Having Trouble?"));
    m_trouble->ShowWindow(SW_SHOW);
    m_trouble_link.ShowWindow(SW_SHOW);
}

void CLauncherDlg::prepareChoose() {
    m_orgname.ShowWindow(SW_HIDE);
    m_username.SetWindowTextW(_T(""));
    m_username_banner->SetWindowTextW(_T("Display Name"));
    CString editText;
    m_username.GetWindowTextW(editText);
    m_username_banner->ShowWindow(editText.GetLength() == 0 ? SW_SHOW : SW_HIDE);
    m_password.ShowWindow(SW_HIDE);
    m_orgname_banner->ShowWindow(SW_HIDE);
    m_password_banner->ShowWindow(SW_HIDE);
    m_action_label->SetWindowTextW(_T("Choose a display name"));
    m_message_label->SetWindowTextW(_T("This is the name that your teammates will see."));
    m_terms->ShowWindow(SW_SHOW);
    m_terms_link.ShowWindow(SW_SHOW);
    m_terms->SetWindowTextW(_T("By signing in, you agree to the High Fidelity"));
    m_terms_link.SetWindowTextW(_T("Terms of Service"));
    setVerticalElement(&m_btnNext, -35, 0, false);
    m_btnNext.ShowWindow(SW_SHOW);
}

void CLauncherDlg::prepareProcess(DrawStep step) {
    m_trouble->ShowWindow(SW_HIDE);
    m_trouble_link.ShowWindow(SW_HIDE);
    m_terms->ShowWindow(SW_HIDE);
    m_terms_link.ShowWindow(SW_HIDE);
    m_orgname_banner->ShowWindow(SW_HIDE);
    m_username_banner->ShowWindow(SW_HIDE);
    m_password_banner->ShowWindow(SW_HIDE);
    m_orgname.ShowWindow(SW_HIDE);
    m_username.ShowWindow(SW_HIDE);
    m_password.ShowWindow(SW_HIDE);
    m_action_label->SetWindowTextW(_T(""));
    m_message_label->SetWindowTextW(_T(""));
    m_btnNext.ShowWindow(SW_HIDE);
    m_action_label->ShowWindow(SW_HIDE);
    m_message_label->ShowWindow(SW_HIDE);
    m_voxel->ShowWindow(SW_SHOW);
    m_progress->ShowWindow(SW_SHOW);
    CString actionText = _T("");
    CString messageText = _T("");
    
    switch (step) {
    case DrawStep::DrawProcessSetup:
        actionText = _T("We're building your virtual HQ");
        messageText = _T("Set up may take several minutes.");
        break;
    case DrawStep::DrawProcessUpdate:
        actionText = _T("Getting updates...");
        messageText = _T("We're getting the latest and greatest for you, one sec.");
        break;
    case DrawStep::DrawProcessFinishHq:
        actionText = _T("Your new HQ is all setup");
        messageText = _T("Thanks for being patient.");
        break;
    case DrawStep::DrawProcessFinishUpdate:
        actionText = _T("You're good to go!");
        messageText = _T("Thanks for being patient.");
        break;
    case DrawStep::DrawProcessUninstall:
        actionText = _T("Uninstalling...");
        messageText = _T("It'll take one sec.");
        break;
    case DrawStep::DrawError:
        actionText = _T("Uh oh.");
        messageText = _T("We seem to have a problem.\nPlease restart HQ.");
        setVerticalElement(m_message2_label, 0, 5, false);
        setVerticalElement(&m_btnNext, 10);
        m_btnNext.ShowWindow(SW_SHOW);
        m_progress->ShowWindow(SW_HIDE);
        break;
    default:
        break;
    }
    m_action2_label->SetWindowTextW(actionText);
    m_message2_label->SetWindowTextW(messageText);
    m_action2_label->ShowWindow(SW_SHOW);
    m_message2_label->ShowWindow(SW_SHOW);
}

BOOL CLauncherDlg::getTextFormat(int resID, TextFormat& formatOut) {
    // Set default values for message
    BOOL isText = TRUE;
    formatOut.color = COLOR_LIGHT_GREY;
    formatOut.isBold = false;
    formatOut.isButton = false;
    formatOut.size = MESSAGE_FONT_SIZE;
    formatOut.underlined = false;
    
    switch (resID) {
    case IDC_VOXEL:
    case IDD_LAUNCHER_DIALOG:
        isText = FALSE;
    case IDC_MESSAGE_LABEL:
    case IDC_MESSAGE2_LABEL:
        // Default values
        break;
    case IDC_ACTION_LABEL:
    case IDC_ACTION2_LABEL:
        formatOut.size = ACTION_FONT_SIZE;
        formatOut.isBold = true;
        formatOut.color = COLOR_LIGHTER_GREY;
        break;
    case IDC_USERNAME:
    case IDC_PASSWORD:
    case IDC_ORGNAME:
        formatOut.color = COLOR_WHITE;
        formatOut.size = FIELDS_FONT_SIZE;
        formatOut.underlined = true;
        break;
    case IDC_USERNAME_BANNER:
    case IDC_PASSWORD_BANNER:
    case IDC_ORGNAME_BANNER:
        formatOut.size = FIELDS_FONT_SIZE;
        formatOut.color = COLOR_GREY;
        break;
    case IDC_VERSION:
    case IDC_TERMS:
        formatOut.size = TERMS_FONT_SIZE;
        break;
    case IDC_TERMS_LINK:
        formatOut.size = TERMS_FONT_SIZE;
        formatOut.isBold = true;
        break;
    case IDC_TROUBLE:
        formatOut.size = TROUBLE_FONT_SIZE;
        formatOut.color = COLOR_BLUE;
        break;
    }
    return isText;
}

HBRUSH CLauncherDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
    TextFormat textFormat;
    int resId = pWnd->GetDlgCtrlID();
    if (getTextFormat(resId, textFormat)) {
        pDC->SetTextColor(textFormat.color);
        pDC->SetBkMode(TRANSPARENT);
        CFont textFont;
        CString fontFamily = textFormat.isBold ? GRAPHIK_SEMIBOLD : GRAPHIK_REGULAR;
        if (LauncherUtils::getFont(fontFamily, textFormat.size, textFormat.isBold, textFont)) {
            pDC->SelectObject(&textFont);
        }
        if (textFormat.underlined) {
            CRect rect;
            pWnd->GetClientRect(&rect);
            int borderThick = 1;
            int padding = 4;
            CRect lineRect = CRect(rect.left + padding, rect.bottom, rect.right - padding, rect.bottom + borderThick);
            lineRect.MoveToY(lineRect.bottom + 1);
            pDC->FillSolidRect(lineRect, COLOR_GREY);
            
        }
    } 
    return (HBRUSH)GetStockObject(BLACK_BRUSH);
}

void CLauncherDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
    CDC dc;
    dc.Attach(lpDrawItemStruct->hDC);
    CRect rect = lpDrawItemStruct->rcItem;
    CRect defrect = rect;
    CString btnName = _T("");
    int xpan = 0;
    if (nIDCtl == IDC_BUTTON_NEXT) {
        if (_drawStep == DrawStep::DrawChoose || _drawStep == DrawStep::DrawLoginLogin) {
            btnName += _drawStep == DrawStep::DrawLoginLogin ? _T("LOG IN") : _T("NEXT");
            int xpan = -20;
            defrect = CRect(rect.left - xpan, rect.top, rect.right + xpan, rect.bottom);
        } else if (_drawStep == DrawStep::DrawError) {
            btnName += _T("RESTART");
        } else {
            btnName += _T("TRY AGAIN");
        }
        int borderThick = 2;
        dc.FillSolidRect(rect, COLOR_BLACK);
        dc.FillSolidRect(defrect, COLOR_WHITE);
        defrect.DeflateRect(borderThick, borderThick, borderThick, borderThick);
        dc.FillSolidRect(defrect, COLOR_BLACK);
        UINT state = lpDrawItemStruct->itemState;
        dc.SetTextColor(COLOR_WHITE);

        CFont buttonFont;
        if (LauncherUtils::getFont(GRAPHIK_SEMIBOLD, BUTTON_FONT_SIZE, true, buttonFont)) {
            dc.SelectObject(buttonFont);
        }
        dc.DrawText(btnName, CRect(rect.left, rect.top + 4, rect.right, rect.bottom - 8), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        dc.Detach();
    } else if (nIDCtl == IDC_TROUBLE_LINK) {
        dc.FillSolidRect(rect, COLOR_BLACK);
        dc.SetTextColor(COLOR_BLUE);
        CFont buttonFont;
        if (LauncherUtils::getFont(GRAPHIK_SEMIBOLD, TROUBLE_FONT_SIZE, true, buttonFont)) {
            dc.SelectObject(buttonFont);
        }
        dc.DrawText(_T("Having Trouble"), CRect(rect.left, rect.top, rect.right, rect.bottom), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    } else if (nIDCtl == IDC_TERMS_LINK) {
        dc.FillSolidRect(rect, COLOR_BLACK);
        dc.SetTextColor(COLOR_LIGHT_GREY);
        CFont buttonFont;
        if (LauncherUtils::getFont(GRAPHIK_SEMIBOLD, TERMS_FONT_SIZE, true, buttonFont)) {
            dc.SelectObject(buttonFont);
        }
        dc.DrawText(_T("Terms of Service"), CRect(rect.left, rect.top, rect.right, rect.bottom), DT_LEFT | DT_TOP | DT_SINGLELINE);
    }

}

void CLauncherDlg::redrawBanner(const CEdit& edit, CStatic* banner) {
    CString editText;
    edit.GetWindowTextW(editText);
    if (editText.GetLength() == 0) {
        banner->Invalidate();
    }
}

void CLauncherDlg::OnOrgEditChangeFocus() {
    redrawBanner(m_username, m_username_banner);
    redrawBanner(m_password, m_password_banner);
}

void CLauncherDlg::OnUserEditChangeFocus() {
    redrawBanner(m_orgname, m_orgname_banner);
    redrawBanner(m_password, m_password_banner);
}

void CLauncherDlg::OnPassEditChangeFocus() {
    redrawBanner(m_orgname, m_orgname_banner);
    redrawBanner(m_username, m_username_banner);
}
BOOL CLauncherDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    if (pWnd->IsKindOf(RUNTIME_CLASS(CButton))) {
        ::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_HAND));
        return TRUE;
    }
    return CDialog::OnSetCursor(pWnd, nHitTest, message);
}

void CLauncherDlg::OnTimer(UINT_PTR nIDEvent) {

    if (theApp._manager.hasFailed() && _drawStep != DrawStep::DrawError) {
        theApp._manager.saveErrorLog();
        prepareProcess(DrawStep::DrawError);
        setDrawDialog(DrawStep::DrawError, false);
    }
    if (_drawStep != DrawStep::DrawError) {
        if (_drawStep == DrawStep::DrawProcessSetup ||
            _drawStep == DrawStep::DrawProcessUpdate ||
            _drawStep == DrawStep::DrawProcessUninstall ||
            _drawStep == DrawStep::DrawProcessFinishHq ||
            _drawStep == DrawStep::DrawProcessFinishUpdate) {
            // Refresh
            setDrawDialog(_drawStep, true);
        }

        if (theApp._manager.needsSelfUpdate()) {
            if (theApp._manager.needsSelfDownload()) {
                theApp._manager.downloadNewLauncher();
            } else {
                if (_splashStep > SPLASH_DURATION && _splashStep < 2 * SPLASH_DURATION) {
                    float progress = (float)(_splashStep - SPLASH_DURATION) / SPLASH_DURATION;
                    if (theApp._manager.willContinueUpdating()) {
                        progress = CONTINUE_UPDATING_GLOBAL_OFFSET * progress;
                        progress = min(progress, CONTINUE_UPDATING_GLOBAL_OFFSET);
                    }                    
                    theApp._manager.updateProgress(LauncherManager::ProcessType::Uninstall, progress);
                    _splashStep++;
                }
                if (theApp._manager.needsRestartNewLauncher()) {
                    if (_splashStep >= 2 * SPLASH_DURATION) {
                        theApp._manager.restartNewLauncher();
                        exit(0);
                    }
                }
            }
        }

        LauncherManager::ContinueActionOnStart continueAction = theApp._manager.getContinueAction();
        if (_showSplash) {
            if (_splashStep == 0) {
                if (theApp._manager.needsUninstall()) {
                    theApp._manager.addToLog(_T("Waiting to uninstall"));
                    setDrawDialog(DrawStep::DrawProcessUninstall);
                } else if (continueAction == LauncherManager::ContinueActionOnStart::ContinueUpdate) {
                    setDrawDialog(DrawStep::DrawProcessUpdate);
                    theApp._manager.updateProgress(LauncherManager::ProcessType::Uninstall, 0.0f);
                } else if (continueAction == LauncherManager::ContinueActionOnStart::ContinueLogIn) {
                    _splashStep = SPLASH_DURATION;
                } else if (continueAction == LauncherManager::ContinueActionOnStart::ContinueFinish) {
                    theApp._manager.updateProgress(LauncherManager::ProcessType::Uninstall, 1.0f);
                    setDrawDialog(DrawStep::DrawProcessFinishUpdate);
                    _splashStep = SPLASH_DURATION;
                    _showSplash = false;
                } else {
                    theApp._manager.addToLog(_T("Start splash screen"));
                    setDrawDialog(DrawStep::DrawLogo);
                }
            } else if (_splashStep > SPLASH_DURATION && !theApp._manager.needsToWait()) {
                _showSplash = false;
                if (theApp._manager.shouldShutDown()) {
                    if (_applicationWND != NULL) {
                        ::SetForegroundWindow(_applicationWND);
                        ::SetActiveWindow(_applicationWND);
                    }
                    if (LauncherUtils::isProcessWindowOpened(L"interface.exe")) {
                        exit(0);
                    }
                } else if (theApp._manager.needsUpdate()) {
                    startProcess();
                } else if (theApp._manager.needsUninstall()) {
                    if (theApp._manager.uninstallApplication()) {
                        theApp._manager.addToLog(_T("HQ uninstalled successfully."));
                        exit(0);
                    } else {
                        theApp._manager.addToLog(_T("HQ failed to uninstall."));
                        theApp._manager.setFailed(true);
                    }
                } else if (theApp._manager.needsSelfUpdate()) {
                    setDrawDialog(DrawStep::DrawProcessUpdate);
                } else {
                    theApp._manager.addToLog(_T("Starting login"));
                    setDrawDialog(DrawStep::DrawLoginLogin);
                }
            } else if (theApp._manager.needsUninstall()) {
                theApp._manager.updateProgress(LauncherManager::ProcessType::Uninstall, (float)_splashStep / SPLASH_DURATION);
            }
            _splashStep++;
        } else if (theApp._manager.shouldShutDown()) {
            if (LauncherUtils::isProcessWindowOpened(L"interface.exe")) {
                exit(0);
            }
        }
        if (theApp._manager.shouldLaunch()) {
            if (theApp._manager.needsInstall() || theApp._manager.needsUpdate()) {
                auto finishProcess = theApp._manager.needsUpdate() ? DrawStep::DrawProcessFinishUpdate : DrawStep::DrawProcessFinishHq;
                setDrawDialog(finishProcess);
            }
            _applicationWND = theApp._manager.launchApplication();
        }
    }
    if (theApp._manager.needsToSelfInstall()) {
        theApp._manager.tryToInstallLauncher(TRUE);
    }
}

void CLauncherDlg::setVerticalElement(CWnd* element, int verticalOffset, int heightOffset, bool fromMainWindowBottom) {
    CRect elementRec;
    CRect windowRec;
    if (element != NULL) {
        element->GetWindowRect(&elementRec);
        ScreenToClient(&elementRec);
        int offset = verticalOffset;
        if (fromMainWindowBottom) {
            GetWindowRect(&windowRec);
            ScreenToClient(&windowRec);
            int currentDistance = windowRec.bottom - elementRec.bottom;
            offset = currentDistance - verticalOffset;
        }
        elementRec.bottom = elementRec.bottom + offset + heightOffset;
        elementRec.top = elementRec.top + offset;
        element->MoveWindow(elementRec, FALSE);
    }
}

void CLauncherDlg::setDrawDialog(DrawStep step, BOOL isUpdate) {
    _drawStep = step;
    float progress = 0.0f;
    auto m_pRenderTarget = GetRenderTarget();
    auto m_voxelRenderTarget = m_voxel->GetRenderTarget();
    auto m_progressRenderTarget = m_progress->GetRenderTarget();
    switch (_drawStep) {
    case DrawStep::DrawLogo: {
        m_pRenderTarget->BeginDraw();
        drawBackground(m_pRenderTarget);
        drawLogo(m_pRenderTarget);
        m_pRenderTarget->EndDraw();
        CRect redrawRec;
        GetClientRect(redrawRec);
        redrawRec.top = redrawRec.bottom - 30;
        RedrawWindow(redrawRec);
        break;
    }
    case DrawStep::DrawLoginLogin:
    case DrawStep::DrawLoginErrorOrg:
    case DrawStep::DrawLoginErrorCred:
        prepareLogin(_drawStep);
        m_pRenderTarget->BeginDraw();
        drawBackground(m_pRenderTarget);
        drawSmallLogo(m_pRenderTarget);
        m_pRenderTarget->EndDraw();
        RedrawWindow();
        break;
    case DrawStep::DrawChoose:
        prepareChoose();
        m_pRenderTarget->BeginDraw();
        drawBackground(m_pRenderTarget);
        drawSmallLogo(m_pRenderTarget);
        m_pRenderTarget->EndDraw();
        RedrawWindow();
        break;
    case DrawStep::DrawError:
    case DrawStep::DrawProcessFinishHq:
    case DrawStep::DrawProcessFinishUpdate:
    case DrawStep::DrawProcessUpdate:
    case DrawStep::DrawProcessUninstall:
    case DrawStep::DrawProcessSetup:    
        if (!isUpdate) {
            m_voxelRenderTarget->BeginDraw();
            m_voxelRenderTarget->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 1.0f));
            m_voxelRenderTarget->EndDraw();
            m_pRenderTarget->BeginDraw();
            prepareProcess(_drawStep);
            drawBackground(m_pRenderTarget);
            drawSmallLogo(m_pRenderTarget);
            m_pRenderTarget->EndDraw();
            RedrawWindow();
        }        
        m_progressRenderTarget->BeginDraw();
        m_progressRenderTarget->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 1.0f));
        drawProgress(m_progressRenderTarget, 1.0f, D2D1::ColorF(0.2f, 0.2f, 0.2f));
        drawProgress(m_progressRenderTarget, theApp._manager.getProgress(), D2D1::ColorF(0.0f, 0.62f, 0.9f));
        m_progressRenderTarget->EndDraw();
        m_voxelRenderTarget->BeginDraw();
        drawVoxel(m_voxelRenderTarget);
        m_voxelRenderTarget->EndDraw();     
        break;
    default:
        break;
    }
}


