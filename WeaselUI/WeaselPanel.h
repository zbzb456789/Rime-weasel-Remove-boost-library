#pragma once
#include <WeaselIPCData.h>
#include <WeaselUI.h>
#include "StandardLayout.h"
#include "Layout.h"
#include <d2d1_1.h>
#include <d3d11.h>
#include <dxgi.h>
#include <dcomp.h>
#include <wincodec.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dcomp.lib")
#pragma comment(lib, "windowscodecs.lib")

using namespace weasel;

typedef CWinTraits<WS_POPUP | WS_CLIPSIBLINGS | WS_DISABLED,
                   WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE |
                       WS_EX_NOREDIRECTIONBITMAP>
    CWeaselPanelTraits;

enum class BackType {
  TEXT = 0,
  CAND = 1,
  BACKGROUND = 2  // background
};

class WeaselPanel
    : public CWindowImpl<WeaselPanel, CWindow, CWeaselPanelTraits> {
 public:
  BEGIN_MSG_MAP(WeaselPanel)
  MESSAGE_HANDLER(WM_CREATE, OnCreate)
  MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
  MESSAGE_HANDLER(WM_DPICHANGED, OnDpiChanged)
  MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
  MESSAGE_HANDLER(WM_MOUSEACTIVATE, OnMouseActivate)
  MESSAGE_HANDLER(WM_LBUTTONUP, OnLeftClickedUp)
  MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLeftClickedDown)
  MESSAGE_HANDLER(WM_MOUSEWHEEL, OnMouseWheel)
  MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
  MESSAGE_HANDLER(WM_MOUSELEAVE, OnMouseLeave)
  MESSAGE_HANDLER(WM_PAINT, OnPaint)
  END_MSG_MAP()

  LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnEraseBackground(UINT uMsg,
                            WPARAM wParam,
                            LPARAM lParam,
                            BOOL& bHandled);
  LRESULT OnMouseActivate(UINT uMsg,
                          WPARAM wParam,
                          LPARAM lParam,
                          BOOL& bHandled);
  LRESULT OnLeftClickedUp(UINT uMsg,
                          WPARAM wParam,
                          LPARAM lParam,
                          BOOL& bHandled);
  LRESULT OnLeftClickedDown(UINT uMsg,
                            WPARAM wParam,
                            LPARAM lParam,
                            BOOL& bHandled);
  LRESULT OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnMouseLeave(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

  WeaselPanel(weasel::UI& ui);
  ~WeaselPanel();

  void MoveTo(RECT const& rc);
  void Refresh();
  void DoPaint(CDCHandle dc);
  bool GetIsReposition() { return m_istorepos; }
  void RedrawWindow();

  static VOID CALLBACK OnTimer(HWND hwnd,
                               UINT uMsg,
                               UINT_PTR idEvent,
                               DWORD dwTime);
  static const int AUTOREV_TIMER = 20240315;
  static inline UINT_PTR ptimer = 0;

 private:
  template <typename T>
  int DPI_SCALE(T t) {
    return (int)(t * dpiScaleLayout);
  }
  void _InitFontRes(bool forced = false);
  void _CaptureRect(CRect& rect);
  bool _EnsureDCompResources();
  void _ReleaseDCompResources();
  void _ResizeDCompSurface(const CRect& rc);
  bool _DrawStatusIcon(const CRect& iconRect, HICON icon);
  bool m_mouse_entry = false;
  void _CreateLayout();
  void _ResizeWindow();
  void _RepositionWindow(const bool& adj = false);
  bool _DrawPreedit(const Text& text, CDCHandle dc, const CRect& rc);
  bool _DrawPreeditBack(const Text& text, CDCHandle dc, const CRect& rc);
  bool _DrawCandidates(CDCHandle& dc, bool back = false);
  void _HighlightText(CDCHandle& dc,
                      const CRect& rc,
                      const COLORREF& color,
                      const COLORREF& shadowColor,
                      const int& radius,
                      const BackType& type,
                      const IsToRoundStruct& rd,
                      const COLORREF& bordercolor);
  void _TextOut(const CRect& rc,
                std::wstring_view psz,
                const size_t& cch,
                const int& inColor,
                IDWriteTextFormat1* const pTextFormat = NULL,
                bool vertical_text = false);

  weasel::Layout* m_layout;
  weasel::Context& m_ctx;
  weasel::Context& m_octx;
  weasel::Status& m_status;
  weasel::UIStyle& m_style;
  weasel::UIStyle& m_ostyle;

  CRect m_inputPos;
  int m_offsetys[MAX_CANDIDATES_COUNT];  // offset y for candidates when
                                         // vertical layout over bottom
  int m_offsety_preedit;
  int m_offsety_aux;
  bool m_istorepos;

  CIcon m_iconDisabled;
  CIcon m_iconEnabled;
  CIcon m_iconAlpha;
  CIcon m_iconFull;
  CIcon m_iconHalf;
  std::wstring m_current_zhung_icon;
  std::wstring m_current_ascii_icon;
  std::wstring m_current_half_icon;
  std::wstring m_current_full_icon;
  UINT dpi;

  CRect rcw;
  CSize m_windowSize{0, 0};
  BYTE m_candidateCount;
  BYTE m_lastCandidateCount;

  bool hide_candidates;
  bool m_sticky;
  // for multi font_face & font_point
  PDWR pDWR;
  std::function<void(size_t* const, size_t* const, bool* const, bool* const)>&
      _UICallback;
  float bar_scale_ = 1.0;
  float dpiScaleLayout = 1.0f;
  int m_hoverIndex = -1;
  HMONITOR m_hMonitor = NULL;
  bool m_redraw_by_monitor_change = false;

  ComPtr<ID3D11Device> m_d3dDevice;
  ComPtr<IDXGIDevice> m_dxgiDevice;
  ComPtr<ID2D1Factory1> m_d2dFactory1;
  ComPtr<ID2D1Device> m_d2dDevice;
  ComPtr<IDCompositionDevice> m_dcompDevice;
  ComPtr<IDCompositionTarget> m_dcompTarget;
  ComPtr<IDCompositionVisual> m_dcompVisual;
  ComPtr<IDCompositionSurface> m_dcompSurface;
  ComPtr<IWICImagingFactory> m_wicFactory;
  UINT m_surfaceWidth = 0;
  UINT m_surfaceHeight = 0;
};
