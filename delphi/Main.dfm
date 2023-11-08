object Form1: TForm1
  Left = 177
  Top = 84
  Width = 616
  Height = 437
  Caption = 'Test for USB-ATK16'
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  OnActivate = FormActivate
  OnClose = FormClose
  OnCreate = FormCreate
  PixelsPerInch = 96
  TextHeight = 13
  object Label1: TLabel
    Left = 8
    Top = 8
    Width = 82
    Height = 13
    Caption = '��� ����������'
  end
  object lbCommand: TLabel
    Left = 536
    Top = 8
    Width = 45
    Height = 13
    Caption = '�������'
  end
  object bnDeviceOpen: TButton
    Left = 8
    Top = 56
    Width = 75
    Height = 25
    Cursor = crHandPoint
    Caption = '�������'
    TabOrder = 0
    OnClick = bnDeviceOpenClick
  end
  object bnDeviceClose: TButton
    Left = 96
    Top = 56
    Width = 75
    Height = 25
    Cursor = crHandPoint
    Caption = '�������'
    Enabled = False
    TabOrder = 1
    OnClick = bnDeviceCloseClick
  end
  object memoInfo: TMemo
    Left = 8
    Top = 88
    Width = 305
    Height = 313
    Color = clInfoBk
    Lines.Strings = (
      '')
    ScrollBars = ssVertical
    TabOrder = 2
    OnChange = memoInfoChange
  end
  object bnPipe2: TButton
    Left = 528
    Top = 56
    Width = 75
    Height = 25
    Cursor = crHandPoint
    Caption = 'Interrupt'
    TabOrder = 3
    OnClick = bnPipe2Click
  end
  object cbInt: TCheckBox
    Left = 456
    Top = 56
    Width = 65
    Height = 17
    Cursor = crHandPoint
    Caption = '�������'
    TabOrder = 4
    OnClick = cbIntClick
  end
  object sgInterrupt: TStringGrid
    Left = 328
    Top = 88
    Width = 273
    Height = 313
    Cursor = crHandPoint
    Color = clAqua
    ColCount = 9
    DefaultColWidth = 25
    DefaultRowHeight = 16
    FixedCols = 0
    RowCount = 17
    TabOrder = 5
    OnDblClick = sgInterruptDblClick
    ColWidths = (
      25
      25
      25
      25
      28
      25
      25
      25
      41)
  end
  object cbCommand: TComboBox
    Left = 536
    Top = 24
    Width = 65
    Height = 21
    Cursor = crHandPoint
    ItemHeight = 13
    TabOrder = 6
    OnDblClick = cbCommandDblClick
    OnKeyDown = cbCommandKeyDown
  end
  object editInterval: TEdit
    Left = 544
    Top = 56
    Width = 41
    Height = 21
    MaxLength = 4
    TabOrder = 7
    Text = '25'
    Visible = False
    OnChange = editIntervalChange
  end
  object UpDown1: TUpDown
    Left = 585
    Top = 56
    Width = 15
    Height = 21
    Cursor = crHandPoint
    Associate = editInterval
    Min = 10
    Max = 9999
    Position = 25
    TabOrder = 8
    Visible = False
    Wrap = False
  end
  object cbInterface: TComboBox
    Left = 8
    Top = 24
    Width = 521
    Height = 21
    ItemHeight = 13
    TabOrder = 9
  end
  object cbCycled: TCheckBox
    Left = 312
    Top = 48
    Width = 49
    Height = 17
    Caption = '����'
    Enabled = False
    TabOrder = 10
    OnClick = cbCycledClick
  end
  object eSymbol: TEdit
    Left = 368
    Top = 56
    Width = 25
    Height = 21
    Enabled = False
    MaxLength = 2
    TabOrder = 11
    Text = '15'
  end
  object cbSpeed: TComboBox
    Left = 400
    Top = 56
    Width = 49
    Height = 21
    ItemHeight = 13
    TabOrder = 12
    Text = '200'
    Items.Strings = (
      '200'
      '100'
      '50')
  end
  object cbAll: TCheckBox
    Left = 312
    Top = 64
    Width = 49
    Height = 17
    Caption = '���'
    TabOrder = 13
    OnClick = cbAllClick
  end
  object Timer1: TTimer
    Enabled = False
    Interval = 25
    OnTimer = Timer1Timer
    Left = 176
    Top = 56
  end
  object Timer2: TTimer
    Enabled = False
    Interval = 40
    OnTimer = Timer2Timer
    Left = 208
    Top = 56
  end
end
