program PowerProfiler;

uses
  Forms,
  MainFrm in 'MainFrm.pas' {frmMain},
  ComPort in 'ComPort.pas',
  HexUtils in 'HexUtils.pas',
  adc_bl702_config in 'adc_bl702_config.pas' {FormAdc2Config},
  WaveStorage in 'WaveStorage.pas';

{$E exe}

{$R *.res}

begin
  Application.Initialize;
  Application.Title := 'PowerProfiler';
  Application.CreateForm(TfrmMain, frmMain);
  Application.CreateForm(TFormAdcConfig, FormAdcConfig);
  Application.Run;
end.
