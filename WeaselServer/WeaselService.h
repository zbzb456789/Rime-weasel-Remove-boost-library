#pragma once

#include <Windows.h>
#include <memory>
#include <thread>            // <-- 新增
#include "WeaselServerApp.h"

class WeaselService {
 public:
  WeaselService(BOOL fCanStop, BOOL fCanShutdown, BOOL fCanPauseContinue);
  ~WeaselService();
  static BOOL Run(WeaselService& serv);

  void Stop();
  // Start the service.
  void Start(DWORD dwArgc, PWSTR* pszArgv);

 protected:
  // Entry point for the service. It registers the handler function for the
  // service and starts the service.
  static void WINAPI ServiceMain(DWORD dwArgc, PWSTR* pszArgv);

  // The function is called by the SCM whenever a control code is sent to
  // the service.
  static void WINAPI ServiceCtrlHandler(DWORD dwCtrl);

  void SetServiceStatus(DWORD dwCurrentState,
                        DWORD dwWin32ExitCode = NO_ERROR,
                        DWORD dwWaitHint = 0);

  // Execute when the system is shutting down.
  void Shutdown();

 private:
  static WeaselService* _service;

  SERVICE_STATUS _status;
  SERVICE_STATUS_HANDLE _statusHandle;

  BOOL _stopping;
  HANDLE _stoppedEvent;

  WeaselServerApp app;

  std::jthread _worker;   // <-- 新增：保存线程对象以防临时对象并消除警告
};

extern wchar_t WEASEL_SERVICE_NAME[];
