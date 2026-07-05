; Inno Setup script for UB_Poly16 (Juno-style 16-voice synth)
#define MyAppName "UB_Poly16"
#define MyAppVersion "0.6.1"
#define MyAppPublisher "TheUNBoRN"
#define MyAppExeName "UB_Poly16.exe"
; artefacts dir — override with ISCC /DRelDir=<path> (e.g. a no-ASIO release build)
#ifndef RelDir
  #define RelDir SourcePath + "..\build\UB_Poly16_artefacts\Release"
#endif

[Setup]
AppId={{4E8A1C7D-2F93-4B5A-9E61-7C0A3D5F2B18}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppName}
DisableProgramGroupPage=yes
OutputDir=Output
OutputBaseFilename=UB_Poly16_Setup_{#MyAppVersion}
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
WizardStyle=modern
UninstallDisplayName={#MyAppName} {#MyAppVersion}

[Languages]
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"
Name: "vst3"; Description: "Installer le plugin VST3 (pour DAW)"; GroupDescription: "Composants :"

[Files]
Source: "{#RelDir}\Standalone\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#RelDir}\VST3\UB_Poly16.vst3\*"; DestDir: "{commoncf}\VST3\UB_Poly16.vst3"; \
    Flags: ignoreversion recursesubdirs createallsubdirs; Tasks: vst3

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autoprograms}\Desinstaller {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#MyAppName}}"; Flags: nowait postinstall skipifsilent
