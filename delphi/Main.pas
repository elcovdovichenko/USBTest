unit Main;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls, IOCTL, Grids, ExtCtrls, ComCtrls, DevIEnum;

type
  TForm1 = class(TForm)
    bnDeviceOpen: TButton;
    Label1: TLabel;
    bnDeviceClose: TButton;
    memoInfo: TMemo;
    bnPipe2: TButton;
    cbInt: TCheckBox;
    sgInterrupt: TStringGrid;
    cbCommand: TComboBox;
    lbCommand: TLabel;
    Timer1: TTimer;
    editInterval: TEdit;
    UpDown1: TUpDown;
    cbInterface: TComboBox;
    cbCycled: TCheckBox;
    eSymbol: TEdit;
    cbSpeed: TComboBox;
    Timer2: TTimer;
    cbAll: TCheckBox;
    procedure bnDeviceOpenClick(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure bnDeviceCloseClick(Sender: TObject);
    procedure bnPipe1Click(Sender: TObject);
    procedure bnPipe2Click(Sender: TObject);
    procedure cbIntClick(Sender: TObject);
    procedure FormActivate(Sender: TObject);
    procedure cbCommandKeyDown(Sender: TObject; var Key: Word;
      Shift: TShiftState);
    procedure cbCommandDblClick(Sender: TObject);
    procedure Timer1Timer(Sender: TObject);
    procedure memoInfoChange(Sender: TObject);
    procedure editIntervalChange(Sender: TObject);
    procedure bnTimerClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure cbCycledClick(Sender: TObject);
    procedure Timer2Timer(Sender: TObject);
    procedure sgInterruptDblClick(Sender: TObject);
    procedure cbAllClick(Sender: TObject);
   private
    { Private declarations }
    Interfaces: TDeviceInterfaceClassEnumerator;
    procedure ReadInterruptPipe;
    procedure WriteBulkPipe;
    procedure CommandsAdd;
    procedure CommandsLoad;
  public
    { Public declarations }
  end;

var
  Form1: TForm1;

implementation

{$R *.DFM}

const { interface GUID }
  USBProbeIID: TGUID = '{F1CDE16F-68E2-4be2-90E1-B1D908EF966A}';

var
  dwErrorCode : Integer;

procedure TForm1.bnDeviceOpenClick(Sender: TObject);
var h1pipe, h2pipe, h3pipe, command, count : DWord;
begin

   if fIsLoaded
   then begin
        memoInfo.Lines.Add('Device is already loaded');
        Exit;
        end;

   bnDeviceOpen.Enabled:=False;
   bnDeviceClose.Enabled:=True;
   cbCycled.Enabled:=True;
  
   dwErrorCode:=DeviceOpen(Interfaces[0]);
   if dwErrorCode = 0
   then begin
        memoInfo.Lines.Add('Device is ready');

        dwErrorCode:=DIOCFunc1(h1pipe, h2pipe, h3pipe);
        if dwErrorCode = 0
        then begin
             memoInfo.Lines.Add('h1pipe=: '+IntToHex(h1pipe,8));
             memoInfo.Lines.Add('h2pipe=: '+IntToHex(h2pipe,8));

             count:=$FFFFFFFF;
             command:=$ff01;
             dwErrorCode:=DIOCFunc2(command, 2, count);
             if dwErrorCode <> 0
             then memoInfo.Lines.Add('Pipe01 Error code: '+IntToStr(dwErrorCode));
             end
        else memoInfo.Lines.Add('DIOCFunc1 Error code: '+IntToStr(dwErrorCode));

        end
   else memoInfo.Lines.Add('Unable to open device,Error code: '+IntToStr(dwErrorCode));

end;

procedure TForm1.FormClose(Sender: TObject; var Action: TCloseAction);
var command, count: DWord;
begin
  command:=$ff00;
  DIOCFunc2(command, 2, count);
  Interfaces.Free;
  DeviceClose;
end;

procedure TForm1.bnDeviceCloseClick(Sender: TObject);
var command, count: DWord;
begin
  bnDeviceOpen.Enabled:=True;
  bnDeviceClose.Enabled:=False;
  Timer1.Enabled:=False;
  Timer2.Enabled:=False;
  cbInt.Checked:=False;
  bnPipe2.Visible:= True;
  editInterval.Visible:= False;
  UpDown1.Visible:= False;
  cbCycled.Enabled:=False;
  cbCycled.Checked:=False;
  eSymbol.Enabled:= False;
  cbSpeed.Enabled:= True;

  count:=$FFFFFFFF;
  command:=$ff00;
  dwErrorCode:=DIOCFunc2(command, 2, count);
  if dwErrorCode <> 0
  then memoInfo.Lines.Add('Pipe01 Error code: '+IntToStr(dwErrorCode));

  if DeviceClose
  then memoInfo.Lines.Add('Device is closed')
  else memoInfo.Lines.Add('Device yet was not open');
end;

procedure TForm1.bnPipe1Click(Sender: TObject);
begin
WriteBulkPipe;
end;

procedure TForm1.WriteBulkPipe;
var command, count : DWord;
begin

   if not fIsLoaded
   then begin
        memoInfo.Lines.Add('Device yet was not open');
        Exit;
        end;

   command:=StrToHexAddr(Copy(cbCommand.Text,1,4));
   if command = $FFFFFFFF
   then begin
        cbCommand.Text:='';
        memoInfo.Lines.Add('Ошибка в команде!');
        exit;
        end;

   CommandsAdd;

   count:=$FFFFFFFF;
   dwErrorCode:=DIOCFunc2(command, 2, count);
   if dwErrorCode = 0
   then memoInfo.Lines.Add('Pipe01 Transfered: '+IntToStr(count))
   else begin
        memoInfo.Lines.Add('Pipe01 Error code: '+IntToStr(dwErrorCode));
        DeviceClose;
        end;

end;

procedure TForm1.bnPipe2Click(Sender: TObject);
begin
ReadInterruptPipe;
end;

procedure TForm1.ReadInterruptPipe;
var data : array[1..16] of Word;
    i, k : Integer;
    count, len : DWord;
    b : Byte;
begin

   if not fIsLoaded
   then begin
        memoInfo.Lines.Add('Device yet was not open');
        Exit;
        end;

   for i:=1 to 16 do data[i]:=0;
   len:=32; count:=$FFFFFFFF;
   dwErrorCode:=DIOCFunc3(data, len, count);
   if dwErrorCode = 0
   then begin
        memoInfo.Lines.Add('Pipe81 Transfered: '+IntToStr(count));
        for i:=1 to 16
        do begin
           b:=Hi(data[i]);
           for k:=7 downto 1
           do begin
              if k = 4
              then begin
                   sgInterrupt.Cells[k,i]:=IntToHex(b mod 2,1);
                   b:=b div 2;
                   sgInterrupt.Cells[k,i]:='  '+IntToHex(b mod 2,1)+sgInterrupt.Cells[k,i];
                   end
              else sgInterrupt.Cells[k,i]:='  '+IntToHex(b mod 2,1);
              b:=b div 2;
              end;
           sgInterrupt.Cells[8,i]:='  '+IntToHex(Lo(data[i]),2);
           end;
        end
   else begin
        memoInfo.Lines.Add('Pipe81 Error code: '+IntToStr(dwErrorCode));
        DeviceClose;
        end;

end;

procedure TForm1.cbIntClick(Sender: TObject);
var T:Integer;
begin
   bnPipe2.Visible:= not cbInt.Checked;
   editInterval.Visible:= cbInt.Checked;
   UpDown1.Visible:= cbInt.Checked;
   T:= StrToInt(editInterval.Text);
   if T > 0 then Timer1.Interval:=T;
   Timer1.Enabled:= cbInt.Checked;
end;

procedure TForm1.FormActivate(Sender: TObject);
var
  i, n : Integer;
begin

  with sgInterrupt do
    begin
     Cells[0,0] := '';
     Cells[1,0] := 'LnC';
     Cells[2,0] := 'TxR';
     Cells[3,0] := 'TxC';
     Cells[4,0] := 'LnS';
     Cells[5,0] := 'OvE';
     Cells[6,0] := 'StE';
     Cells[7,0] := 'RxR';
     Cells[8,0] := 'DATA';

     for i := 1 to 16 do
       Cells[0,i] := '-'+IntToStr(i-1);
    end;

  CommandsLoad;

  i:=Interfaces.Count;
  if i > 0
  then begin
       cbInterface.Text:=PChar(Interfaces[0]);
       for n:=0 to i-1
       do cbInterface.Items.Add(PChar(Interfaces[n]));
       end;

end;

procedure TForm1.cbCommandKeyDown(Sender: TObject; var Key: Word;
  Shift: TShiftState);
begin
   Case Key Of
     VK_Return: WriteBulkPipe;
   end;
end;

procedure TForm1.CommandsAdd;
var F:TextFile; i:Integer;
begin
   for i:=0 to cbCommand.Items.Count-1
   do if cbCommand.Items[i]=cbCommand.Text
      then Exit;

   cbCommand.Items.Add(cbCommand.Text);
   {$I-}
   AssignFile(F,'control.txt');
   Append(F);
   if IOResult = 0 then Writeln(F,cbCommand.Text);
   CloseFile(F);
   {$I+}
end;

procedure TForm1.CommandsLoad;
var F:TextFile; S:String;
begin
   {$I-}
   AssignFile(F,'control.txt');
   Reset(F);
   if IOResult = 0
   then begin
        repeat
          Readln(F,S); if IOResult <> 0 then Break;
          cbCommand.Items.Add(S);
        until EoF(F);
        CloseFile(F);
        end
   else begin
        Rewrite(F);
        if IOResult=0 then CloseFile(F);
        end;
   {$I+}
end;

procedure TForm1.cbCommandDblClick(Sender: TObject);
begin
   WriteBulkPipe;
end;

procedure TForm1.Timer1Timer(Sender: TObject);
begin
   if fIsLoaded then ReadInterruptPipe;
end;

procedure TForm1.memoInfoChange(Sender: TObject);
begin
   if memoInfo.Lines.Count > 1000
   then memoInfo.Lines.Clear;
end;

procedure TForm1.editIntervalChange(Sender: TObject);
var T : Integer;
begin
   T:= StrToInt(editInterval.Text);
   if T > 0 then Timer1.Interval:=T;
end;

procedure TForm1.bnTimerClick(Sender: TObject);
var command, count : DWord;
begin

   if not fIsLoaded
   then begin
        memoInfo.Lines.Add('Device yet was not open');
        Exit;
        end;

   dwErrorCode:=DIOCFunc4(command, 2, count);
   if dwErrorCode = 0
   then memoInfo.Lines.Add('Timer: '+IntToStr(count))
   else memoInfo.Lines.Add('Timer Error code: '+IntToStr(dwErrorCode));

end;

procedure TForm1.FormCreate(Sender: TObject);
var i, n : integer;
begin

   Interfaces:=TDeviceInterfaceClassEnumerator.Create(@USBProbeIID);
   if Interfaces.Handle = INVALID_HANDLE_VALUE
   then begin
        memoInfo.Lines.Add('Device Interface Class is not exist');
        Interfaces.Free;
        Exit;
        end;

   i:=Interfaces.Count;
   memoInfo.Lines.Add(IntToStr(i)+' device interface(s) is found');
   if i > 0
   then for n:=0 to i-1 do memoInfo.Lines.Add(PChar(Interfaces[n]))
   else Exit;

end;

procedure TForm1.cbCycledClick(Sender: TObject);
var i, command, count: DWord;
begin
  eSymbol.Enabled:= cbCycled.Checked;
  cbSpeed.Enabled:= not cbCycled.Checked;
  if cbCycled.Checked
  then begin
       case StrToInt(cbSpeed.Text) of
         50:  begin
              Timer2.Interval:=160;
              command:=$20E1;
              end;
         100: begin
              Timer2.Interval:=80;
              command:=$20E9;
              end;
         200: begin
              Timer2.Interval:=40;
              command:=$20F1;
              end;
         end;

       for i:= 0 to 15
       do begin
          count:=$FFFFFFFF;
          dwErrorCode:=DIOCFunc2(command, 2, count);
          if dwErrorCode <> 0
          then begin
               memoInfo.Lines.Add('Pipe01 Error code: '+IntToStr(dwErrorCode));
               break;
               end;
          command:=command+$100;
          end;

       end;
  Timer2.Enabled:= cbCycled.Checked;
end;

procedure TForm1.Timer2Timer(Sender: TObject);
var i, command, count: DWord;
begin

   command:=StrToHexAddr(Copy(eSymbol.Text,1,2));
   if command = $FFFFFFFF
   then begin
        command:=0;
        memoInfo.Lines.Add('Ошибка в команде!');
        end;

   for i:= 0 to 15
   do begin
      if sgInterrupt.Cells[0,i+1][1] = '+'
      then begin
           count:=$FFFFFFFF;
           dwErrorCode:=DIOCFunc2(command, 2, count);
           if dwErrorCode <> 0
           then begin
                memoInfo.Lines.Add('Pipe01 Error code: '+IntToStr(dwErrorCode));
                break;
                end;
           end;     
      command:=command+$100;
      end;
end;

procedure TForm1.sgInterruptDblClick(Sender: TObject);
begin
  if not cbAll.Checked
  then with sgInterrupt do
       if Cells[0,Row][1] = '+'
       then Cells[0,Row]:='-'+IntToStr(Row-1)
       else Cells[0,Row]:='+'+IntToStr(Row-1);
end;

procedure TForm1.cbAllClick(Sender: TObject);
var i: integer;
begin
  if cbAll.Checked
  then for i := 1 to 16 do sgInterrupt.Cells[0,i]:='+'+IntToStr(i-1);
end;


end.
