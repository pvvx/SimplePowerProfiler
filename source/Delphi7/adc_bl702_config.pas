unit adc_bl702_config;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, ExtCtrls, Spin;
const
  ADC2shlcount = 11;

type
  TFormAdc2Config = class(TForm)
    RadioGroupAdcChannels: TRadioGroup;
    ButtonOk: TButton;
    ButtonCancel: TButton;
    LabelUz: TLabel;
    LabelIz: TLabel;
    EditUz: TEdit;
    EditIz: TEdit;
    ButtonCopyUz: TButton;
    ButtonCopyIz: TButton;
    SpinEditAdcSmps: TSpinEdit;
    LabelSPS: TLabel;
    EditUk: TEdit;
    Label1: TLabel;
    EditIk: TEdit;
    LabelIk: TLabel;
    RadioGroupUIadc: TRadioGroup;
    GroupBox1: TGroupBox;
    SpinEditPC1PGA: TSpinEdit;
    SpinEditPC2PGA: TSpinEdit;
    Label2: TLabel;
    Label3: TLabel;
    function Checksmps(smps: integer) : integer;
    procedure FormActivate(Sender: TObject);
    procedure ButtonCopyUzClick(Sender: TObject);
    procedure ButtonCopyIzClick(Sender: TObject);
    procedure ButtonOkClick(Sender: TObject);
    procedure ShowScrParms;
    procedure GetScrParms;
    procedure GetParams;
//    procedure SetParamIU(i : double ; u : double);
  private
    { Private declarations }
    ChgEna : boolean;
  public
    { Public declarations }
  end;

var
  FormAdc2Config: TFormAdc2Config;

  ADC2_channel : byte = 1;
  ADC2_smps : dword = 10416;
{ sps:
 * 3906,  16 bits
 * 5208,  16 bits
 * 6250,  16 bits
 * 7812,  16 bits
 * 10416, 16 bits
 * 12500, 15 bits
 * 15625, 15 bits
 * 20833, 15 bits
 * 25000, 14 bits
 * 31250, 14 bits
 * 41666, 14 bits
 * 62500,  13 bits
 * 83333,  13 bits
 * 100000, 13 bits
 * 125000, 13 bits
 * 166666, 13 bits
 - 1000000, 12 bits
 - 1333333, 12 bits
 - 1600000, 12 bits
 - 2000000, 12 bits
 - 2666666, 12 bits
}
  PGA20dbA2 : byte = 0;
  PGA2db5A2 : byte = 0;

  UI_adc2 : integer = 1;

  Uk_adc2 : double = 0.000048828125;
  Uz_adc2 : double = 0;
  Ik_adc2 : double = 0.000048828125;
  Iz_adc2 : double = 0;

  Adc2_channels_tab : array [0..ADC2shlcount - 1] of byte = (
    6,  // GPIO_7
    0,  // GPIO_8
    7,  // GPIO_9
    5,  // GPIO_14
    1,  // GPIO_15
    2,  // GPIO_17
    12,  // DACA
    13,  // DACB
    16,  // VREF
    18,  // VBAT
    23   // GND
  );

implementation

{$R *.dfm}
Uses MainFrm;

function TFormAdc2Config.Checksmps(smps: integer) : integer;
var
//resolution,
clk, x : integer;
clk_div : double;
begin
    if smps <= 32000000/256/12 then begin
    	//resolution := 16;
      clk := 32000000 div 256;
    end else if smps <= 32000000/128/12 then begin
    	//resolution := 15;
      clk := 32000000 div 128;
    end else if smps <= 32000000/64/12 then begin
    	//resolution := 14;
      clk := 32000000 div 643;
    end else if smps <= 32000000/16/12 then begin
     	//resolution := 13;
      clk := 32000000 div 16;
{    end else if smps <= 32000000/12 then begin
     	//resolution := 12;
      clk := 32000000; }
    end else begin // max sps = 1Msps
      smps := 1000000;
      // resolution := 12;
      clk := 32000000;
    end;
   	clk_div := clk/smps;
    x := Round(clk_div);
    if x < 15 then
      x := 12
    else if x < 19 then
      x := 16
    else if x < 23 then
      x := 24
    else if x < 29 then
      x := 32
    else
      x := 32;
    result := clk div x;
end;

procedure TFormAdc2Config.GetScrParms;
begin
    DecimalSeparator := '.';
    Uk_adc2 := StrToFloat(EditUk.Text);
    Uz_adc2 := StrToFloat(EditUz.Text);
    Ik_adc2 := StrToFloat(EditIk.Text);
    Iz_adc2 := StrToFloat(EditIz.Text);

    PGA20dbA2 := 0;
    PGA2db5A2 := (SpinEditPC1PGA.Value and $0f) + ((SpinEditPC2PGA.Value and $0f) shl 4);

    UI_adc2 := RadioGroupUIadc.ItemIndex;
    ADC2_channel := Adc2_channels_tab[RadioGroupAdcChannels.ItemIndex];
    ADC2_smps := Checksmps(SpinEditAdcSmps.Value);
end;

procedure TFormAdc2Config.FormActivate(Sender: TObject);
begin
     ChgEna := False;
     ShowScrParms;
     ChgEna := True;
end;

procedure TFormAdc2Config.GetParams;
begin
  if (UI_adc2 <> 0) then begin
    U_zero := Uz_adc2;
    Uk := Uk_adc2;
  end else begin
    I_zero := Iz_adc2;
    Ik := Ik_adc2;
  end;
end;

procedure TFormAdc2Config.ShowScrParms;
var
i : integer;
begin
    DecimalSeparator := '.';
    EditUz.Text:=FormatFloat('0.00000000', Uz_adc2);
    EditUk.Text:=FormatFloat('0.00000000', Uk_adc2);
    EditIz.Text:=FormatFloat('0.00000000', Iz_adc2);
    EditIk.Text:=FormatFloat('0.00000000', Ik_adc2);
    RadioGroupUIadc.ItemIndex := UI_adc2;
//    EditIz.Text:=FormatFloat('0.00000000', Iz_adc);
//    ADC_channel := (ADC_channel and $9F) or $20;
    for i:=0 to ADC2shlcount do begin
      if ADC2_channel = Adc2_channels_tab[i] then break;
    end;
    RadioGroupAdcChannels.ItemIndex := i;
    ADC2_smps := Checksmps(ADC2_smps);
    SpinEditAdcSmps.Value := ADC2_smps;
    SpinEditPC1PGA.Value := PGA2db5A2 and $0f;
    SpinEditPC2PGA.Value := (PGA2db5A2 shr 4) and $0f;
end;

procedure TFormAdc2Config.ButtonCopyUzClick(Sender: TObject);
begin
    EditUz.Text:=FormatFloat('0.00000000', -OldsU);
end;

procedure TFormAdc2Config.ButtonCopyIzClick(Sender: TObject);
begin
    EditIz.Text:=FormatFloat('0.00000000', -OldsI);
end;

procedure TFormAdc2Config.ButtonOkClick(Sender: TObject);
begin
    GetScrParms;
    ModalResult := mrOk;
    Exit;
end;

end.
