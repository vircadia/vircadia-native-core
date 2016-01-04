!include LogicLib.nsh
!include x64.nsh
!include MUI2.nsh

;------------------------------------------------------------------------------------------------------
; Source Directory Definition
;
; installer_srcdir = Source directory for Interface
; scripts_srcdir = Source directory for JS scripts

!define installer_srcdir "$%INSTALLER_SOURCE_DIR%"
!define scripts_srcdir "$%INSTALLER_SCRIPTS_DIR%"

; Install Directories, Icons and Registry entries
;
; setup = Name of the installer executable that will be produced
; uninstaller = Name of the uninstaller executable
; company = String to use for company name, includes build type suffix [eg. High Fidelity - PR] for non-release installers
; install_directory = Subdirectory where this specific version will be installed, in the case of dev and pr builds, its a
;                     unique subdirectory inside a company parent [eg. \High Fidelity - PR\1234\ ]

!define setup "$%INSTALLER_NAME%"
!define uninstaller "uninstall.exe"
!define company "$%INSTALLER_COMPANY%"
!define install_directory "$%INSTALLER_DIRECTORY%"

; Executables and icons for GUI applications that will be added as shortcuts.
!define interface_exec "interface.exe"
!define console_exec "server-console.exe"
!define interface_icon "interface.ico"
!define console_icon "server-console.ico"

; Registry entries
!define regkey "Software\${install_directory}"
!define uninstkey "Software\Microsoft\Windows\CurrentVersion\Uninstall\${install_directory}"
!define instdir "$PROGRAMFILES64\${install_directory}"

; Start Menu program group
!define startmenu_company "$SMPROGRAMS\${install_directory}"

;------------------------------------------------------------------------------------------------------
; Local Variables and Other Options

var ChosenInstallDir

SetCompressor bzip2
ShowInstDetails hide
ShowUninstDetails hide
AutoCloseWindow true
ShowInstDetails show
SetDateSave on
SetDatablockOptimize on
CRCCheck on
SilentInstall normal
Icon "${installer_srcdir}\${interface_icon}"
UninstallIcon "${installer_srcdir}\${interface_icon}"
UninstallText "This will uninstall ${company}."
Name "${company}"
Caption "${company}"
OutFile "${setup}"

;------------------------------------------------------------------------------------------------------
; Components

Section /o "DDE Face Recognition" "DDE"
    SetOutPath "$ChosenInstallDir"
    CreateDirectory $ChosenInstallDir\dde
    NSISdl::download "https://s3-us-west-1.amazonaws.com/hifi-production/optionals/dde-installer.exe" "$ChosenInstallDir\dde-installer.exe"
    ExecWait '"$ChosenInstallDir\dde-installer.exe" /q:a /t:"$ChosenInstallDir\dde"'
SectionEnd

Section /o "Default Content Set" "CONTENT"
    SetOutPath "$ChosenInstallDir\resources"
    NSISdl::download "https://s3-us-west-1.amazonaws.com/hifi-production/content/temp.exe" "$ChosenInstallDir\content.exe"
    ExecWait '"$ChosenInstallDir\content.exe" /S'
    Delete "$ChosenInstallDir\content.exe"
SectionEnd

Section "Registry Entries and Protocol Handler" "REGISTRY"
    SetRegView 64
    SectionIn RO
    WriteRegStr HKLM "${regkey}" "Install_Dir" "$ChosenInstallDir"
    WriteRegStr HKLM "${uninstkey}" "DisplayName" "${install_directory} (remove only)"
    WriteRegStr HKLM "${uninstkey}" "UninstallString" '"$ChosenInstallDir\${uninstaller}"'
    WriteRegStr HKCR "${company}\Shell\open\command\" "" '"$ChosenInstallDir\${interface_exec} "%1"'
    WriteRegStr HKCR "${company}\DefaultIcon" "" "$ChosenInstallDir\${interface_icon}"

    ; hifi:// protocol handler registry entries
    WriteRegStr HKCR 'hifi' '' 'URL:Alert Protocol'
    WriteRegStr HKCR 'hifi' 'URL Protocol' ''
    WriteRegStr HKCR 'hifi\DefaultIcon' '' '$ChosenInstallDir\${interface_icon},1'
    WriteRegStr HKCR 'hifi\shell\open\command' '' '$ChosenInstallDir\${interface_exec} --url "%1"'

    SetOutPath $ChosenInstallDir
    WriteUninstaller "$ChosenInstallDir\${uninstaller}"
    Exec '"$ChosenInstallDir\2013_vcredist_x64.exe" /q /norestart'
    Exec '"$ChosenInstallDir\2010_vcredist_x86.exe" /q /norestart'
SectionEnd

Section "Base Files" "BASE"
    SectionIn RO
    SetOutPath $ChosenInstallDir
    File /r /x assignment-client.* /x domain-server.* /x interface.* /x server-console.* "${installer_srcdir}\"
SectionEnd

Section "High Fidelity Interface" "CLIENT"
    SetOutPath $ChosenInstallDir
    File /a "${installer_srcdir}\interface.*"
    File /a "${installer_srcdir}\${interface_icon}"
SectionEnd

Section "High Fidelity Server" "SERVER"
    SetOutPath $ChosenInstallDir
    File /a "${installer_srcdir}\server-console.*"
    File /a "${installer_srcdir}\domain-server.*"
    File /a "${installer_srcdir}\assignment-client.*"
SectionEnd

Section "Start Menu Shortcuts" "SHORTCUTS"
    SetShellVarContext all
    CreateDirectory "${startmenu_company}"
    SetOutPath $ChosenInstallDir
    CreateShortCut "${startmenu_company}\High Fidelity.lnk" "$ChosenInstallDir\${interface_exec}" "" "$ChosenInstallDir\${interface_icon}"
    CreateShortCut "${startmenu_company}\Server Console.lnk" "$ChosenInstallDir\${console_exec}" "" "$ChosenInstallDir\${console_icon}"
    CreateShortCut "${startmenu_company}\Uninstall ${company}.lnk" "$ChosenInstallDir\${uninstaller}"
SectionEnd

Section "Uninstall"
    SetRegView 64
    Delete "$INSTDIR\${uninstaller}"
    Delete "$SMSTARTUP\High Fidelity Server Console.lnk"
    RMDir /r "$INSTDIR"
    RMDir /r "${startmenu_company}"
    RMDir /r "$0"
    DeleteRegKey HKLM "${uninstkey}"
    DeleteRegKey HKLM "${regkey}"
    DeleteRegKey HKCR "${company}"
    DeleteRegKey HKCR 'hifi'
SectionEnd

;------------------------------------------------------------------------------------------------------
; Functions

Function .onInit
    StrCpy $ChosenInstallDir "${instdir}"
    SectionSetText ${REGISTRY} ""
    SectionSetText ${SHORTCUTS} ""
    SectionSetText ${BASE} ""
FunctionEnd

var ServerCheckBox
var ServerCheckBox_state
var RunOnStartupCheckBox
var RunOnStartupCheckBox_state

Function RunCheckboxes
    ${If} ${SectionIsSelected} ${SERVER}
        ${NSD_CreateCheckbox} 36.2% 56% 100% 10u "&Start Home Server"
        Pop $ServerCheckBox
        SetCtlColors $ServerCheckBox "" "ffffff"
        ${NSD_CreateCheckbox} 36.2% 65% 100% 10u "&Always launch your Home Server on startup"
        Pop $RunOnStartupCheckBox
        SetCtlColors $RunOnStartupCheckBox "" "ffffff"
    ${EndIf}
FunctionEnd

Function HandleCheckBoxes
    ${If} ${SectionIsSelected} ${SERVER}
        ${NSD_GetState} $ServerCheckBox $ServerCheckBox_state
        ${If} $ServerCheckBox_state == ${BST_CHECKED}
            SetOutPath $ChosenInstallDir
            ExecShell "" '"$ChosenInstallDir\${console_exec}"'
        ${EndIf}
        ${NSD_GetState} $RunOnStartupCheckBox $RunOnStartupCheckBox_state
        ${If} $ServerCheckBox_state == ${BST_CHECKED}
            CreateShortCut "$SMSTARTUP\High Fidelity Server Console.lnk" "$ChosenInstallDir\${console_exec}" "" "$ChosenInstallDir\${console_icon}"
        ${EndIf}
    ${EndIf}
FunctionEnd

;------------------------------------------------------------------------------------------------------
; User interface macros and other definitions

!define MUI_WELCOMEFINISHPAGE_BITMAP "installer_vertical.bmp"
!define MUI_WELCOMEPAGE_TITLE "High Fidelity - Integrated Installer"
!define MUI_WELCOMEPAGE_TEXT "Welcome to High Fidelity! This installer includes both High Fidelity Interface for VR access as well as the High Fidelity Server for you to host your own domain in the metaverse."
!insertmacro MUI_PAGE_WELCOME

!define MUI_PAGE_HEADER_TEXT "Please select the components you want to install"
!define MUI_COMPONENTSPAGE_TEXT_DESCRIPTION_INFO "Hover over a component for a brief description"
!insertmacro MUI_PAGE_COMPONENTS

!define MUI_DIRECTORYPAGE_VARIABLE $ChosenInstallDir
!define MUI_PAGE_HEADER_TEXT "High Fidelity"
!define MUI_PAGE_HEADER_SUBTEXT ""
!define MUI_DIRECTORYPAGE_TEXT_TOP "Choose a location for your High Fidelity installation."
!define MUI_DIRECTORYPAGE_TEXT_DESTINATION "Install Directory"
!insertmacro MUI_PAGE_DIRECTORY

!insertmacro MUI_PAGE_INSTFILES

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!define MUI_PAGE_CUSTOMFUNCTION_SHOW RunCheckboxes
!define MUI_PAGE_CUSTOMFUNCTION_LEAVE HandleCheckboxes
!define MUI_FINISHPAGE_RUN_NOTCHECKED
!define MUI_FINISHPAGE_RUN "$ChosenInstallDir\interface.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Start High Fidelity Interface"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${DDE} "DDE enables facial gesture recognition using a standard 2D webcam"
    !insertmacro MUI_DESCRIPTION_TEXT ${CONTENT} "Demo content set for your home server"
    !insertmacro MUI_DESCRIPTION_TEXT ${CLIENT} "The High Fidelity Interface Client for connection to domains in the metaverse."
    !insertmacro MUI_DESCRIPTION_TEXT ${SERVER} "The High Fidelity Server - run your own home domain"
!insertmacro MUI_FUNCTION_DESCRIPTION_END

!insertmacro MUI_LANGUAGE "English"
