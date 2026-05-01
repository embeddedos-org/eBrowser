; eBrowser Windows Installer — Inno Setup Script
; Build from repo root: iscc /DEBROWSER_VERSION=x.y.z installer/windows/ebrowser-setup.iss

#ifndef EBROWSER_VERSION
#define EBROWSER_VERSION "2.0.0"
#endif

#define MyAppName "eBrowser"
#define MyAppPublisher "EmbeddedOS"
#define MyAppURL "https://github.com/embeddedos-org/eBrowser"
#define MyAppExeName "eBrowser.exe"

; Paths relative to this .iss file location (installer/windows/)
#define RepoRoot "..\.."
#define ReleaseBin RepoRoot + "\release\bin"

[Setup]
AppId={{E8B0F4A2-7C3D-4F1E-9A5B-2D6E8F0A1B3C}
AppName={#MyAppName}
AppVersion={#EBROWSER_VERSION}
AppVerName={#MyAppName} {#EBROWSER_VERSION}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}/issues
AppUpdatesURL={#MyAppURL}/releases
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
LicenseFile={#RepoRoot}\LICENSE
OutputDir={#RepoRoot}\build\installer
OutputBaseFilename=eBrowser-{#EBROWSER_VERSION}-Setup-x64
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
MinVersion=10.0
UninstallDisplayIcon={app}\{#MyAppExeName}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "addtopath"; Description: "Add eBrowser to system PATH"; GroupDescription: "System integration:"; Flags: unchecked

[Files]
; Main application
Source: "{#ReleaseBin}\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
; All DLLs (SDL2, OpenSSL, etc.)
Source: "{#ReleaseBin}\*.dll"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
; Documentation
Source: "{#RepoRoot}\README.md"; DestDir: "{app}\docs"; Flags: ignoreversion
Source: "{#RepoRoot}\CHANGELOG.md"; DestDir: "{app}\docs"; Flags: ignoreversion
Source: "{#RepoRoot}\LICENSE"; DestDir: "{app}\docs"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Registry]
; Add to PATH if selected
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; \
    ValueType: expandsz; ValueName: "Path"; ValueData: "{olddata};{app}"; \
    Tasks: addtopath; Check: NeedsAddPath(ExpandConstant('{app}'))

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; \
    Flags: nowait postinstall skipifsilent

[Code]
function NeedsAddPath(Param: string): Boolean;
var
  OrigPath: string;
begin
  if not RegQueryStringValue(HKEY_LOCAL_MACHINE,
    'SYSTEM\CurrentControlSet\Control\Session Manager\Environment',
    'Path', OrigPath)
  then begin
    Result := True;
    exit;
  end;
  Result := Pos(';' + Uppercase(Param) + ';', ';' + Uppercase(OrigPath) + ';') = 0;
end;
