Set objShell = CreateObject("Shell.Application")
Set fso = CreateObject("Scripting.FileSystemObject")
scriptDir = fso.GetParentFolderName(WScript.ScriptFullName)
ps1File = scriptDir & "\AddWifiStaticRoute.ps1"
objShell.ShellExecute "powershell.exe", "-NoProfile -NoExit -ExecutionPolicy Bypass -File """ & ps1File & """", scriptDir, "runas", 1
