; RimeUI installation script
!include FileFunc.nsh
!include LogicLib.nsh
!include MUI2.nsh
!include x64.nsh
!include winVer.nsh

Unicode true

;--------------------------------
; General

!ifndef WEASEL_VERSION
!define WEASEL_VERSION 0.1.0
!endif

!ifndef WEASEL_BUILD
!define WEASEL_BUILD 0
!endif

!define WEASEL_ROOT $INSTDIR\rimeui-${WEASEL_VERSION}
!define REG_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\RimeUI"

; The name of the installer
Name "RimeUI ${WEASEL_VERSION}"

; The file to write
OutFile "archives\rimeui-${WEASEL_VERSION}-installer.exe"

VIProductVersion "${WEASEL_VERSION}.${WEASEL_BUILD}"
VIAddVersionKey /LANG=2052 "ProductName" "RimeUI"
VIAddVersionKey /LANG=2052 "Comments" "Powered by RIME | 中州韻輸入法引擎"
VIAddVersionKey /LANG=2052 "CompanyName" "式恕堂"
VIAddVersionKey /LANG=2052 "LegalCopyright" "Copyleft RIME Developers"
VIAddVersionKey /LANG=2052 "FileDescription" "RimeUI輸入法"
VIAddVersionKey /LANG=2052 "FileVersion" "${WEASEL_VERSION}"

!define MUI_ICON ..\resource\rimeui.ico
SetCompressor /SOLID lzma


; Request application privileges for Windows Vista
RequestExecutionLevel admin

;--------------------------------

; Pages

!insertmacro MUI_PAGE_LICENSE "LICENSE.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
;!define MUI_FINISHPAGE_RUN "$INSTDIR\RimeUISettings.exe"
;!define MUI_FINISHPAGE_RUN_NOTCHECKED ; 默认不勾选（如果不加这行则默认勾选）

!define MUI_FINISHPAGE_REBOOTLATER_DEFAULT ; <--- 将默认单选框指向“稍后再重新启动”
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

;--------------------------------

; Languages

!insertmacro MUI_LANGUAGE "TradChinese"
LangString DISPLAYNAME ${LANG_TRADCHINESE} "RimeUI輸入法"
LangString LNKFORMANUAL ${LANG_TRADCHINESE} "【RimeUI】說明書"
LangString LNKFORSETTING ${LANG_TRADCHINESE} "【RimeUI】輸入法設定"
LangString LNKFORDICT ${LANG_TRADCHINESE} "【RimeUI】用戶詞典管理"
LangString LNKFORSYNC ${LANG_TRADCHINESE} "【RimeUI】用戶資料同步"
LangString LNKFORDEPLOY ${LANG_TRADCHINESE} "【RimeUI】重新部署"
LangString LNKFORSERVER ${LANG_TRADCHINESE} "RimeUI算法服務"
LangString LNKFORUSERFOLDER ${LANG_TRADCHINESE} "【RimeUI】用戶文件夾"
LangString LNKFORAPPFOLDER ${LANG_TRADCHINESE} "【RimeUI】程序文件夾"
LangString LNKFORUPDATER ${LANG_TRADCHINESE} "【RimeUI】檢查新版本"
LangString LNKFORSETUP ${LANG_TRADCHINESE} "【RimeUI】安裝選項"
LangString LNKFORUNINSTALL ${LANG_TRADCHINESE} "卸載RimeUI"
LangString CONFIRMATION ${LANG_TRADCHINESE} "安裝前，請先卸載舊版本的RimeUI。$\n$\n按下「確定」移除舊版本，按下「取消」放棄本次安裝。"
LangString SYSTEMVERSIONNOTOK ${LANG_TRADCHINESE} "您的系统不被支持，最低系統要求:Windows 8.1!"
LangString AUTOCHKUPDATE ${LANG_TRADCHINESE} "自動檢查版本更新？"

!insertmacro MUI_LANGUAGE "SimpChinese"
LangString DISPLAYNAME ${LANG_SIMPCHINESE} "RimeUI输入法"
LangString LNKFORMANUAL ${LANG_SIMPCHINESE} "【RimeUI】说明书"
LangString LNKFORSETTING ${LANG_SIMPCHINESE} "【RimeUI】输入法设定"
LangString LNKFORDICT ${LANG_SIMPCHINESE} "【RimeUI】用户词典管理"
LangString LNKFORSYNC ${LANG_SIMPCHINESE} "【RimeUI】用户资料同步"
LangString LNKFORDEPLOY ${LANG_SIMPCHINESE} "【RimeUI】重新部署"
LangString LNKFORSERVER ${LANG_SIMPCHINESE} "RimeUI算法服务"
LangString LNKFORUSERFOLDER ${LANG_SIMPCHINESE} "【RimeUI】用户文件夹"
LangString LNKFORAPPFOLDER ${LANG_SIMPCHINESE} "【RimeUI】程序文件夹"
LangString LNKFORUPDATER ${LANG_SIMPCHINESE} "【RimeUI】检查新版本"
LangString LNKFORSETUP ${LANG_SIMPCHINESE} "【RimeUI】安装选项"
LangString LNKFORUNINSTALL ${LANG_SIMPCHINESE} "卸载RimeUI"
LangString CONFIRMATION ${LANG_SIMPCHINESE} '安装前，请先卸载旧版本的RimeUI。$\n$\n点击 "确定" 移除旧版本，或点击 "取消" 放弃本次安装。'
LangString SYSTEMVERSIONNOTOK ${LANG_SIMPCHINESE} "您的系統不被支持，最低系统要求:Windows 8.1!"
LangString AUTOCHKUPDATE ${LANG_SIMPCHINESE} "自动检查版本更新？"

!insertmacro MUI_LANGUAGE "English"
LangString DISPLAYNAME ${LANG_ENGLISH} "RimeUI"
LangString LNKFORMANUAL ${LANG_ENGLISH} "RimeUI Manual"
LangString LNKFORSETTING ${LANG_ENGLISH} "RimeUI Settings"
LangString LNKFORDICT ${LANG_ENGLISH} "RimeUI Dictionary Manager"
LangString LNKFORSYNC ${LANG_ENGLISH} "RimeUI Sync User Profile"
LangString LNKFORDEPLOY ${LANG_ENGLISH} "RimeUI Deploy"
LangString LNKFORSERVER ${LANG_ENGLISH} "RimeUI Server"
LangString LNKFORUSERFOLDER ${LANG_ENGLISH} "RimeUI User Folder"
LangString LNKFORAPPFOLDER ${LANG_ENGLISH} "RimeUI App Folder"
LangString LNKFORUPDATER ${LANG_ENGLISH} "RimeUI Check for Updates"
LangString LNKFORSETUP ${LANG_ENGLISH} "RimeUI Installation Preference"
LangString LNKFORUNINSTALL ${LANG_ENGLISH} "Uninstall RimeUI"
LangString CONFIRMATION ${LANG_ENGLISH} "Before installation, please uninstall the old version of RimeUI.$\n$\nPress 'OK' to remove the old version, or 'Cancel' to abort installation."
LangString SYSTEMVERSIONNOTOK ${LANG_ENGLISH} "Your system not supported, minimium system required: Windows 8.1!"
LangString AUTOCHKUPDATE ${LANG_ENGLISH} "Automatically check for updates?"

;--------------------------------

Function .onInit
  ; if not version >= 8.1, quit and MessageBox(if not silent)
  ${IfNot} ${AtLeastWin8.1}
    IfSilent toquit
    MessageBox MB_OK '$(SYSTEMVERSIONNOTOK)'
toquit:
    Quit
  ${EndIf}

  ReadRegStr $R0 HKLM "Software\RimeUI" "InstallDir"
  StrCmp $R0 "" 0 skip
  ; The default installation directory
  ; install x64 build for NativeARM64_WINDOWS11 and NativeAMD64_WINDOWS11
  ${If} ${AtLeastWin11} ; Windows 11 and above
    ${If} ${IsNativeARM64}
      StrCpy $INSTDIR "$PROGRAMFILES64\RimeUI"
    ${ElseIf} ${IsNativeAMD64}
      StrCpy $INSTDIR "$PROGRAMFILES64\RimeUI"
    ${Else}
      StrCpy $INSTDIR "$PROGRAMFILES\RimeUI"
    ${Endif}
  ; install x64 build for NativeAMD64_BELLOW_WINDOWS11
  ${Else} ; Windows 10 or bellow
    ${If} ${IsNativeAMD64}
      StrCpy $INSTDIR "$PROGRAMFILES64\RimeUI"
    ${Else}
      StrCpy $INSTDIR "$PROGRAMFILES\RimeUI"
    ${Endif}
  ${Endif}
skip:
  ReadRegStr $R0 HKLM \
  "Software\Microsoft\Windows\CurrentVersion\Uninstall\RimeUI" \
  "UninstallString"
  StrCmp $R0 "" done

  StrCpy $0 "Upgrade"
  IfSilent uninst 0
  MessageBox MB_OKCANCEL|MB_ICONINFORMATION "$(CONFIRMATION)" IDOK uninst
  Abort

uninst:
  ; Backup data directory from previous installation, user files may exist
  ReadRegStr $R1 HKLM SOFTWARE\RimeUI "RimeUIRoot"
  
  
  ; 使用寄存器 $R2 存储最终找到的路径，初始化为空
  StrCpy $R2 ""

  ; --- 步骤 1: 默认视图读取 (对应 RegistryView.Default) ---
  ; 如果是64位系统，先尝试切到64位视图（保持默认行为）
  ${If} ${RunningX64}
    SetRegView 64
  ${EndIf}
  
  ReadRegStr $R2 HKCU "Software\RimeUI" "RimeUserDir"
  
  ; 检查路径是否有效 (非空且存在)
  ${If} $R2 != ""
  ${AndIf} ${FileExists} "$R2\*.*"
      DetailPrint "通过注册表(默认视图)找到用户路径: $R2"
      Goto DoBackup
  ${EndIf}

  ; --- 步骤 2: 兼容视图读取 (对应 RegistryView.Registry32) ---
  SetRegView 32
  ReadRegStr $R2 HKCU "Software\RimeUI" "RimeUserDir"
  
  ; 恢复视图设置，避免影响后续操作
  ${If} ${RunningX64}
    SetRegView 64
  ${EndIf}

  ; 检查路径是否有效
  ${If} $R2 != ""
  ${AndIf} ${FileExists} "$R2\*.*"
      DetailPrint "通过注册表(32位视图)找到用户路径: $R2"
      Goto DoBackup
  ${EndIf}

  ; --- 步骤 3: 默认路径 (对应 Environment.GetFolderPath) ---
  ; 如果注册表都没找到，使用默认 AppData 路径
  StrCpy $R2 "$APPDATA\RimeUI"
  DetailPrint "使用默认用户路径: $R2"

  ; --- 执行备份操作 ---
  DoBackup:
  
  ; 再次确认目录存在才备份
  ${If} ${FileExists} "$R2\*.*"
      ; === 新增：获取当前时间并格式化 ===
      ${GetTime} "" "L" $R9 $R8 $R7 $R6 $R5 $R4 $R3
      ; $R7=年, $R8=月, $R9=日, $R5=时, $R4=分, $R3=秒
      StrCpy $2 "$R7$R8$R9_$R5$R4$R3" 

      ; 修改：将时间戳 $2 加入到备份路径中
      StrCpy $1 "$DESKTOP\Rime_Backup_$2"
      
      DetailPrint "正在备份用户数据到: $1"
      CreateDirectory "$1"
      
      CopyFiles /SILENT "$R2\*" "$1"
      
      MessageBox MB_OK "检测到旧的用户数据，已备份至桌面的 [Rime_Backup_$2] 文件夹。"
  ${Else}
      DetailPrint "未找到有效的用户数据目录，跳过备份。"
  ${EndIf}

  ;===================================================================
  ; [结束] 用户数据备份逻辑
  ;===================================================================
  
  
  StrCmp $R1 "" call_uninstaller
  IfFileExists $R1\data\*.* 0 call_uninstaller
  
  ExecWait '"$R1\RimeUIServer.exe" /quit'
  RMDir /r "$R2"

call_uninstaller:
  ExecWait '"$R1\RimeUIServer.exe" /quit'
  ExecWait '"$R1\RimeUISetup.exe" /u'
  ; Remove registry keys
  DeleteRegKey HKLM SOFTWARE\RimeUI
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\RimeUI"
  ; don't redirect on 64 bit system for auto run setting
  ${If} ${IsNativeARM64}
    SetRegView 64
  ${ElseIf} ${IsNativeAMD64}
    SetRegView 64
  ${Endif}
  DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Run" "RimeUIServer"
  ; recover back to 32bit view
  SetRegView 32
  ; Remove files and uninstaller
  Delete  "$R1\data\opencc\*.*"
  Delete  "$R1\data\preview\*.*"
  Delete  "$R1\data\*.*"
  Delete  "$R1\*.*"
  RMDir   "$R1\data\opencc"
  RMDir   "$R1\data\preview"
  RMDir   "$R1\data"
  RMDir   "$R1"
  SetShellVarContext all
  Delete  "$SMPROGRAMS\$(DISPLAYNAME)\*.*"
  RMDir  "$SMPROGRAMS\$(DISPLAYNAME)"
  ; Prompt reboot
  SetRebootFlag true
  Sleep 800

done:
FunctionEnd

; Registry key to check for directory (so if you install again, it will
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\RimeUI" "InstallDir"

; The stuff to install
Section "RimeUI"

  SectionIn RO

  ; Write the new installation path into the registry
  ; redirect on 64 bit system
  ; HKLM SOFTWARE\WOW6432Node\Rime\RimeUI "InstallDir" "$INSTDIR"
  WriteRegStr HKLM SOFTWARE\RimeUI "InstallDir" "$INSTDIR"

  ; Reset INSTDIR for the new version
  StrCpy $INSTDIR "${WEASEL_ROOT}"

  IfFileExists "$INSTDIR\RimeUIServer.exe" 0 +2
  ExecWait '"$INSTDIR\RimeUIServer.exe" /quit'

  SetOverwrite try
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR

  ;IfFileExists $TEMP\rimeui-backup\*.* 0 program_files
  ;CreateDirectory $INSTDIR\data
  ;CopyFiles $TEMP\rimeui-backup\*.* $INSTDIR\data
  ;RMDir /r $TEMP\rimeui-backup

program_files:
  File "LICENSE.txt"
  File "README.txt"
  File "7-zip-license.txt"
  File "7z.dll"
  File "7z.exe"
  File "COPYING-curl.txt"
  File "curl.exe"
  File "curl-ca-bundle.crt"
  File "install.bat"
  File "rime-install-config.bat"
  File "start_service.bat"
  File "stop_service.bat"
  File "rimeui.dll"
  File "RimeUISettings.exe"
  File "data.zip"
  File "av_libglesv2.dll"
  File "libHarfBuzzSharp.dll"
  File "libSkiaSharp.dll"
  ${If} ${RunningX64}
    File "rimeuix64.dll"
  ${EndIf}
  ${If} ${IsNativeARM64}
    File /nonfatal "rimeuiARM.dll"
    File /nonfatal "rimeuiARM64.dll"
    File /nonfatal "rimeuiARM64X.dll"
  ${EndIf}
  File "rimeui.ime"
  ${If} ${RunningX64}
    File "rimeuix64.ime"
  ${EndIf}
  ${If} ${IsNativeARM64}
    File /nonfatal "rimeuiARM.ime"
    File /nonfatal "rimeuiARM64.ime"
    File /nonfatal "rimeuiARM64X.ime"
  ${EndIf}
  ; install x64 build for NativeARM64_WINDOWS11 and NativeAMD64_WINDOWS11
  ${If} ${AtLeastWin11} ; Windows 11 and above
    ${If} ${IsNativeARM64}
      File "RimeUIDeployer.exe"
      File "RimeUIServer.exe"
      File "rime.dll"
      File "WinSparkle.dll"
    ${ElseIf} ${IsNativeAMD64}
      File "RimeUIDeployer.exe"
      File "RimeUIServer.exe"
      File "rime.dll"
      File "WinSparkle.dll"
    ${Else}
      File "Win32\RimeUIDeployer.exe"
      File "Win32\RimeUIServer.exe"
      File "Win32\rime.dll"
      File "Win32\WinSparkle.dll"
    ${Endif}
  ; install x64 build for NativeAMD64_BELLOW_WINDOWS11
  ${Else} ; Windows 10 or bellow
    ${If} ${IsNativeAMD64}
      File "RimeUIDeployer.exe"
      File "RimeUIServer.exe"
      File "rime.dll"
      File "WinSparkle.dll"
    ${Else}
      File "Win32\RimeUIDeployer.exe"
      File "Win32\RimeUIServer.exe"
      File "Win32\rime.dll"
      File "Win32\WinSparkle.dll"
    ${Endif}
  ${Endif}

  File "RimeUISetup.exe"
  ; shared data files
  SetOutPath $INSTDIR\data
  File "data\*.yaml"
  File /nonfatal "data\*.txt"
  ;File /nonfatal "data\*.gram"
  ; opencc data files
  SetOutPath $INSTDIR\data\opencc
  File "data\opencc\*.json"
  File "data\opencc\*.ocd*"
  ; images
  ;SetOutPath $INSTDIR\data\preview
  ;File "data\preview\*.png"

  SetOutPath $INSTDIR

  ; test /T flag for zh_TW locale
  StrCpy $R2 "/i"
  ${GetParameters} $R0
  ClearErrors
  ${GetOptions} $R0 "/S" $R1
  IfErrors +2 0
  StrCpy $R2 "/s"
  ${GetOptions} $R0 "/T" $R1
  IfErrors +2 0
  StrCpy $R2 "/t"

  ExecWait '"$INSTDIR\RimeUISetup.exe" $R2'

  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "${REG_UNINST_KEY}" "DisplayName" "$(DISPLAYNAME)"
  WriteRegStr HKLM "${REG_UNINST_KEY}" "DisplayIcon" '"$INSTDIR\RimeUIServer.exe"'
  WriteRegStr HKLM "${REG_UNINST_KEY}" "DisplayVersion" "${WEASEL_VERSION}.${WEASEL_BUILD}"
  WriteRegStr HKLM "${REG_UNINST_KEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr HKLM "${REG_UNINST_KEY}" "Publisher" "式恕堂"
  WriteRegStr HKLM "${REG_UNINST_KEY}" "URLInfoAbout" "https://rime.im/"
  WriteRegStr HKLM "${REG_UNINST_KEY}" "HelpLink" "https://rime.im/docs/"
  WriteRegDWORD HKLM "${REG_UNINST_KEY}" "NoModify" 1
  WriteRegDWORD HKLM "${REG_UNINST_KEY}" "NoRepair" 1
  WriteUninstaller "$INSTDIR\uninstall.exe"

  ; run as user...
  IfSilent deploy_silently
  ;ExecWait "$INSTDIR\RimeUISetup.exe"
  ;ExecWait "$INSTDIR\RimeUIDeployer.exe /install"
  GoTo deploy_done

  deploy_silently:
  ;ExecWait "$INSTDIR\RimeUISetup.exe"
  deploy_done:

  ; don't redirect on 64 bit system for auto run setting
  ${If} ${IsNativeARM64}
    SetRegView 64
  ${ElseIf} ${IsNativeAMD64}
    SetRegView 64
  ${Endif}
  ; Write autorun key
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Run" "RimeUIServer" "$INSTDIR\RimeUIServer.exe"
  ; Start RimeUIServer
  Exec "$INSTDIR\RimeUIServer.exe"

  ; option CheckForUpdates
  ;IfSilent DisableAutoCheckUpdate
  ;MessageBox MB_YESNO|MB_ICONINFORMATION "$(AUTOCHKUPDATE)" IDYES EnableAutoCheckUpdate
  ;DisableAutoCheckUpdate:
  WriteRegStr HKCU "Software\RimeUI\Updates" "CheckForUpdates" "0"
  ;GoTo end
  ;EnableAutoCheckUpdate:
  ;WriteRegStr HKCU "Software\RimeUI\Updates" "CheckForUpdates" "1"
  ;end:

  ; Prompt reboot
  StrCmp $0 "Upgrade" 0 +2
  SetRebootFlag true
  

SectionEnd

; Optional section (can be disabled by the user)
Section "Uninstall"

  ;--------------------------------
  ; 停止服务
  ;--------------------------------
  IfFileExists "$INSTDIR\RimeUIServer.exe" 0 +2
  ExecWait '"$INSTDIR\RimeUIServer.exe" /quit'

  IfFileExists "$INSTDIR\RimeUISetup.exe" 0 +2
  ExecWait '"$INSTDIR\RimeUISetup.exe" /u'

  Sleep 800

  ;===================================================================
  ; [新增功能] 用户数据备份逻辑
  ; 对应 C# GetUserDataDir() 逻辑
  ;===================================================================
  
  ; 使用寄存器 $0 存储最终找到的路径，初始化为空
  StrCpy $0 ""

  ; --- 步骤 1: 默认视图读取 (对应 RegistryView.Default) ---
  ; 如果是64位系统，先尝试切到64位视图（保持默认行为）
  ${If} ${RunningX64}
    SetRegView 64
  ${EndIf}
  
  ReadRegStr $0 HKCU "Software\RimeUI" "RimeUserDir"
  
  ; 检查路径是否有效 (非空且存在)
  ${If} $0 != ""
  ${AndIf} ${FileExists} "$0\*.*"
      DetailPrint "通过注册表(默认视图)找到用户路径: $0"
      Goto DoBackup
  ${EndIf}

  ; --- 步骤 2: 兼容视图读取 (对应 RegistryView.Registry32) ---
  SetRegView 32
  ReadRegStr $0 HKCU "Software\RimeUI" "RimeUserDir"
  
  ; 恢复视图设置，避免影响后续操作
  ${If} ${RunningX64}
    SetRegView 64
  ${EndIf}

  ; 检查路径是否有效
  ${If} $0 != ""
  ${AndIf} ${FileExists} "$0\*.*"
      DetailPrint "通过注册表(32位视图)找到用户路径: $0"
      Goto DoBackup
  ${EndIf}

  ; --- 步骤 3: 默认路径 (对应 Environment.GetFolderPath) ---
  ; 如果注册表都没找到，使用默认 AppData 路径
  StrCpy $0 "$APPDATA\RimeUI"
  DetailPrint "使用默认用户路径: $0"

  ; --- 执行备份操作 ---
  DoBackup:
  
  ; 再次确认目录存在才备份
  ${If} ${FileExists} "$0\*.*"
      ; === 新增：获取当前时间并格式化 ===
      ${GetTime} "" "L" $R9 $R8 $R7 $R6 $R5 $R4 $R3
      ; $R7=年, $R8=月, $R9=日, $R5=时, $R4=分, $R3=秒
      StrCpy $2 "$R7$R8$R9_$R5$R4$R3" 

      ; 修改：将时间戳 $2 加入到备份路径中
      StrCpy $1 "$DESKTOP\Rime_Backup_$2"
      
      DetailPrint "正在备份用户数据到: $1"
      CreateDirectory "$1"
      
      CopyFiles /SILENT "$0\*" "$1"
      
      MessageBox MB_OK "检测到旧的用户数据，已备份至桌面的 [Rime_Backup_$2] 文件夹。"
  ${Else}
      DetailPrint "未找到有效的用户数据目录，跳过备份。"
  ${EndIf}

  ;===================================================================
  ; [结束] 用户数据备份逻辑
  ;===================================================================

  ;--------------------------------
  ; 删除注册表（64位 + 32位都清理）
  ;--------------------------------

  ; 切换到64位注册表视图
  ${If} ${RunningX64}
    SetRegView 64
  ${EndIf}

  DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Run" "RimeUIServer"
  DeleteRegKey HKLM "Software\RimeUI"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\RimeUI"

  ; 切回32位视图
  SetRegView 32

  DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Run" "RimeUIServer"
  DeleteRegKey HKLM "Software\RimeUI"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\RimeUI"

  ; 删除当前用户配置
  DeleteRegKey HKCU "Software\RimeUI"

  ;--------------------------------
  ; 删除文件
  ;--------------------------------

  SetOutPath $TEMP

  ; 强制删除整个安装目录
  RMDir /r "$INSTDIR"

  ;--------------------------------
  ; 删除开始菜单
  ;--------------------------------
  SetShellVarContext all
  RMDir /r "$SMPROGRAMS\$(DISPLAYNAME)"

  ;--------------------------------
  ; 删除用户数据目录（如果存在）
  ;--------------------------------
  ; 注意：前面已经备份过了，这里可以放心删除 AppData 下的数据
  ; 但如果用户自定义了路径（不在AppData下），下面的命令只会删除AppData下的默认路径
  ; 如果你想连自定义路径的源文件也删掉，请取消注释下面的逻辑
  
  SetShellVarContext current
  
  ; 删除默认 AppData 路径
  IfFileExists "$APPDATA\RimeUI" 0 +2
  RMDir /r "$APPDATA\RimeUI"

  ; [可选] 如果你想把注册表里读取到的那个自定义路径的源文件也删掉（因为已经备份了）
  ${If} $0 != "$APPDATA\RimeUI"
  ${AndIf} $0 != ""
    RMDir /r "$0"
  ${EndIf}

  ;--------------------------------
  ; 强制标记重启
  ;--------------------------------
  SetRebootFlag true

SectionEnd


Function .onGUIEnd
    Exec '"$INSTDIR\RimeUISettings.exe"'
FunctionEnd
