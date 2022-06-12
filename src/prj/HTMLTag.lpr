library HTMLTag;

uses
  SysUtils,
  Classes,
  Windows,
  Messages,
  U_Npp_HTMLTag in '..\U_Npp_HTMLTag.pas',
  L_VersionInfoW in '..\common\L_VersionInfoW.pas',
  L_SpecialFolders in '..\common\L_SpecialFolders.pas',
  RegExpr in '..\common\RegExpr.pas',
  NppSimpleObjects in '..\LibNppPlugin\NppSimpleObjects.pas',
  nppplugin in '..\LibNppPlugin\nppplugin.pas';

{$R *.res}
{$Include '..\Include\NppPluginInclude.pas'}

begin
  DLL_PROCESS_DETACH_Hook := @DLLEntryPoint;
  DLLEntryPoint(DLL_PROCESS_ATTACH);
end.