#pragma once
#include "Deserializer.h"

class ContextUpdater : public weasel::Deserializer {
 public:
  ContextUpdater(weasel::ResponseParser* pTarget);
  virtual ~ContextUpdater();
  virtual void Store(const weasel::Deserializer::KeyType& key,
                     const std::wstring& value);

  void _StoreText(weasel::Text& target,
                  const Deserializer::KeyType& k,
                  const std::wstring& value);
  void _StoreCand(const Deserializer::KeyType& k, const std::wstring& value);

  static weasel::Deserializer::Ptr Create(weasel::ResponseParser* pTarget);
};

class StatusUpdater : public weasel::Deserializer {
 public:
  StatusUpdater(weasel::ResponseParser* pTarget);
  virtual ~StatusUpdater();
  virtual void Store(const weasel::Deserializer::KeyType& key,
                     const std::wstring& value);

  static weasel::Deserializer::Ptr Create(weasel::ResponseParser* pTarget);
};
