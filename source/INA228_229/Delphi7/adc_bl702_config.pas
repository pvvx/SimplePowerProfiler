unit adc_bl702_config;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, ExtCtrls, Spin;
const
  ADCshlcount = 3;

type
  TFormAdcConfig = class(TForm)
    RadioGroupAdcChannels: TRadioGroup;
    ButtonOk: TButton;
    ButtonCancel: TButton;
    LabelUz: TLabel;
    LabelIz: TLabel;
    EditUz: TEdit;
    EditIz: TEdit;
    ButtonCopyUz: TButton;
    ButtonCopyIz: TButton;
    LabelSPS: TLabel;
    EditUk: TEdit;
    Label1: TLabel;
    EditIk: TEdit;
    LabelIk: TLabel;
    RadioGroupShunt: TRadioGroup;
    ComboBoxSps: TComboBox;
    procedure FormActivate(Sender: TObject);
    procedure ButtonCopyUzClick(Sender: TObject);
    procedure ButtonCopyIzClick(Sender: TObject);
    procedure ButtonOkClick(Sender: TObject);
    procedure ShowScrParms;
    procedure GetScrParms;
    procedure GetParams;
    procedure RadioGroupShuntClick(Sender: TObject);
    procedure ComboBoxSpsChange(Sender: TObject);
//    procedure SetParamIU(i : double ; u : double);
  private
    { Private declarations }
    ChgEna : boolean;
  public
    { Public declarations }
  end;

var
  FormAdcConfig: TFormAdcConfig;
  ADC_shunt : byte = 0;
  ADC_channel : byte = 3;
  ADC_smps : dword = 11905;

  TabSps : array [0..7] of word =
   (20000, 11905, 6666,
   3571, 1851, 950, 482, 242);

//  ADC_freq : dword = 24576000;

  Uk_adc : double = 0.0000120703125; // 0.000193125/16
  Uz_adc : double = 0.0;
  Ik_adc : double =  0.000000048828125;
  Iz_adc : double = 0;
  Ik_adc0 : double = 0.000001953125; // 0.0003125/16/1000 (mA, 10 Îì) 0.0000001953125
  Iz_adc0 : double = 0;
  Ik_adc1 : double = 0.00000048828125; // 0.00078125/16/1000 (mA, 10 Îì)
  Iz_adc1 : double = 0;

  Adc_channels_tab : array [0..ADCshlcount - 1] of byte = (
    $01,  // Shunt channel
    $02,  // VBus channel
    $03   // Shunt + VBus channel
  );

implementation

{$R *.dfm}
Uses MainFrm;


procedure TFormAdcConfig.GetScrParms;
begin
    DecimalSeparator := '.';

    ADC_shunt := RadioGroupShunt.ItemIndex;

    Uk_adc := StrToFloat(EditUk.Text);
    Uz_adc := StrToFloat(EditUz.Text);
    Ik_adc := StrToFloat(EditIk.Text);
    Iz_adc := StrToFloat(EditIz.Text);

    if ADC_shunt <> 0 then begin
      Iz_adc1 := Iz_adc;
      Ik_adc1 := Ik_adc;
    end else begin
      Iz_adc0 := Iz_adc;
      Ik_adc0 := Ik_adc;
    end;

    ADC_channel := Adc_channels_tab[RadioGroupAdcChannels.ItemIndex];
    ADC_smps := TabSps[ComboBoxSps.ItemIndex];
//    ADC_freq := ADC_smps * 512;
//    SpinEditAdcSmps.Value := ADC_smps;
end;

procedure TFormAdcConfig.FormActivate(Sender: TObject);
begin
     ChgEna := False;
     ShowScrParms;
     ChgEna := True;
end;

procedure TFormAdcConfig.GetParams;
begin
    if ADC_shunt <> 0 then begin
      Iz_adc := Iz_adc1;
      Ik_adc := Ik_adc1;
    end else begin
      Iz_adc := Iz_adc0;
      Ik_adc := Ik_adc0;
    end;
   U_zero := Uz_adc;
   Uk := Uk_adc;
   I_zero := Iz_adc;
   I_zero10 := Iz_adc / 10;
   Ik := Ik_adc;
   Ik10 := Ik_adc / 10;
end;

procedure TFormAdcConfig.ShowScrParms;
var
i : integer;
begin
    DecimalSeparator := '.';

    RadioGroupShunt.ItemIndex := ADC_shunt and 1;
    if ADC_shunt <> 0 then begin
      Iz_adc := Iz_adc1;
      Ik_adc := Ik_adc1;
    end else begin
      Iz_adc := Iz_adc0;
      Ik_adc := Ik_adc0;
    end;

    EditUz.Text:=FormatFloat('0.00000000000', Uz_adc);
    EditUk.Text:=FormatFloat('0.00000000000', Uk_adc);
    EditIz.Text:=FormatFloat('0.00000000000', Iz_adc);
    EditIk.Text:=FormatFloat('0.00000000000', Ik_adc);
    for i:=0 to ADCshlcount do begin
      if ADC_channel = Adc_channels_tab[i] then break;
    end;
    RadioGroupAdcChannels.ItemIndex := i;
    for i:=0 to 7 do begin
      if ADC_smps >= TabSps[i] then begin
          ADC_smps := TabSps[i];
          ComboBoxSps.ItemIndex := i;
          break;
      end;
    end;
end;

procedure TFormAdcConfig.ButtonCopyUzClick(Sender: TObject);
begin
    EditUz.Text:=FormatFloat('0.00000000', -OldsU);
end;

procedure TFormAdcConfig.ButtonCopyIzClick(Sender: TObject);
begin
    EditIz.Text:=FormatFloat('0.00000000', -OldsI);
end;

procedure TFormAdcConfig.ButtonOkClick(Sender: TObject);
begin
    GetScrParms;
    ModalResult := mrOk;
    Exit;
end;


procedure TFormAdcConfig.RadioGroupShuntClick(Sender: TObject);
begin
    ADC_shunt := RadioGroupShunt.ItemIndex;
    ShowScrParms();
end;

procedure TFormAdcConfig.ComboBoxSpsChange(Sender: TObject);
begin
    ADC_smps := TabSps[ComboBoxSps.ItemIndex];
end;

end.
