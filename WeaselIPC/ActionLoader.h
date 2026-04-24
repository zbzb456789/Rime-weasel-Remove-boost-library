#pragma once
#include "Deserializer.h"

class ActionLoader : public weasel::Deserializer {
 public:
  ActionLoader(weasel::ResponseParser* pTarget);
  virtual ~ActionLoader();
  // store data
  virtual void Store(const weasel::Deserializer::KeyType& key,
                     const std::wstring& value);
  // factory method
  static weasel::Deserializer::Ptr Create(weasel::ResponseParser* pTarget);
};
