#include "stdafx.h"
#include "WeaselPanel.h"
#include <algorithm>
#include <utility>
#include <ShellScalingApi.h>
#include <VersionHelpers.hpp>
#include <WeaselIPCData.h>

#include "VerticalLayout.h"
#include "HorizontalLayout.h"
#include "FullScreenLayout.h"
#include "VHorizontalLayout.h"

// for IDI_ZH, IDI_EN
#include <resource.h>
#define COLORTRANSPARENT(color) ((color & 0xff000000) == 0)
#define COLORNOTTRANSPARENT(color) ((color & 0xff000000) != 0)
#define TRANS_COLOR 0x00000000
#define D2DCOLOR_FROM_COLORREF(color)                                      \
  D2D1::ColorF((float)GetRValue(color) / 255.0f,                          \
               (float)GetGValue(color) / 255.0f,                          \
               (float)GetBValue(color) / 255.0f,                          \
               (float)((color >> 24) & 0xff) / 255.0f)
#define HALF_ALPHA_COLOR(color) \
  ((((color & 0xff000000) >> 25) & 0xff) << 24) | (color & 0x00ffffff)

#pragma comment(lib, "Shcore.lib")

template <class t0, class t1, class t2>
inline void LoadIconNecessary(t0& a, t1& b, t2& c, int d) {
  if (a == b)
    return;
  a = b;
  if (b.empty())
    c.LoadIconW(d, STATUS_ICON_SIZE, STATUS_ICON_SIZE, LR_DEFAULTCOLOR);
  else
    c = (HICON)LoadImage(NULL, b.c_str(), IMAGE_ICON, STATUS_ICON_SIZE,
                         STATUS_ICON_SIZE, LR_LOADFROMFILE);
}

static inline void ReconfigRoundInfo(IsToRoundStruct& rd,
                                     const int& i,
                                     const int& m_candidateCount) {
  if (i == 0 && m_candidateCount > 1) {
    std::swap(rd.IsTopLeftNeedToRound, rd.IsBottomLeftNeedToRound);
    std::swap(rd.IsTopRightNeedToRound, rd.IsBottomRightNeedToRound);
  }
  if (i == m_candidateCount - 1) {
    std::swap(rd.IsTopLeftNeedToRound, rd.IsBottomLeftNeedToRound);
    std::swap(rd.IsTopRightNeedToRound, rd.IsBottomRightNeedToRound);
  }
}

WeaselPanel::WeaselPanel(weasel::UI& ui)
    : m_layout(NULL),
      m_ctx(ui.ctx()),
      m_octx(ui.octx()),
      m_status(ui.status()),
      m_style(ui.style()),
      m_ostyle(ui.ostyle()),
      m_candidateCount(0),
      m_lastCandidateCount(0),
      m_current_zhung_icon(),
      m_inputPos(CRect()),
      m_sticky(false),
      dpi(96),
      hide_candidates(false),
      pDWR(ui.pdwr()),
      _UICallback(ui.uiCallback()) {
  m_iconDisabled.LoadIconW(IDI_RELOAD, STATUS_ICON_SIZE, STATUS_ICON_SIZE,
                           LR_DEFAULTCOLOR);
  m_iconEnabled.LoadIconW(IDI_ZH, STATUS_ICON_SIZE, STATUS_ICON_SIZE,
                          LR_DEFAULTCOLOR);
  m_iconAlpha.LoadIconW(IDI_EN, STATUS_ICON_SIZE, STATUS_ICON_SIZE,
                        LR_DEFAULTCOLOR);
  m_iconFull.LoadIconW(IDI_FULL_SHAPE, STATUS_ICON_SIZE, STATUS_ICON_SIZE,
                       LR_DEFAULTCOLOR);
  m_iconHalf.LoadIconW(IDI_HALF_SHAPE, STATUS_ICON_SIZE, STATUS_ICON_SIZE,
                       LR_DEFAULTCOLOR);
  HMONITOR hMonitor = MonitorFromRect(m_inputPos, MONITOR_DEFAULTTONEAREST);
  UINT dpiX = 96, dpiY = 96;
  if (hMonitor) {
    GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
    m_hMonitor = hMonitor;
  }
  dpi = dpiX;
  _InitFontRes();
  m_ostyle = m_style;
}

WeaselPanel::~WeaselPanel() {
  _ReleaseDCompResources();
  delete m_layout;
  m_layout = NULL;
  // pDWR.reset();
}

void WeaselPanel::_ResizeWindow() {
  CSize m_size = m_layout->GetContentSize();
  if (m_size.cx == m_windowSize.cx && m_size.cy == m_windowSize.cy)
    return;
  SetWindowPos(NULL, 0, 0, m_size.cx, m_size.cy,
               SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW);
  m_windowSize = m_size;
}

void WeaselPanel::_CreateLayout() {
  if (m_layout != NULL)
    delete m_layout;

  Layout* layout = NULL;
  if (m_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT) {
    layout = new VHorizontalLayout(m_style, m_ctx, m_status, pDWR);
  } else {
    if (m_style.layout_type == UIStyle::LAYOUT_VERTICAL ||
        m_style.layout_type == UIStyle::LAYOUT_VERTICAL_FULLSCREEN) {
      layout = new VerticalLayout(m_style, m_ctx, m_status, pDWR);
    } else if (m_style.layout_type == UIStyle::LAYOUT_HORIZONTAL ||
               m_style.layout_type == UIStyle::LAYOUT_HORIZONTAL_FULLSCREEN) {
      layout = new HorizontalLayout(m_style, m_ctx, m_status, pDWR);
    }

    if (IS_FULLSCREENLAYOUT(m_style)) {
      layout = new FullScreenLayout(m_style, m_ctx, m_status, m_inputPos,
                                    layout, pDWR);
    }
  }
  m_layout = layout;
}

// 更新界面
void WeaselPanel::Refresh() {
  bool should_show_icon =
      (m_status.ascii_mode || !m_status.composing || !m_ctx.aux.empty());
  m_candidateCount = (BYTE)m_ctx.cinfo.candies.size();
  // When the candidate window changes from having content to having no content,
  // reset the sticky state
  if (m_lastCandidateCount > 0 && m_candidateCount == 0) {
    m_sticky = false;
  }
  m_lastCandidateCount = m_candidateCount;
  // check if to hide candidates window
  // show tips status, two kind of situation: 1) only aux strings, don't care
  // icon status; 2)only icon(ascii mode switching)
  bool show_tips =
      (!m_ctx.aux.empty() && m_ctx.cinfo.empty() && m_ctx.preedit.empty()) ||
      (m_ctx.empty() && should_show_icon);
  // show schema menu status: schema_id == L".default"
  bool show_schema_menu = m_status.schema_id == L".default";
  bool margin_negative =
      (DPI_SCALE(m_style.margin_x) < 0 || DPI_SCALE(m_style.margin_y) < 0);
  // when to hide_cadidates?
  // 1. margin_negative, and not in show tips mode( ascii switching / half-full
  // switching / simp-trad switching / error tips), and not in schema menu
  // 2. inline preedit without candidates
  bool inline_no_candidates =
      (m_style.inline_preedit && m_candidateCount == 0) && !show_tips;
  hide_candidates = inline_no_candidates ||
                    (margin_negative && !show_tips && !show_schema_menu);

  // only RedrawWindow if no need to hide candidates window, or
  // inline_no_candidates
  if (!hide_candidates || inline_no_candidates) {
    _InitFontRes();
    _CreateLayout();

    m_layout->DoLayout(CDCHandle(), pDWR);
    _ResizeWindow();
    _RepositionWindow();
    if (m_ctx != m_octx) {
      m_octx = m_ctx;
      RedrawWindow();
    }
  }
}

void WeaselPanel::_InitFontRes(bool forced) {
  HMONITOR hMonitor = MonitorFromRect(m_inputPos, MONITOR_DEFAULTTONEAREST);
  UINT dpiX = 96, dpiY = 96;
  if (hMonitor)
    GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
  // prepare d2d1 resources
  // if style changed, or dpi changed, or pDWR NULL, re-initialize directwrite
  // resources
  if (forced || (pDWR == NULL) || (m_ostyle != m_style) || (dpiX != dpi)) {
    pDWR.reset();
    pDWR = std::make_shared<DirectWriteResources>(m_style, dpiX);
  }
  m_ostyle = m_style;
  dpi = dpiX;
  dpiScaleLayout = (float)dpi / 96.0f;
}

bool WeaselPanel::_EnsureDCompResources() {
  if (m_dcompDevice && m_dcompTarget && m_dcompVisual) {
    if (!m_wicFactory) {
      ::CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                         CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_wicFactory));
    }
    return true;
  }

  _ReleaseDCompResources();

  HRESULT hr = ::CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                                  CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&m_wicFactory));
  if (FAILED(hr))
    m_wicFactory.Reset();

  UINT createFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
  D3D_FEATURE_LEVEL featureLevels[] = {
      D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1,
      D3D_FEATURE_LEVEL_10_0};
  D3D_FEATURE_LEVEL selectedLevel = D3D_FEATURE_LEVEL_10_0;
  ComPtr<ID3D11DeviceContext> d3dContext;
  hr = ::D3D11CreateDevice(
      nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createFlags, featureLevels,
      ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &m_d3dDevice, &selectedLevel,
      &d3dContext);
  if (FAILED(hr)) {
    hr = ::D3D11CreateDevice(
        nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createFlags, featureLevels,
        ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &m_d3dDevice,
        &selectedLevel, &d3dContext);
  }
  if (FAILED(hr))
    return false;

  hr = m_d3dDevice.As(&m_dxgiDevice);
  if (FAILED(hr))
    return false;

  hr = ::D2D1CreateFactory(
      D2D1_FACTORY_TYPE_MULTI_THREADED, __uuidof(ID2D1Factory1),
      reinterpret_cast<void**>(m_d2dFactory1.ReleaseAndGetAddressOf()));
  if (FAILED(hr))
    return false;

  hr = m_d2dFactory1->CreateDevice(m_dxgiDevice.Get(), &m_d2dDevice);
  if (FAILED(hr))
    return false;

  hr = ::DCompositionCreateDevice2(m_d2dDevice.Get(),
                                   IID_PPV_ARGS(&m_dcompDevice));
  if (FAILED(hr))
    return false;

  hr = m_dcompDevice->CreateTargetForHwnd(m_hWnd, TRUE, &m_dcompTarget);
  if (FAILED(hr))
    return false;

  hr = m_dcompDevice->CreateVisual(&m_dcompVisual);
  if (FAILED(hr))
    return false;

  m_surfaceWidth = 0;
  m_surfaceHeight = 0;
  return true;
}

void WeaselPanel::_ReleaseDCompResources() {
  m_dcompSurface.Reset();
  m_dcompVisual.Reset();
  m_dcompTarget.Reset();
  m_dcompDevice.Reset();
  m_d2dDevice.Reset();
  m_d2dFactory1.Reset();
  m_dxgiDevice.Reset();
  m_d3dDevice.Reset();
  m_wicFactory.Reset();
  m_surfaceWidth = 0;
  m_surfaceHeight = 0;
}

void WeaselPanel::_ResizeDCompSurface(const CRect& rc) {
  const UINT width = (std::max)(1, rc.Width());
  const UINT height = (std::max)(1, rc.Height());
  if (m_dcompSurface && width == m_surfaceWidth && height == m_surfaceHeight)
    return;

  m_dcompSurface.Reset();
  HRESULT hr = m_dcompDevice->CreateSurface(
      width, height, DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_ALPHA_MODE_PREMULTIPLIED,
      &m_dcompSurface);
  if (SUCCEEDED(hr)) {
    m_surfaceWidth = width;
    m_surfaceHeight = height;
  }
}

bool WeaselPanel::_DrawStatusIcon(const CRect& iconRect, HICON icon) {
  if (!icon || !m_wicFactory || !pDWR || !pDWR->pRenderTarget)
    return false;

  ComPtr<IWICBitmap> wicBitmap;
  HRESULT hr = m_wicFactory->CreateBitmapFromHICON(icon, &wicBitmap);
  if (FAILED(hr) || !wicBitmap)
    return false;

  ComPtr<ID2D1Bitmap> d2dBitmap;
  hr = pDWR->pRenderTarget->CreateBitmapFromWicBitmap(wicBitmap.Get(), nullptr,
                                                      &d2dBitmap);
  if (FAILED(hr) || !d2dBitmap)
    return false;

  const D2D1_RECT_F dest =
      D2D1::RectF((float)iconRect.left, (float)iconRect.top,
                  (float)iconRect.right, (float)iconRect.bottom);
  pDWR->pRenderTarget->DrawBitmap(
      d2dBitmap.Get(), dest, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
  return true;
}

static HBITMAP CopyDCToBitmap(HDC hDC, LPRECT lpRect) {
  if (!hDC || !lpRect || IsRectEmpty(lpRect))
    return NULL;
  HDC hMemDC;
  HBITMAP hBitmap, hOldBitmap;
  int nX, nY, nX2, nY2;
  int nWidth, nHeight;

  nX = lpRect->left;
  nY = lpRect->top;
  nX2 = lpRect->right;
  nY2 = lpRect->bottom;
  nWidth = nX2 - nX;
  nHeight = nY2 - nY;

  hMemDC = CreateCompatibleDC(hDC);
  hBitmap = CreateCompatibleBitmap(hDC, nWidth, nHeight);
  hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);
  StretchBlt(hMemDC, 0, 0, nWidth, nHeight, hDC, nX, nY, nWidth, nHeight,
             SRCCOPY);
  hBitmap = (HBITMAP)SelectObject(hMemDC, hOldBitmap);

  DeleteDC(hMemDC);
  DeleteObject(hOldBitmap);
  return hBitmap;
}

void WeaselPanel::_CaptureRect(CRect& rect) {
  HDC ScreenDC = ::GetDC(NULL);
  CRect rc;
  GetWindowRect(&rc);
  POINT WindowPosAtScreen = {rc.left, rc.top};
  rect.OffsetRect(WindowPosAtScreen);
  // capture input window
  if (OpenClipboard()) {
    HBITMAP bmp = CopyDCToBitmap(ScreenDC, LPRECT(rect));
    EmptyClipboard();
    SetClipboardData(CF_BITMAP, bmp);
    CloseClipboard();
    DeleteObject(bmp);
  }
  ReleaseDC(ScreenDC);
}

LRESULT WeaselPanel::OnMouseActivate(UINT uMsg,
                                     WPARAM wParam,
                                     LPARAM lParam,
                                     BOOL& bHandled) {
  bHandled = true;
  return MA_NOACTIVATE;
}

LRESULT WeaselPanel::OnMouseWheel(UINT uMsg,
                                  WPARAM wParam,
                                  LPARAM lParam,
                                  BOOL& bHandled) {
  int delta = GET_WHEEL_DELTA_WPARAM(wParam);
  if (_UICallback && delta != 0) {
    bool nextpage = delta < 0;
    _UICallback(NULL, NULL, NULL, &nextpage);
  }
  bHandled = true;
  return 0;
}

LRESULT WeaselPanel::OnLeftClickedUp(UINT uMsg,
                                     WPARAM wParam,
                                     LPARAM lParam,
                                     BOOL& bHandled) {
  if (hide_candidates) {
    bHandled = true;
    return 0;
  }
  CPoint point;
  point.x = GET_X_LPARAM(lParam);
  point.y = GET_Y_LPARAM(lParam);

  ::KillTimer(m_hWnd, AUTOREV_TIMER);
  bar_scale_ = 1.0;
  ptimer = 0;
  {
    // select by click
    CRect rect = m_layout->GetCandidateRect((int)m_ctx.cinfo.highlighted);
    if (m_istorepos)
      rect.OffsetRect(0, m_offsetys[m_ctx.cinfo.highlighted]);
    rect.InflateRect(DPI_SCALE(m_style.hilite_padding_x),
                     DPI_SCALE(m_style.hilite_padding_y));
    if (rect.PtInRect(point)) {
      size_t i = m_ctx.cinfo.highlighted;
      if (_UICallback) {
        m_mouse_entry = false;
        _UICallback(&i, NULL, NULL, NULL);
        if (!m_status.composing)
          DestroyWindow();
      }
    } else {
      RedrawWindow();
    }
  }
  bHandled = true;
  return 0;
}

LRESULT WeaselPanel::OnLeftClickedDown(UINT uMsg,
                                       WPARAM wParam,
                                       LPARAM lParam,
                                       BOOL& bHandled) {
  if (hide_candidates) {
    bHandled = true;
    return 0;
  }
  CPoint point;
  point.x = GET_X_LPARAM(lParam);
  point.y = GET_Y_LPARAM(lParam);

  // capture
  if (m_style.click_to_capture) {
    CRect recth = m_layout->GetCandidateRect((int)m_ctx.cinfo.highlighted);
    if (m_istorepos)
      recth.OffsetRect(0, m_offsetys[m_ctx.cinfo.highlighted]);
    recth.InflateRect(DPI_SCALE(m_style.hilite_padding_x),
                      DPI_SCALE(m_style.hilite_padding_y));
    // capture widow
    if (recth.PtInRect(point))
      _CaptureRect(recth);
    else {
      // if shadow_color transparent, decrease the capture rectangle size
      if (COLORTRANSPARENT(m_style.shadow_color) &&
          DPI_SCALE(m_style.shadow_radius) != 0) {
        CRect crc(rcw);
        int shadow_gap = (DPI_SCALE(m_style.shadow_offset_x) == 0 &&
                          DPI_SCALE(m_style.shadow_offset_y) == 0)
                             ? 2 * DPI_SCALE(m_style.shadow_radius)
                             : DPI_SCALE(m_style.shadow_radius) +
                                   DPI_SCALE(m_style.shadow_radius) / 2;
        int ofx = DPI_SCALE(m_style.hilite_padding_x) +
                              abs(DPI_SCALE(m_style.shadow_offset_x)) +
                              shadow_gap >
                          abs(DPI_SCALE(m_style.margin_x))
                      ? DPI_SCALE(m_style.hilite_padding_x) +
                            abs(DPI_SCALE(m_style.shadow_offset_x)) +
                            shadow_gap - abs(DPI_SCALE(m_style.margin_x))
                      : 0;
        int ofy = DPI_SCALE(m_style.hilite_padding_y) +
                              abs(DPI_SCALE(m_style.shadow_offset_y)) +
                              shadow_gap >
                          abs(DPI_SCALE(m_style.margin_y))
                      ? DPI_SCALE(m_style.hilite_padding_y) +
                            abs(DPI_SCALE(m_style.shadow_offset_y)) +
                            shadow_gap - abs(DPI_SCALE(m_style.margin_y))
                      : 0;
        crc.DeflateRect(m_layout->offsetX - ofx, m_layout->offsetY - ofy);
        _CaptureRect(crc);
      } else {
        _CaptureRect(rcw);
      }
    }
  }
  {
    if (!m_style.inline_preedit && m_candidateCount != 0 &&
        COLORNOTTRANSPARENT(m_style.prevpage_color) &&
        COLORNOTTRANSPARENT(m_style.nextpage_color)) {
      // click prepage
      if (m_ctx.cinfo.currentPage != 0) {
        CRect prc = m_layout->GetPrepageRect();
        if (m_istorepos)
          prc.OffsetRect(0, m_offsety_preedit);
        if (prc.PtInRect(point)) {
          bool nextPage = false;
          if (_UICallback)
            _UICallback(NULL, NULL, &nextPage, NULL);
          bHandled = true;
          return 0;
        }
      }
      // click nextpage
      if (!m_ctx.cinfo.is_last_page) {
        CRect prc = m_layout->GetNextpageRect();
        if (m_istorepos)
          prc.OffsetRect(0, m_offsety_preedit);
        if (prc.PtInRect(point)) {
          bool nextPage = true;
          if (_UICallback)
            _UICallback(NULL, NULL, &nextPage, NULL);
          bHandled = true;
          return 0;
        }
      }
    }
    // select by click relative actions
    for (size_t i = 0; i < m_candidateCount && i < MAX_CANDIDATES_COUNT; ++i) {
      CRect rect = m_layout->GetCandidateRect((int)i);
      if (m_istorepos)
        rect.OffsetRect(0, m_offsetys[i]);
      rect.InflateRect(DPI_SCALE(m_style.hilite_padding_x),
                       DPI_SCALE(m_style.hilite_padding_y));
      if (rect.PtInRect(point)) {
        bar_scale_ = 0.8f;
        // modify highlighted
        if (i != m_ctx.cinfo.highlighted) {
          if (_UICallback)
            _UICallback(NULL, &i, NULL, NULL);
        } else {
          RedrawWindow();
        }
        ptimer = UINT_PTR(this);
        ::SetTimer(m_hWnd, AUTOREV_TIMER, 1000, &WeaselPanel::OnTimer);
        bHandled = true;
        return 0;
      }
    }
  }
  bHandled = true;
  return 0;
}

// UINT_PTR WeaselPanel::ptimer = 0;
VOID CALLBACK WeaselPanel::OnTimer(_In_ HWND hwnd,
                                   _In_ UINT uMsg,
                                   _In_ UINT_PTR idEvent,
                                   _In_ DWORD dwTime) {
  ::KillTimer(hwnd, idEvent);
  WeaselPanel* self = (WeaselPanel*)ptimer;
  ptimer = 0;
  if (self) {
    self->bar_scale_ = 1.0;
    self->RedrawWindow();
  }
}

LRESULT WeaselPanel::OnMouseMove(UINT uMsg,
                                 WPARAM wParam,
                                 LPARAM lParam,
                                 BOOL& bHandled) {
  if (m_style.hover_type == UIStyle::NONE)
    return 0;
  if (m_mouse_entry == false) {
    TRACKMOUSEEVENT tme;
    tme.cbSize = sizeof(TRACKMOUSEEVENT);
    tme.dwFlags = TME_LEAVE;
    tme.dwHoverTime = 20;  // unit: ms
    tme.hwndTrack = m_hWnd;
    TrackMouseEvent(&tme);
  }
  bHandled = true;
  m_mouse_entry = true;
  CPoint point;
  point.x = GET_X_LPARAM(lParam);
  point.y = GET_Y_LPARAM(lParam);

  for (size_t i = 0; i < m_candidateCount && i < MAX_CANDIDATES_COUNT; ++i) {
    CRect rect = m_layout->GetCandidateRect((int)i);
    if (m_istorepos)
      rect.OffsetRect(0, m_offsetys[i]);
    rect.InflateRect(DPI_SCALE(m_style.hilite_padding_x),
                     DPI_SCALE(m_style.hilite_padding_y));
    if (rect.PtInRect(point)) {
      if (i != m_ctx.cinfo.highlighted) {
        if (m_style.hover_type == UIStyle::HoverType::HILITE) {
          if (_UICallback)
            _UICallback(NULL, &i, NULL, NULL);
        } else if (m_hoverIndex != i) {
          m_hoverIndex = static_cast<int>(i);
          InvalidateRect(&rcw, true);
        }
      } else if (m_style.hover_type == UIStyle::HoverType::SEMI_HILITE &&
                 m_hoverIndex != -1) {
        m_hoverIndex = -1;
        InvalidateRect(&rcw, true);
      }
    }
  }
  return 0;
}

LRESULT WeaselPanel::OnMouseLeave(UINT uMsg,
                                  WPARAM wParam,
                                  LPARAM lParam,
                                  BOOL& bHandled) {
  m_hoverIndex = -1;
  InvalidateRect(&rcw, true);
  m_mouse_entry = false;
  return 0;
}

void WeaselPanel::_HighlightText(CDCHandle& dc,
                                 const CRect& rc,
                                 const COLORREF& color,
                                 const COLORREF& shadowColor,
                                 const int& radius,
                                 const BackType& type = BackType::TEXT,
                                 const IsToRoundStruct& rd = IsToRoundStruct(),
                                 const COLORREF& bordercolor = TRANS_COLOR) {
  if (!pDWR || !pDWR->pRenderTarget || !m_layout)
    return;

  const D2D1_RECT_F rcf =
      D2D1::RectF((float)rc.left, (float)rc.top, (float)rc.right,
                  (float)rc.bottom);

  bool roundTopLeft = true;
  bool roundTopRight = true;
  bool roundBottomRight = true;
  bool roundBottomLeft = true;
  float shapeRadius = (float)radius;

  if (rd.Hemispherical && type != BackType::BACKGROUND &&
      NOT_FULLSCREENLAYOUT(m_style)) {
    roundTopLeft = rd.IsTopLeftNeedToRound;
    roundTopRight = rd.IsTopRightNeedToRound;
    roundBottomRight = rd.IsBottomRightNeedToRound;
    roundBottomLeft = rd.IsBottomLeftNeedToRound;
    shapeRadius =
        (float)(DPI_SCALE(m_style.round_corner_ex) -
                (DPI_SCALE(m_style.border) % 2 ? DPI_SCALE(m_style.border) / 2
                                               : 0));
  }

  const bool anyRound = roundTopLeft || roundTopRight || roundBottomRight ||
                        roundBottomLeft;
  const bool allRound = roundTopLeft && roundTopRight && roundBottomRight &&
                        roundBottomLeft;
  ComPtr<ID2D1PathGeometry> mixedRoundPath;
  if (anyRound && !allRound) {
    mixedRoundPath = pDWR->CreateRoundRectGeometry(
        rcf, shapeRadius, shapeRadius, roundTopLeft, roundTopRight,
        roundBottomRight, roundBottomLeft);
  }

  if (DPI_SCALE(m_style.shadow_radius) && COLORNOTTRANSPARENT(shadowColor) &&
      NOT_FULLSCREENLAYOUT(m_style)) {
    pDWR->DrawSoftShadow(
        rcf, shapeRadius, D2DCOLOR_FROM_COLORREF(shadowColor),
        (float)DPI_SCALE(m_style.shadow_radius),
        (float)DPI_SCALE(m_style.shadow_offset_x),
        (float)DPI_SCALE(m_style.shadow_offset_y),
        (float)m_layout->offsetX, (float)m_layout->offsetY, roundTopLeft,
        roundTopRight, roundBottomRight, roundBottomLeft);
  }

  if (COLORNOTTRANSPARENT(color)) {
    if (allRound) {
      pDWR->FillRoundRect(rcf, D2DCOLOR_FROM_COLORREF(color), shapeRadius,
                          shapeRadius, roundTopLeft, roundTopRight,
                          roundBottomRight, roundBottomLeft);
    } else if (anyRound && mixedRoundPath) {
      pDWR->pBrush->SetColor(D2DCOLOR_FROM_COLORREF(color));
      pDWR->pRenderTarget->FillGeometry(mixedRoundPath.Get(),
                                        pDWR->pBrush.Get());
    } else if (anyRound) {
      pDWR->FillRoundRect(rcf, D2DCOLOR_FROM_COLORREF(color), shapeRadius,
                          shapeRadius, roundTopLeft, roundTopRight,
                          roundBottomRight, roundBottomLeft);
    } else {
      pDWR->pBrush->SetColor(D2DCOLOR_FROM_COLORREF(color));
      pDWR->pRenderTarget->FillRectangle(rcf, pDWR->pBrush.Get());
    }
  }

  if (COLORNOTTRANSPARENT(bordercolor) && DPI_SCALE(m_style.border) > 0) {
    const float borderWidth = (float)DPI_SCALE(m_style.border);
    if (type == BackType::BACKGROUND) {
      pDWR->DrawRoundRect(rcf, D2DCOLOR_FROM_COLORREF(bordercolor),
                          borderWidth,
                          (float)DPI_SCALE(m_style.round_corner_ex),
                          (float)DPI_SCALE(m_style.round_corner_ex));
    } else if (type != BackType::TEXT) {
      if (allRound) {
        pDWR->DrawRoundRect(rcf, D2DCOLOR_FROM_COLORREF(bordercolor),
                            borderWidth, shapeRadius, shapeRadius, roundTopLeft,
                            roundTopRight, roundBottomRight, roundBottomLeft);
      } else if (anyRound && mixedRoundPath) {
        pDWR->pBrush->SetColor(D2DCOLOR_FROM_COLORREF(bordercolor));
        pDWR->pRenderTarget->DrawGeometry(mixedRoundPath.Get(),
                                          pDWR->pBrush.Get(), borderWidth);
      } else if (anyRound) {
        pDWR->DrawRoundRect(rcf, D2DCOLOR_FROM_COLORREF(bordercolor),
                            borderWidth, shapeRadius, shapeRadius, roundTopLeft,
                            roundTopRight, roundBottomRight, roundBottomLeft);
      } else {
        pDWR->pBrush->SetColor(D2DCOLOR_FROM_COLORREF(bordercolor));
        pDWR->pRenderTarget->DrawRectangle(rcf, pDWR->pBrush.Get(),
                                           borderWidth);
      }
    }
  }
}

// draw preedit text, text only
bool WeaselPanel::_DrawPreedit(const Text& text,
                               CDCHandle dc,
                               const CRect& rc) {
  bool drawn = false;
  std::wstring const& t = text.str;
  IDWriteTextFormat1* txtFormat = pDWR->pPreeditTextFormat.Get();

  if (!t.empty()) {
    weasel::TextRange range = m_layout->GetPreeditRange();

    if (range.start < range.end) {
      CSize beforeSz = m_layout->GetBeforeSize();
      CSize hilitedSz = m_layout->GetHilitedSize();
      CSize afterSz = m_layout->GetAfterSize();

      int x = rc.left;
      int y = rc.top;

      if (range.start > 0) {
        // zzz
        std::wstring_view str_before = std::wstring_view(t).substr(0, range.start);
        CRect rc_before;
        if (m_layout->IsVerticalPreedit())
          rc_before = CRect(rc.left, y, rc.right, y + beforeSz.cy);
        else
          rc_before = CRect(x, rc.top, rc.left + beforeSz.cx, rc.bottom);
        _TextOut(rc_before, str_before, str_before.length(),
                 m_style.text_color, txtFormat);
        if (m_layout->IsVerticalPreedit())
          y += beforeSz.cy + DPI_SCALE(m_style.hilite_spacing);
        else
          x += beforeSz.cx + DPI_SCALE(m_style.hilite_spacing);
      }
      {
        // zzz[yyy]
        std::wstring_view str_highlight = std::wstring_view(t).substr(range.start, (size_t)range.end - range.start);
        CRect rc_hi;

        if (m_layout->IsVerticalPreedit())
          rc_hi = CRect(rc.left, y, rc.right, y + hilitedSz.cy);
        else
          rc_hi = CRect(x, rc.top, x + hilitedSz.cx, rc.bottom);
        _TextOut(rc_hi, str_highlight, str_highlight.length(),
                 m_style.hilited_text_color, txtFormat);
        if (m_layout->IsVerticalPreedit())
          y += rc_hi.Height() + DPI_SCALE(m_style.hilite_spacing);
        else
          x += rc_hi.Width() + DPI_SCALE(m_style.hilite_spacing);
      }
      if (range.end < static_cast<int>(t.length())) {
        // zzz[yyy]xxx
        std::wstring_view str_after = std::wstring_view(t).substr(range.end);
        CRect rc_after;
        if (m_layout->IsVerticalPreedit())
          rc_after = CRect(rc.left, y, rc.right, y + afterSz.cy);
        else
          rc_after = CRect(x, rc.top, x + afterSz.cx, rc.bottom);
        _TextOut(rc_after, str_after, str_after.length(),
                 m_style.text_color, txtFormat);
      }
    } else {
      CRect rcText(rc.left, rc.top, rc.right, rc.bottom);
      _TextOut(rcText, t, t.length(), m_style.text_color, txtFormat);
    }
    // draw pager mark if not inline_preedit if necessary
    if (m_candidateCount && !m_style.inline_preedit &&
        COLORNOTTRANSPARENT(m_style.prevpage_color) &&
        COLORNOTTRANSPARENT(m_style.nextpage_color)) {
      const std::wstring pre = L"<";
      const std::wstring next = L">";
      CRect prc = m_layout->GetPrepageRect();
      // clickable color / disabled color
      int color =
          m_ctx.cinfo.currentPage ? m_style.prevpage_color : m_style.text_color;
      if (m_istorepos)
        prc.OffsetRect(0, m_offsety_preedit);
      _TextOut(prc, pre, pre.length(), color, txtFormat);

      CRect nrc = m_layout->GetNextpageRect();
      // clickable color / disabled color
      color = m_ctx.cinfo.is_last_page ? m_style.text_color
                                       : m_style.nextpage_color;
      if (m_istorepos)
        nrc.OffsetRect(0, m_offsety_preedit);
      _TextOut(nrc, next, next.length(), color, txtFormat);
    }
    drawn = true;
  }
  return drawn;
}

// draw hilited back color, back only
bool WeaselPanel::_DrawPreeditBack(const Text& text,
                                   CDCHandle dc,
                                   const CRect& rc) {
  bool drawn = false;
  std::wstring const& t = text.str;
  IDWriteTextFormat1* txtFormat = pDWR->pPreeditTextFormat.Get();

  if (!t.empty()) {
    weasel::TextRange range = m_layout->GetPreeditRange();

    if (range.start < range.end) {
      CSize beforeSz = m_layout->GetBeforeSize();
      CSize hilitedSz = m_layout->GetHilitedSize();

      int x = rc.left;
      int y = rc.top;

      if (range.start > 0) {
        if (m_layout->IsVerticalPreedit())
          y += beforeSz.cy + DPI_SCALE(m_style.hilite_spacing);
        else
          x += beforeSz.cx + DPI_SCALE(m_style.hilite_spacing);
      }
      {
        CRect rc_hi;
        if (m_layout->IsVerticalPreedit())
          rc_hi = CRect(rc.left, y, rc.right, y + hilitedSz.cy);
        else
          rc_hi = CRect(x, rc.top, x + hilitedSz.cx, rc.bottom);
        // if preedit rect size smaller than icon, fill the gap to
        // STATUS_ICON_SIZE
        if (m_layout->ShouldDisplayStatusIcon()) {
          if ((m_style.layout_type == UIStyle::LAYOUT_HORIZONTAL ||
               m_style.layout_type == UIStyle::LAYOUT_VERTICAL) &&
              hilitedSz.cy < STATUS_ICON_SIZE)
            rc_hi.InflateRect(0, (STATUS_ICON_SIZE - hilitedSz.cy) / 2);
          if (m_layout->IsVerticalPreedit() &&
              hilitedSz.cx < STATUS_ICON_SIZE)
            rc_hi.InflateRect((STATUS_ICON_SIZE - hilitedSz.cx) / 2, 0);
        }

        rc_hi.InflateRect(DPI_SCALE(m_style.hilite_padding_x),
                          DPI_SCALE(m_style.hilite_padding_y));
        IsToRoundStruct rd = m_layout->GetTextRoundInfo();
        if (m_istorepos) {
          std::swap(rd.IsTopLeftNeedToRound, rd.IsBottomLeftNeedToRound);
          std::swap(rd.IsTopRightNeedToRound, rd.IsBottomRightNeedToRound);
        }
        _HighlightText(dc, rc_hi, m_style.hilited_back_color,
                       m_style.hilited_shadow_color,
                       DPI_SCALE(m_style.round_corner), BackType::TEXT, rd);
      }
    }
    drawn = true;
  }
  return drawn;
}

bool WeaselPanel::_DrawCandidates(CDCHandle& dc, bool back) {
  bool drawn = false;
  const std::vector<Text>& candidates(m_ctx.cinfo.candies);
  const std::vector<Text>& comments(m_ctx.cinfo.comments);
  const std::vector<Text>& labels(m_ctx.cinfo.labels);
  // prevent all text format nullptr
  if (pDWR->pTextFormat.Get() == nullptr &&
      pDWR->pLabelTextFormat.Get() == nullptr &&
      pDWR->pCommentTextFormat.Get() == nullptr) {
    return drawn;
  }
  ComPtr<IDWriteTextFormat1> txtFormat = pDWR->pTextFormat;
  ComPtr<IDWriteTextFormat1> labeltxtFormat = pDWR->pLabelTextFormat;
  ComPtr<IDWriteTextFormat1> commenttxtFormat = pDWR->pCommentTextFormat;
  BackType bkType = BackType::CAND;
  // candidate columns run top to bottom in vertical text mode
  const bool vertical_candidates =
      m_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT;

  CRect rect;
  // draw back color and shadow color with Direct2D
  if (back) {
    // Draw each non-highlighted candidate in one pass. The soft shadow, fill
    // and border are drawn together instead of issuing separate geometry
    // batches for dozens of candidates.
    if (COLORNOTTRANSPARENT(m_style.candidate_shadow_color) ||
        COLORNOTTRANSPARENT(m_style.candidate_back_color) ||
        COLORNOTTRANSPARENT(m_style.candidate_border_color)) {
      std::vector<ShadowShape> candidateShapes;
      candidateShapes.reserve(m_candidateCount);
      for (auto i = 0; i < m_candidateCount && i < MAX_CANDIDATES_COUNT; ++i) {
        if (i == m_ctx.cinfo.highlighted || i == m_hoverIndex)
          continue;
        rect = m_layout->GetCandidateRect((int)i);
        IsToRoundStruct rd = m_layout->GetRoundInfo(i);
        if (m_istorepos) {
          rect.OffsetRect(0, m_offsetys[i]);
          ReconfigRoundInfo(rd, i, m_candidateCount);
        }
        rect.InflateRect(DPI_SCALE(m_style.hilite_padding_x),
                         DPI_SCALE(m_style.hilite_padding_y));

        bool roundTopLeft = true;
        bool roundTopRight = true;
        bool roundBottomRight = true;
        bool roundBottomLeft = true;
        float shapeRadius = (float)DPI_SCALE(m_style.round_corner);
        if (rd.Hemispherical && NOT_FULLSCREENLAYOUT(m_style)) {
          roundTopLeft = rd.IsTopLeftNeedToRound;
          roundTopRight = rd.IsTopRightNeedToRound;
          roundBottomRight = rd.IsBottomRightNeedToRound;
          roundBottomLeft = rd.IsBottomLeftNeedToRound;
          shapeRadius =
              (float)(DPI_SCALE(m_style.round_corner_ex) -
                      (DPI_SCALE(m_style.border) % 2
                           ? DPI_SCALE(m_style.border) / 2
                           : 0));
        }
        candidateShapes.push_back(
            {D2D1::RectF((float)rect.left, (float)rect.top,
                         (float)rect.right, (float)rect.bottom),
             shapeRadius, roundTopLeft, roundTopRight, roundBottomRight,
             roundBottomLeft});
      }

      if (!candidateShapes.empty()) {
        if (COLORNOTTRANSPARENT(m_style.candidate_shadow_color)) {
          pDWR->DrawSoftShadowBatch(
              candidateShapes,
              D2DCOLOR_FROM_COLORREF(m_style.candidate_shadow_color),
              (float)DPI_SCALE(m_style.shadow_radius),
              (float)DPI_SCALE(m_style.shadow_offset_x),
              (float)DPI_SCALE(m_style.shadow_offset_y),
              (float)m_layout->offsetX, (float)m_layout->offsetY);
        }
        if (COLORNOTTRANSPARENT(m_style.candidate_back_color)) {
          pDWR->FillRoundRectBatch(
              candidateShapes,
              D2DCOLOR_FROM_COLORREF(m_style.candidate_back_color));
        }
        if (COLORNOTTRANSPARENT(m_style.candidate_border_color) &&
            DPI_SCALE(m_style.border) > 0) {
          pDWR->DrawRoundRectBatch(
              candidateShapes,
              D2DCOLOR_FROM_COLORREF(m_style.candidate_border_color),
              (float)DPI_SCALE(m_style.border));
        }
        drawn = true;
      }
    }
    // draw semi-hilite background and shadow
    if (m_hoverIndex >= 0) {
      rect = m_layout->GetCandidateRect(m_hoverIndex);
      IsToRoundStruct rd = m_layout->GetRoundInfo(m_hoverIndex);
      if (m_istorepos) {
        rect.OffsetRect(0, m_offsetys[m_hoverIndex]);
        ReconfigRoundInfo(rd, m_hoverIndex, m_candidateCount);
      }
      rect.InflateRect(DPI_SCALE(m_style.hilite_padding_x),
                       DPI_SCALE(m_style.hilite_padding_y));
      _HighlightText(dc, rect,
                     HALF_ALPHA_COLOR(m_style.hilited_candidate_back_color),
                     HALF_ALPHA_COLOR(m_style.hilited_candidate_shadow_color),
                     DPI_SCALE(m_style.round_corner), bkType, rd,
                     HALF_ALPHA_COLOR(m_style.hilited_candidate_border_color));
    }
    // draw highlighted background and shadow
    {
      rect = m_layout->GetHighlightRect();
      bool markSt = bar_scale_ == 1.0 || (!m_style.mark_text.empty());
      IsToRoundStruct rd = m_layout->GetRoundInfo(m_ctx.cinfo.highlighted);
      if (m_istorepos) {
        rect.OffsetRect(0, m_offsetys[m_ctx.cinfo.highlighted]);
        ReconfigRoundInfo(rd, m_ctx.cinfo.highlighted, m_candidateCount);
      }
      rect.InflateRect(DPI_SCALE(m_style.hilite_padding_x),
                       DPI_SCALE(m_style.hilite_padding_y));
      _HighlightText(dc, rect, m_style.hilited_candidate_back_color,
                     markSt ? m_style.hilited_candidate_shadow_color : 0,
                     DPI_SCALE(m_style.round_corner), bkType, rd,
                     m_style.hilited_candidate_border_color);
      if (m_style.mark_text.empty() &&
          COLORNOTTRANSPARENT(m_style.hilited_mark_color)) {
        int height =
            min(rect.Height() - DPI_SCALE(m_style.hilite_padding_y) * 2,
                rect.Height() - DPI_SCALE(m_style.round_corner) * 2);
        int width = min(rect.Width() - DPI_SCALE(m_style.hilite_padding_x) * 2,
                        rect.Width() - DPI_SCALE(m_style.round_corner) * 2);
        width = min(width, static_cast<int>(rect.Width() * 0.618));
        height = min(height, static_cast<int>(rect.Height() * 0.618));
        if (bar_scale_ != 1.0f) {
          width = static_cast<int>(width * bar_scale_);
          height = static_cast<int>(height * bar_scale_);
        }
        if (m_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT) {
          int x = rect.left + (rect.Width() - width) / 2;
          CRect mkrc{x, rect.top, x + width, rect.top + m_layout->mark_height};
          pDWR->FillRoundRect(
              D2D1::RectF((float)mkrc.left, (float)mkrc.top,
                          (float)mkrc.right, (float)mkrc.bottom),
              D2DCOLOR_FROM_COLORREF(m_style.hilited_mark_color),
              (float)mkrc.Height() / 2.0f, (float)mkrc.Height() / 2.0f);
        } else {
          int y = rect.top + (rect.Height() - height) / 2;
          CRect mkrc{rect.left, y, rect.left + m_layout->mark_width,
                     y + height};
          pDWR->FillRoundRect(
              D2D1::RectF((float)mkrc.left, (float)mkrc.top,
                          (float)mkrc.right, (float)mkrc.bottom),
              D2DCOLOR_FROM_COLORREF(m_style.hilited_mark_color),
              (float)mkrc.Width() / 2.0f, (float)mkrc.Width() / 2.0f);
        }
      }
      drawn = true;
    }
  }
  // draw text with direct write
  else {
    // begin draw candidate texts
    int label_text_color, candidate_text_color, comment_text_color;
    for (auto i = 0; i < m_candidateCount && i < MAX_CANDIDATES_COUNT; ++i) {
      if (i == m_ctx.cinfo.highlighted || i == m_hoverIndex) {
        label_text_color = m_style.hilited_label_text_color;
        candidate_text_color = m_style.hilited_candidate_text_color;
        comment_text_color = m_style.hilited_comment_text_color;
      } else {
        label_text_color = m_style.label_text_color;
        candidate_text_color = m_style.candidate_text_color;
        comment_text_color = m_style.comment_text_color;
      }
      // Draw label
      std::wstring label = m_layout->GetLabelText(
          labels, (int)i, m_style.label_text_format.c_str());
      if (!label.empty()) {
        rect = m_layout->GetCandidateLabelRect((int)i);
        if (m_istorepos)
          rect.OffsetRect(0, m_offsetys[i]);
        _TextOut(rect, label.c_str(), label.length(), label_text_color,
                 labeltxtFormat.Get(), vertical_candidates);
      }
      // Draw text
      std::wstring text = candidates.at(i).str;
      if (!text.empty()) {
        rect = m_layout->GetCandidateTextRect((int)i);
        if (m_istorepos)
          rect.OffsetRect(0, m_offsetys[i]);
        _TextOut(rect, text.c_str(), text.length(), candidate_text_color,
                 txtFormat.Get(), vertical_candidates);
      }
      // Draw comment
      std::wstring comment = comments.at(i).str;
      if (!comment.empty() && COLORNOTTRANSPARENT(comment_text_color)) {
        rect = m_layout->GetCandidateCommentRect((int)i);
        if (m_istorepos)
          rect.OffsetRect(0, m_offsetys[i]);
        _TextOut(rect, comment.c_str(), comment.length(), comment_text_color,
                 commenttxtFormat.Get(), vertical_candidates);
      }
      drawn = true;
    }
    // draw highlight mark
    {
      if (!m_style.mark_text.empty() &&
          COLORNOTTRANSPARENT(m_style.hilited_mark_color)) {
        CRect rc = m_layout->GetHighlightRect();
        if (m_istorepos)
          rc.OffsetRect(0, m_offsetys[m_ctx.cinfo.highlighted]);
        rc.InflateRect(DPI_SCALE(m_style.hilite_padding_x),
                       DPI_SCALE(m_style.hilite_padding_y));
        int vgap = m_layout->mark_height
                       ? (rc.Height() - m_layout->mark_height) / 2
                       : 0;
        int hgap =
            m_layout->mark_width ? (rc.Width() - m_layout->mark_width) / 2 : 0;
        CRect hlRc;
        if (m_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT)
          hlRc = CRect(rc.left + hgap,
                       rc.top + DPI_SCALE(m_style.hilite_padding_y),
                       rc.left + hgap + m_layout->mark_width,
                       rc.top + DPI_SCALE(m_style.hilite_padding_y) +
                           m_layout->mark_height);
        else
          hlRc = CRect(rc.left + DPI_SCALE(m_style.hilite_padding_x),
                       rc.top + vgap,
                       rc.left + DPI_SCALE(m_style.hilite_padding_x) +
                           m_layout->mark_width,
                       rc.bottom - vgap);
        _TextOut(hlRc, m_style.mark_text.c_str(), m_style.mark_text.length(),
                 m_style.hilited_mark_color, pDWR->pTextFormat.Get(),
                 vertical_candidates);
      }
    }
  }
  return drawn;
}

// draw client area
void WeaselPanel::DoPaint(CDCHandle dc) {
  if (!::IsWindow(m_hWnd))
    return;

  // DirectComposition handles per-pixel alpha. Keep the window non-transparent
  // for hit-testing after the first frame.
  ModifyStyleEx(WS_EX_TRANSPARENT, 0);
  GetClientRect(&rcw);
  if (rcw.IsRectEmpty())
    return;
  if (!pDWR ||
      (pDWR->pTextFormat.Get() == nullptr &&
       pDWR->pLabelTextFormat.Get() == nullptr &&
       pDWR->pCommentTextFormat.Get() == nullptr)) {
    _InitFontRes(true);
  }
  if (!_EnsureDCompResources())
    return;
  if (pDWR && m_d2dFactory1)
    pDWR->SetD2DFactory(m_d2dFactory1.Get());
  _ResizeDCompSurface(rcw);
  if (!m_dcompSurface || !pDWR)
    return;

  ComPtr<ID2D1DeviceContext> d2dContext;
  POINT updateOffset = {};
  HRESULT hr = m_dcompSurface->BeginDraw(
      nullptr, __uuidof(ID2D1DeviceContext),
      reinterpret_cast<void**>(d2dContext.ReleaseAndGetAddressOf()),
      &updateOffset);
  if (FAILED(hr) || !d2dContext) {
    m_dcompSurface.Reset();
    return;
  }

  hr = pDWR->SetRenderTarget(d2dContext.Get());
  if (FAILED(hr)) {
    d2dContext.Reset();
    m_dcompSurface->EndDraw();
    m_dcompSurface.Reset();
    return;
  }

  d2dContext->SetDpi(96.0f, 96.0f);
  d2dContext->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
  if (updateOffset.x != 0 || updateOffset.y != 0) {
    d2dContext->SetTransform(D2D1::Matrix3x2F::Translation(
        static_cast<float>(updateOffset.x),
        static_cast<float>(updateOffset.y)));
  }

  bool drawn = false;
  if (!hide_candidates) {
    CRect auxrc = m_layout->GetAuxiliaryRect();
    CRect preeditrc = m_layout->GetPreeditRect();
    if (m_istorepos) {
      CRect rects[MAX_CANDIDATES_COUNT];
      int btmys[MAX_CANDIDATES_COUNT];
      for (auto i = 0; i < m_candidateCount && i < MAX_CANDIDATES_COUNT; ++i) {
        rects[i] = m_layout->GetCandidateRect(i);
        btmys[i] = rects[i].bottom;
      }
      if (m_candidateCount) {
        if (!m_layout->IsInlinePreedit() && !m_ctx.preedit.str.empty())
          m_offsety_preedit =
              rects[m_candidateCount - 1].bottom - preeditrc.bottom;
        if (!m_ctx.aux.str.empty())
          m_offsety_aux = rects[m_candidateCount - 1].bottom - auxrc.bottom;
      } else {
        m_offsety_preedit = 0;
        m_offsety_aux = 0;
      }
      int base_gap = 0;
      if (!m_ctx.aux.str.empty())
        base_gap = auxrc.Height() + m_style.spacing;
      else if (!m_layout->IsInlinePreedit() && !m_ctx.preedit.str.empty())
        base_gap = preeditrc.Height() + m_style.spacing;

      for (auto i = 0; i < m_candidateCount && i < MAX_CANDIDATES_COUNT; ++i) {
        if (i == 0)
          m_offsetys[i] =
              btmys[m_candidateCount - i - 1] - base_gap - rects[i].bottom;
        else
          m_offsetys[i] = (rects[i - 1].top + m_offsetys[i - 1] -
                           DPI_SCALE(m_style.candidate_spacing)) -
                          rects[i].bottom;
      }
    }
    // background and candidates back, hilite back drawing start
    if ((!m_ctx.empty() && !m_style.inline_preedit) ||
        (m_style.inline_preedit && (m_candidateCount || !m_ctx.aux.empty()))) {
      CRect backrc = m_layout->GetContentRect();
      _HighlightText(dc, backrc, m_style.back_color, m_style.shadow_color,
                     DPI_SCALE(m_style.round_corner_ex), BackType::BACKGROUND,
                     IsToRoundStruct(), m_style.border_color);
    }
    if (!m_ctx.aux.str.empty()) {
      if (m_istorepos)
        auxrc.OffsetRect(0, m_offsety_aux);
      drawn |= _DrawPreeditBack(m_ctx.aux, dc, auxrc);
    }
    if (!m_layout->IsInlinePreedit() && !m_ctx.preedit.str.empty()) {
      if (m_istorepos)
        preeditrc.OffsetRect(0, m_offsety_preedit);
      drawn |= _DrawPreeditBack(m_ctx.preedit, dc, preeditrc);
    }
    if (m_candidateCount)
      drawn |= _DrawCandidates(dc, true);
    // background and candidates back, hilite back drawing end

    // Draw all text with DirectWrite into the DirectComposition surface.
    if (!m_ctx.aux.str.empty())
      drawn |= _DrawPreedit(m_ctx.aux, dc, auxrc);
    if (!m_layout->IsInlinePreedit() && !m_ctx.preedit.str.empty())
      drawn |= _DrawPreedit(m_ctx.preedit, dc, preeditrc);
    if (m_candidateCount)
      drawn |= _DrawCandidates(dc);

    // status icon (I guess Metro IME stole my idea :)
    if (m_layout->ShouldDisplayStatusIcon()) {
      // decide if custom schema zhung icon to show
      LoadIconNecessary(m_current_zhung_icon, m_style.current_zhung_icon,
                        m_iconEnabled, IDI_ZH);
      LoadIconNecessary(m_current_ascii_icon, m_style.current_ascii_icon,
                        m_iconAlpha, IDI_EN);
      LoadIconNecessary(m_current_half_icon, m_style.current_half_icon,
                        m_iconHalf, IDI_HALF_SHAPE);
      LoadIconNecessary(m_current_full_icon, m_style.current_full_icon,
                        m_iconFull, IDI_FULL_SHAPE);
      CRect iconRect(m_layout->GetStatusIconRect());
      if (m_istorepos && !m_ctx.aux.str.empty())
        iconRect.OffsetRect(0, m_offsety_aux);
      else if (m_istorepos && !m_layout->IsInlinePreedit() &&
               !m_ctx.preedit.str.empty())
        iconRect.OffsetRect(0, m_offsety_preedit);

      CIcon& icon(
          m_status.disabled ? m_iconDisabled
          : m_status.ascii_mode
              ? m_iconAlpha
              : (m_status.type == SCHEMA
                     ? m_iconEnabled
                     : (m_status.full_shape ? m_iconFull : m_iconHalf)));
      _DrawStatusIcon(iconRect, icon.m_hIcon);
      drawn = true;
    }
    /* Nothing drawn, hide candidate window */
    if (!drawn)
      ShowWindow(SW_HIDE);
  }

  hr = S_OK;
  pDWR->ClearRenderTarget();
  if (FAILED(hr)) {
    m_dcompSurface.Reset();
  } else {
    hr = m_dcompSurface->EndDraw();
    if (SUCCEEDED(hr)) {
      m_dcompVisual->SetContent(m_dcompSurface.Get());
      m_dcompTarget->SetRoot(m_dcompVisual.Get());
      hr = m_dcompDevice->Commit();
      if (FAILED(hr))
        m_dcompSurface.Reset();
    } else {
      m_dcompSurface.Reset();
    }
  }
  d2dContext.Reset();
}

// 由于某些软件并不依赖 WM_PAINT 消息来重绘，在消息循环中直接忽略掉了 WM_PAINT
// 消息， 导致 DoPaint() 永远不会被调用，这里手动调用 DoPaint() 强制重绘
void WeaselPanel::RedrawWindow() {
  DoPaint(CDCHandle());
}

LRESULT WeaselPanel::OnCreate(UINT uMsg,
                              WPARAM wParam,
                              LPARAM lParam,
                              BOOL& bHandled) {
  m_mouse_entry = false;
  m_hoverIndex = -1;
  Refresh();
  return TRUE;
}

LRESULT WeaselPanel::OnEraseBackground(UINT uMsg,
                                       WPARAM wParam,
                                       LPARAM lParam,
                                       BOOL& bHandled) {
  bHandled = TRUE;
  return 1;
}

LRESULT WeaselPanel::OnPaint(UINT uMsg,
                             WPARAM wParam,
                             LPARAM lParam,
                             BOOL& bHandled) {
  bHandled = TRUE;
  DoPaint(CDCHandle());
  ::ValidateRect(m_hWnd, nullptr);
  return 0;
}

LRESULT WeaselPanel::OnDestroy(UINT uMsg,
                               WPARAM wParam,
                               LPARAM lParam,
                               BOOL& bHandled) {
  m_hoverIndex = -1;
  m_sticky = false;
  m_windowSize = CSize(0, 0);
  _ReleaseDCompResources();
  delete m_layout;
  m_layout = NULL;
  return 0;
}

LRESULT WeaselPanel::OnDpiChanged(UINT uMsg,
                                  WPARAM wParam,
                                  LPARAM lParam,
                                  BOOL& bHandled) {
  Refresh();
  return LRESULT();
}

void WeaselPanel::MoveTo(RECT const& rc) {
  if (!m_layout)
    return;  // avoid handling nullptr in _RepositionWindow
  m_redraw_by_monitor_change = false;
  // The conditions for resetting the sticky state:
  // 1. When the input session ends (ctx.empty() is true)
  // 2. When the input position changes significantly (the position change
  // exceeds the threshold)
  // 3. When the content of the candidate window is empty
  bool should_reset_sticky =
      (m_ctx.empty() || (abs(rc.left - m_inputPos.left) > 50) ||
       (abs(rc.bottom - m_inputPos.bottom) > 50));
  if (should_reset_sticky && m_sticky) {
    m_sticky = false;
    // Force reposition the window
    m_inputPos = rc;
    m_inputPos.OffsetRect(0, 6);
    _RepositionWindow(true);
    RedrawWindow();
    return;
  }
  // if ascii_tip_follow_cursor set, move tip icon to mouse cursor
  if (m_style.ascii_tip_follow_cursor && m_ctx.empty() &&
      (!m_status.composing) && m_layout->ShouldDisplayStatusIcon()) {
    // ascii icon follow cursor
    POINT p;
    ::GetCursorPos(&p);
    RECT irc{p.x - STATUS_ICON_SIZE, p.y - STATUS_ICON_SIZE, p.x, p.y};
    m_inputPos = irc;
    _RepositionWindow(true);
    RedrawWindow();
  } else if (!(rc.left == m_inputPos.left && rc.bottom != m_inputPos.bottom &&
               abs(rc.bottom - m_inputPos.bottom) < 6) ||
             m_layout->ShouldDisplayStatusIcon()) {
    // in some apps like word 2021, with inline_preedit set,
    // bottom of rc would flicker 1 px or 2, make the candidate flickering
    m_inputPos = rc;
    m_inputPos.OffsetRect(0, 6);
    // buffer current m_istorepos status
    bool m_istorepos_buf = m_istorepos;
    // with parameter to avoid vertical flicker
    _RepositionWindow(true);
    // m_istorepos status changed by _RepositionWindow, or tips to show,
    // redrawing is required
    if (m_istorepos != m_istorepos_buf || !m_ctx.aux.empty() ||
        m_layout->ShouldDisplayStatusIcon() || m_redraw_by_monitor_change)
      RedrawWindow();
  }
}

void WeaselPanel::_RepositionWindow(const bool& adj) {
  RECT rcWorkArea;
  memset(&rcWorkArea, 0, sizeof(rcWorkArea));
  HMONITOR hMonitor = MonitorFromRect(m_inputPos, MONITOR_DEFAULTTONEAREST);
  if (hMonitor) {
    MONITORINFO info;
    info.cbSize = sizeof(MONITORINFO);
    if (GetMonitorInfo(hMonitor, &info)) {
      rcWorkArea = info.rcWork;
    }
    if (hMonitor != m_hMonitor) {
      m_hMonitor = hMonitor;
      m_redraw_by_monitor_change = true;
    }
  }
  RECT rcWindow;
  GetWindowRect(&rcWindow);
  int width = (rcWindow.right - rcWindow.left);
  int height = (rcWindow.bottom - rcWindow.top);
  // keep panel visible
  rcWorkArea.right -= width;
  rcWorkArea.bottom -= height;
  int x = m_inputPos.left;
  int y = m_inputPos.bottom;
  if (DPI_SCALE(m_style.shadow_radius)) {
    x -= (DPI_SCALE(m_style.shadow_offset_x) >= 0 ||
          COLORTRANSPARENT(m_style.shadow_color))
             ? m_layout->offsetX
             : (m_layout->offsetX / 2);
    if (adj)
      y -= (DPI_SCALE(m_style.shadow_offset_y) > 0 ||
            COLORTRANSPARENT(m_style.shadow_color))
               ? m_layout->offsetY
               : (m_layout->offsetY / 2);
  }
  // for vertical text layout, flow right to left, make window left side
  if (m_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT &&
      !m_style.vertical_text_left_to_right) {
    x += m_layout->offsetX - width;
    if (DPI_SCALE(m_style.shadow_offset_x) < 0)
      x += m_layout->offsetX;
  }
  if (adj)
    m_istorepos = false;
  if (x > rcWorkArea.right)
    x = rcWorkArea.right;  // over workarea right
  if (x < rcWorkArea.left)
    x = rcWorkArea.left;  // over workarea left
  // show panel above the input focus if we're around the bottom
  if (y > rcWorkArea.bottom || m_sticky) {
    if (!m_sticky)
      m_sticky = true;
    y = m_inputPos.top - height - 6;  // over workarea bottom
    if (DPI_SCALE(m_style.shadow_radius) &&
        DPI_SCALE(m_style.shadow_offset_y) > 0)
      y -= DPI_SCALE(m_style.shadow_offset_y);
    m_istorepos = (m_style.vertical_auto_reverse &&
                   m_style.layout_type == UIStyle::LAYOUT_VERTICAL);
    if (DPI_SCALE(m_style.shadow_radius) > 0)
      y += (DPI_SCALE(m_style.shadow_offset_y) < 0 ||
            COLORTRANSPARENT(m_style.shadow_color))
               ? m_layout->offsetY
               : (m_layout->offsetY / 2);
  }
  if (y < rcWorkArea.top)
    y = rcWorkArea.top;  // over workarea top
  // memorize adjusted position (to avoid window bouncing on height change)
  m_inputPos.bottom = y;
  SetWindowPos(HWND_TOPMOST, x, y, 0, 0,
               SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREDRAW);
}

void WeaselPanel::_TextOut(const CRect& rc,
                           std::wstring_view psz,
                           const size_t& cch,
                           const int& inColor,
                           IDWriteTextFormat1* const pTextFormat,
                           bool vertical_text) {
  if (pTextFormat == NULL)
    return;
  float r = (float)(GetRValue(inColor)) / 255.0f;
  float g = (float)(GetGValue(inColor)) / 255.0f;
  float b = (float)(GetBValue(inColor)) / 255.0f;
  float alpha = (float)((inColor >> 24) & 255) / 255.0f;
  HRESULT hr = S_OK;
  if (pDWR->pBrush == NULL) {
    HR(pDWR->CreateBrush(D2D1::ColorF(r, g, b, alpha)));
  } else
    pDWR->SetBrushColor(D2D1::ColorF(r, g, b, alpha));

  // The text format already carries the writing direction (the candidate
  // formats are vertical in vertical text mode, the preedit format used by the
  // top band is always horizontal), so the layout is created as is.
  HR(pDWR->CreateTextLayoutCached(psz, (int)cch, pTextFormat,
                                  (float)rc.Width(), (float)rc.Height(), false,
                                  false));

  // offsetx for font glyph over left
  float offsetx = (float)rc.left;
  float offsety = (float)rc.top;
  // prepare for space when first character overhanged
  DWRITE_OVERHANG_METRICS omt;
  HR(pDWR->GetLayoutOverhangMetrics(&omt));
  if (!vertical_text && omt.left > 0)
    offsetx += omt.left;
  if (vertical_text && omt.top > 0)
    offsety += omt.top;

  if (pDWR->pTextLayout != NULL) {
    pDWR->DrawTextLayoutAt({offsetx, offsety});
#if 0
    D2D1_RECT_F rectf =  D2D1::RectF(offsetx, offsety, offsetx + rc.Width(), offsety + rc.Height());
    pDWR->DrawRect(&rectf);
#endif
  }
}
