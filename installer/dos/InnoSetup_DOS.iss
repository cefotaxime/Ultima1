#define AppName "울티마 I 한국어판 (DOS)"
#define AppVersion "1.0K"
#ifndef Payload
  #error Payload build parameter is required
#endif
#ifndef ReleaseDir
  #error ReleaseDir build parameter is required
#endif

[Setup]
AppId={{F20DAB1E-8AF8-4B32-94B5-1D8BC3A6B7B1}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher=Ultima I Korean Localization Project
DefaultDirName={userdocs}\Ultima1K(DOS)
DisableProgramGroupPage=yes
OutputDir={#ReleaseDir}
OutputBaseFilename=Ultima1K(DOS)_Setup
Compression=lzma2
SolidCompression=yes
PrivilegesRequired=lowest
WizardStyle=modern
UsePreviousAppDir=no
UninstallDisplayName={#AppName}
SetupLogging=yes
ArchitecturesAllowed=x86 x64compatible

[Files]
Source: "{#Payload}\ULTIHF99.EXE"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#Payload}\CWSDPMI.EXE"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#Payload}\DOSBox\*"; DestDir: "{app}\DOSBox"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Payload}\FONTS\*"; DestDir: "{app}\FONTS"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Payload}\Ultima1K.bat"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#Payload}\dosbox_ultima1.conf"; DestDir: "{app}"; Flags: ignoreversion

[Dirs]
Name: "{app}\SAVEGAME"
Name: "{app}\GAMEDATA"

[Icons]
Name: "{autoprograms}\Ultima I 한국어판 (DOS)"; Filename: "{app}\Ultima1K.bat"; WorkingDir: "{app}"

[Code]
var OriginalPage: TInputDirWizardPage;
const RequiredNames = 'ULTIMA.EXE|GEN.EXE|OUT.EXE|MONDAIN.EXE|SPACE.EXE|MAP.BIN|EGATILES.BIN|EGATOWN.BIN|TCD.BIN';
const DataNames = 'CASTLE.16|CASTLE.4|CGAFIGHT.BIN|CGAMOND.BIN|CGASPACE.BIN|CGATILES.BIN|CGATOWN.BIN|EGAFIGHT.BIN|EGAMOND.BIN|EGASPACE.BIN|EGATILES.BIN|EGATOWN.BIN|MAP.BIN|NIF.BIN|T1KFIGHT.BIN|T1KMOND.BIN|T1KSPACE.BIN|T1KTILES.BIN|T1KTOWN.BIN|TCD.BIN|ULTIMA.EXE|GEN.EXE|OUT.EXE|MONDAIN.EXE|SPACE.EXE';

function SplitPipe(S: String): TArrayOfString;
var P, N: Integer;
begin
  SetArrayLength(Result, 0);
  N := 0;
  while Length(S) > 0 do begin
    P := Pos('|', S);
    if P = 0 then P := Length(S) + 1;
    SetArrayLength(Result, N + 1);
    Result[N] := Copy(S, 1, P - 1);
    N := N + 1;
    Delete(S, 1, P);
  end;
end;

function ParameterOriginal(): String;
begin
  Result := ExpandConstant('{param:ORIGINAL|}');
end;

procedure InitializeWizard;
begin
  OriginalPage := CreateInputDirPage(wpSelectDir, '정품 울티마 I 폴더', '원본 DOS 게임 파일 선택',
    'ULTIMA.EXE 등이 있는 정품 Ultima I 폴더를 선택하세요. 원본 파일은 변경되지 않습니다.', False, '');
  OriginalPage.Add('정품 Ultima I 원본 폴더:');
  OriginalPage.Values[0] := ParameterOriginal();
end;

function PathInside(A, B: String): Boolean;
begin
  A := AddBackslash(Uppercase(ExpandFileName(A)));
  B := AddBackslash(Uppercase(ExpandFileName(B)));
  Result := (Copy(A, 1, Length(B)) = B);
end;

function ValidOriginal(S: String; ErrorMessage: String): Boolean;
var
  Names: TArrayOfString;
  I: Integer;
begin
  Result := False;
  if not DirExists(S) then begin
    ErrorMessage := '정품 원본 폴더를 선택하세요.';
    Exit;
  end;
  Names := SplitPipe(RequiredNames);
  for I := 0 to GetArrayLength(Names)-1 do
    if not FileExists(AddBackslash(S)+Names[I]) then begin
      ErrorMessage := '필수 원본 파일이 없습니다: ' + Names[I];
      Exit;
    end;
  if PathInside(WizardDirValue(), S) or PathInside(S, WizardDirValue()) then begin
    ErrorMessage := '원본 폴더와 설치 폴더가 서로 겹칩니다.';
    Exit;
  end;
  Result := True;
end;

function NextButtonClick(CurPageID: Integer): Boolean;
var ErrorMessage: String;
begin
  Result := True;
  if (CurPageID = OriginalPage.ID) or (CurPageID = wpReady) then begin
    Result := ValidOriginal(OriginalPage.Values[0], ErrorMessage);
    if not Result then MsgBox(ErrorMessage, mbError, MB_OK);
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  Names: TArrayOfString;
  I: Integer;
  Src, Dst, Name: String;
begin
  if CurStep <> ssPostInstall then Exit;
  Src := AddBackslash(OriginalPage.Values[0]);
  Dst := ExpandConstant('{app}\GAMEDATA\');
  Names := SplitPipe(DataNames);
  for I := 0 to GetArrayLength(Names)-1 do begin
    Name := Names[I];
    if FileExists(Src+Name) then
      if not FileCopy(Src+Name, Dst+Name, False) then
        RaiseException('원본 게임 데이터 복사 실패: ' + Name);
  end;
  SaveStringToFile(ExpandConstant('{app}\original_path.txt'), OriginalPage.Values[0], False);
end;
