#include "stdafx.h"
#include <initguid.h>
#include <d2d1_1.h>
#include <d2d1effects.h>
#include <string>
#include <algorithm>
#include <map>
#include <cmath>
#include <vector>
#include <WeaselUI.h>

using namespace weasel;
#define STYLEORWEIGHT (L":[^:]*[^a-f0-9:]+[^:]*")

vector<wstring> ws_split(const wstring& in, const wstring& delim) {
  std::wregex re{delim};
  return vector<wstring>{
      std::wsregex_token_iterator(in.begin(), in.end(), re, -1),
      std::wsregex_token_iterator()};
}

DirectWriteResources::DirectWriteResources(weasel::UIStyle& style,
                                           UINT dpi = 96)
    : _style(style),
      dpiScaleFontPoint(0),
      dpiScaleLayout(0),
      pD2d1Factory(NULL),
      pDWFactory(NULL),
      pRenderTarget(NULL),
      pBrush(NULL),
      pTextLayout(NULL),
      pPreeditTextFormat(NULL),
      pTextFormat(NULL),
      pLabelTextFormat(NULL),
      pCommentTextFormat(NULL),
      textAntialiasMode_(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE) {
  D2D1_TEXT_ANTIALIAS_MODE mode =
      _style.antialias_mode <= 3
          ? (D2D1_TEXT_ANTIALIAS_MODE)(_style.antialias_mode)
          : D2D1_TEXT_ANTIALIAS_MODE_FORCE_DWORD;  // prepare d2d1 resources
                                                   // create factory
  HR(::D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED,
                         pD2d1Factory.ReleaseAndGetAddressOf()));
  // create IDWriteFactory
  HR(DWriteCreateFactory(
      DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
      reinterpret_cast<IUnknown**>(pDWFactory.ReleaseAndGetAddressOf())));
  textAntialiasMode_ = mode;
  // get the dpi information
  dpiScaleFontPoint = dpiScaleLayout = (float)dpi;
  dpiScaleFontPoint /= 72.0f;
  dpiScaleLayout /= 96.0f;

  InitResources(style, dpi);
}

DirectWriteResources::~DirectWriteResources() {}

HRESULT DirectWriteResources::SetRenderTarget(ID2D1RenderTarget* renderTarget) {
  if (!renderTarget)
    return E_POINTER;

  // D2D 资源和 device 绑定；同一 device 的跨帧绘制可以复用 brush/阴影资源。
  ComPtr<ID2D1Device> device;
  ComPtr<ID2D1DeviceContext> deviceContext;
  if (SUCCEEDED(renderTarget->QueryInterface(
          __uuidof(ID2D1DeviceContext),
          reinterpret_cast<void**>(deviceContext.GetAddressOf())))) {
    deviceContext->GetDevice(device.GetAddressOf());
  }
  if (!device || pCachedDevice.Get() != device.Get()) {
    pBrush.Reset();
    pShadowBlurEffect.Reset();
    pShadowBitmap.Reset();
    shadowBitmapWidth = 0;
    shadowBitmapHeight = 0;
    pCachedDevice = device;
  }
  pRenderTarget = renderTarget;
  pRenderTarget->SetTextAntialiasMode(textAntialiasMode_);
  pRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
  if (pBrush)
    return S_OK;
  return pRenderTarget->CreateSolidColorBrush(
      D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f),
      pBrush.ReleaseAndGetAddressOf());
}

void DirectWriteResources::ClearRenderTarget() {
  // 保留缓存的 brush/阴影资源，只释放当前帧的 render target 引用。
  pRenderTarget.Reset();
}

HRESULT DirectWriteResources::CreateTextLayoutCached(
    std::wstring_view text,
    const int& nCount,
    IDWriteTextFormat1* const txtFormat,
    const float& width,
    const float& height,
    bool verticalText,
    bool leftToRight) {
  if (!txtFormat)
    return E_POINTER;

  const TextLayoutKey key{std::wstring(text.substr(0, nCount)), txtFormat,
                          width, height, verticalText, leftToRight};
  auto it = textLayoutCache_.find(key);
  if (it != textLayoutCache_.end()) {
    pTextLayout = it->second;
    return S_OK;
  }

  HRESULT hr = pDWFactory->CreateTextLayout(
      text.data(), nCount, txtFormat, width, height,
      reinterpret_cast<IDWriteTextLayout**>(
          pTextLayout.ReleaseAndGetAddressOf()));
  if (FAILED(hr))
    return hr;

  if (verticalText) {
    const DWRITE_FLOW_DIRECTION flow =
        leftToRight ? DWRITE_FLOW_DIRECTION_LEFT_TO_RIGHT
                    : DWRITE_FLOW_DIRECTION_RIGHT_TO_LEFT;
    pTextLayout->SetReadingDirection(DWRITE_READING_DIRECTION_TOP_TO_BOTTOM);
    pTextLayout->SetFlowDirection(flow);
  }

  if (textLayoutCache_.size() >= 1024) {
    // 只淘汰最旧的一批，避免多候选时整表清空导致缓存反复失效。
    for (size_t i = 0; i < 256 && !textLayoutCache_.empty(); ++i)
      textLayoutCache_.erase(textLayoutCache_.begin());
  }
  textLayoutCache_.emplace(key, pTextLayout);
  return S_OK;
}

bool DirectWriteResources::GetCachedTextExtent(
    std::wstring_view text,
    IDWriteTextFormat1* const txtFormat,
    float width,
    float height,
    bool verticalText,
    bool leftToRight,
    SIZE* size) const {
  if (!size || !txtFormat)
    return false;
  const TextExtentKey key{std::wstring(text), txtFormat, width, height,
                          verticalText, leftToRight};
  auto it = textExtentCache_.find(key);
  if (it == textExtentCache_.end())
    return false;
  *size = it->second;
  return true;
}

void DirectWriteResources::CacheTextExtent(
    std::wstring_view text,
    IDWriteTextFormat1* const txtFormat,
    float width,
    float height,
    bool verticalText,
    bool leftToRight,
    const SIZE& size) {
  if (!txtFormat)
    return;
  if (textExtentCache_.size() >= 4096) {
    for (size_t i = 0; i < 1024 && !textExtentCache_.empty(); ++i)
      textExtentCache_.erase(textExtentCache_.begin());
  }
  const TextExtentKey key{std::wstring(text), txtFormat, width, height,
                          verticalText, leftToRight};
  textExtentCache_.insert_or_assign(key, size);
}

ComPtr<ID2D1PathGeometry> DirectWriteResources::CreateRoundRectGeometry(
    const D2D1_RECT_F& rect,
    float radiusX,
    float radiusY,
    bool roundTopLeft,
    bool roundTopRight,
    bool roundBottomRight,
    bool roundBottomLeft) {
  ComPtr<ID2D1PathGeometry> path;
  HR(pD2d1Factory->CreatePathGeometry(path.ReleaseAndGetAddressOf()));
  ComPtr<ID2D1GeometrySink> sink;
  HR(path->Open(sink.ReleaseAndGetAddressOf()));
  _AppendRoundRectPath(sink.Get(), rect, radiusX, radiusY, roundTopLeft,
                       roundTopRight, roundBottomRight, roundBottomLeft);
  HR(sink->Close());
  return path;
}

ComPtr<ID2D1PathGeometry> DirectWriteResources::CreateRoundRectGeometryBatch(
    const std::vector<ShadowShape>& shapes) {
  if (shapes.empty())
    return nullptr;

  ComPtr<ID2D1PathGeometry> path;
  HR(pD2d1Factory->CreatePathGeometry(path.ReleaseAndGetAddressOf()));
  ComPtr<ID2D1GeometrySink> sink;
  HR(path->Open(sink.ReleaseAndGetAddressOf()));
  for (const ShadowShape& shape : shapes) {
    _AppendRoundRectPath(sink.Get(), shape.rect, shape.radius, shape.radius,
                         shape.roundTopLeft, shape.roundTopRight,
                         shape.roundBottomRight, shape.roundBottomLeft);
  }
  HR(sink->Close());
  return path;
}

void DirectWriteResources::_AppendRoundRectPath(
    ID2D1GeometrySink* sink,
    const D2D1_RECT_F& rect,
    float radiusX,
    float radiusY,
    bool roundTopLeft,
    bool roundTopRight,
    bool roundBottomRight,
    bool roundBottomLeft) {
  const float left = rect.left;
  const float top = rect.top;
  const float right = rect.right;
  const float bottom = rect.bottom;
  const float width = (std::max)(0.0f, right - left);
  const float height = (std::max)(0.0f, bottom - top);
  const float rx = (std::min)((std::max)(0.0f, radiusX), width / 2.0f);
  const float ry = (std::min)((std::max)(0.0f, radiusY), height / 2.0f);
  const bool anyRound = roundTopLeft || roundTopRight || roundBottomRight ||
                        roundBottomLeft;

  if (!anyRound || rx <= 0.0f || ry <= 0.0f) {
    sink->BeginFigure(D2D1::Point2F(left, top), D2D1_FIGURE_BEGIN_FILLED);
    sink->AddLine(D2D1::Point2F(right, top));
    sink->AddLine(D2D1::Point2F(right, bottom));
    sink->AddLine(D2D1::Point2F(left, bottom));
    sink->EndFigure(D2D1_FIGURE_END_CLOSED);
    return;
  }

  sink->BeginFigure(
      D2D1::Point2F(left + (roundTopLeft ? rx : 0.0f), top),
      D2D1_FIGURE_BEGIN_FILLED);

  sink->AddLine(
      D2D1::Point2F(right - (roundTopRight ? rx : 0.0f), top));
  if (roundTopRight) {
    sink->AddArc(D2D1::ArcSegment(
        D2D1::Point2F(right, top + ry), D2D1::SizeF(rx, ry), 0.0f,
        D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
  }

  sink->AddLine(
      D2D1::Point2F(right, bottom - (roundBottomRight ? ry : 0.0f)));
  if (roundBottomRight) {
    sink->AddArc(D2D1::ArcSegment(
        D2D1::Point2F(right - rx, bottom), D2D1::SizeF(rx, ry), 0.0f,
        D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
  }

  sink->AddLine(
      D2D1::Point2F(left + (roundBottomLeft ? rx : 0.0f), bottom));
  if (roundBottomLeft) {
    sink->AddArc(D2D1::ArcSegment(
        D2D1::Point2F(left, bottom - ry), D2D1::SizeF(rx, ry), 0.0f,
        D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
  }

  sink->AddLine(
      D2D1::Point2F(left, top + (roundTopLeft ? ry : 0.0f)));
  if (roundTopLeft) {
    sink->AddArc(D2D1::ArcSegment(
        D2D1::Point2F(left + rx, top), D2D1::SizeF(rx, ry), 0.0f,
        D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
  }

  sink->EndFigure(D2D1_FIGURE_END_CLOSED);
}

void DirectWriteResources::FillRoundRect(const D2D1_RECT_F& rect,
                                         const D2D1_COLOR_F& color,
                                         float radiusX,
                                         float radiusY,
                                         bool roundTopLeft,
                                         bool roundTopRight,
                                         bool roundBottomRight,
                                         bool roundBottomLeft) {
  if (roundTopLeft && roundTopRight && roundBottomRight && roundBottomLeft &&
      radiusX > 0.0f && radiusY > 0.0f) {
    const D2D1_ROUNDED_RECT roundedRect =
        D2D1::RoundedRect(rect, radiusX, radiusY);
    pBrush->SetColor(color);
    pRenderTarget->FillRoundedRectangle(&roundedRect, pBrush.Get());
    return;
  }
  if ((!roundTopLeft && !roundTopRight && !roundBottomRight &&
       !roundBottomLeft) ||
      radiusX <= 0.0f || radiusY <= 0.0f) {
    pBrush->SetColor(color);
    pRenderTarget->FillRectangle(rect, pBrush.Get());
    return;
  }
  ComPtr<ID2D1PathGeometry> path = CreateRoundRectGeometry(
      rect, radiusX, radiusY, roundTopLeft, roundTopRight, roundBottomRight,
      roundBottomLeft);
  pBrush->SetColor(color);
  pRenderTarget->FillGeometry(path.Get(), pBrush.Get());
}

void DirectWriteResources::DrawRoundRect(const D2D1_RECT_F& rect,
                                         const D2D1_COLOR_F& color,
                                         float strokeWidth,
                                         float radiusX,
                                         float radiusY,
                                         bool roundTopLeft,
                                         bool roundTopRight,
                                         bool roundBottomRight,
                                         bool roundBottomLeft) {
  if (roundTopLeft && roundTopRight && roundBottomRight && roundBottomLeft &&
      radiusX > 0.0f && radiusY > 0.0f) {
    const D2D1_ROUNDED_RECT roundedRect =
        D2D1::RoundedRect(rect, radiusX, radiusY);
    pBrush->SetColor(color);
    pRenderTarget->DrawRoundedRectangle(&roundedRect, pBrush.Get(),
                                        strokeWidth);
    return;
  }
  if ((!roundTopLeft && !roundTopRight && !roundBottomRight &&
       !roundBottomLeft) ||
      radiusX <= 0.0f || radiusY <= 0.0f) {
    pBrush->SetColor(color);
    pRenderTarget->DrawRectangle(rect, pBrush.Get(), strokeWidth);
    return;
  }
  ComPtr<ID2D1PathGeometry> path = CreateRoundRectGeometry(
      rect, radiusX, radiusY, roundTopLeft, roundTopRight, roundBottomRight,
      roundBottomLeft);
  pBrush->SetColor(color);
  pRenderTarget->DrawGeometry(path.Get(), pBrush.Get(), strokeWidth);
}

void DirectWriteResources::DrawSoftShadow(const D2D1_RECT_F& rect,
                                          float radius,
                                          const D2D1_COLOR_F& color,
                                          float blurRadius,
                                          float offsetX,
                                          float offsetY,
                                          float blurMarginX,
                                          float blurMarginY,
                                          bool roundTopLeft,
                                          bool roundTopRight,
                                          bool roundBottomRight,
                                          bool roundBottomLeft) {
  if (!pRenderTarget || blurRadius <= 0.0f || color.a <= 0.0f)
    return;

  const float width = rect.right - rect.left;
  const float height = rect.bottom - rect.top;
  if (width <= 0.0f || height <= 0.0f)
    return;

  ComPtr<ID2D1DeviceContext> deviceContext;
  if (FAILED(pRenderTarget.As(&deviceContext)) || !deviceContext)
    return;

  const float marginX = (std::max)(0.0f, blurMarginX);
  const float marginY = (std::max)(0.0f, blurMarginY);
  const UINT bitmapWidth =
      static_cast<UINT>((std::max)(1.0f, width + marginX * 2.0f));
  const UINT bitmapHeight =
      static_cast<UINT>((std::max)(1.0f, height + marginY * 2.0f));

  if (!pShadowBitmap || shadowBitmapWidth < bitmapWidth ||
      shadowBitmapHeight < bitmapHeight) {
    pShadowBitmap.Reset();
    D2D1_BITMAP_PROPERTIES1 bitmapProperties = {};
    bitmapProperties.pixelFormat =
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
                          D2D1_ALPHA_MODE_PREMULTIPLIED);
    bitmapProperties.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET;
    bitmapProperties.dpiX = 96.0f;
    bitmapProperties.dpiY = 96.0f;
    const D2D1_SIZE_U size = {bitmapWidth, bitmapHeight};
    if (FAILED(deviceContext->CreateBitmap(
            size, nullptr, 0, &bitmapProperties,
            pShadowBitmap.ReleaseAndGetAddressOf())) ||
        !pShadowBitmap) {
      return;
    }
    shadowBitmapWidth = bitmapWidth;
    shadowBitmapHeight = bitmapHeight;
  }

  if (!pShadowBlurEffect) {
    if (FAILED(deviceContext->CreateEffect(
            CLSID_D2D1GaussianBlur,
            pShadowBlurEffect.ReleaseAndGetAddressOf())) ||
        !pShadowBlurEffect) {
      return;
    }
  }

  ComPtr<ID2D1Image> originalTarget;
  deviceContext->GetTarget(originalTarget.ReleaseAndGetAddressOf());
  D2D1_MATRIX_3X2_F originalTransform;
  deviceContext->GetTransform(&originalTransform);

  deviceContext->SetTarget(pShadowBitmap.Get());
  deviceContext->SetTransform(D2D1::Matrix3x2F::Identity());
  deviceContext->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));

  const D2D1_RECT_F shadowRect =
      D2D1::RectF(marginX + offsetX, marginY + offsetY,
                  marginX + width + offsetX, marginY + height + offsetY);
  FillRoundRect(shadowRect, color, radius, radius, roundTopLeft, roundTopRight,
                roundBottomRight, roundBottomLeft);

  deviceContext->SetTarget(originalTarget.Get());
  deviceContext->SetTransform(originalTransform);

  pShadowBlurEffect->SetInput(0, pShadowBitmap.Get());
  pShadowBlurEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION,
                              blurRadius * 1.6f);//阴影模糊半径
  pShadowBlurEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_BORDER_MODE,
                              D2D1_BORDER_MODE_HARD);
  deviceContext->DrawImage(
      pShadowBlurEffect.Get(),
      D2D1::Point2F(rect.left - marginX, rect.top - marginY));
}

void DirectWriteResources::DrawSoftShadowBatch(
    const std::vector<ShadowShape>& shapes,
    const D2D1_COLOR_F& color,
    float blurRadius,
    float offsetX,
    float offsetY,
    float blurMarginX,
    float blurMarginY) {
  if (!pRenderTarget || shapes.empty() || blurRadius <= 0.0f ||
      color.a <= 0.0f)
    return;

  ComPtr<ID2D1DeviceContext> deviceContext;
  if (FAILED(pRenderTarget.As(&deviceContext)) || !deviceContext)
    return;

  const float marginX = (std::max)(0.0f, blurMarginX);
  const float marginY = (std::max)(0.0f, blurMarginY);

  float minX = shapes.front().rect.left - marginX;
  float minY = shapes.front().rect.top - marginY;
  float maxX = shapes.front().rect.right + marginX;
  float maxY = shapes.front().rect.bottom + marginY;
  for (const ShadowShape& shape : shapes) {
    minX = (std::min)(minX, shape.rect.left - marginX);
    minY = (std::min)(minY, shape.rect.top - marginY);
    maxX = (std::max)(maxX, shape.rect.right + marginX);
    maxY = (std::max)(maxY, shape.rect.bottom + marginY);
  }

  const float width = maxX - minX;
  const float height = maxY - minY;
  if (width <= 0.0f || height <= 0.0f)
    return;

  const UINT bitmapWidth =
      static_cast<UINT>((std::max)(1.0f, std::ceil(width)));
  const UINT bitmapHeight =
      static_cast<UINT>((std::max)(1.0f, std::ceil(height)));

  if (!pShadowBitmap || shadowBitmapWidth < bitmapWidth ||
      shadowBitmapHeight < bitmapHeight) {
    pShadowBitmap.Reset();
    D2D1_BITMAP_PROPERTIES1 bitmapProperties = {};
    bitmapProperties.pixelFormat =
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
                          D2D1_ALPHA_MODE_PREMULTIPLIED);
    bitmapProperties.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET;
    bitmapProperties.dpiX = 96.0f;
    bitmapProperties.dpiY = 96.0f;
    const D2D1_SIZE_U size = {bitmapWidth, bitmapHeight};
    if (FAILED(deviceContext->CreateBitmap(
            size, nullptr, 0, &bitmapProperties,
            pShadowBitmap.ReleaseAndGetAddressOf())) ||
        !pShadowBitmap) {
      return;
    }
    shadowBitmapWidth = bitmapWidth;
    shadowBitmapHeight = bitmapHeight;
  }

  if (!pShadowBlurEffect) {
    if (FAILED(deviceContext->CreateEffect(
            CLSID_D2D1GaussianBlur,
            pShadowBlurEffect.ReleaseAndGetAddressOf())) ||
        !pShadowBlurEffect) {
      return;
    }
  }

  ComPtr<ID2D1Image> originalTarget;
  deviceContext->GetTarget(originalTarget.ReleaseAndGetAddressOf());
  D2D1_MATRIX_3X2_F originalTransform;
  deviceContext->GetTransform(&originalTransform);

  deviceContext->SetTarget(pShadowBitmap.Get());
  deviceContext->SetTransform(D2D1::Matrix3x2F::Identity());
  deviceContext->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));

  for (const ShadowShape& shape : shapes) {
    const D2D1_RECT_F localRect = D2D1::RectF(
        shape.rect.left + offsetX - minX, shape.rect.top + offsetY - minY,
        shape.rect.right + offsetX - minX, shape.rect.bottom + offsetY - minY);
    FillRoundRect(localRect, color, shape.radius, shape.radius,
                  shape.roundTopLeft, shape.roundTopRight,
                  shape.roundBottomRight, shape.roundBottomLeft);
  }

  deviceContext->SetTarget(originalTarget.Get());
  deviceContext->SetTransform(originalTransform);

  pShadowBlurEffect->SetInput(0, pShadowBitmap.Get());
  pShadowBlurEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION,
                              blurRadius * 1.6f);  // 阴影模糊半径
  pShadowBlurEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_BORDER_MODE,
                              D2D1_BORDER_MODE_HARD);
  deviceContext->DrawImage(pShadowBlurEffect.Get(),
                           D2D1::Point2F(minX, minY));
}

void DirectWriteResources::FillRoundRectBatch(
    const std::vector<ShadowShape>& shapes,
    const D2D1_COLOR_F& color) {
  if (shapes.empty() || !pRenderTarget || color.a <= 0.0f)
    return;
  ComPtr<ID2D1PathGeometry> path = CreateRoundRectGeometryBatch(shapes);
  if (!path)
    return;
  pBrush->SetColor(color);
  pRenderTarget->FillGeometry(path.Get(), pBrush.Get());
}

void DirectWriteResources::DrawRoundRectBatch(
    const std::vector<ShadowShape>& shapes,
    const D2D1_COLOR_F& color,
    float strokeWidth) {
  if (shapes.empty() || !pRenderTarget || color.a <= 0.0f || strokeWidth <= 0.0f)
    return;
  ComPtr<ID2D1PathGeometry> path = CreateRoundRectGeometryBatch(shapes);
  if (!path)
    return;
  pBrush->SetColor(color);
  pRenderTarget->DrawGeometry(path.Get(), pBrush.Get(), strokeWidth);
}

HRESULT DirectWriteResources::InitResources(const wstring& label_font_face,
                                            const int& label_font_point,
                                            const wstring& font_face,
                                            const int& font_point,
                                            const wstring& comment_font_face,
                                            const int& comment_font_point,
                                            const bool& vertical_text) {
  // prepare d2d1 resources
  DWRITE_WORD_WRAPPING wrapping =
      ((_style.max_width == 0 &&
        _style.layout_type != UIStyle::LAYOUT_VERTICAL_TEXT) ||
       (_style.max_height == 0 &&
        _style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT))
          ? DWRITE_WORD_WRAPPING_NO_WRAP
          : DWRITE_WORD_WRAPPING_WHOLE_WORD;
  DWRITE_WORD_WRAPPING wrapping_preedit =
      ((_style.max_width == 0 &&
        _style.layout_type != UIStyle::LAYOUT_VERTICAL_TEXT) ||
       (_style.max_height == 0 &&
        _style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT))
          ? DWRITE_WORD_WRAPPING_NO_WRAP
          : DWRITE_WORD_WRAPPING_CHARACTER;
  DWRITE_FLOW_DIRECTION flow = _style.vertical_text_left_to_right
                                   ? DWRITE_FLOW_DIRECTION_LEFT_TO_RIGHT
                                   : DWRITE_FLOW_DIRECTION_RIGHT_TO_LEFT;

  // set main font a invalid font name, to make every font range customizable
  const wstring _mainFontFace = L"_InvalidFontName_";
  DWRITE_FONT_WEIGHT fontWeight = DWRITE_FONT_WEIGHT_NORMAL;
  DWRITE_FONT_STYLE fontStyle = DWRITE_FONT_STYLE_NORMAL;
  // convert percentage to float
  float linespacing = dpiScaleFontPoint * ((float)_style.linespacing / 100.0f);
  float baseline = dpiScaleFontPoint * ((float)_style.baseline / 100.0f);
  if (_style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT)
    baseline = linespacing / 2;

  // `vertical` is only set for the formats used by the candidate columns: the
  // encoding (preedit) text is always laid out horizontally in the upper band.
  auto init_font = [&](const wstring& fontface, int fontpoint,
                       ComPtr<IDWriteTextFormat1>& _pTextFormat,
                       DWRITE_WORD_WRAPPING wrap, bool vertical) {
    vector<wstring> fontFaceStrVector;
    // text font text format set up
    fontFaceStrVector = ws_split(fontface, L",");
    // setup weight and style by the first unit of fontface setting string
    _ParseFontFace(fontface, fontWeight, fontStyle);
    fontFaceStrVector[0] =
        std::regex_replace(fontFaceStrVector[0],
                           std::wregex(STYLEORWEIGHT, std::wregex::icase), L"");
    // create text format with invalid font point will 'FAILED', no HR
    pDWFactory->CreateTextFormat(_mainFontFace.c_str(), NULL, fontWeight,
                                 fontStyle, DWRITE_FONT_STRETCH_NORMAL,
                                 fontpoint * dpiScaleFontPoint, L"",
                                 reinterpret_cast<IDWriteTextFormat**>(
                                     _pTextFormat.ReleaseAndGetAddressOf()));
    if (_pTextFormat != NULL) {
      if (vertical) {
        HR(_pTextFormat->SetFlowDirection(flow));
        HR(_pTextFormat->SetReadingDirection(
            DWRITE_READING_DIRECTION_TOP_TO_BOTTOM));
        // keep every glyph upright and stack the glyphs along the column
        // instead of rotating them 90 degrees
        HR(_pTextFormat->SetVerticalGlyphOrientation(
            DWRITE_VERTICAL_GLYPH_ORIENTATION_STACKED));
      }
      HR(_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING));

      HR(_pTextFormat->SetParagraphAlignment(
          DWRITE_PARAGRAPH_ALIGNMENT_CENTER));
      HR(_pTextFormat->SetWordWrapping(wrapping));
      _SetFontFallback(_pTextFormat, fontFaceStrVector);
      if (_style.linespacing && _style.baseline)
        _pTextFormat->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM,
                                     fontpoint * linespacing,
                                     fontpoint * baseline);
    }
    decltype(fontFaceStrVector)().swap(fontFaceStrVector);
  };
  init_font(font_face, font_point, pTextFormat, wrapping, vertical_text);
  init_font(font_face, font_point, pPreeditTextFormat, wrapping_preedit, false);
  init_font(label_font_face, label_font_point, pLabelTextFormat, wrapping,
            vertical_text);
  init_font(comment_font_face, comment_font_point, pCommentTextFormat, wrapping,
            vertical_text);
  return S_OK;
}

HRESULT DirectWriteResources::InitResources(const UIStyle& style,
                                            const UINT& dpi = 96) {
  _style = style;
  if (dpi) {
    dpiScaleFontPoint = dpi / 72.0f;
    dpiScaleLayout = dpi / 96.0f;
  }
  return InitResources(style.label_font_face, style.label_font_point,
                       style.font_face, style.font_point,
                       style.comment_font_face, style.comment_font_point,
                       style.layout_type == UIStyle::LAYOUT_VERTICAL_TEXT);
}

void weasel::DirectWriteResources::SetDpi(const UINT& dpi) {
  dpiScaleFontPoint = dpi / 72.0f;
  dpiScaleLayout = dpi / 96.0f;

  InitResources(_style);
}

static wstring _MatchWordsOutLowerCaseTrim1st(const wstring& wstr,
                                              const wstring& pat) {
  wstring mat = L"";
  std::wsmatch mc;
  std::wregex pattern(pat, std::wregex::icase);
  wstring::const_iterator iter = wstr.cbegin();
  wstring::const_iterator end = wstr.cend();
  while (regex_search(iter, end, mc, pattern)) {
    for (const auto& m : mc) {
      mat = m;
      mat = mat.substr(1);
      break;
    }
    iter = mc.suffix().first;
  }
  wstring res;
  std::transform(mat.begin(), mat.end(), std::back_inserter(res), ::tolower);
  return res;
}

void DirectWriteResources::_ParseFontFace(const wstring& fontFaceStr,
                                          DWRITE_FONT_WEIGHT& fontWeight,
                                          DWRITE_FONT_STYLE& fontStyle) {
  const wstring patWeight(
      L"(:thin|:extra_light|:ultra_light|:light|:semi_light|:medium|:demi_bold|"
      L":semi_bold|:bold|:extra_bold|:ultra_bold|:black|:heavy|:extra_black|:"
      L"ultra_black)");
  const std::map<wstring, DWRITE_FONT_WEIGHT> _mapWeight = {
      {L"thin", DWRITE_FONT_WEIGHT_THIN},
      {L"extra_light", DWRITE_FONT_WEIGHT_EXTRA_LIGHT},
      {L"ultra_light", DWRITE_FONT_WEIGHT_ULTRA_LIGHT},
      {L"light", DWRITE_FONT_WEIGHT_LIGHT},
      {L"semi_light", DWRITE_FONT_WEIGHT_SEMI_LIGHT},
      {L"medium", DWRITE_FONT_WEIGHT_MEDIUM},
      {L"demi_bold", DWRITE_FONT_WEIGHT_DEMI_BOLD},
      {L"semi_bold", DWRITE_FONT_WEIGHT_SEMI_BOLD},
      {L"bold", DWRITE_FONT_WEIGHT_BOLD},
      {L"extra_bold", DWRITE_FONT_WEIGHT_EXTRA_BOLD},
      {L"ultra_bold", DWRITE_FONT_WEIGHT_ULTRA_BOLD},
      {L"black", DWRITE_FONT_WEIGHT_BLACK},
      {L"heavy", DWRITE_FONT_WEIGHT_HEAVY},
      {L"extra_black", DWRITE_FONT_WEIGHT_EXTRA_BLACK},
      {L"normal", DWRITE_FONT_WEIGHT_NORMAL},
      {L"ultra_black", DWRITE_FONT_WEIGHT_ULTRA_BLACK}};
  wstring weight = _MatchWordsOutLowerCaseTrim1st(fontFaceStr, patWeight);
  auto it = _mapWeight.find(weight);
  fontWeight =
      (it != _mapWeight.end()) ? it->second : DWRITE_FONT_WEIGHT_NORMAL;

  const wstring patStyle(L"(:italic|:oblique|:normal)");
  const std::map<wstring, DWRITE_FONT_STYLE> _mapStyle = {
      {L"italic", DWRITE_FONT_STYLE_ITALIC},
      {L"oblique", DWRITE_FONT_STYLE_OBLIQUE},
      {L"normal", DWRITE_FONT_STYLE_NORMAL},
  };
  wstring style = _MatchWordsOutLowerCaseTrim1st(fontFaceStr, patStyle);
  auto it2 = _mapStyle.find(style);
  fontStyle = (it2 != _mapStyle.end()) ? it2->second : DWRITE_FONT_STYLE_NORMAL;
}

static UINT GetValue(const wstring& wstr, const int fallback) {
  try {
    return std::stoul(wstr.c_str(), 0, 16);
  } catch (...) {
    return fallback;
  }
}

void DirectWriteResources::_SetFontFallback(
    ComPtr<IDWriteTextFormat1>& textFormat,
    const vector<wstring>& fontVector) {
  ComPtr<IDWriteFontFallback> pSysFallback;
  HR(pDWFactory->GetSystemFontFallback(pSysFallback.ReleaseAndGetAddressOf()));
  ComPtr<IDWriteFontFallback> pFontFallback = NULL;
  ComPtr<IDWriteFontFallbackBuilder> pFontFallbackBuilder = NULL;
  HR(pDWFactory->CreateFontFallbackBuilder(
      pFontFallbackBuilder.ReleaseAndGetAddressOf()));
  vector<wstring> fallbackFontsVector;
  for (UINT32 i = 0; i < fontVector.size(); i++) {
    fallbackFontsVector = ws_split(fontVector[i], L":");
    wstring _fontFaceWstr, firstWstr, lastWstr;
    if (fallbackFontsVector.size() == 3) {
      _fontFaceWstr = fallbackFontsVector[0];
      firstWstr = fallbackFontsVector[1];
      lastWstr = fallbackFontsVector[2];
      if (lastWstr.empty())
        lastWstr = L"10ffff";
      if (firstWstr.empty())
        firstWstr = L"0";
    } else if (fallbackFontsVector.size() == 2)  // fontName : codepoint
    {
      _fontFaceWstr = fallbackFontsVector[0];
      firstWstr = fallbackFontsVector[1];
      if (firstWstr.empty())
        firstWstr = L"0";
      lastWstr = L"10ffff";
    } else if (fallbackFontsVector.size() ==
               1)  // if only font defined, use all range
    {
      _fontFaceWstr = fallbackFontsVector[0];
      firstWstr = L"0";
      lastWstr = L"10ffff";
    }
    UINT first = GetValue(firstWstr, 0), last = GetValue(lastWstr, 0x10ffff);
    DWRITE_UNICODE_RANGE range = {first, last};
    const WCHAR* familys = {_fontFaceWstr.c_str()};
    HR(pFontFallbackBuilder->AddMapping(&range, 1, &familys, 1));
    decltype(fallbackFontsVector)().swap(fallbackFontsVector);
  }
  // add system defalt font fallback
  HR(pFontFallbackBuilder->AddMappings(pSysFallback.Get()));
  HR(pFontFallbackBuilder->CreateFontFallback(
      pFontFallback.ReleaseAndGetAddressOf()));
  HR(textFormat->SetFontFallback(pFontFallback.Get()));
  decltype(fallbackFontsVector)().swap(fallbackFontsVector);
}
