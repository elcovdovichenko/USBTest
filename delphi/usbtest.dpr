program usbtest;

uses
  Forms,
  Main in 'Main.pas' {Form1},
  IOCTL in 'Ioctl.pas';

{$R *.RES}

begin
  Application.Initialize;
  Application.CreateForm(TForm1, Form1);
  Application.Run;
end.
