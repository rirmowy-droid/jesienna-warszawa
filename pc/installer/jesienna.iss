; Instalator Jesiennej Warszawy (VST3 + aplikacja + dane)
#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#define Stage "..\stage"

[Setup]
AppId={{6C1F2D7A-3B7E-4E55-9C2A-5A1E0B7D9A11}
AppName=Jesienna Warszawa
AppVersion={#AppVersion}
AppPublisher=Jesienna Warszawa
AppPublisherURL=https://github.com/rirmowy-droid/jesienna-warszawa
DefaultDirName={autopf}\Jesienna Warszawa
DefaultGroupName=Jesienna Warszawa
DisableProgramGroupPage=yes
OutputDir=..\dist
OutputBaseFilename=JesiennaWarszawa-Setup
Compression=lzma2/max
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
LicenseFile={#Stage}\LICENSE.txt
WizardStyle=modern
UninstallDisplayName=Jesienna Warszawa (gitara + zespół)

[Languages]
Name: "pl"; MessagesFile: "compiler:Languages\Polish.isl"

[Types]
Name: "full"; Description: "Pełna (wtyczka VST3 + aplikacja)"
Name: "custom"; Description: "Własna"; Flags: iscustom

[Components]
Name: "vst3"; Description: "Wtyczka VST3 (FL Studio, Reaper, Ableton, Cubase…)"; Types: full custom
Name: "app"; Description: "Samodzielna aplikacja (bez DAW)"; Types: full custom
Name: "data"; Description: "Dane: IR kolumn, przestrzenie, sample zespołu"; Types: full custom; Flags: fixed

[Tasks]
Name: "desktopicon"; Description: "Skrót na pulpicie"; Components: app

[Files]
Source: "{#Stage}\vst3\*"; DestDir: "{commoncf64}\VST3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Stage}\app\*"; DestDir: "{app}"; Components: app; Flags: ignoreversion recursesubdirs
Source: "{#Stage}\data\*"; DestDir: "{commonappdata}\Jesienna Warszawa"; Components: data; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Stage}\LICENSE.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#Stage}\CZYTAJ.txt"; DestDir: "{app}"; Flags: ignoreversion isreadme

[Icons]
Name: "{group}\Jesienna Warszawa"; Filename: "{app}\Jesienna Warszawa.exe"; Components: app
Name: "{group}\Czytaj mnie"; Filename: "{app}\CZYTAJ.txt"
Name: "{group}\Odinstaluj"; Filename: "{uninstallexe}"
Name: "{autodesktop}\Jesienna Warszawa"; Filename: "{app}\Jesienna Warszawa.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\Jesienna Warszawa.exe"; Description: "Uruchom aplikację"; Components: app; Flags: postinstall nowait skipifsilent unchecked
