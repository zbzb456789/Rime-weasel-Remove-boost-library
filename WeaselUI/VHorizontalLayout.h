#pragma once

#include "StandardLayout.h"

namespace weasel {
class VHorizontalLayout : public StandardLayout {
 public:
  VHorizontalLayout(const UIStyle& style,
                    const Context& context,
                    const Status& status,
                    PDWR pDWR)
      : StandardLayout(style, context, status, pDWR) {}
  virtual void DoLayout(CDCHandle dc, PDWR pDWR = NULL);
  // The encoding bar is drawn horizontally along the top of the window, so the
  // preedit / auxiliary text is never a vertical column in this layout.
  virtual bool IsVerticalPreedit() const { return false; }

 private:
  void DoLayoutWithWrap(CDCHandle dc, PDWR pDWR = NULL);
};
};  // namespace weasel
