; Inno Setup script for NF D-42 (Windows x64)
; Build: ISCC /DMyAppVersion=X.Y.Z [/DWITH_AAX] NFD42.iss  (the version comes from CMakeLists.txt)
; Expects Installer\Windows\payload\NF D-42.vst3 and, with /DWITH_AAX,
; Installer\Windows\payload\NF D-42.aaxplugin (already PACE-signed with wraptool).

#ifndef MyAppVersion
  #error "Build with /DMyAppVersion=X.Y.Z"
#endif
#define MyAppName "NF D-42"
#define MyAppPublisher "NF Audio Tools"

[Setup]
AppId={{196C6222-2991-4D46-AFD2-A6FE2FF8D877}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={commonpf64}\NF Audio Tools\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
OutputDir=..
OutputBaseFilename={#MyAppName} {#MyAppVersion} Setup
Compression=lzma
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
DisableDirPage=yes
DisableReadyPage=yes
UninstallDisplayIcon={uninstallexe}
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

#ifdef WITH_AAX
[Types]
Name: "full"; Description: "Full installation"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 Plug-in"; Types: full custom; Flags: fixed
Name: "aax"; Description: "AAX Plug-in (Pro Tools)"; Types: full
#endif

[Files]
#ifdef WITH_AAX
Source: "payload\NF D-42.vst3\*"; DestDir: "{commoncf64}\VST3\{#MyAppName}.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: vst3
Source: "payload\NF D-42.aaxplugin\*"; DestDir: "{commoncf64}\Avid\Audio\Plug-Ins\{#MyAppName}.aaxplugin"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: aax
#else
Source: "payload\NF D-42.vst3\*"; DestDir: "{commoncf64}\VST3\{#MyAppName}.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
#endif

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\{#MyAppName}.vst3"
#ifdef WITH_AAX
Type: filesandordirs; Name: "{commoncf64}\Avid\Audio\Plug-Ins\{#MyAppName}.aaxplugin"
#endif
