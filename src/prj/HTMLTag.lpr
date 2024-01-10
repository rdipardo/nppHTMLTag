library HTMLTag;

uses
  SysUtils,
  Classes,
  Windows,
  Messages,
  U_Npp_HTMLTag in '..\U_Npp_HTMLTag.pas',
  NppSimpleObjects in '..\LibNppPlugin\NppSimpleObjects.pas',
  nppplugin in '..\LibNppPlugin\nppplugin.pas';

{$R *.res}
{$Include '..\Include\NppPluginInclude.pas'}
{$if NOT DECLARED(useheaptrace)}
  {$SetPEOptFlags $40}
{$endif}

begin
  DLL_PROCESS_DETACH_Hook := @DLLEntryPoint;
  DLLEntryPoint(DLL_PROCESS_ATTACH);
end.
