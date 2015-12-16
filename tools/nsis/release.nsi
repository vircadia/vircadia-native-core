!include LogicLib.nsh
!include x64.nsh

!define srcdir "E:\development\md-hifi\build\full-stack-deployment" ;$%INSTALLER_SOURCE_DIR%
!define setup "E:\development\md-hifi\build\installer-test.exe" ;$%INSTALLER_NAME%
!define scriptsdir "E:\development\md-hifi\examples" ;$%INSTALLER_SCRIPTS_DIR%
!define company "High Fidelity"
!define prodname "Interface"
!define exec "interface.exe"
!define icon "${srcdir}\interface.ico"
!define regkey "Software\${company}\${prodname}"
!define uninstkey "Software\Microsoft\Windows\CurrentVersion\Uninstall\${prodname}"
!define install_dir_company "$PROGRAMFILES64\${company}"
!define install_dir_product "${install_dir_company}\${prodname}"
!define startmenu_company "$SMPROGRAMS\${company}"
!define startmenu_product "${startmenu_company}\${prodname}"
!define uninstaller "uninstall.exe"

;--------------------------------

XPStyle on
ShowInstDetails hide
ShowUninstDetails hide

Name "${prodname}"
Caption "${prodname}"

!ifdef icon
    Icon "${icon}"
!endif

OutFile "${setup}"

SetDateSave on
SetDatablockOptimize on
CRCCheck on
SilentInstall normal

InstallDir "${install_dir_product}"
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
    WriteRegStr HKLM "${uninstkey}" "DisplayName" "${prodname} (remove only)"
    WriteRegStr HKLM "${uninstkey}" "UninstallString" '"$INSTDIR\${uninstaller}"'
    WriteRegStr HKCR "${prodname}\Shell\open\command\" "" '"$INSTDIR\${exec} "%1"'

    !ifdef icon
        WriteRegStr HKCR "${prodname}\DefaultIcon" "" "$INSTDIR\${icon}"
    !endif

    ; hifi:// protocol handler registry entries
    WriteRegStr HKCR 'hifi' '' 'URL:Alert Protocol'
    WriteRegStr HKCR 'hifi' 'URL Protocol' ''
    WriteRegStr HKCR 'hifi\DefaultIcon' '' '$INSTDIR\${icon},1'
    WriteRegStr HKCR 'hifi\shell\open\command' '' '$INSTDIR\${exec} --url "%1"'

    SetOutPath $INSTDIR

    ; package all files, recursively, preserving attributes
    ; assume files are in the correct places
    File /r "${srcdir}\"
    !ifdef icon
        File /a "${icon}"
    !endif
    ; any application-specific files
    !ifdef files
        !include "${files}"
    !endif
    SetOutPath "$DOCUMENTS\${company}\Scripts"
    File /r "${scriptsdir}\"
    SetOutPath $INSTDIR
    WriteUninstaller "${uninstaller}"
    Exec '"$INSTDIR\2013_vcredist_x64.exe" /q /norestart'
    Exec '"$INSTDIR\2010_vcredist_x86.exe" /q /norestart'
SectionEnd

; create shortcuts
Section "Start Menu Shortcuts" SEC03

    SectionIn RO

    ; This should install the shortcuts for "All Users"
    SetShellVarContext all
    CreateDirectory "${startmenu_product}"
    SetOutPath $INSTDIR ; for working directory
    !ifdef icon
        CreateShortCut "${startmenu_product}\${prodname}.lnk" "$INSTDIR\${exec}" "" "$INSTDIR\${icon}"
    !else
        CreateShortCut "${startmenu_product}\${prodname}.lnk" "$INSTDIR\${exec}"
    !endif

    CreateShortCut "${startmenu_product}\Uninstall ${prodname}.lnk" "$INSTDIR\${uninstaller}"
SectionEnd

; Uninstaller
; All section names prefixed by "Un" will be in the uninstaller

UninstallText "This will uninstall ${prodname}."

!ifdef icon
    UninstallIcon "${icon}"
!endif

Section "Uninstall" SEC04

    SectionIn RO

    ; Explicitly remove all added shortcuts
    SetShellVarContext all
    DELETE "${startmenu_product}\${prodname}.lnk"
    DELETE "${startmenu_product}\Uninstall ${prodname}.lnk"

    RMDIR "${startmenu_product}"
    ; This should remove the High Fidelity folder in Start Menu if it's empty
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