using System;
using System.IO;
using System.IO.Compression;
using System.Reflection;
using System.Drawing;
using System.Windows.Forms;
using System.Collections.Generic;
using System.Text;

internal static class Installer {
#if DOS
 const string Edition="DOS";
 const string Folder="Ultima1K(DOS)";
#else
 const string Edition="Windows";
 const string Folder="Ultima1K(Win)";
#endif
 static readonly string[] Required={"ULTIMA.EXE","GEN.EXE","OUT.EXE","MONDAIN.EXE","SPACE.EXE","MAP.BIN","EGATILES.BIN","EGATOWN.BIN","TCD.BIN"};
 static readonly string[] Data={"CASTLE.16","CASTLE.4","CGAFIGHT.BIN","CGAMOND.BIN","CGASPACE.BIN","CGATILES.BIN","CGATOWN.BIN","EGAFIGHT.BIN","EGAMOND.BIN","EGASPACE.BIN","EGATILES.BIN","EGATOWN.BIN","MAP.BIN","NIF.BIN","T1KFIGHT.BIN","T1KMOND.BIN","T1KSPACE.BIN","T1KTILES.BIN","T1KTOWN.BIN","TCD.BIN","ULTIMA.EXE","GEN.EXE","OUT.EXE","MONDAIN.EXE","SPACE.EXE"};
 [STAThread] static void Main(){
  Application.EnableVisualStyles();
  Application.SetCompatibleTextRenderingDefault(false);
  Form form=new Form{Text="울티마 I 한국어판 ("+Edition+") 설치 프로그램",ClientSize=new Size(650,300),StartPosition=FormStartPosition.CenterScreen,FormBorderStyle=FormBorderStyle.FixedDialog,MaximizeBox=false};
  Label intro=new Label{Text="정품 DOS Ultima I의 폴더를 지정하세요. 원본 파일과 저장 파일은 변경하지 않습니다.",Location=new Point(18,16),Size=new Size(615,42)};
  Label dl=new Label{Text="한국어판 설치 위치 (아래에 "+Folder+" 폴더 생성)",Location=new Point(18,64),AutoSize=true};
  TextBox dest=new TextBox{Location=new Point(18,87),Size=new Size(505,23),Text=Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments)};
  Button db=new Button{Text="찾아보기",Location=new Point(533,85),Size=new Size(98,27)};
  Label sl=new Label{Text="정품 DOS Ultima I 원본 폴더 (ULTIMA.EXE가 있는 폴더)",Location=new Point(18,135),AutoSize=true};
  TextBox src=new TextBox{Location=new Point(18,158),Size=new Size(505,23)};
  Button sb=new Button{Text="찾아보기",Location=new Point(533,156),Size=new Size(98,27)};
  Label status=new Label{Text="저장 파일(SAVEGAME)은 유지됩니다.",Location=new Point(18,206),Size=new Size(610,37)};
  Button cancel=new Button{Text="닫기",Location=new Point(411,252),Size=new Size(105,30)};
  Button install=new Button{Text="설치",Location=new Point(526,252),Size=new Size(105,30)};
  db.Click+=(s,e)=>Choose(dest); sb.Click+=(s,e)=>Choose(src);
  cancel.Click+=(s,e)=>form.Close();form.CancelButton=cancel;
  install.Click+=(s,e)=>{
   install.Enabled=false;
   try{
    string target=Install(src.Text,dest.Text);
    status.Text="설치 완료: "+target;
    MessageBox.Show(form,"설치가 완료되었습니다.\n"+target,"울티마 I 한국어판",MessageBoxButtons.OK,MessageBoxIcon.Information);
    form.Close();
   }catch(Exception ex){
    status.Text="설치 실패: "+ex.Message;
    MessageBox.Show(form,ex.Message,"설치 오류",MessageBoxButtons.OK,MessageBoxIcon.Error);
   }finally{install.Enabled=true;}
  };
  form.Controls.AddRange(new Control[]{intro,dl,dest,db,sl,src,sb,status,cancel,install});
  Application.Run(form);
 }
 static void Choose(TextBox box){using(FolderBrowserDialog dlg=new FolderBrowserDialog()){if(dlg.ShowDialog()==DialogResult.OK)box.Text=dlg.SelectedPath;}}
 static string Norm(string p){return Path.GetFullPath(p).TrimEnd(Path.DirectorySeparatorChar,Path.AltDirectorySeparatorChar);}
 static bool Under(string p,string root){return p.Equals(root,StringComparison.OrdinalIgnoreCase)||p.StartsWith(root+Path.DirectorySeparatorChar,StringComparison.OrdinalIgnoreCase);}
 static string Install(string originalInput,string destInput){
  if(!Directory.Exists(originalInput))throw new IOException("정품 DOS 원본 폴더를 지정하세요.");
  if(String.IsNullOrWhiteSpace(destInput))throw new IOException("설치 위치를 지정하세요.");
  string original=Norm(originalInput),target=Norm(Path.Combine(Norm(destInput),Folder));
  if(Under(target,original)||Under(original,target))throw new IOException("설치 폴더와 원본 폴더가 겹칠 수 없습니다.");
  List<string> missing=new List<string>();
  foreach(string f in Required)if(!File.Exists(Path.Combine(original,f)))missing.Add(f);
  if(missing.Count>0)throw new IOException("원본 파일 누락: "+String.Join(", ",missing.ToArray()));
  Directory.CreateDirectory(target);
  Directory.CreateDirectory(Path.Combine(target,"SAVEGAME"));
  using(Stream stream=Assembly.GetExecutingAssembly().GetManifestResourceStream("payload.zip")){
   if(stream==null)throw new IOException("내장 게임 페이로드를 찾을 수 없습니다.");
   using(ZipArchive zip=new ZipArchive(stream,ZipArchiveMode.Read)){
    foreach(ZipArchiveEntry entry in zip.Entries){
     if(String.IsNullOrEmpty(entry.Name))continue;
     string relative=entry.FullName.Replace('/',Path.DirectorySeparatorChar);
     if(relative.Equals("SAVEGAME",StringComparison.OrdinalIgnoreCase)||relative.StartsWith("SAVEGAME"+Path.DirectorySeparatorChar,StringComparison.OrdinalIgnoreCase))continue;
     string output=Norm(Path.Combine(target,relative));
     if(!Under(output,target)||output==target)throw new IOException("잘못된 파일 경로: "+entry.FullName);
     Directory.CreateDirectory(Path.GetDirectoryName(output));
     using(Stream inp=entry.Open())using(FileStream outp=new FileStream(output,FileMode.Create,FileAccess.Write,FileShare.None))inp.CopyTo(outp);
    }
   }
  }
#if DOS
  string dataDir=Path.Combine(target,"GAMEDATA");
  Directory.CreateDirectory(dataDir);
  foreach(string filename in Data){
   string from=Path.Combine(original,filename);
   if(File.Exists(from))File.Copy(from,Path.Combine(dataDir,filename),true);
  }
#endif
  File.WriteAllText(Path.Combine(target,"original_path.txt"),original,new UTF8Encoding(false));
  return target;
 }
}
