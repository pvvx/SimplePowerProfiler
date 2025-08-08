unit MainFrm;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, ComCtrls, Spin, ExtCtrls, Buttons, TeEngine, Series,
  TeeProcs, Chart, ArrowCha, TeeOpenGL, DB, TeeData, TeeGalleryPanel,
  TeeDraw3D, TeeEdit, TeeComma, TeCanvas, TeeStore, TeeSurfa, TeePoin3,
  TeeSeriesStats,  IniFiles;

const
    ADC_DEVICE_ID = $23;
    I2C_DEVICE_ID = $24;

    SMP_BUF_CNT = $3fffff;
    MAX_BLK_CNT = 116*2;
    COM_BUF_CNT = (MAX_BLK_CNT*2)+2;
    // DEV command id:
    CMD_DEV_VER = $00; // Get Ver
    // Save cfg
    CMD_DEV_SCF = $02; // Store CFG/ini in Flash
    // Status
    CMD_DEV_STA = $03; // Status
    // ADC cfg
    CMD_DEV_CAD = $38; // Get/Set CFG/ini ADC & Start measure
    // ADC out samples
    CMD_DEV_ADC = $3A; // blk out regs ADC data
    // Runtime Error
    CMD_DEV_ERR = $0F; // Runtime Error

    CHART_I_MASK  = 1;
    CHART_U_MASK  = 2;
    CHART_UI_MASK  = 3;

    CHART_I_NUM  = 1;
    CHART_U_NUM  = 0;

type
  xName = (xByte, xWord, xData, xIna2Data, xIna3Data);

 Tcfg_ini = packed record
   none   : dword;
 end;


type

  TCommThread = class(TThread)
  private
    procedure QueryPort;
  protected
    procedure Execute; override;
  end;

  TfrmMain = class(TForm)
    StatusBar: TStatusBar;
    Panel1: TPanel;
    btnQueryDosDevice: TButton;
    ComboBox: TComboBox;
    ButtonOpen: TButton;
    Panel2: TPanel;
    Timer1: TTimer;
    ButtonClrGrf: TButton;
    ButtonStop: TButton;
    Chart: TChart;
    ButtonSaveGRF: TButton;
    ButtonPrtGrf: TButton;
    PrintDialog: TPrintDialog;
    SaveDialog: TSaveDialog;
    PrinterSetupDialog: TPrinterSetupDialog;
    SetColorI: TLabel;
    SetColorU: TLabel;
    ColorDialog: TColorDialog;
    ButtonAsize: TButton;
    ButtonScalP: TButton;
    ButtonScalM: TButton;
    TeeOpenGL1: TTeeOpenGL;
    ChartPreviewer: TChartPreviewer;
    TeeCommander: TTeeCommander;
    ChartTool1: TSeriesStatsTool;
    CheckBoxOpenGL: TCheckBox;
    Series2: TFastLineSeries;
    Series1: TFastLineSeries;
    EditSizeGrf: TEdit;
    EditTriggerI: TEdit;
    CheckBoxTrigerRiseI: TCheckBox;
    LabelMXI: TLabel;
    LabelMXU: TLabel;
    CheckBoxTrigerRiseU: TCheckBox;
    EditTriggerU: TEdit;
    ButtonStartADC: TButton;
    ButtonADCcfg: TButton;
    ButtonSaveWav: TButton;
    Label1: TLabel;
    procedure btnReScanComDevices(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure ReScanComDevices;
    procedure ButtonOpenClick(Sender: TObject);
    function SetComName: boolean;
    procedure ChComName(Sender: TObject);
    function GetDevVersion: boolean;

    function  IndexOfComName(const S: String): Integer;
    procedure TimerTimer(Sender: TObject);
    procedure ButtonClrGrfClick(Sender: TObject);
    procedure ButtonStopClick(Sender: TObject);
    procedure ButtonPrtGrfClick(Sender: TObject);
    procedure ButtonSaveGRFClick(Sender: TObject);

    procedure AddCrfSamples;
    function GrfSetColor(ChNum : integer): integer;
    procedure ButtonAsizeClick(Sender: TObject);
    procedure ButtonScalMClick(Sender: TObject);
    procedure ButtonScalPClick(Sender: TObject);

    procedure ReadIni;
    procedure WriteIni;
    procedure GetScrParms;
    procedure ShowScrParms;
    procedure ShowLabelsMX;
    procedure ShowSmps;
    procedure SetGrfMarging;
    procedure SetParStart;
    function SetAdcIniCfg(mode : integer) : boolean;
    function ReadBlk(id : byte) : boolean;
    function SendBlk(data_count : byte) : boolean;
    function ReadStatus : boolean;
    function StopReadDevice: boolean;
    procedure ClearGrf;
    procedure CheckBoxOpenGLClick(Sender: TObject);
    procedure CheckBoxTrigerClick(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure SetColorIMouseDown(Sender: TObject; Button: TMouseButton;
      Shift: TShiftState; X, Y: Integer);
    procedure SetColorUMouseDown(Sender: TObject; Button: TMouseButton;
      Shift: TShiftState; X, Y: Integer);
    procedure ButtonSaveWavClick(Sender: TObject);
    procedure ButtonStartADCClick(Sender: TObject);
    procedure ButtonADCcfgClick(Sender: TObject);
  private
    flgValueChg : boolean;
    Ini_Cfg : Tcfg_ini;
    ina_max_regs : integer;

    TriggerI : Double;
    TriggerU : Double;
    TriggerUEnable : boolean;
    TriggerULow : boolean;
    TriggerIEnable : boolean;
    TriggerILow : boolean;

    SamplesCount : dword;
    MaxSamples: dword;
    SamplesAutoStop : boolean;
    ConnectStartTime : TDateTime;
    ConnectTime : TDateTime;
    ChartEnables : byte;
    OldCurI, OldCurU: double;
    CurI, CurU: double;
    SumI, SumU: double;
    FormConfigOk : integer;
  public
    { Public declarations }
  end;

var
  frmMain: TfrmMain;
  CommThread: TCommThread;

  dev_type : byte;
  dev_id : byte;
  dev_ver: word; // BCD 0x1234 - > 1.2.3.4

  BuildInfoString: string;

  Uk : double;
  Ik : double;
  Ik10 : double;
  I_zero : double;
  U_zero : double;
  I_zero10 : double;
  OldsI, OldsU: double;
  shunt : byte;

  old_sps : double;

   SamplesEna : boolean;
   bufsmp : array [0..SMP_BUF_CNT] of DWORD;
   ismprx, ismptx : dword;

   purge_com : byte;

   buftx : array [0..63] of byte;
    lentx : integer;
    errtx : byte;

   bufrx : array [0..63] of byte;
    lenrx : integer;
    wait_id : byte;
    wait_cnt : dword;

   bufcom : array [0..COM_BUF_CNT] of Byte;
    idx_bufcom : dword;

    dev_not_send_count : dword;
    dev_all_send_count : dword;
    dev_send_err : byte;

   work_adc : boolean;

  IniFile : TIniFile = nil;
  IniFileName : string = '.\PowerProfiler.ini';

implementation

uses
  StrUtils, ComPort, HexUtils, adc_bl702_config, WaveStorage;

{$R *.dfm}

procedure StartComThread;
begin
  if CommThread = nil then begin
    CommThread := TCommThread.Create(False);
    idx_bufcom := 0;
    wait_id := 0;
    wait_cnt := 0;
    lentx := 0;
    ismprx := 0;
    ismptx := 0;
    errtx := 0;
    bufcom[0] := 0;
  end;
  if CommThread = nil then begin
    SysErrorMessage(GetLastError);
//    Exit;
  end;
end;

procedure CloseComThread;
begin
  if CommThread <> nil then
    CommThread.Terminate;
  CommThread := nil;
end;

procedure TCommThread.Execute;
begin
  repeat
    QueryPort;
  until Terminated;
end;

procedure TCommThread.QueryPort;
var
  Buff: array[0..MAX_BLK_CNT+2] of Byte;
  ByteReaded, ByteWrited, itx: Dword;
  i, cnt : integer;
begin
  if purge_com <> 0 then begin
    purge_com := 0;
    PurgeComm(hCom,PURGE_TXCLEAR or PURGE_RXCLEAR);
    idx_bufcom := 0;
    ismptx := 0;
    ismprx := 0;
  end;
  if lentx <> 0 then begin
     if not WriteFile(hCom, buftx, lentx, ByteWrited, nil) then begin
       errtx := 1;
//       ClearCommError(hCom,dErr,Nil);
//    SysErrorMessage(GetLastError);
     end
     else if dword(lentx) <> ByteWrited then begin
       errtx := 2;
     end
     else errtx := 0;
     lentx := 0;
  end;
  if not ReadFile(hCom, Buff, SizeOf(Buff), ByteReaded, nil) then begin
    idx_bufcom := 0;
//    ClearCommError(hCom,dErr,Nil);
//    SysErrorMessage(GetLastError);
    Exit;
  end;
  if ByteReaded > 0 then  begin
    if ByteReaded + idx_bufcom > sizeof(bufcom) then begin
      // error !
      idx_bufcom := 0;
    end;
    // буферизовать ввод...
    move(Buff, bufcom[idx_bufcom], ByteReaded);

    ByteReaded := ByteReaded + idx_bufcom;

    if bufcom[0] <= MAX_BLK_CNT then begin
      while (bufcom[0] + 2 <= ByteReaded) do begin
        // что-то есть
        if ((work_adc) and ((bufcom[1] and $7F) = CMD_DEV_ADC)) then begin
          // блок данных для графиков
          if ((bufcom[1] and $80) = 0) then begin
            if SamplesEna then begin
              if (bufcom[0] > 1) then begin
                cnt := bufcom[0] div 3;
                itx := ismptx and (SMP_BUF_CNT - 1);
                for i:=1 to cnt do begin
                  bufsmp[itx] := bufcom[i*3 - 1] or (bufcom[i*3] shl 8) or (bufcom[i*3 + 1] shl 16);
                  Inc(itx);
                  itx := itx and SMP_BUF_CNT;
                end;
                ismptx := itx;
              end;
            end
          end
          else begin
            // ошибки при чтении регистров по таймеру
            if (bufcom[0] = 9) then begin
                dev_all_send_count :=  bufcom[0+2] or (bufcom[1+2] shl 8) or (bufcom[2+2] shl 16) or (bufcom[3+2] shl 24);
                dev_not_send_count :=  bufcom[4+2] or (bufcom[5+2] shl 8) or (bufcom[6+2] shl 16) or (bufcom[7+2] shl 24);
                dev_send_err := bufcom[8+2]; // номер ошибки
            end
            else begin
                dev_all_send_count := 0;
                dev_not_send_count := 0;
                dev_send_err := 255; // общая ошибка
            end;
//            SendMessage(frmMain.Handle, WM_NEW_ADC_BLK, 1, 0);
          end;
        end else begin
        // не $07 (выборка по фильтру cmd: wait_id)
          if (wait_cnt > 0) then begin
            //  ожидание ответа команде
            if ((bufcom[1] and $7f) = wait_id) then begin
              // ответ пришел
              if ((bufcom[1] and $80) <> 0) then
                  wait_id := wait_id or $80; // timeout or error
              if bufcom[0] > 0 then
                  move(bufcom[2], bufrx, bufcom[0]);
              lenrx := bufcom[0];
              wait_cnt := 0;
//              SendMessage(frmMain.Handle, WM_RD_COMMAND, 1, 0);
            end else begin
              Inc(wait_cnt);
              if wait_cnt > 3 then begin
                // ответ не пришел
                lenrx := 0;
                wait_cnt := 0;
                wait_id := wait_id or $80; // timeout or error
//                SendMessage(frmMain.Handle, WM_RD_COMMAND, 1, 0);
//                StatusBar.Panels[2].Text:='Нет ответа от устройства в '+ sComNane+'!';
                exit;
              end;
            end;
          end;
        end;
        ByteReaded := ByteReaded - 2 - bufcom[0];
        if ByteReaded > 0 then begin
          move(bufcom[2 + bufcom[0]], bufcom, ByteReaded);
          if bufcom[0] > MAX_BLK_CNT then begin
             // сбой фреймов протокола
             ByteReaded := 0;
             bufcom[0] := 0;
             idx_bufcom := 0;
             break;
          end;
        end;
        idx_bufcom := ByteReaded;
      end;
    end else begin
        // сбой фреймов протокола
        idx_bufcom := 0;
        bufcom[0] := 0;
//        break;
    end;
  end;
end;

function TfrmMain.SendBlk( data_count : byte) : boolean;
var
x : integer;
begin
   if CommThread = nil then begin
     result := false;
     exit;
   end;
   x := 0;
   result := FALSE;
   while(lentx <> 0) do begin
    sleep(10);
    Inc(x);
    if x > 5 then begin
      lentx := 0;
      exit;
    end;
   end;
   buftx[0]:= data_count;
   wait_id := buftx[1];
   wait_cnt := 1;
   lentx := data_count + 2;
   while(lentx <> 0) do begin
    sleep(10);
    Inc(x);
    if x > 5 then begin
      lentx := 0;
      exit;
    end;
   end;
   if errtx <> 0 then exit;
   result := TRUE;
end;


function TfrmMain.ReadBlk(id : byte) : boolean;
var
x : integer;
begin
   if CommThread = nil then begin
     result := false;
     exit;
   end;
   result := true;
   x := 0;
   while(wait_cnt <> 0) do begin
      sleep(10);
      Inc(x);
      if x > 5 then begin
         wait_cnt := 0;
         result := false;
         exit;
      end;
   end;
   if (wait_id and $80) <> 0 then
     result := false;
end;

function TfrmMain.ReadStatus : boolean;
var
 ft : boolean;
begin
   ft := Timer1.Enabled;
   Timer1.Enabled := False;
   result := true;
   buftx[1] := CMD_DEV_STA; // Cmd: Get Status
   if SendBlk(0) then begin
     if ReadBlk(buftx[1]) and (lenrx = 8) then begin
        dev_all_send_count :=  bufrx[0] or (bufrx[1] shl 8) or (bufrx[2] shl 16) or (bufrx[3] shl 24);
        dev_not_send_count :=  bufrx[4] or (bufrx[5] shl 8) or (bufrx[6] shl 16) or (bufrx[7] shl 24);
     end
     else begin
      StatusBar.Panels[2].Text:='Ошибки при чении статуса устройства на '+ sComNane+'!';
      result := false;
     end;
   end
   else begin
      StatusBar.Panels[2].Text:='Ошибки при чении статуса устройства на '+ sComNane+'!';
      result := false;
   end;
   Timer1.Enabled := ft;
end;

// Чтение переменных из .ini
procedure TfrmMain.ReadIni;
begin
   if IniFile = nil then IniFile := TIniFile.Create(IniFileName);
   if IniFile.ReadString('System','Version','') = '' then begin
     IniFile.WriteString('System','Version', BuildInfoString);
     IniFile.WriteInteger('System','MaxSamples', MaxSamples);
     IniFile.WriteFloat('System','TriggerI', TriggerI);
     IniFile.WriteFloat('System','TriggerU', TriggerU);
     IniFile.WriteInteger('System', 'ChIColor', SetColorI.Color);
     IniFile.WriteInteger('System', 'ChUColor', SetColorU.Color);
     IniFile.WriteBool('System','TriggerUena', CheckBoxTrigerRiseU.Checked);
     IniFile.WriteBool('System','TriggerIena', CheckBoxTrigerRiseI.Checked);

     IniFile.WriteInteger('Setup','Top',Top);
     IniFile.WriteInteger('Setup','Left',Left);
     IniFile.WriteInteger('Setup','Width',Width);
     IniFile.WriteInteger('Setup','Height',Height);

     IniFile.WriteInteger('INA229','Smps', ADC_smps);
     IniFile.WriteInteger('INA229','Chnl', ADC_channel);
     IniFile.WriteInteger('INA229','Shunt', ADC_shunt);
     IniFile.WriteFloat('INA229','Uk', Uk_adc);
     IniFile.WriteFloat('INA229','Uz', Uz_adc);
     IniFile.WriteFloat('INA229','Ik0', Ik_adc0);
     IniFile.WriteFloat('INA229','Iz0', Iz_adc0);
     IniFile.WriteFloat('INA229','Ik1', Ik_adc1);
     IniFile.WriteFloat('INA229','Iz1', Iz_adc1);
   end
   else begin
     MaxSamples := IniFile.ReadInteger('System','MaxSamples', MaxSamples);
     TriggerI := IniFile.ReadFloat('System','TriggerI', TriggerI);
     TriggerU := IniFile.ReadFloat('System','TriggerU', TriggerU);
     CheckBoxTrigerRiseU.Checked := IniFile.ReadBool('System','TriggerUena', CheckBoxTrigerRiseU.Checked);
     CheckBoxTrigerRiseI.Checked := IniFile.ReadBool('System','TriggerIena', CheckBoxTrigerRiseI.Checked);

     Top := IniFile.ReadInteger('Setup','Top',10);
     Left := IniFile.ReadInteger('Setup','Left',10);
     if Screen.DesktopHeight < Top then Top := 10;
     if Screen.DesktopWidth < Left then Left := 10;
     Height := IniFile.ReadInteger('Setup','Height',Height);
     Width := IniFile.ReadInteger('Setup','Width',Width);
     sComNane := IniFile.ReadString('Setup','ComPort', sComNane);

     SetColorI.Color := IniFile.ReadInteger('System', 'ChIColor', SetColorI.Color);
     SetColorU.Color := IniFile.ReadInteger('System', 'ChUColor', SetColorU.Color);

     ADC_smps := IniFile.ReadInteger('INA229','Smps', ADC_smps);
     ADC_channel := IniFile.ReadInteger('INA229','Chnl', ADC_channel);
     ADC_shunt := IniFile.ReadInteger('INA229','Shunt', ADC_shunt);

     Uk_adc := IniFile.ReadFloat('INA229','Uk', Uk_adc);
     Uz_adc := IniFile.ReadFloat('INA229','Uz', Uz_adc);

     Ik_adc0 := IniFile.ReadFloat('INA229','Ik0', Ik_adc0);
     Iz_adc0 := IniFile.ReadFloat('INA229','Iz0', Iz_adc0);

     Ik_adc1 := IniFile.ReadFloat('INA229','Ik1', Ik_adc1);
     Iz_adc1 := IniFile.ReadFloat('INA229','Iz1', Iz_adc1);

   end;
   IniFile.UpdateFile;
end;

// Запись переменных в *.ini
procedure TfrmMain.WriteIni;
begin
     IniFile.WriteString('System','Version', BuildInfoString);
     IniFile.WriteInteger('System','MaxSamples', MaxSamples);
     IniFile.WriteFloat('System','TriggerI', TriggerI);
     IniFile.WriteFloat('System','TriggerU', TriggerU);
     IniFile.WriteBool('System','TriggerUena', CheckBoxTrigerRiseU.Checked);
     IniFile.WriteBool('System','TriggerIena', CheckBoxTrigerRiseI.Checked);

     IniFile.WriteInteger('Setup','Top',Top);
     IniFile.WriteInteger('Setup','Left',Left);
     IniFile.WriteInteger('Setup','Width',Width);
     IniFile.WriteInteger('Setup','Height',Height);
     IniFile.WriteString('Setup','ComPort',sComNane);

     IniFile.WriteInteger('System', 'ChIColor', SetColorI.Color);
     IniFile.WriteInteger('System', 'ChUColor', SetColorU.Color);

     IniFile.WriteInteger('INA229','Smps', ADC_smps);
     IniFile.WriteInteger('INA229','Chnl', ADC_channel);
     IniFile.WriteInteger('INA229','Shunt', ADC_shunt);
     IniFile.WriteFloat('INA229','Uk', Uk_adc);
     IniFile.WriteFloat('INA229','Uz', Uz_adc);
     IniFile.WriteFloat('INA229','Ik0', Ik_adc0);
     IniFile.WriteFloat('INA229','Iz0', Iz_adc0);
     IniFile.WriteFloat('INA229','Ik1', Ik_adc1);
     IniFile.WriteFloat('INA229','Iz1', Iz_adc1);

     IniFile.UpdateFile;
end;


function Str2dword(const s: string): dword;
var
 i: integer;
 o : string;
 hex : boolean;
begin
    i := 1;
    hex := False;
//    result := 0;
    while (i <= Length(s)) do begin
      if ((s[i] = '0') and (s[i+1] = 'x')) then begin
       hex := True;
       inc(i);
      end
      else if (s[i] = '$') then  hex := True
      else if (((s[i] >= 'a') and (s[i] <= 'f'))
      or ((s[i] >= 'A') and (s[i] <= 'F'))) then begin
       hex := True;
       o := o + s[i];
      end
      else if ((s[i] >= '0') and (s[i] <= '9')) then o := o + s[i];
      inc(i);
    end;
    if hex then result := StrToIntDef('$'+o,0)
    else result := StrToIntDef(o,0)
end;

procedure GetBuildInfo(var V1, V2, V3, V4: word);
var
  VerInfoSize, VerValueSize, Dummy: DWORD;
  VerInfo: Pointer;
  VerValue: PVSFixedFileInfo;
begin
  VerInfoSize := GetFileVersionInfoSize(PChar(ParamStr(0)), Dummy);
  if VerInfoSize > 0 then
  begin
      GetMem(VerInfo, VerInfoSize);
      try
        if GetFileVersionInfo(PChar(ParamStr(0)), 0, VerInfoSize, VerInfo) then
        begin

          VerQueryValue(VerInfo, '\', Pointer(VerValue), VerValueSize);
          with VerValue^ do
          begin
            V1 := dwFileVersionMS shr 16;
            V2 := dwFileVersionMS and $FFFF;
            V3 := dwFileVersionLS shr 16;
            V4 := dwFileVersionLS and $FFFF;
          end;
        end;
      finally
        FreeMem(VerInfo, VerInfoSize);
      end;
  end;
end;

function GetBuildInfoAsString: string;
var
  V1, V2, V3, V4: word;
begin
  GetBuildInfo(V1, V2, V3, V4);
  Result := IntToStr(V1) + '.' + IntToStr(V2) + '.' +
    IntToStr(V3) + '.' + IntToStr(V4);
end;

//Достает из строки с нуль-терминированными подстроками следующую нуль-терминированную
//подстроку начиная с позиции aStartPos, потом устанавливает aStartPos на символ
//следующий за терминирующим #0.
function GetNextSubstring(aBuf: string; var aStartPos: integer): string;
var
  vLastPos: integer;
begin
  if (aStartPos < 1) then
    begin
      raise ERangeError.Create('aStartPos должен быть больше 0');
    end;

  if (aStartPos > Length(aBuf) ) then
    begin
      Result := '';
      Exit;
    end;

  vLastPos := PosEx(#0, aBuf, aStartPos);
  Result := Copy(aBuf, aStartPos, vLastPos - aStartPos);
  aStartPos := aStartPos + (vLastPos - aStartPos) + 1;
end;

// Заполняет список aList наденными в системе COM портами
procedure GetComPorts(aList: TStrings; aNameStart: string);
var
  vBuf: string;
  vRes: integer;
  vErr: Integer;
  vBufSize: Integer;
  vNameStartPos: Integer;
  vName: string;
begin
  vBufSize := 1024 * 5;
  vRes := 0;

  while vRes = 0 do
    begin
      setlength(vBuf, vBufSize) ;
      SetLastError(ERROR_SUCCESS);
      vRes := QueryDosDevice(nil, @vBuf[1], vBufSize) ;
      vErr := GetLastError();

      // Вариант для двухтонки
      if (vRes <> 0) and (vErr = ERROR_INSUFFICIENT_BUFFER) then
        begin
          vBufSize := vRes;
          vRes := 0;
        end;

      if (vRes = 0) and (vErr = ERROR_INSUFFICIENT_BUFFER) then
        begin
          vBufSize := vBufSize + 1024;
        end;

      if (vErr <> ERROR_SUCCESS) and (vErr <> ERROR_INSUFFICIENT_BUFFER) then
        begin
          raise Exception.Create(SysErrorMessage(vErr) );
        end
    end;
  setlength(vBuf, vRes) ;

  vNameStartPos := 1;
  vName := GetNextSubstring(vBuf, vNameStartPos);

  aList.BeginUpdate();
  try
    aList.Clear();
    while vName <> '' do
      begin
        if AnsiStartsStr(aNameStart, vName) then
          aList.Add(vName);
        vName := GetNextSubstring(vBuf, vNameStartPos);
      end;
  finally
    aList.EndUpdate();
  end;
end;


procedure TfrmMain.btnReScanComDevices(Sender: TObject);
begin
  ReScanComDevices;
end;

function TfrmMain.IndexOfComName(const S: String): Integer;
var
  I : Integer;
begin
  Result := -1;
  for I := 0 to ComboBox.Items.Count-1 do
  begin
    if ComboBox.Items.Strings[I] = S then
    begin
      Result := I;
      Exit;
    end;
  end;
end;

function TfrmMain.SetComName: boolean;
begin
  result:=False;
  sComNane:=ComboBox.Items.Strings[ComboBox.ItemIndex];
  if(sComNane='') then begin
    StatusBar.Panels[0].Text:='COM?';
    StatusBar.Panels[2].Text:='Не выбран COM порт!';
  end
  else begin
    StatusBar.Panels[0].Text:=sComNane;
    StatusBar.Panels[2].Text:='Выбран '+sComNane+'.';
    result:=True;
  end;
end;

procedure TfrmMain.ReScanComDevices;
begin
  ComboBox.Items.Clear;
  GetComPorts(ComboBox.Items, 'COM');
  if(sComNane <> '') then begin
    ComboBox.ItemIndex := IndexOfComName(sComNane);
  end
  else
    ComboBox.ItemIndex:=0;
  SetComName;
end;


procedure TfrmMain.GetScrParms;
begin
    DecimalSeparator := '.';
    MaxSamples := Str2dword(EditSizeGrf.Text);
    TriggerI := StrToFloat(EditTriggerI.Text);
    TriggerU := StrToFloat(EditTriggerU.Text);
end;


procedure TfrmMain.ShowScrParms;
begin
    DecimalSeparator := '.';
    EditSizeGrf.Text := IntToStr(MaxSamples);
    EditTriggerI.Text:=FormatFloat('0.0000', TriggerI);
    EditTriggerU.Text:=FormatFloat('0.0000', TriggerU);
end;

procedure TfrmMain.ShowLabelsMX;
begin
        if ((ChartEnables and CHART_I_MASK) <> 0)  and (SamplesCount <> 0) then begin
            OldsI := SumI/SamplesCount;
            LabelMXI.Caption := 'I:' + FormatFloat('# ##0.000 000', OldsI);
        end
        else
            LabelMXI.Caption := 'I#' + FormatFloat('# ##0.000 000', OldCurI);
        if ((ChartEnables and CHART_U_MASK) <> 0) and (SamplesCount <> 0) then begin
            OldsU := SumU/SamplesCount;
            LabelMXU.Caption := 'U:' + FormatFloat('# ##0.000 000', SumU/SamplesCount);
        end
        else
            LabelMXU.Caption := 'U#' + FormatFloat('# ##0.000 000', OldCurU);
end;

procedure TfrmMain.ShowSmps;
begin
    old_sps := ADC_smps;
    StatusBar.Panels[1].Text := FormatFloat('# ##0.0', old_sps) + ' sps';
end;

procedure TfrmMain.FormCreate(Sender: TObject);
begin
  if Screen.DesktopHeight <= Top then Top := 10;
  if Screen.DesktopWidth <= Left then Left := 10;

  DecimalSeparator := '.';
  flgValueChg := False;
  work_adc := False;

  BuildInfoString := GetBuildInfoAsString;
  Caption := Application.Title + ' ver ' + BuildInfoString;
  SetGrfMarging;

  SamplesAutoStop := False;

  Ini_Cfg.none := $00;
  dev_type := 0;
  dev_id := 0;
  dev_send_err := 0;
  ChartEnables := 0;

  GetScrParms;

  SetColorU.Color := Chart.Series[CHART_U_NUM].Color;
  SetColorI.Color := Chart.Series[CHART_I_NUM].Color;

  ReadIni;

  Chart.Series[CHART_U_NUM].Color := SetColorU.Color;
  Chart.Series[CHART_I_NUM].Color := SetColorI.Color;

  ReScanComDevices;

  ShowScrParms;
  flgValueChg := True;
end;

procedure TfrmMain.ChComName(Sender: TObject);
begin
  SetComName;
end;

procedure TfrmMain.FormClose(Sender: TObject; var Action: TCloseAction);
begin
  GetScrParms;
  WriteIni;
end;

procedure TfrmMain.ButtonOpenClick(Sender: TObject);
begin
   Timer1.Enabled := False;
   SamplesEna := False;
   if not flgComOpen then begin
    if SetComName then begin
      if OpenCom(sComNane) then begin
        purge_com := 1;
        ClearGrf;
        ChartEnables := 0;
        StartComThread;
        if not GetDevVersion then begin
          CloseComThread;
          CloseCom;
          if (dev_id = ADC_DEVICE_ID) or (dev_id = I2C_DEVICE_ID) then begin
            ShowMessage('Error read device config!'+#13#10+'Com closed.');
            StatusBar.Panels[2].Text:='Ошибки в чении конфигурации устройства на '+ sComNane+'!';
          end else begin
            ShowMessage('Error device ID!'+#13#10+'Com closed.');
            StatusBar.Panels[2].Text:='Не тот ID у устройства на '+ sComNane+'!';
          end
        end else begin
          ComboBox.Enabled := False;
          btnQueryDosDevice.Enabled := False;
          ButtonOpen.Caption := 'Close';
          ButtonStop.Enabled := True;
          purge_com := 1;
//          if (dev_id = ADC_DEVICE_ID) or (dev_id = I2C_DEVICE_ID) then  begin
              ButtonStartADC.Enabled := True;
              ButtonStartADC.Visible := True;
              ButtonADCcfg.Enabled := True;
              ButtonADCcfg.Visible := True;
//          end;
          purge_com := 1;
          ClearGrf;
          ConnectStartTime := GetTime;
        end
      end
      else begin
        StatusBar.Panels[2].Text:='Ошибка открытия '+ sComNane+'!';
        ShowMessage(StatusBar.Panels[2].Text);
      end;
    end
    else begin
      StatusBar.Panels[2].Text:='Выберите COM порт!';
    end;
  end
  else begin
     CloseComThread;
     CloseCom;
     ButtonOpen.Caption := 'Open';
     ComboBox.Enabled := True;
     btnQueryDosDevice.Enabled := True;

     ButtonStop.Enabled := False;
     ButtonStartADC.Enabled := False;
     StatusBar.Panels[2].Text:=sComNane+' закрыт.';
  end;
end;


procedure TfrmMain.ButtonStopClick(Sender: TObject);
begin
   SamplesEna := False;
   Timer1.Enabled := False;
   if ReadStatus then begin
      if (dev_id = ADC_DEVICE_ID) or (dev_id = I2C_DEVICE_ID) then begin
        if not SetAdcIniCfg(0)then
          exit;
      end;
      StatusBar.Panels[2].Text:='Непрерывное чтение устройства на '+ sComNane+' остановлено. Пропуск ' + IntToStr(dev_not_send_count) + ' блоков, счетчик: ' + IntToStr(dev_all_send_count) + '.'; // +    IntToStr(dev_send_smps);
      sleep(10);
      purge_com := 1;
      ismptx := 0;
      ismprx := 0;
      SetParStart;
   end;
end;

function TfrmMain.StopReadDevice: boolean;
var
 i : integer;
begin
  result := False;
  for i:=0 to 5 do begin
    purge_com := 1;

    buftx[1]:= CMD_DEV_CAD; // Set/Get CFG/ini & Start measure
    buftx[2]:=0; // read regs = 0

    if SendBlk(1) then begin
      if ReadBlk(buftx[1]) and
      (lenrx >= 6) then begin
        //move(bufrx, blk_cfg, SizeOf(blk_cfg));
        sleep(10);
        purge_com := 1;
        result := True;
        break;
      end;
    end;
  end;
end;

function TfrmMain.GetDevVersion: boolean;
var
 i : integer;
begin
  result := False;
  for i:=0 to 5 do begin
    purge_com := 1;
    buftx[1]:= CMD_DEV_VER; // Get Version
    if SendBlk(0) then begin
      if ReadBlk(buftx[1]) and (lenrx = 4) then begin
        dev_type := bufrx[1];
        dev_id := bufrx[0];
        dev_ver := bufrx[2] or (bufrx[3] shl 8);
        StatusBar.Panels[2].Text:='Устройство ID:' + IntToHex(dev_type, 2) + '-' + IntToHex(dev_id, 2) +' версии '+IntToStr((dev_ver shr 12) and $0f) +'.'+IntToStr((dev_ver shr 8) and $0f)+'.'+IntToStr((dev_ver shr 4) and $0f)+'.'+IntToStr(dev_ver and $0f)+' подключено на '+ sComNane +'.';
        if (dev_id = ADC_DEVICE_ID) or (dev_id = I2C_DEVICE_ID)  then begin
          if StopReadDevice then begin
              result := True;
              exit;
          end else
            if (dev_id = ADC_DEVICE_ID) or (dev_id = I2C_DEVICE_ID) then begin
              result := True;
              exit;
            end;
        end;
      end;
    end;
  end;
end;


procedure TfrmMain.AddCrfSamples;
var
idx, t : double;
delta : double;
flg : boolean;
irx,itx : dword;
begin
  if ChartEnables = CHART_UI_MASK then begin
    t := 2000.0/ADC_smps;
  end
  else begin
    t := 1000.0/ADC_smps;
  end;
  idx := SamplesCount*t;
  if idx >= MaxSamples then begin
    SamplesAutoStop := True;
    ShowLabelsMX;
    exit;
  end;
  irx := ismprx and SMP_BUF_CNT;
  itx := ismptx and SMP_BUF_CNT;
  while(irx <> itx) do begin
    if (ChartEnables and CHART_U_MASK) <> 0 then begin
      OldCurU := CurU;
      //if bufsmp[irx] >= $800000 then
      //    CurU := (Longint(bufsmp[irx])-$1000000)*Uk + U_zero
      //else
      CurU := bufsmp[irx]*Uk + U_zero;
      Inc(irx);
      irx := irx and SMP_BUF_CNT;
    end;
    if (ChartEnables and CHART_I_MASK) <> 0 then begin
      if bufsmp[irx] >= $800000 then
   	    CurI := (Longint(bufsmp[irx])-$1000000)*Ik + I_zero
      else
  	    CurI := bufsmp[irx]*Ik + I_zero;
      Inc(irx);
      irx := irx and SMP_BUF_CNT;
    end;
    flg := True;
    if SamplesCount = 0 then begin
      if ((ChartEnables and CHART_I_MASK) <> 0) then begin
        if TriggerIEnable then begin
          // ждем триггера I
          if (CurI < TriggerI) then begin
            TriggerILow := True;
            flg := False;
          end
          else begin
            if not TriggerILow then
              flg := False;
          end;
        end;
      end;
      if ((ChartEnables and CHART_U_MASK) <> 0) then begin
        if TriggerUEnable then begin
          // ждем триггера U
          if (CurU < TriggerU) then begin
            TriggerULow := True;
            flg := False;
          end
          else begin
            if not TriggerULow then
              flg := False;
          end;
        end;
      end;
      if flg then begin
        if (TriggerIEnable or TriggerUEnable) then begin // поехали ?
          // Втсавить пред. точку
          if (ChartEnables and CHART_I_MASK) <> 0 then begin
            Chart.Series[CHART_I_NUM].Clear;
            Chart.Series[CHART_I_NUM].AddXY(idx, OldCurI,'',0);
            SumI := SumI + CurI;
          end;
          if (ChartEnables and CHART_U_MASK) <> 0 then begin
            Chart.Series[CHART_U_NUM].Clear;
            Chart.Series[CHART_U_NUM].AddXY(idx, OldCurU,'',0);
            SumU := SumU + CurU;
          end;
          Inc(SamplesCount);
          idx := SamplesCount*t; //  idx := idx + k;
        end;
      end;
    end; // if SamplesCount = 0
    if (ChartEnables = CHART_UI_MASK) then begin
      Chart.Series[CHART_I_NUM].AddXY(idx, CurI,'',0);
      Chart.Series[CHART_U_NUM].AddXY(idx, CurU,'',0);
      if flg then begin
        SumU := SumU + CurU;
        SumI := SumI + CurI;
      end;
    end
    else if (ChartEnables and CHART_I_MASK) <> 0 then begin
      Chart.Series[CHART_I_NUM].AddXY(idx, CurI,'',0);
      if flg then begin
        SumI := SumI + CurI;
      end;
    end
    else if (ChartEnables and CHART_U_MASK) <> 0 then begin
      Chart.Series[CHART_U_NUM].AddXY(idx, CurU,'',0);
      if flg then begin
        SumU := SumU + CurU;
      end;
    end;
    if flg then begin
      Inc(SamplesCount);
      idx := SamplesCount*t; // idx := idx + k;
      if idx >= MaxSamples then begin
//        ShowSmps;
        ShowLabelsMX;
        if (not ReadStatus)
        or (not SetAdcIniCfg(0)) then
          StatusBar.Panels[2].Text:='Ошибка передачи в устройство на '+ sComNane+'!'
        else begin
          StatusBar.Panels[2].Text:='Непрерывное чтение остановлено по концу буфера. Пропуск ' + IntToStr(dev_not_send_count) + ' блоков, счетчик: ' + IntToStr(dev_all_send_count) + '.';
          SamplesEna := False;
          Timer1.Enabled := False;
          purge_com := 1;
        end;
        SamplesAutoStop := True;
        break;
      end; // if idx >= MaxSamples
    end; // if flg
  end; // while(i < count)
  ismprx := irx;
end;



procedure TfrmMain.TimerTimer(Sender: TObject);
var
 t : TDateTime;
begin
    if SamplesEna then begin
      if ismptx <> ismprx then begin
         AddCrfSamples();
      end;
      if dev_send_err <> 0 then begin
        if not StopReadDevice then
          StatusBar.Panels[2].Text:='Ошибка передачи команды останова в устройство на '+ sComNane+'!'
        else begin
          StatusBar.Panels[2].Text:='Сбой получения данных, ошибка N' + IntToStr(dev_send_err)
          +': пропуск ' + IntToStr(dev_not_send_count) + ' блоков, счетчик: ' + IntToStr(dev_all_send_count) + '!';
          SamplesEna := False;
          Timer1.Enabled := False;
          purge_com := 1;
        end;
        dev_send_err := 0;
        SamplesAutoStop := True;
      end;
      t := GetTime;
      if (t - ConnectStartTime) > 2/864000.0 then begin
        ConnectTime := ConnectTime + t - ConnectStartTime;
        ConnectStartTime := t;
//        StatusBar.Panels[1].Text := FormatFloat('# ##0.', (SamplesCount/86400.0)/ConnectTime ) + ' sps';
//        ShowSmps;
        ShowLabelsMX;
      end;
    end;
end;


procedure TfrmMain.ClearGrf;
var
i : integer;
begin
    for i:=0 to Chart.SeriesCount-1 do Chart.Series[i].Clear;
    ConnectTime := 0;
    ConnectStartTime := GetTime;
    SamplesCount := 0;
    SumI := 0.0;
    SumU := 0.0;
end;

procedure TfrmMain.ButtonClrGrfClick(Sender: TObject);
var
tmp1, tmp2 : boolean;
begin
    tmp1:=SamplesEna;
    tmp2:=Timer1.Enabled;
    if flgComOpen then begin
      SamplesEna := False;
      Timer1.Enabled := False;
      purge_com := 1;
      ismptx := 0;
      ismprx := 0;
      ClearGrf;
      SetParStart;
      ConnectStartTime := GetTime;
      SamplesEna:=tmp1;
      Timer1.Enabled:=tmp2;
    end
    else
      ClearGrf;
end;


procedure TfrmMain.ButtonPrtGrfClick(Sender: TObject);
begin
    ChartPreviewer.Execute;
end;

procedure TfrmMain.ButtonSaveGRFClick(Sender: TObject);
var
i, data_cont : dword;
bfile : THandle;
s: string;
x,y : double;
begin
    case ChartEnables of
     CHART_I_MASK : data_cont := Chart.Series[CHART_I_NUM].Count;
     CHART_U_MASK : data_cont := Chart.Series[CHART_U_NUM].Count;
     CHART_UI_MASK : data_cont := Chart.Series[CHART_U_NUM].Count;
    else data_cont := 0;
    end;
    if (data_cont <> 0) then begin
     with SaveDialog do begin
      FilterIndex:=0;
      FileName := 'data.csv';
      InitialDir := '.';
      DefaultExt := 'csv';
      Filter := 'csv files (*.csv)|*.csv';
      Options:=Options+[ofFileMustExist]-[ofHideReadOnly]
        +[ofNoChangeDir]-[ofNoLongNames]-[ofNoNetworkButton]-[ofHideReadOnly]
        -[ofOldStyleDialog]-[ofOverwritePrompt]+[ofPathMustExist]
        -[ofReadOnly]-[ofShareAware]-[ofShowHelp];
      Title:='Сохранить данные в файл';
     end;//with
     if SaveDialog.Execute then begin
      Repaint;
      bfile := FileCreate(SaveDialog.FileName);
      if bfile<>INVALID_HANDLE_VALUE then begin
        s:='X;Y;SPS '+FormatFloat('0.000',old_sps)+#13+#10;
        FileWrite(bfile, s[1], Length(s));
        for i:=0 to data_cont -1  do begin
          if (ChartEnables and 1) <> 0 then begin
            try
             x := Chart.Series[CHART_U_NUM].YValue[i];
            except
             x := 0;
            end;
          end else x := 0;
          if (ChartEnables and 2) <> 0 then begin
            try
             y:=Chart.Series[CHART_I_NUM].YValue[i];
            except
             y := -4096;
            end;
          end else y := 0;
          s:=FloatToStr(x)+';'
           +FloatToStr(y)+#13+#10;
          FileWrite(bfile, s[1],Length(s));
        end;
      end;
      FileClose(bfile);
     end;
    end;
end;

function TfrmMain.GrfSetColor(ChNum : integer): integer;
begin
  ColorDialog.Color := Chart.Series[ChNum].SeriesColor; //xShape[ChNum].Brush.Color;
  if ColorDialog.Execute then
   Chart.Series[ChNum].SeriesColor:=ColorDialog.Color;
  result := ColorDialog.Color
end;


procedure TfrmMain.ButtonAsizeClick(Sender: TObject);
begin
   if(Chart.LeftAxis.Automatic) then begin
    ButtonASize.Caption:='Auto';
    Chart.LeftAxis.Automatic:=False;
    Chart.RightAxis.Automatic:=False;
    ButtonScalM.Visible:=True;
    ButtonScalP.Visible:=True;
  end
  else begin
    ButtonASize.Caption:='Manual';
    Chart.LeftAxis.Automatic:=True;
    Chart.RightAxis.Automatic:=True;
    ButtonScalM.Visible:=False;
    ButtonScalP.Visible:=False;
  end;
end;

procedure TfrmMain.ButtonScalMClick(Sender: TObject);
var
d : double;
begin
  if(not Chart.LeftAxis.Automatic) then begin
    d:= (Chart.LeftAxis.Maximum - Chart.LeftAxis.Minimum)/25;
    Chart.LeftAxis.Minimum:=Chart.LeftAxis.Minimum - d;
    Chart.LeftAxis.Maximum:=Chart.LeftAxis.Maximum + d;
    d:= (Chart.RightAxis.Maximum - Chart.RightAxis.Minimum)/25;
    Chart.RightAxis.Minimum:=Chart.RightAxis.Minimum - d;
    Chart.RightAxis.Maximum:=Chart.RightAxis.Maximum + d;
  end;
end;


procedure TfrmMain.ButtonScalPClick(Sender: TObject);
var
d : double;
begin
  if(not Chart.LeftAxis.Automatic) then begin
    d:= (Chart.LeftAxis.Maximum - Chart.LeftAxis.Minimum)/25;
    Chart.LeftAxis.Minimum:=Chart.LeftAxis.Minimum + d;
    Chart.LeftAxis.Maximum:=Chart.LeftAxis.Maximum - d;
    d:= (Chart.RightAxis.Maximum - Chart.RightAxis.Minimum)/25;
    Chart.RightAxis.Minimum:=Chart.RightAxis.Minimum + d;
    Chart.RightAxis.Maximum:=Chart.RightAxis.Maximum - d;
  end;
end;


procedure TfrmMain.SetGrfMarging;
begin
  if CheckBoxOpenGL.Checked then begin
    Chart.MarginBottom := 5;
    Chart.MarginTop := 2;
    Chart.MarginRight := 2;
    Chart.MarginLeft := 2;
  end else begin
    Chart.MarginBottom := 2;
    Chart.MarginLeft := 2;
    Chart.MarginRight := 2;
    Chart.MarginTop := 2;
  end;
end;

procedure TfrmMain.CheckBoxOpenGLClick(Sender: TObject);
begin
  TeeOpenGL1.Active := CheckBoxOpenGL.Checked;
  SetGrfMarging;
end;


procedure TfrmMain.CheckBoxTrigerClick(Sender: TObject);
begin
  if flgValueChg then
    SetParStart;
end;

procedure TfrmMain.SetParStart;
begin
        GetScrParms;
        ShowScrParms;

        TriggerIEnable := CheckBoxTrigerRiseI.Checked;
        TriggerUEnable := CheckBoxTrigerRiseU.Checked;
        TriggerILow := False;
        TriggerULow := False;

        if work_adc then begin
          FormAdcConfig.GetParams;
          exit;
        end;
end;

procedure TfrmMain.SetColorIMouseDown(Sender: TObject;
  Button: TMouseButton; Shift: TShiftState; X, Y: Integer);
begin
  if Button = mbRight then // mbLeft
   SetColorI.Color := GrfSetColor(CHART_I_NUM)
  else
    if Chart.Series[CHART_I_NUM].Visible then
      Chart.Series[CHART_I_NUM].Visible := False
    else
      Chart.Series[CHART_I_NUM].Visible := True;

end;

procedure TfrmMain.SetColorUMouseDown(Sender: TObject;
  Button: TMouseButton; Shift: TShiftState; X, Y: Integer);
begin
  if Button = mbRight then // mbLeft
    SetColorU.Color := GrfSetColor(CHART_U_NUM)
  else
   if Chart.Series[CHART_U_NUM].Visible then
    Chart.Series[CHART_U_NUM].Visible := False
   else
    Chart.Series[CHART_U_NUM].Visible := True;
end;

function TfrmMain.SetAdcIniCfg(mode : integer) : boolean;
const
	MAX_OUT_SPS_D1 = 62500; // 250000 div 4;
  MIN_OUT_SPS = 8065;
var
  smpr : dword;
begin
   result := False;
   Timer1.Enabled := False;
   smpr := ADC_smps;

   buftx[0] := 5;  // size
   buftx[1] := CMD_DEV_CAD;  //  Get/Set CFG/ini ADC & Start measure
   buftx[2] := mode; // enable/disable
   buftx[3] := ADC_channel; // channel
   buftx[4] := smpr;  // freq
   buftx[5] := smpr shr 8; // freq
   buftx[6] := ADC_shunt and 1;
   if SendBlk(5) then begin
     if ReadBlk(buftx[1]) and (lenrx >= 5) then begin
       ADC_channel :=  bufrx[1];
       ADC_smps :=  bufrx[2] or (bufrx[3] shl 8);
       StatusBar.Panels[2].Text:='Конфигурация ADC передана в устройство на '+ sComNane+'.';
       result := True;
     end
     else begin
       StatusBar.Panels[2].Text:='Ошибка записи ADC конфигурации в устройство на '+ sComNane+'!';
     end;
   end;
   if SamplesEna then Timer1.Enabled := True;
end;

procedure TfrmMain.ButtonStartADCClick(Sender: TObject);
var
s : dword;
maxs : dword;
begin
    SamplesEna := False;
    Timer1.Enabled := False;
    ButtonStartADC.Caption := 'Start ADC';
    purge_com := 1;
    if SetAdcIniCfg(1) then begin
        ConnectStartTime := GetTime;
        StatusBar.Panels[2].Text:='Запущено непрерывное чтение ADC в устройстве на '+ sComNane+'.';
        work_adc := True;
        SetParStart;
        if (ADC_channel = 2) then
            ChartEnables := CHART_U_MASK
        else if (ADC_channel = 1) then
            ChartEnables := CHART_I_MASK
        else
            ChartEnables := CHART_UI_MASK;
        s := ADC_smps;
        if s <> 0 then begin
          maxs := 300000; // ms
          if MaxSamples > maxs then begin
            MaxSamples := maxs; // ms
            EditSizeGrf.Text := IntToStr(MaxSamples); // ms
          end;
        end;
        ShowSmps;
        if SamplesAutoStop then begin
           SamplesAutoStop := False;
           ClearGrf;
        end;
        ismptx := 0;
        ismprx := 0;
        SamplesEna := True;
        Timer1.Enabled := True;
   end;
end;

procedure TfrmMain.ButtonADCcfgClick(Sender: TObject);
begin
    FormAdcConfig.Left := Left + (Width div 2) - FormAdcConfig.Width div 2;
    FormAdcConfig.Top := Top + (Height div 2) - FormAdcConfig.Height div 2;
    FormConfigOk := FormAdcConfig.ShowModal;
end;

procedure TfrmMain.ButtonSaveWavClick(Sender: TObject);
var
i, data_cont : dword;
stereo : boolean;
bfile : THandle;
f,x,y : double;
val : array [0..1] of dword;
begin
    case ChartEnables of
     CHART_I_MASK : data_cont := Chart.Series[CHART_I_NUM].Count;
     CHART_U_MASK : data_cont := Chart.Series[CHART_U_NUM].Count;
     CHART_UI_MASK : data_cont := Chart.Series[CHART_U_NUM].Count;
    else data_cont := 0;
    end;
    if (data_cont <> 0) then begin
     with SaveDialog do begin
      FilterIndex:=0;
      FileName := 'data.wav';
      InitialDir := '.';
      DefaultExt := 'wav';
      Filter := 'wav files (*.wav)|*.wav';
      Options:=Options+[ofFileMustExist]-[ofHideReadOnly]
        +[ofNoChangeDir]-[ofNoLongNames]-[ofNoNetworkButton]-[ofHideReadOnly]
        -[ofOldStyleDialog]-[ofOverwritePrompt]+[ofPathMustExist]
        -[ofReadOnly]-[ofShareAware]-[ofShowHelp];
      Title:='Сохранить данные в файл';
     end;//with
     if SaveDialog.Execute then begin
      Repaint;
      bfile := FileCreate(SaveDialog.FileName);
      if bfile<>INVALID_HANDLE_VALUE then begin
        f := old_sps;
//        if (not work_adc) and (ChartEnables = CHART_UI_MASK) then
//          f := old_sps/2;
        if f = 0.0 then
          f := 8000.0
        else if f < 100.0 then
          f := f * 1000.0;
        if(ChartEnables = CHART_UI_MASK) then
          stereo := True
        else
          stereo := False;
        SetWavHeader(stereo, Round(f), data_cont, 24);
        FileWrite(bfile, wav_header, sizeof(wav_header));
        for i:=0 to data_cont -1  do begin
          if (ChartEnables and CHART_U_MASK) <> 0 then begin
            try
             x := (Chart.Series[CHART_U_NUM].YValue[i]-U_zero)/Uk;
            except
             x := 0;
            end;
            if(x > 8388607.0) then x := 8388607.0
            else if(x < -8388608.0) then x := -8388608.0;
            val[0] := Round(x); //Trunc(x);
          end else val[0] := 0;

          if (ChartEnables and CHART_I_MASK) <> 0 then begin
            try
             y := (Chart.Series[CHART_I_NUM].YValue[i]-I_zero)/Ik;
            except
             y := 0;
            end;
            //if work_adc then y := y - 8388608.0;
            if(y > 8388607.0) then y := 8388607.0
            else if(y < -8388608.0) then y := -8388608.0;
            val[1] := Round(y); // Trunc(y);
          end else val[1] := 0;
          case ChartEnables of
{
           CHART_U_MASK : FileWrite(bfile, val[0] , sizeof(val[0]));
           CHART_I_MASK : FileWrite(bfile, val[1] , sizeof(val[1]));
           CHART_UI_MASK : FileWrite(bfile, val , sizeof(val));
}
           CHART_U_MASK : FileWrite(bfile, val[0] , 3); //sizeof(val[0]));
           CHART_I_MASK : FileWrite(bfile, val[1] , 3); //sizeof(val[1]));
           CHART_UI_MASK : begin
                 FileWrite(bfile, val[0] , 3); //sizeof(val[0]));
                 FileWrite(bfile, val[1] , 3); //sizeof(val[1]));
               end;
          end;
        end;
      end;
      FileClose(bfile);
     end;
    end;
end;

end.

