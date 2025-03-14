;BASED ON DISCODSP'S INNO SETUP SCRIPT

#define PluginName "Everything Bundle"

[Setup]
DisableDirPage=yes
ArchitecturesInstallIn64BitMode=x64
AppName={#PluginName}
AppPublisherURL=https://vst.unplug.red/
AppSupportURL=https://vst.unplug.red/
AppUpdatesURL=https://vst.unplug.red/
AppVerName={#PluginName}
AppVersion=1.0
Compression=lzma2/ultra64
DefaultDirName={commonpf}\UnplugRed\
DefaultGroupName=UnplugRed
DisableReadyPage=false
DisableWelcomePage=no
LanguageDetectionMethod=uilanguage
OutputBaseFilename={#PluginName} Installer
SetupIconFile=assets\icon.ico
ShowLanguageDialog=no
VersionInfoCompany=UnplugRed
VersionInfoCopyright=UnplugRed
VersionInfoDescription={#PluginName}
VersionInfoProductName={#PluginName}
VersionInfoProductVersion=1.0
VersionInfoVersion=1.0
WizardImageFile=assets\image\{#PluginName}.bmp
WizardImageStretch=false
WizardSmallImageFile=assets\smallimage\{#PluginName}.bmp

[Files]
Source: "build_win\paid\Plastic Funeral.vst3"; DestDir: "{commoncf64}\VST3\UnplugRed\"; Components: VST3; Flags: ignoreversion
Source: "build_win\paid\VU.vst3"; DestDir: "{commoncf64}\VST3\UnplugRed\"; Components: VST3; Flags: ignoreversion
Source: "build_win\free\ClickBox.vst3"; DestDir: "{commoncf64}\VST3\UnplugRed\"; Components: VST3; Flags: ignoreversion
Source: "build_win\paid\Pisstortion.vst3"; DestDir: "{commoncf64}\VST3\UnplugRed\"; Components: VST3; Flags: ignoreversion
Source: "build_win\paid\PNCH.vst3"; DestDir: "{commoncf64}\VST3\UnplugRed\"; Components: VST3; Flags: ignoreversion
Source: "build_win\paid\Red Bass.vst3"; DestDir: "{commoncf64}\VST3\UnplugRed\"; Components: VST3; Flags: ignoreversion
Source: "build_win\free\MPaint.vst3"; DestDir: "{commoncf64}\VST3\UnplugRed\"; Components: VST3; Flags: ignoreversion
Source: "build_win\other\MPaint\*"; DestDir: "{commoncf64}\VST3\UnplugRed\"; Components: VST3; Flags: recursesubdirs onlyifdoesntexist
Source: "build_win\paid\CRMBL.vst3"; DestDir: "{commoncf64}\VST3\Unplugred\"; Components: VST3; Flags: ignoreversion
Source: "build_win\paid\Prisma.vst3"; DestDir: "{commoncf64}\VST3\UnplugRed\"; Components: VST3; Flags: ignoreversion
Source: "build_win\paid\Prismon.vst3"; DestDir: "{commoncf64}\VST3\UnplugRed\"; Components: VST3; Flags: ignoreversion
Source: "build_win\paid\SunBurnt.vst3"; DestDir: "{commoncf64}\VST3\UnplugRed\"; Components: VST3; Flags: ignoreversion
Source: "build_win\paid\Diet Audio.vst3"; DestDir: "{commoncf64}\VST3\UnplugRed\"; Components: VST3; Flags: ignoreversion
Source: "build_win\paid\Scope.vst3"; DestDir: "{commoncf64}\VST3\UnplugRed\"; Components: VST3; Flags: ignoreversion
Source: "build_win\paid\Magic Carpet.vst3"; DestDir: "{commoncf64}\VST3\UnplugRed\"; Components: VST3; Flags: ignoreversion
Source: "build_win\paid\Plastic Funeral.clap"; DestDir: {code:GetDir|0}; Components: CLAP; Flags: ignoreversion
Source: "build_win\paid\VU.clap"; DestDir: {code:GetDir|0}; Components: CLAP; Flags: ignoreversion
Source: "build_win\free\ClickBox.clap"; DestDir: {code:GetDir|0}; Components: CLAP; Flags: ignoreversion
Source: "build_win\paid\Pisstortion.clap"; DestDir: {code:GetDir|0}; Components: CLAP; Flags: ignoreversion
Source: "build_win\paid\PNCH.clap"; DestDir: {code:GetDir|0}; Components: CLAP; Flags: ignoreversion
Source: "build_win\paid\Red Bass.clap"; DestDir: {code:GetDir|0}; Components: CLAP; Flags: ignoreversion
Source: "build_win\free\MPaint.clap"; DestDir: {code:GetDir|0}; Components: CLAP; Flags: ignoreversion
Source: "build_win\other\MPaint\*"; DestDir: "{code:GetDir|0}"; Components: CLAP; Flags: recursesubdirs onlyifdoesntexist
Source: "build_win\paid\CRMBL.clap"; DestDir: {code:GetDir|0}; Components: CLAP; Flags: ignoreversion
Source: "build_win\paid\Prisma.clap"; DestDir: {code:GetDir|0}; Components: CLAP; Flags: ignoreversion
Source: "build_win\paid\Prismon.clap"; DestDir: {code:GetDir|0}; Components: CLAP; Flags: ignoreversion
Source: "build_win\paid\SunBurnt.clap"; DestDir: {code:GetDir|0}; Components: CLAP; Flags: ignoreversion
Source: "build_win\paid\Diet Audio.clap"; DestDir: {code:GetDir|0}; Components: CLAP; Flags: ignoreversion
Source: "build_win\paid\Scope.clap"; DestDir: {code:GetDir|0}; Components: CLAP; Flags: ignoreversion
Source: "build_win\paid\Magic Carpet.clap"; DestDir: {code:GetDir|0}; Components: CLAP; Flags: ignoreversion
Source: "build_win\paid\VU.exe"; DestDir: {code:GetDir|1}; Components: Standalone; Flags: ignoreversion
Source: "build_win\free\MPaint.exe"; DestDir: {code:GetDir|1}; Components: Standalone; Flags: ignoreversion
Source: "build_win\paid\Scope.exe"; DestDir: {code:GetDir|1}; Components: Standalone; Flags: ignoreversion
Source: "build_win\other\MPaint\*"; DestDir: "{code:GetDir|1}"; Components: Standalone; Flags: recursesubdirs onlyifdoesntexist

[Icons]
Name: {group}\Uninstall {#PluginName}; Filename: {uninstallexe}

[Types]
Name: "custom"; Description: "Custom"; Flags: iscustom

[Components]
Name: "VST3"; Description: "VST3"; Types: custom;
Name: "CLAP"; Description: "CLAP"; Types: custom;
Name: "Standalone"; Description: "Standalone"; Types: custom;

[Code]
var
  DirPage: TInputDirWizardPage;
  TypesComboOnChangePrev: TNotifyEvent;

procedure ComponentsListCheckChanges;
begin
  WizardForm.NextButton.Enabled := (WizardSelectedComponents(False) <> '');
end;

procedure ComponentsListClickCheck(Sender: TObject);
begin
  ComponentsListCheckChanges;
end;

procedure TypesComboOnChange(Sender: TObject);
begin
  TypesComboOnChangePrev(Sender);
  ComponentsListCheckChanges;
end;

procedure InitializeWizard;
begin

  WizardForm.ComponentsList.OnClickCheck := @ComponentsListClickCheck;
  TypesComboOnChangePrev := WizardForm.TypesCombo.OnChange;
  WizardForm.TypesCombo.OnChange := @TypesComboOnChange;

  DirPage := CreateInputDirPage(wpSelectComponents,
  'Confirm Plugin Directory', '',
  'Select the folder in which setup should install the plugins, then click Next.',
  False, '');

  DirPage.Add('CLAP folder');
  DirPage.Values[0] := GetPreviousData('CLAP', ExpandConstant('{reg:HKLM\SOFTWARE\CLAP,CLAPPluginsPath|{commonpf}\Common Files\CLAP\UnplugRed}'));
  DirPage.Add('Standalone folder');
  DirPage.Values[1] := GetPreviousData('Standalone', ExpandConstant('{reg:HKLM\SOFTWARE\CLAP,CLAPPluginsPath|{commonpf}\Common Files\CLAP\UnplugRed}'));

end;

procedure CurPageChanged(CurPageID: Integer);
begin
  if CurPageID = DirPage.ID then
  begin
    DirPage.Buttons[0].Enabled := WizardIsComponentSelected('CLAP');
    DirPage.PromptLabels[0].Enabled := DirPage.Buttons[0].Enabled;
    DirPage.Edits[0].Enabled := DirPage.Buttons[0].Enabled;

    DirPage.Buttons[1].Enabled := WizardIsComponentSelected('Standalone');
    DirPage.PromptLabels[1].Enabled := DirPage.Buttons[1].Enabled;
    DirPage.Edits[1].Enabled := DirPage.Buttons[1].Enabled;
  end;

  if CurPageID = wpSelectComponents then
  begin
    ComponentsListCheckChanges;
  end;
end;

function ShouldSkipPage(PageID: Integer): Boolean;
begin
  if PageID = DirPage.ID then
  begin
    If (not WizardIsComponentSelected('CLAP')) and (not WizardIsComponentSelected('Standalone')) then
      begin
        Result := True
      end;
  end;
end;

function GetDir(Param: string): string;
begin
    Result := DirPage.Values[StrToInt(Param)];
end;

procedure RegisterPreviousData(PreviousDataKey: Integer);
begin
  SetPreviousData(PreviousDataKey, 'CLAP', DirPage.Values[0]);
  SetPreviousData(PreviousDataKey, 'Standalone', DirPage.Values[1]);
end;