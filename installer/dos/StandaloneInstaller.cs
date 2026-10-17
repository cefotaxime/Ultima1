using System;
using System.IO;
using System.Reflection;
using System.Diagnostics;
using System.Windows.Forms;
class InstallerEntry {
 [STAThread] static void Main() {
  string temp=Path.Combine(Path.GetTempPath(),"Ultima1KR_Installer_"+Guid.NewGuid().ToString("N"));
  try {
   Directory.CreateDirectory(temp);
   foreach(string name in new[]{"setup.ps1","payload.zip"}) {
    using(Stream input=Assembly.GetExecutingAssembly().GetManifestResourceStream(name)) {
     if(input==null)throw new Exception("Missing embedded resource: "+name);
     using(FileStream output=File.Create(Path.Combine(temp,name)))input.CopyTo(output);
    }
   }
   Process p=new Process();
   p.StartInfo.FileName="powershell.exe";
   p.StartInfo.Arguments="-NoProfile -ExecutionPolicy Bypass -STA -File \""+Path.Combine(temp,"setup.ps1")+"\"";
   p.StartInfo.UseShellExecute=false;
   p.Start();p.WaitForExit();
  }catch(Exception ex){MessageBox.Show(ex.Message,"Ultima I Korean DOS Setup");}
  finally{try{Directory.Delete(temp,true);}catch{}}
 }
}
