;BASED ON DISCODSP'S INNO SETUP SCRIPT

#define PluginName "PNCH"

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
DefaultDirName={pf}\UnplugRed\
DefaultGroupName=UnplugRed
DisableReadyPage=false
DisableWelcomePage=no
LanguageDetectionMethod=uilanguage
OutputBaseFilename={#PluginName} Free Installer
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
Source: "build_windows\free\{#PluginName}.vst3"; DestDir: "{cf64}\VST3\UnplugRed\"; Components: VST3; Flags: ignoreversion
Source: "build_windows\free\{#PluginName}.clap"; DestDir: {code:GetDir|0}; Components: CLAP; Flags: ignoreversion

[Icons]
Name: {group}\Uninstall {#PluginName}; Filename: {uninstallexe}

[Types]
Name: "custom"; Description: "Custom"; Flags: iscustom

[Components]
Name: "VST3"; Description: "VST3"; Types: custom;
Name: "CLAP"; Description: "CLAP"; Types: custom;

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
  DirPage.Values[0] := GetPreviousData('CLAP', ExpandConstant('{reg:HKLM\SOFTWARE\CLAP,CLAPPluginsPath|{pf}\Common Files\CLAP\UnplugRed}'));

end;

procedure CurPageChanged(CurPageID: Integer);
begin
  if CurPageID = DirPage.ID then
  begin
    DirPage.Buttons[0].Enabled := IsComponentSelected('CLAP');
    DirPage.PromptLabels[0].Enabled := DirPage.Buttons[0].Enabled;
    DirPage.Edits[0].Enabled := DirPage.Buttons[0].Enabled;
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
    If (not IsComponentSelected('CLAP')) then
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
end;