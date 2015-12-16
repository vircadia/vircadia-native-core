!include LogicLib.nsh
!include x64.nsh

!define srcdir "$%INSTALLER_SOURCE_DIR%"
!define setup "$%INSTALLER_NAME%"
!define scriptsdir "$%INSTALLER_SCRIPTS_DIR%"
!define company "$%INSTALLER_COMPANY%"
!define install_directory "$%INSTALLER_DIRECTORY%"
!define interface_exec "interface.exe"
!define stack_manager_exec "stack-manager.exe"
!define interface_icon "interface.ico"
!define stack_manager_icon "stack-manager.ico"
!define regkey "Software\${company}"
!define uninstkey "Software\Microsoft\Windows\CurrentVersion\Uninstall\${company}"
!define install_dir_company "$PROGRAMFILES64\${install_directory}"
!define startmenu_company "$SMPROGRAMS\${install_directory}"
!define uninstaller "uninstall.exe"

;--------------------------------

XPStyle on
ShowInstDetails hide
ShowUninstDetails hide

Name "${company}"
Caption "${company}"

!ifdef icon
    Icon "${icon}"
!endif

OutFile "${setup}"

SetDateSave on
SetDatablockOptimize on
CRCCheck on
SilentInstall normal

InstallDir "${install_dir_company}"
InstallDirRegKey HKLM "${regkey}" ""

; Page components
Page directory
Page components
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

AutoCloseWindow true
ShowInstDetails show


!ifdef screenimage

    ; set up background image
    ; uses BgImage plugin

    Function .onGUIInit
        ; extract background BMP into temp plugin directory
        InitPluginsDir
        File /oname=$PLUGINSDIR\1.bmp "${screenimage}"

        BgImage::SetBg /NOUNLOAD /FILLSCREEN $PLUGINSDIR\1.bmp
        BgImage::Redraw /NOUNLOAD
    FunctionEnd

    Function .onGUIEnd
        ; Destroy must not have /NOUNLOAD so NSIS will be able to unload and delete BgImage before it exits
        BgImage::Destroy
    FunctionEnd

!endif

; Optional Component Selection
Section /o "DDE Face Recognition" SEC01
    SetOutPath "$INSTDIR"
    CreateDirectory $INSTDIR\dde
    NSISdl::download "https://s3-us-west-1.amazonaws.com/hifi-production/optionals/dde-installer.exe" "$INSTDIR\dde-installer.exe"
    ExecWait '"$INSTDIR\dde-installer.exe" /q:a /t:"$INSTDIR\dde"'
SectionEnd

; beginning (invisible) section
Section "Registry Entries and Procotol Handler" SEC02

    SectionIn RO

    WriteRegStr HKLM "${regkey}" "Install_Dir" "$INSTDIR"
    WriteRegStr HKLM "${uninstkey}" "DisplayName" "${company} (remove only)"
    WriteRegStr HKLM "${uninstkey}" "UninstallString" '"$INSTDIR\${uninstaller}"'
    WriteRegStr HKCR "${company}\Shell\open\command\" "" '"$INSTDIR\${interface_exec} "%1"'
    WriteRegStr HKCR "${company}\DefaultIcon" "" "$INSTDIR\${interface_icon}"

    ; hifi:// protocol handler registry entries
    WriteRegStr HKCR 'hifi' '' 'URL:Alert Protocol'
    WriteRegStr HKCR 'hifi' 'URL Protocol' ''
    WriteRegStr HKCR 'hifi\DefaultIcon' '' '$INSTDIR\${interface_icon},1'
    WriteRegStr HKCR 'hifi\shell\open\command' '' '$INSTDIR\${interface_exec} --url "%1"'

    SetOutPath $INSTDIR

    ; package all files, recursively, preserving attributes
    ; assume files are in the correct places
    File /r "${srcdir}\"
    File /a "${srcdir}\${interface_icon}"
    File /a "${srcdir}\${stack_manager_icon}"
    ; any application-specific files
    !ifdef files
        !include "${files}"
    !endif
    WriteUninstaller "${uninstaller}"
    Exec '"$INSTDIR\2013_vcredist_x64.exe" /q /norestart'
    Exec '"$INSTDIR\2010_vcredist_x86.exe" /q /norestart'
SectionEnd

; create shortcuts
Section "Start Menu Shortcuts" SEC03

    SectionIn RO

    ; This should install the shortcuts for "All Users"
    SetShellVarContext all
    CreateDirectory "${startmenu_company}"
    SetOutPath $INSTDIR ; for working directory
    CreateShortCut "${startmenu_company}\Interface.lnk" "$INSTDIR\${interface_exec}" "" "$INSTDIR\${interface_icon}"
    CreateShortCut "${startmenu_company}\Stack Manager.lnk" "$INSTDIR\${stack_manager_exec}" "" "$INSTDIR\${stack_manager_icon}"
    CreateShortCut "${startmenu_company}\Uninstall ${company}.lnk" "$INSTDIR\${uninstaller}"
SectionEnd

; Uninstaller
; All section names prefixed by "Un" will be in the uninstaller

UninstallText "This will uninstall ${company}."

!ifdef icon
    UninstallIcon "${interface_icon}"
!endif

Section "Uninstall" SEC04

    SectionIn RO

    ; Explicitly remove all added shortcuts
    SetShellVarContext all
    DELETE "${startmenu_company}\Interface.lnk"
    DELETE "${startmenu_company}\Stack Manager.lnk"
    DELETE "${startmenu_company}\Uninstall ${company}.lnk"

    RMDIR "${startmenu_company}"

    RMDIR /r "$INSTDIR"
    ; This should remove the High Fidelity folder in Program Files if it's empty
    RMDIR "${install_dir_company}"

    !ifdef unfiles
        !include "${unfiles}"
    !endif
    ; It's good practice to put the registry key removal at the very end
    DeleteRegKey HKLM "${uninstkey}"
    DeleteRegKey HKLM "${regkey}"
    DeleteRegKey HKCR 'hifi'
SectionEnd