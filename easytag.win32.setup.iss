; ---------------------------------------------------------
; To build this setup follow these steps:
;
; - Download and install Inno Setup QuickStart Pack from
;   http://files.jrsoftware.org/ispack/ispack-5.1.9.exe
;
; - Create a subdirectory named "win32-install-dir" of the
;   directory that contains this script. Then, copy into the
;   directory "win32-install-dir" all the EasyTAG's files
;   (easytag.exe, easytag.dll, etc).
;
; - Finally open this script in Inno Setup and click on
;   "Build -> Compile", or simply "Compile" in the context
;   menu of the file in Windows Explorer.
;
; ---------------------------------------------------------

; Here you can specify the current version of EasyTAG.
#define APP_NAME 	"EasyTAG"
#define APP_VERSION "2.1.2"
#define APP_URL 	"http://easytag.sourceforge.net"
#define APP_NAME_2 	"easytag"
#define DEV_NAME 	"Jérôme COUDERC"

; Here you can specify the minimum required version of GTK+
; and the download URL for the Windows version.
#define GTK_VERSION  "2.8"
#define GTK_DOWNLOAD "http://sourceforge.net/project/showfiles.php?group_id=121075&package_id=132255"

; Defining MULTILANGUAGE the installer will support other
; languages, not only English. See also below in sections
; [Languages] and [CustomMessages].
#define MULTILANGUAGE

[Setup]
AppVerName={#APP_NAME} {#APP_VERSION}
VersionInfoVersion={#APP_VERSION}
AppName={#APP_NAME}
AppPublisher={#DEV_NAME}
AppPublisherURL={#APP_URL}
AppSupportURL={#APP_URL}
AppUpdatesURL={#APP_URL}
AppCopyright={#DEV_NAME}
VersionInfoCopyright=This program is open-source!
DefaultDirName={pf}\{#APP_NAME}
DefaultGroupName={#APP_NAME}
UninstallDisplayIcon={app}\{#APP_NAME}.exe,0
DisableStartupPrompt=yes
DirExistsWarning=no
OutputDir=.\
OutputBaseFilename={#APP_NAME_2}-{#APP_VERSION}
Compression=lzma/max
SolidCompression=yes
AllowNoIcons=yes
PrivilegesRequired=poweruser
ShowLanguageDialog=auto

WizardImageFile=compiler:WizModernImage-IS.bmp
WizardSmallImageFile=compiler:WizModernSmallImage-IS.bmp

; Here you can specify which languages are available for the setup
; GUI. Additional languages can be found at this location:
; http://www.jrsoftware.org/files/istrans/
[Languages]
Name: en; MessagesFile: compiler:Default.isl
#ifdef MULTILANGUAGE
Name: cs; MessagesFile: compiler:Languages\Czech.isl
Name: da; MessagesFile: compiler:Languages\Danish.isl
Name: de; MessagesFile: compiler:Languages\German.isl
Name: es; MessagesFile: compiler:Languages\Spanish.isl
Name: fr; MessagesFile: compiler:Languages\French.isl
Name: hu; MessagesFile: compiler:Languages\Hungarian.isl
Name: it; MessagesFile: compiler:Languages\Italian.isl
#endif

; These are the labels displayed in the wizard and not contained in
; the Inno Setup's translation files
[CustomMessages]
; ** ENGLISH **
en.FoldersLink=Add {#APP_NAME} to folders' context menu
en.MissingGTK=The GTK+ libraries v{#GTK_VERSION} (or above) are not installed. Would you like do download them now?
#ifdef MULTILANGUAGE
; ** CZECH **
cs.FoldersLink=Pridat {#APP_NAME} do kontextového menu adresáre
cs.MissingGTK=Knihovny GTK+ v{#GTK_VERSION} (nebo vyšší) nejsou nainstalovány. Chcete je nyní stáhnout?
; ** DANISH **
da.FoldersLink=Tilføj {#APP_NAME} til mappers højre-klik menu
da.MissingGTK=GTK+ bibliotekerne v{#GTK_VERSION} (eller højere) er ikke installeret. Vil du hente dem nu?
; ** GERMAN **
de.FoldersLink=Füge {#APP_NAME} zum Kontextmenü von Ordnern hinzu
de.MissingGTK=GTK+ v{#GTK_VERSION} (oder neuer) ist nicht installiert. Wollen sie es jetzt herunterladen?
; ** SPANISH ** ---> NOT TRANSLATED YET! <---
es.FoldersLink=Add {#APP_NAME} to folders' context menu
es.MissingGTK=The GTK+ libraries v{#GTK_VERSION} (or above) are not installed. Would you like do download them now?
; ** FRENCH **
fr.FoldersLink=Ajouter {#APP_NAME} au menu contextuel des dossiers
fr.MissingGTK=La librairie GTK+ v{#GTK_VERSION} (ou supérieure) n'est pas installée. Voulez-vous la télécharger maintenant?
; ** HUNGARIAN **
hu.FoldersLink=Külso program {#APP_NAME} beépítése a menüszerkezetbe
hu.MissingGTK=A GTK+ {#GTK_VERSION} (vagy nagyobb) környezet nincs telepítve. Szeretné most letölteni?
; ** ITALIAN **
it.FoldersLink=Aggiungi {#APP_NAME} al menù contestuale delle cartelle
it.MissingGTK=Le librerie GTK+ v{#GTK_VERSION} (o superiore) non sono installate. Vuoi scaricarle adesso?
#endif

[Files]
Source: .\win32-install-dir\*; DestDir: {app}; Flags: ignoreversion recursesubdirs; Tasks: ; Languages: 

[Tasks]
Name: desktopicon; Description: {cm:CreateDesktopIcon}; GroupDescription: {cm:AdditionalIcons}
;Name: desktopicon\common; Description: "For all users"; GroupDescription: {cm:AdditionalIcons}; Flags: exclusive
;Name: desktopicon\user; Description: "For the current user only"; GroupDescription: {cm:AdditionalIcons}; Flags: exclusive unchecked
Name: quicklaunchicon; Description: {cm:CreateQuickLaunchIcon}; GroupDescription: {cm:AdditionalIcons}; Flags: unchecked
Name: folderslink; Description: {cm:FoldersLink}; GroupDescription: {cm:AdditionalIcons}

[Registry]
Root: HKCR; Subkey: Directory\shell\{#APP_NAME}; Flags: uninsdeletekey dontcreatekey
Root: HKCR; Subkey: Directory\shell\{#APP_NAME}; ValueType: string; ValueName: ; ValueData: {#APP_NAME}; Tasks: folderslink
Root: HKCR; Subkey: Directory\shell\{#APP_NAME}\Command; ValueType: string; ValueName: ; ValueData: """{app}\{#APP_NAME}.exe"" ""%1"""; Tasks: folderslink

[Icons]
Name: {group}\{#APP_NAME} {#APP_VERSION}; Filename: {app}\{#APP_NAME}.exe; WorkingDir: {app}
Name: {group}\{cm:UninstallProgram,{#APP_NAME}}; Filename: {uninstallexe}; WorkingDir: {app}
Name: {group}\{cm:ProgramOnTheWeb,{#APP_NAME}}; Filename: {#APP_URL}
Name: {commondesktop}\{#APP_NAME}; Filename: {app}\{#APP_NAME}.exe; WorkingDir: {app}; Tasks: desktopicon

[Run]
Filename: {app}\{#APP_NAME}.exe; Description: {cm:LaunchProgram,{#APP_NAME} {#APP_VERSION}}; Flags: nowait postinstall skipifsilent

[Code]
// Parse a version string in a array of Integer ('1.2.3.4' -> [1,2,3,4])
// -- taken from the Inno Setup forum --
procedure DecodeVersion(i_oStrVersion: String; var o_panVersion: array of Integer);
var
  l_nIndex   : Integer;
  l_nPos     : Integer;
  l_oStrTemp : String;
begin
  o_panVersion := [0,0,0,0];

  l_nIndex := 0;
  while ((Length(i_oStrVersion) > 0) and (l_nIndex < 4)) do begin
    l_nPos := pos('.', i_oStrVersion);
    if l_nPos > 0 then begin
      if l_nPos = 1 then begin
        l_oStrTemp := '0'
      end else begin
        l_oStrTemp := Copy(i_oStrVersion, 1, l_nPos - 1);
      end;

      o_panVersion[l_nIndex] := StrToIntDef(l_oStrTemp, 0);
      l_nIndex := l_nIndex + 1;
      i_oStrVersion := Copy(i_oStrVersion, l_nPos + 1, Length(i_oStrVersion));
    end else begin
      o_panVersion[l_nIndex] := StrToIntDef(i_oStrVersion, 0);
      i_oStrVersion := '';
    end;
  end;
end;

// Compare two array of Integer from their string counterpart
// -- taken from the Inno Setup forum --
function CompareVersion(i_oStrVersionDest, i_oStrVersionFrom: String) : Integer;
var
  l_anVersionDest : array of Integer;
  l_anVersionFrom : array of Integer;
  l_nIndex        : Integer;
begin
  SetArrayLength(l_anVersionDest, 4);
  DecodeVersion(i_oStrVersionDest, l_anVersionDest);

  SetArrayLength(l_anVersionFrom, 4);
  DecodeVersion(i_oStrVersionFrom, l_anVersionFrom);

  Result := 0;
  l_nIndex := 0;

  while ((Result = 0) and (l_nIndex < 4)) do begin
    if l_anVersionDest[l_nIndex] > l_anVersionFrom[l_nIndex] then begin
      Result := 1    // i_oStrVersionDest > i_oStrVersionFrom
    end else begin
      if l_anVersionDest[l_nIndex] < l_anVersionFrom[l_nIndex] then begin
        Result := -1 // i_oStrVersionDest < i_oStrVersionFrom
      end else begin
        Result := 0; // i_oStrVersionDest = i_oStrVersionFrom
      end;
    end;

    l_nIndex := l_nIndex + 1;
  end;
end;

// Checks, when setup starts, if the installed version of GTK+ mathches requirements
function InitializeSetup(): Boolean;
var
  iRetVal: Integer;
  sGtkVer: String;
begin
  Result := True;
  sGtkVer := '0.0.0.0';
  RegQueryStringValue(HKLM, 'SOFTWARE\GTK\2.0', 'Version', sGtkVer);

  if(CompareVersion(sGtkVer, '{#GTK_VERSION}') = -1) then begin
    case MsgBox(CustomMessage('MissingGTK'), mbError, MB_YESNOCANCEL) of
      IDYES: begin
        Result := False;
        ShellExec('open', '{#GTK_DOWNLOAD}', '', '', SW_SHOW, ewNoWait, iRetVal);
      end

      IDCANCEL: begin
        Result := False;
      end
    end
  end
end;
[_ISTool]
UseAbsolutePaths=false
