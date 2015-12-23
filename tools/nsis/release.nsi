!include LogicLib.nsh
!include x64.nsh
!include MUI2.nsh

;------------------------------------------------------------------------------------------------------
; Source Directory Definition
;
; frontend_srcdir = Source directory for Interface
; backend_srcdir = Source directory for Stack Manager and server stack
; scripts_srcdir = Source directory for JS scripts

!define frontend_srcdir "$%INSTALLER_FRONTEND_DIR%"
!define backend_srcdir "$%INSTALLER_BACKEND_DIR%"
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
!define stack_manager_exec "stack-manager.exe"
!define interface_icon "interface.ico"
!define stack_manager_icon "stack-manager.ico"

; Registry entries
!define regkey "Software\${install_directory}"
!define uninstkey "Software\Microsoft\Windows\CurrentVersion\Uninstall\${install_directory}"
!define frontend_instdir "$PROGRAMFILES64\${install_directory}"
!define backend_instdir "$APPDATA\${install_directory}"

; Start Menu program group
!define startmenu_company "$SMPROGRAMS\${install_directory}"

;------------------------------------------------------------------------------------------------------
; Local Variables and Other Options

Var ChosenFrontEndInstallDir
Var ChosenBackEndInstallDir

ShowInstDetails hide
ShowUninstDetails hide
AutoCloseWindow true
ShowInstDetails show
SetDateSave on
SetDatablockOptimize on
CRCCheck on
SilentInstall normal
Icon "${frontend_srcdir}\${interface_icon}"
UninstallIcon "${frontend_srcdir}\${interface_icon}"
UninstallText "This will uninstall ${company}."
Name "${company}"
Caption "${company}"
OutFile "${setup}"

;------------------------------------------------------------------------------------------------------
; Components

Section /o "DDE Face Recognition" SEC01
    SetOutPath "$ChosenFrontEndInstallDir"
    CreateDirectory $ChosenFrontEndInstallDir\dde
    NSISdl::download "https://s3-us-west-1.amazonaws.com/hifi-production/optionals/dde-installer.exe" "$ChosenFrontEndInstallDir\dde-installer.exe"
    ExecWait '"$ChosenFrontEndInstallDir\dde-installer.exe" /q:a /t:"$ChosenFrontEndInstallDir\dde"'
SectionEnd

Section "Registry Entries and Procotol Handler" SEC02
    SectionIn RO
    WriteRegStr HKLM "${regkey}" "Install_Dir" "$ChosenFrontEndInstallDir"
    WriteRegStr HKLM "${uninstkey}" "DisplayName" "${install_directory} (remove only)"
    WriteRegStr HKLM "${uninstkey}" "UninstallString" '"$ChosenFrontEndInstallDir\${uninstaller}"'
    WriteRegStr HKCR "${company}\Shell\open\command\" "" '"$ChosenFrontEndInstallDir\${interface_exec} "%1"'
    WriteRegStr HKCR "${company}\DefaultIcon" "" "$ChosenFrontEndInstallDir\${interface_icon}"

    ; hifi:// protocol handler registry entries
    WriteRegStr HKCR 'hifi' '' 'URL:Alert Protocol'
    WriteRegStr HKCR 'hifi' 'URL Protocol' ''
    WriteRegStr HKCR 'hifi\DefaultIcon' '' '$ChosenFrontEndInstallDir\${interface_icon},1'
    WriteRegStr HKCR 'hifi\shell\open\command' '' '$ChosenFrontEndInstallDir\${interface_exec} --url "%1"'

    SetOutPath $ChosenFrontEndInstallDir
    WriteUninstaller "$ChosenFrontEndInstallDir\${uninstaller}"
    Exec '"$ChosenFrontEndInstallDir\2013_vcredist_x64.exe" /q /norestart'
    Exec '"$ChosenFrontEndInstallDir\2010_vcredist_x86.exe" /q /norestart'
SectionEnd

Section "Interface Client" SEC03
    SetOutPath $ChosenFrontEndInstallDir
    File /r "${frontend_srcdir}\"
    File /a "${frontend_srcdir}\${interface_icon}"
SectionEnd

Section "Stack Manager Bundle" SEC04
    SetOutPath $ChosenBackEndInstallDir
    File /r "${backend_srcdir}\"
    File /a "${backend_srcdir}\${stack_manager_icon}"
SectionEnd

Section "Start Menu Shortcuts" SEC05
    SetShellVarContext all
    CreateDirectory "${startmenu_company}"
    SetOutPath $ChosenFrontEndInstallDir
    CreateShortCut "${startmenu_company}\Interface.lnk" "$ChosenFrontEndInstallDir\${interface_exec}" "" "$ChosenFrontEndInstallDir\${interface_icon}"
    CreateShortCut "${startmenu_company}\Stack Manager.lnk" "$ChosenBackEndInstallDir\${stack_manager_exec}" "" "$ChosenBackEndInstallDir\${stack_manager_icon}"
    CreateShortCut "${startmenu_company}\Uninstall ${company}.lnk" "$ChosenFrontEndInstallDir\${uninstaller}"
SectionEnd

Section "Uninstall" 
    SetShellVarContext all
    SetOutPath $TEMP

    DELETE "${startmenu_company}\Interface.lnk"
    DELETE "${startmenu_company}\Stack Manager.lnk"
    DELETE "${startmenu_company}\Uninstall ${company}.lnk"

    RMDIR "${startmenu_company}"
    RMDIR /r "$ChosenBackEndInstallDir"
    RMDIR /r "$ChosenFrontEndInstallDir"
    RMDIR "${install_directory}"
    DeleteRegKey HKLM "${uninstkey}"
    DeleteRegKey HKLM "${regkey}"
    DeleteRegKey HKCR 'hifi'
SectionEnd

;------------------------------------------------------------------------------------------------------
; Functions

Function .onInit
    StrCpy $ChosenFrontEndInstallDir "${frontend_instdir}"
    StrCpy $ChosenBackEndInstallDir "${backend_instdir}"
FunctionEnd

Function isInterfaceSelected
    ${IfNot} ${SectionIsSelected} ${SEC03}
    Abort
    ${EndIf}
FunctionEnd

Function isStackManagerSelected
    ${IfNot} ${SectionIsSelected} ${SEC04}
    Abort
    ${EndIf}
FunctionEnd

;------------------------------------------------------------------------------------------------------
; User interface macros and other definitions

!define MUI_WELCOMEFINISHPAGE_BITMAP "installer_vertical.bmp"
!define MUI_WELCOMEPAGE_TITLE "High Fidelity - Integrated Installer"
!define MUI_WELCOMEPAGE_TEXT "Welcome to High Fidelity! This installer includes both the Interface client for VR access as well as the Stack Manager and required server components for you to host your own domain in the metaverse."
!insertmacro MUI_PAGE_WELCOME

!define MUI_PAGE_HEADER_TEXT "Please select the components you want to install"
!define MUI_COMPONENTSPAGE_TEXT_DESCRIPTION_INFO "Hover over a component for a brief description"
!insertmacro MUI_PAGE_COMPONENTS

!define MUI_PAGE_CUSTOMFUNCTION_PRE isInterfaceSelected
!define MUI_DIRECTORYPAGE_VARIABLE $ChosenFrontEndInstallDir
!define MUI_PAGE_HEADER_TEXT "Interface client"
!define MUI_PAGE_HEADER_SUBTEXT ""
!define MUI_DIRECTORYPAGE_TEXT_TOP "Choose a location to install the High Fidelity Interface client. You will use the Interface client to connect to domains in the metaverse."
!define MUI_DIRECTORYPAGE_TEXT_DESTINATION "Install Directory"
!insertmacro MUI_PAGE_DIRECTORY

!define MUI_PAGE_CUSTOMFUNCTION_PRE isStackManagerSelected
!define MUI_DIRECTORYPAGE_VARIABLE $ChosenBackEndInstallDir
!define MUI_PAGE_HEADER_TEXT "Stack Manager"
!define MUI_PAGE_HEADER_SUBTEXT ""
!define MUI_DIRECTORYPAGE_TEXT_TOP "Choose a location to install the High Fidelity Stack Manager bundle, including back end components. NOTE: If you change the default path, make sure you're selecting an unprivileged location, otherwise you will be forced to run the application as administrator for correct functioning."
!define MUI_DIRECTORYPAGE_TEXT_DESTINATION "Install Directory - must be writable"
!insertmacro MUI_PAGE_DIRECTORY

!insertmacro MUI_PAGE_INSTFILES

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC01} "DDE enables facial gesture recognition using a standard 2D webcam"
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC02} "Registry entries are required by the system, we will also add a hifi:// protocol handler"
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC03} "Interface is the GUI client to access domains running the High Fidelity VR stack"
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC04} "The Stack Manager allows you to run a domain of your own and connect it to the High Fidelity metaverse"
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC05} "Adds a program group and shortcuts to the Start Menu"
!insertmacro MUI_FUNCTION_DESCRIPTION_END

!insertmacro MUI_LANGUAGE "English"