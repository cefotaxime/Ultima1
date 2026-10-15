Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
[System.Windows.Forms.Application]::EnableVisualStyles()
$form=New-Object Windows.Forms.Form
$form.Text='울티마 1 한국어판 (Windows) 설치 프로그램'
$form.Size=New-Object Drawing.Size(660,335)
$form.StartPosition='CenterScreen'
$form.FormBorderStyle='FixedDialog'
$form.MaximizeBox=$false
$intro=New-Object Windows.Forms.Label
$intro.Location=New-Object Drawing.Point(20,18)
$intro.Size=New-Object Drawing.Size(610,43)
$intro.Text="원본 DOS Ultima I의 설치 폴더를 선택하세요. 원본 파일은 복사하거나 변경하지 않습니다."
$form.Controls.Add($intro)
$lbl1=New-Object Windows.Forms.Label
$lbl1.Location=New-Object Drawing.Point(20,70)
$lbl1.Size=New-Object Drawing.Size(590,20)
$lbl1.Text='한국어판 설치 위치'
$form.Controls.Add($lbl1)
$dest=New-Object Windows.Forms.TextBox
$dest.Location=New-Object Drawing.Point(20,93)
$dest.Size=New-Object Drawing.Size(510,25)
$dest.Text=[Environment]::GetFolderPath('MyDocuments')
$form.Controls.Add($dest)
$b1=New-Object Windows.Forms.Button
$b1.Text='찾아보기'
$b1.Location=New-Object Drawing.Point(537,91)
$b1.Size=New-Object Drawing.Size(88,27)
$b1.Add_Click({
 $dlg=New-Object Windows.Forms.FolderBrowserDialog
 $dlg.Description='한국어판 설치 위치 선택'
 if($dlg.ShowDialog() -eq 'OK') {$dest.Text=$dlg.SelectedPath}
})
$form.Controls.Add($b1)
$lbl2=New-Object Windows.Forms.Label
$lbl2.Location=New-Object Drawing.Point(20,139)
$lbl2.Size=New-Object Drawing.Size(590,20)
$lbl2.Text='정품 DOS Ultima I 설치 위치 (ULTIMA.EXE와 데이터 파일이 있는 폴더)'
$form.Controls.Add($lbl2)
$src=New-Object Windows.Forms.TextBox
$src.Location=New-Object Drawing.Point(20,162)
$src.Size=New-Object Drawing.Size(510,25)
$form.Controls.Add($src)
$b2=New-Object Windows.Forms.Button
$b2.Text='찾아보기'
$b2.Location=New-Object Drawing.Point(537,160)
$b2.Size=New-Object Drawing.Size(88,27)
$b2.Add_Click({
 $dlg=New-Object Windows.Forms.FolderBrowserDialog
 $dlg.Description='DOS Ultima I 원본 폴더 선택'
 if($dlg.ShowDialog() -eq 'OK') {$src.Text=$dlg.SelectedPath}
})
$form.Controls.Add($b2)
$status=New-Object Windows.Forms.Label
$status.Location=New-Object Drawing.Point(20,204)
$status.Size=New-Object Drawing.Size(610,34)
$status.Text='Windows 한국어판 세이브는 SAVEGAME 폴더에 별도로 저장됩니다.'
$form.Controls.Add($status)
$install=New-Object Windows.Forms.Button
$install.Text='설치'
$install.Location=New-Object Drawing.Point(514,246)
$install.Size=New-Object Drawing.Size(110,33)
$install.Add_Click({
 try {
  $required=@('ULTIMA.EXE','GEN.EXE','OUT.EXE','MONDAIN.EXE','SPACE.EXE','MAP.BIN','EGATILES.BIN','EGATOWN.BIN','TCD.BIN')
  $missing=@($required | Where-Object {-not (Test-Path -LiteralPath (Join-Path $src.Text $_) -PathType Leaf)})
  if($missing.Count -gt 0){throw ('원본 파일 누락: '+($missing -join ', '))}
  if(-not $dest.Text.Trim()){throw '설치 경로를 선택하세요.'}
  $target=[IO.Path]::Combine([IO.Path]::GetFullPath($dest.Text),'Ultima1K(Win)')
  if($target.TrimEnd('\') -eq [IO.Path]::GetFullPath($src.Text).TrimEnd('\')){throw '원본 DOS 폴더와 한국어판 설치 폴더는 달라야 합니다.'}
  [IO.Directory]::CreateDirectory($target)|Out-Null
  $save=[IO.Path]::Combine($target,'SAVEGAME')
  [IO.Directory]::CreateDirectory($save)|Out-Null
  Add-Type -AssemblyName System.IO.Compression.FileSystem
  $archive=[IO.Compression.ZipFile]::OpenRead((Join-Path $PSScriptRoot 'payload.zip'))
  try {
   foreach($entry in $archive.Entries){
    if(-not $entry.Name){continue}
    $out=[IO.Path]::Combine($target,$entry.FullName.Replace('/','\'))
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($out))|Out-Null
    [IO.Compression.ZipFileExtensions]::ExtractToFile($entry,$out,$true)
   }
  } finally {$archive.Dispose()}
  [IO.File]::WriteAllText([IO.Path]::Combine($target,'original_path.txt'),[IO.Path]::GetFullPath($src.Text),[Text.UTF8Encoding]::new($false))
  $status.Text='설치 완료: '+$target
  [Windows.Forms.MessageBox]::Show('설치가 완료됐습니다.'+[Environment]::NewLine+$target,'울티마 1 한국어판 (Windows)')|Out-Null
  Start-Process explorer.exe -ArgumentList ('"'+$target+'"')
  $form.DialogResult=[System.Windows.Forms.DialogResult]::OK
  $form.Close()
 }catch{
  $status.Text='설치 실패: '+$_.Exception.Message
  [Windows.Forms.MessageBox]::Show($_.Exception.Message,'설치 오류','OK','Error')|Out-Null
 }
})
$form.Controls.Add($install)
$close=New-Object Windows.Forms.Button
$close.Text='닫기'
$close.Location=New-Object Drawing.Point(395,246)
$close.Size=New-Object Drawing.Size(110,33)
$close.Add_Click({$form.Close()})
$form.Controls.Add($close)
$form.CancelButton=$close
[void]$form.ShowDialog()