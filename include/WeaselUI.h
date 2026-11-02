#pragma once

#include <WeaselIPCData.h>
#include <vector>
#include <regex>
#include <iterator>
#include <map>
#include <d2d1_1.h>
#include <d2d1effects.h>
#include <dwrite_2.h>
#include <memory>
#include <functional>
#include <WeaselUtility.h>

namespace weasel {

enum ClientCapabilities {
  INLINE_PREEDIT_CAPABLE = 1,
};

class UIImpl;
class DirectWriteResources;
template <class T>
using an = std::shared_ptr<T>;

using PDWR = an<DirectWriteResources>;

struct ShadowShape {
  D2D1_RECT_F rect;
  float radius;
  bool roundTopLeft;
  bool roundTopRight;
  bool roundBottomRight;
  bool roundBottomLeft;
};

//
// 输入法界面接口类
//
class UI {
 public:
  UI() : pimpl_(0) {}

  virtual ~UI() {
    if (pimpl_)
      Destroy(true);
    if (pDWR) {
      pDWR.reset();
    }
  }

  // 创建输入法界面
  bool Create(HWND parent);

  // 销毁界面
  void Destroy(bool full = false);

  // 界面显隐
  void Show();
  void Hide();
  void ShowWithTimeout(size_t millisec);
  bool IsCountingDown() const;
  bool IsShown() const;

  // 重绘界面
  void Refresh();

  // 置输入焦点位置（光标跟随时移动候选窗）但不重绘
  void UpdateInputPosition(RECT const& rc);

  // 更新界面显示内容
  void Update(Context const& ctx, Status const& status);

  Context& ctx() { return ctx_; }
  Context& octx() { return octx_; }
  Status& status() { return status_; }
  UIStyle& style() { return style_; }
  UIStyle& ostyle() { return ostyle_; }
  PDWR pdwr() { return pDWR; }
  bool GetIsReposition();

  std::function<void(size_t* const, size_t* const, bool* const, bool* const)>&
  uiCallback() {
    return _UICallback;
  }
  void SetUICallBack(
      std::function<
          void(size_t* const, size_t* const, bool* const, bool* const)> const&
          func) {
    _UICallback = func;
  }

 private:
  UIImpl* pimpl_;
  PDWR pDWR;

  Context ctx_;
  Context octx_;
  Status status_;
  UIStyle style_;
  UIStyle ostyle_;
  std::function<void(size_t* const, size_t* const, bool* const, bool* const)>
      _UICallback;
};

class DirectWriteResources {
 public:
  DirectWriteResources(weasel::UIStyle& style, UINT dpi);
  ~DirectWriteResources();

  HRESULT InitResources(const std::wstring& label_font_face,
                        const int& label_font_point,
                        const std::wstring& font_face,
                        const int& font_point,
                        const std::wstring& comment_font_face,
                        const int& comment_font_point,
                        const bool& vertical_text = false);
  HRESULT InitResources(const UIStyle& style, const UINT& dpi);

  HRESULT CreateTextLayout(std::wstring_view text,
                           const int& nCount,
                           IDWriteTextFormat1* const txtFormat,
                           const float& width,
                           const float& height) {
    return pDWFactory->CreateTextLayout(
        text.data(), nCount, txtFormat, width, height,
        reinterpret_cast<IDWriteTextLayout**>(
            pTextLayout.ReleaseAndGetAddressOf()));
  }
  HRESULT CreateTextLayoutCached(std::wstring_view text,
                                 const int& nCount,
                                 IDWriteTextFormat1* const txtFormat,
                                 const float& width,
                                 const float& height,
                                 bool verticalText,
                                 bool leftToRight);
  bool GetCachedTextExtent(std::wstring_view text,
                           IDWriteTextFormat1* const txtFormat,
                           float width,
                           float height,
                           bool verticalText,
                           bool leftToRight,
                           SIZE* size) const;
  void CacheTextExtent(std::wstring_view text,
                       IDWriteTextFormat1* const txtFormat,
                       float width,
                       float height,
                       bool verticalText,
                       bool leftToRight,
                       const SIZE& size);
  void DrawRect(D2D1_RECT_F* const rect,
                const float& strokeWidth = 1.0f,
                ID2D1StrokeStyle* const sstyle = (ID2D1StrokeStyle*)0) {
    pRenderTarget->DrawRectangle(rect, pBrush.Get(), strokeWidth, sstyle);
  }
  ComPtr<ID2D1PathGeometry> CreateRoundRectGeometry(
      const D2D1_RECT_F& rect,
      float radiusX,
      float radiusY,
      bool roundTopLeft = true,
      bool roundTopRight = true,
      bool roundBottomRight = true,
      bool roundBottomLeft = true);
  ComPtr<ID2D1PathGeometry> CreateRoundRectGeometryBatch(
      const std::vector<ShadowShape>& shapes);
  void FillRoundRect(const D2D1_RECT_F& rect,
                     const D2D1_COLOR_F& color,
                     float radiusX,
                     float radiusY,
                     bool roundTopLeft = true,
                     bool roundTopRight = true,
                     bool roundBottomRight = true,
                     bool roundBottomLeft = true);
  void DrawRoundRect(const D2D1_RECT_F& rect,
                     const D2D1_COLOR_F& color,
                     float strokeWidth,
                     float radiusX,
                     float radiusY,
                     bool roundTopLeft = true,
                     bool roundTopRight = true,
                     bool roundBottomRight = true,
                     bool roundBottomLeft = true);
  void DrawSoftShadow(const D2D1_RECT_F& rect,
                      float radius,
                      const D2D1_COLOR_F& color,
                      float blurRadius,
                      float offsetX,
                      float offsetY,
                      float blurMarginX = 0.0f,
                      float blurMarginY = 0.0f,
                      bool roundTopLeft = true,
                      bool roundTopRight = true,
                      bool roundBottomRight = true,
                      bool roundBottomLeft = true);
  void DrawSoftShadowBatch(const std::vector<ShadowShape>& shapes,
                           const D2D1_COLOR_F& color,
                           float blurRadius,
                           float offsetX,
                           float offsetY,
                           float blurMarginX,
                           float blurMarginY);
  void FillRoundRectBatch(const std::vector<ShadowShape>& shapes,
                          const D2D1_COLOR_F& color);
  void DrawRoundRectBatch(const std::vector<ShadowShape>& shapes,
                          const D2D1_COLOR_F& color,
                          float strokeWidth);
  HRESULT SetRenderTarget(ID2D1RenderTarget* renderTarget);
  void SetD2DFactory(ID2D1Factory* factory) { pD2d1Factory = factory; }
  void ClearRenderTarget();
  HRESULT GetLayoutOverhangMetrics(DWRITE_OVERHANG_METRICS* overhangMetrics) {
    return pTextLayout->GetOverhangMetrics(overhangMetrics);
  }
  HRESULT GetLayoutMetrics(DWRITE_TEXT_METRICS* metrics) {
    return pTextLayout->GetMetrics(metrics);
  }
  HRESULT SetLayoutReadingDirection(const DWRITE_READING_DIRECTION& direct) {
    return pTextLayout->SetReadingDirection(direct);
  }
  HRESULT SetLayoutFlowDirection(const DWRITE_FLOW_DIRECTION& direct) {
    return pTextLayout->SetFlowDirection(direct);
  }
  void DrawTextLayoutAt(const D2D1_POINT_2F& point) {
    pRenderTarget->DrawTextLayout(point, pTextLayout.Get(), pBrush.Get(),
                                  D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
  }
  HRESULT CreateBrush(const D2D1_COLOR_F& color) {
    return pRenderTarget->CreateSolidColorBrush(
        color, pBrush.ReleaseAndGetAddressOf());
  }
  void ResetLayout() { pTextLayout.Reset(); }
  void SetBrushColor(const D2D1_COLOR_F& color) { pBrush->SetColor(color); }
  void SetDpi(const UINT& dpi);

  float dpiScaleFontPoint, dpiScaleLayout;
  ComPtr<ID2D1Factory> pD2d1Factory;
  ComPtr<IDWriteFactory2> pDWFactory;
  ComPtr<ID2D1RenderTarget> pRenderTarget;
  ComPtr<ID2D1Device> pCachedDevice;
  ComPtr<IDWriteTextFormat1> pPreeditTextFormat;
  ComPtr<IDWriteTextFormat1> pTextFormat;
  ComPtr<IDWriteTextFormat1> pLabelTextFormat;
  ComPtr<IDWriteTextFormat1> pCommentTextFormat;
  ComPtr<IDWriteTextLayout2> pTextLayout;
  ComPtr<ID2D1SolidColorBrush> pBrush;
  ComPtr<ID2D1Effect> pShadowBlurEffect;
  ComPtr<ID2D1Bitmap1> pShadowBitmap;
  UINT shadowBitmapWidth = 0;
  UINT shadowBitmapHeight = 0;

 private:
  struct TextLayoutKey {
    std::wstring text;
    IDWriteTextFormat1* format = nullptr;
    float width = 0.0f;
    float height = 0.0f;
    bool verticalText = false;
    bool leftToRight = false;

    bool operator<(const TextLayoutKey& other) const {
      if (text != other.text)
        return text < other.text;
      if (format != other.format)
        return format < other.format;
      if (width != other.width)
        return width < other.width;
      if (height != other.height)
        return height < other.height;
      if (verticalText != other.verticalText)
        return verticalText < other.verticalText;
      return leftToRight < other.leftToRight;
    }
  };

  struct TextExtentKey {
    std::wstring text;
    IDWriteTextFormat1* format = nullptr;
    float width = 0.0f;
    float height = 0.0f;
    bool verticalText = false;
    bool leftToRight = false;

    bool operator<(const TextExtentKey& other) const {
      if (text != other.text)
        return text < other.text;
      if (format != other.format)
        return format < other.format;
      if (width != other.width)
        return width < other.width;
      if (height != other.height)
        return height < other.height;
      if (verticalText != other.verticalText)
        return verticalText < other.verticalText;
      return leftToRight < other.leftToRight;
    }
  };

  std::map<TextLayoutKey, ComPtr<IDWriteTextLayout2>> textLayoutCache_;
  std::map<TextExtentKey, SIZE> textExtentCache_;
  UIStyle& _style;
  D2D1_TEXT_ANTIALIAS_MODE textAntialiasMode_;
  void _AppendRoundRectPath(ID2D1GeometrySink* sink,
                            const D2D1_RECT_F& rect,
                            float radiusX,
                            float radiusY,
                            bool roundTopLeft,
                            bool roundTopRight,
                            bool roundBottomRight,
                            bool roundBottomLeft);
  void _ParseFontFace(const std::wstring& fontFaceStr,
                      DWRITE_FONT_WEIGHT& fontWeight,
                      DWRITE_FONT_STYLE& fontStyle);
  void _SetFontFallback(ComPtr<IDWriteTextFormat1>& pTextFormat,
                        const std::vector<std::wstring>& fontVector);
};
}  // namespace weasel
