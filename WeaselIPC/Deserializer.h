#pragma once
#include <ResponseParser.h>
#include <functional>

namespace weasel {

template <typename T>
void TryDeserialize(weasel::archive::text_wiarchive& ia, T& t) {
  try {
    ia >> t;
  } catch (...) {
    // Silently ignore deserialization errors:
    // A MessageBox in an IME Dll inevitably freezes the system message pump (Explorer)
  }
}
class Deserializer {
 public:
  typedef std::vector<std::wstring> KeyType;
  typedef std::shared_ptr<Deserializer> Ptr;
  typedef std::function<Ptr(ResponseParser* pTarget)> Factory;

  Deserializer(ResponseParser* pTarget) : m_pTarget(pTarget) {}
  virtual ~Deserializer() {}
  virtual void Store(const KeyType& key, const std::wstring& value) {}

  static void Initialize(ResponseParser* pTarget);
  static void Define(const std::wstring& action, Factory factory);
  static bool Require(const std::wstring& action, ResponseParser* pTarget);

 protected:
  ResponseParser* m_pTarget;

 private:
  static std::map<std::wstring, Factory> s_factories;
};

}  // namespace weasel
