[Setup]
#define PluginVersion "1.2.11"
#define PluginStem "LSP_Simple_Open_DRT_" + PluginVersion
AppId={{9F3D3F6D-5D8B-4D16-A9A0-6A8D6F7E7A10}
AppName=Simple_Open_DRT
AppVersion={#PluginVersion}
AppVerName=Simple_Open_DRT OFX v{#PluginVersion} (OpenDRT 1.1.0)
AppPublisher=Loïs Plagnard
AppPublisherURL=https://github.com/Lo1s-pgn/Simple-Open-DRT
AppSupportURL=https://github.com/Lo1s-pgn/Simple-Open-DRT/issues
AppUpdatesURL=https://github.com/Lo1s-pgn/Simple-Open-DRT
DefaultDirName={commoncf}\OFX\Plugins
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
DisableDirPage=yes
DisableProgramGroupPage=yes
OutputDir=.
OutputBaseFilename=LSP_Simple_Open_DRT_v{#PluginVersion}_Windows_cuda_opencl_Installer
Compression=lzma
SolidCompression=yes
PrivilegesRequired=admin

[Files]
Source: "..\..\release\{#PluginStem}.ofx.bundle\*"; DestDir: "{commoncf64}\OFX\Plugins\{#PluginStem}.ofx.bundle"; Flags: ignoreversion recursesubdirs createallsubdirs

[Code]
function ResolveRunning: Boolean;
var
  ResultCode: Integer;
begin
  Result := False;

  if Exec(
      ExpandConstant('{sys}\WindowsPowerShell\v1.0\powershell.exe'),
      '-NoProfile -ExecutionPolicy Bypass -Command "if (Get-Process Resolve -ErrorAction SilentlyContinue) { exit 1 } else { exit 0 }"',
      '',
      SW_HIDE,
      ewWaitUntilTerminated,
      ResultCode) then
  begin
    Result := (ResultCode = 1);
  end;
end;

function InitializeSetup(): Boolean;
var
  Clicked: Integer;
begin
  while ResolveRunning() do
  begin
    Clicked := SuppressibleMsgBox(
      'DaVinci Resolve is currently running.' + #13#10 +
      'Please close Resolve before installing Simple_Open_DRT.',
      mbError,
      MB_RETRYCANCEL,
      IDRETRY);

    if Clicked = IDCANCEL then
    begin
      Result := False;
      Exit;
    end;
  end;

  Result := True;
end;
