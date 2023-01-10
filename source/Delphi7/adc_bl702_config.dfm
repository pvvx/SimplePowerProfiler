object FormAdc2Config: TFormAdc2Config
  Left = 1524
  Top = 543
  Width = 487
  Height = 280
  Caption = 'BL702 ADC Config'
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
    Left = 232
    Top = 60
    Width = 37
    Height = 13
    Caption = 'U offset'
  end
  object LabelIz: TLabel
    Left = 234
    Top = 12
    Width = 32
    Height = 13
    Caption = 'I offset'
  end
  object LabelSPS: TLabel
    Left = 327
    Top = 156
    Width = 26
    Height = 13
    Caption = 'Smps'
  end
  object Label1: TLabel
    Left = 235
    Top = 84
    Width = 32
    Height = 13
    Caption = 'U coef'
  end
  object LabelIk: TLabel
    Left = 239
    Top = 36
    Width = 27
    Height = 13
    Caption = 'I coef'
  end
  object RadioGroupAdcChannels: TRadioGroup
    Left = 8
    Top = 8
    Width = 137
    Height = 225
    Caption = 'ADC Channel'
    ItemIndex = 4
    Items.Strings = (
      'GPIO_7'
      'GPIO_8'
      'GPIO_9'
      'GPIO_14'
      'GPIO_15'
      'GPIO_17'
      'DACA'
      'DACB'
      'VREF'
      'VBAT'
      'GND')
    TabOrder = 0
  end
  object ButtonOk: TButton
    Left = 296
    Top = 208
    Width = 75
    Height = 25
    Caption = 'Ok'
    TabOrder = 1
    OnClick = ButtonOkClick
  end
  object ButtonCancel: TButton
    Left = 384
    Top = 208
    Width = 75
    Height = 25
    Caption = 'Cancel'
    ModalResult = 2
    TabOrder = 2
  end
  object EditUz: TEdit
    Left = 280
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
    Left = 280
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
    Left = 376
    Top = 56
    Width = 89
    Height = 25
    Caption = 'Copy Us to Uz'
    TabOrder = 5
    OnClick = ButtonCopyUzClick
  end
  object ButtonCopyIz: TButton
    Left = 376
    Top = 8
    Width = 89
    Height = 25
    Caption = 'Copy Is to Iz'
    TabOrder = 6
    OnClick = ButtonCopyIzClick
  end
  object SpinEditAdcSmps: TSpinEdit
    Left = 364
    Top = 152
    Width = 62
    Height = 22
    Hint = #1047#1072#1084#1077#1088#1086#1074' '#1074' '#1089#1077#1082
    Increment = 50
    MaxValue = 166666
    MinValue = 3906
    ParentShowHint = False
    ShowHint = True
    TabOrder = 7
    Value = 7812
  end
  object EditUk: TEdit
    Left = 280
    Top = 82
    Width = 89
    Height = 21
    Hint = #1050#1086#1101#1092'. '#1076#1083#1103' U'
    ParentShowHint = False
    ShowHint = True
    TabOrder = 8
    Text = '?.?'
  end
  object EditIk: TEdit
    Left = 280
    Top = 34
    Width = 89
    Height = 21
    Hint = #1050#1086#1101#1092'. '#1076#1083#1103' I'
    ParentShowHint = False
    ShowHint = True
    TabOrder = 9
    Text = '?.?'
  end
  object RadioGroupUIadc: TRadioGroup
    Left = 152
    Top = 8
    Width = 73
    Height = 65
    Hint = #1058#1086#1082' '#1080#1083#1080' '#1085#1072#1087#1088#1103#1078#1077#1085#1080#1077
    Caption = 'I or U'
    ItemIndex = 1
    Items.Strings = (
      'I'
      'U')
    ParentShowHint = False
    ShowHint = True
    TabOrder = 10
  end
  object GroupBox1: TGroupBox
    Left = 152
    Top = 120
    Width = 129
    Height = 113
    Caption = 'PGA'
    TabOrder = 11
    object Label2: TLabel
      Left = 48
      Top = 24
      Width = 76
      Height = 13
      Caption = 'PGA1 x 2.5dB ?'
    end
    object Label3: TLabel
      Left = 48
      Top = 46
      Width = 45
      Height = 13
      Caption = 'PGA2 x 2'
    end
    object SpinEditPC1PGA: TSpinEdit
      Left = 10
      Top = 19
      Width = 33
      Height = 22
      Hint = #1047#1072#1084#1077#1088#1086#1074' '#1074' '#1089#1077#1082
      MaxValue = 7
      MinValue = 0
      ParentShowHint = False
      ShowHint = True
      TabOrder = 0
      Value = 1
    end
    object SpinEditPC2PGA: TSpinEdit
      Left = 10
      Top = 43
      Width = 33
      Height = 22
      Hint = #1047#1072#1084#1077#1088#1086#1074' '#1074' '#1089#1077#1082
      MaxValue = 7
      MinValue = 0
      ParentShowHint = False
      ShowHint = True
      TabOrder = 1
      Value = 1
    end
  end
end
