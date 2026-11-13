#!/usr/bin/env python3
"""Generate an NSIS installer with an explicit, non-destructive uninstall file list."""
import argparse,pathlib,subprocess
p=argparse.ArgumentParser();p.add_argument('package',type=pathlib.Path);p.add_argument('output',type=pathlib.Path);p.add_argument('--makensis',default='makensis');a=p.parse_args()
root=pathlib.Path(__file__).resolve().parents[1];package=a.package.resolve();output=a.output.resolve()
def quote(s):return str(s).replace('$','$$').replace('"','$\\"')
lines=[
'Unicode true','!include "MUI2.nsh"','!include "x64.nsh"','!include "LogicLib.nsh"',
'Name "Perplex Movie Studio"',f'OutFile "{quote(output)}"','RequestExecutionLevel user',
'InstallDir "$LOCALAPPDATA\\Programs\\PerplexMovieStudio"','SetCompressor /SOLID lzma',
f'Icon "{quote(root/"assets/perplex.ico")}"',
'VIProductVersion "0.1.1.2"','VIAddVersionKey /LANG=1033 "ProductName" "Perplex Movie Studio"',
'VIAddVersionKey /LANG=1033 "FileDescription" "Perplex Movie Studio development installer"',
'VIAddVersionKey /LANG=1033 "FileVersion" "0.1.1-dev-r2"',
'VIAddVersionKey /LANG=1033 "LegalCopyright" "Copyright 2026 Michael Lappenbusch and contributors"',
'!define MUI_ABORTWARNING',f'!insertmacro MUI_PAGE_LICENSE "{quote(root/"LICENSE")}"',
'!insertmacro MUI_PAGE_DIRECTORY','!insertmacro MUI_PAGE_INSTFILES',
'!insertmacro MUI_UNPAGE_CONFIRM','!insertmacro MUI_UNPAGE_INSTFILES',
'!insertmacro MUI_LANGUAGE "English"','!insertmacro MUI_LANGUAGE "German"',
'Function .onInit','  ${IfNot} ${RunningX64}','    MessageBox MB_ICONSTOP "Perplex Movie Studio requires Windows x64."','    Abort','  ${EndIf}',
'  !insertmacro MUI_LANGDLL_DISPLAY','FunctionEnd','Section "Perplex Movie Studio"',
'SetShellVarContext current', 'Delete "$INSTDIR\\docs\\RESEARCH.md"']
files=sorted(p for p in package.rglob('*') if p.is_file())
parent=None
for file in files:
 rel=file.relative_to(package);dest=str(rel.parent).replace('/','\\')
 if dest!=parent:lines.append('SetOutPath "$INSTDIR'+('' if dest=='.' else '\\'+quote(dest))+'"');parent=dest
 lines.append(f'File "{quote(file)}"')
lines += ['CreateDirectory "$SMPROGRAMS\\Perplex Movie Studio"','CreateShortCut "$SMPROGRAMS\\Perplex Movie Studio\\Perplex Movie Studio.lnk" "$INSTDIR\\PerplexMovieStudio.exe"','CreateShortCut "$SMPROGRAMS\\Perplex Movie Studio\\Uninstall.lnk" "$INSTDIR\\Uninstall.exe"',
'WriteUninstaller "$INSTDIR\\Uninstall.exe"',
'WriteRegStr HKCU "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\PerplexMovieStudio" "DisplayName" "Perplex Movie Studio 0.1.1-dev-r2"',
'WriteRegStr HKCU "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\PerplexMovieStudio" "UninstallString" \'$\\"$INSTDIR\\Uninstall.exe$\\"\'',
'WriteRegStr HKCU "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\PerplexMovieStudio" "Publisher" "Michael Lappenbusch"',
'WriteRegStr HKCU "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\PerplexMovieStudio" "DisplayVersion" "0.1.1-dev-r2"',
'SectionEnd','Section "Uninstall"','SetShellVarContext current']
for file in files:lines.append('Delete "$INSTDIR\\'+quote(str(file.relative_to(package)).replace('/','\\'))+'"')
for directory in sorted((p for p in package.rglob('*') if p.is_dir()),key=lambda p:len(p.parts),reverse=True):lines.append('RMDir "$INSTDIR\\'+quote(str(directory.relative_to(package)).replace('/','\\'))+'"')
lines += ['Delete "$INSTDIR\\Uninstall.exe"','RMDir "$INSTDIR"','Delete "$SMPROGRAMS\\Perplex Movie Studio\\Perplex Movie Studio.lnk"','Delete "$SMPROGRAMS\\Perplex Movie Studio\\Uninstall.lnk"','RMDir "$SMPROGRAMS\\Perplex Movie Studio"','DeleteRegKey HKCU "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\PerplexMovieStudio"','SectionEnd']
script=output.with_suffix('.nsi');script.write_text('\n'.join(lines)+'\n',encoding='utf-8');subprocess.run([a.makensis,'-V2',str(script)],check=True)
print(output)
