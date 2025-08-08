object FormAdcConfig: TFormAdcConfig
  Left = 1333
  Top = 1563
  Width = 399
  Height = 207
  Caption = 'ADC Config'
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  OnActivate = FormActivate
  PixelsPerInch = 96
  TextHeight = 13
  object LabelUz: TLabel
    Left = 144
    Top = 60
    Width = 37
    Height = 13
    Caption = 'U offset'
  end
  object LabelIz: TLabel
    Left = 146
    Top = 12
    Width = 32
    Height = 13
    Caption = 'I offset'
  end
  object LabelSPS: TLabel
    Left = 87
    Top = 138
    Width = 16
    Height = 13
    Caption = 'sps'
  end
  object Label1: TLabel
    Left = 147
    Top = 84
    Width = 32
    Height = 13
    Caption = 'U coef'
  end
  object LabelIk: TLabel
    Left = 151
    Top = 36
    Width = 27
    Height = 13
    Caption = 'I coef'
  end
  object RadioGroupAdcChannels: TRadioGroup
    Left = 8
    Top = 8
    Width = 129
    Height = 73
    Caption = 'ADC Channel'
    ItemIndex = 2
    Items.Strings = (
      'Shunt (I)'
      'Vbus (U)'
      'Shunt + Vbus (UI)')
    TabOrder = 0
  end
  object ButtonOk: TButton
    Left = 184
    Top = 128
    Width = 75
    Height = 25
    Caption = 'Ok'
    TabOrder = 1
    OnClick = ButtonOkClick
  end
  object ButtonCancel: TButton
    Left = 272
    Top = 128
    Width = 75
    Height = 25
    Caption = 'Cancel'
    ModalResult = 2
    TabOrder = 2
  end
  object EditUz: TEdit
    Left = 192
    Top = 58
    Width = 89
    Height = 21
    Hint = #1057#1084#1077#1097#1077#1085#1080#1077' '#1076#1083#1103' U'
    ParentShowHint = False
    ShowHint = True
    TabOrder = 3
    Text = '?.?'
  end
  object EditIz: TEdit
    Left = 192
    Top = 10
    Width = 89
    Height = 21
    Hint = #1057#1084#1077#1097#1077#1085#1080#1077' '#1076#1083#1103' U'
    ParentShowHint = False
    ShowHint = True
    TabOrder = 4
    Text = '?.?'
  end
  object ButtonCopyUz: TButton
    Left = 288
    Top = 56
    Width = 89
    Height = 25
    Caption = 'Copy Us to Uz'
    TabOrder = 5
    OnClick = ButtonCopyUzClick
  end
  object ButtonCopyIz: TButton
    Left = 288
    Top = 8
    Width = 89
    Height = 25
    Caption = 'Copy Is to Iz'
    TabOrder = 6
    OnClick = ButtonCopyIzClick
  end
  object EditUk: TEdit
    Left = 192
    Top = 82
    Width = 89
    Height = 21
    Hint = #1050#1086#1101#1092'. '#1076#1083#1103' U'
    ParentShowHint = False
    ShowHint = True
    TabOrder = 7
    Text = '?.?'
  end
  object EditIk: TEdit
    Left = 192
    Top = 34
    Width = 89
    Height = 21
    Hint = #1050#1086#1101#1092'. '#1076#1083#1103' I'
    ParentShowHint = False
    ShowHint = True
    TabOrder = 8
    Text = '?.?'
  end
  object RadioGroupShunt: TRadioGroup
    Left = 8
    Top = 80
    Width = 129
    Height = 49
    Caption = 'Shunt'
    ItemIndex = 0
    Items.Strings = (
      '163.84 mV'
      '40.96 mV')
    TabOrder = 9
    OnClick = RadioGroupShuntClick
  end
  object ComboBoxSps: TComboBox
    Left = 8
    Top = 134
    Width = 73
    Height = 21
    ItemHeight = 13
    ItemIndex = 0
    TabOrder = 10
    Text = '20000'
    OnChange = ComboBoxSpsChange
    Items.Strings = (
      '20000'
      '11905'
      '6666'
      '3571'
      '1851'
      '950'
      '482'
      '242')
  end
end
