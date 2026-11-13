#!/usr/bin/env python3
"""Assemble a self-contained, replaceable-DLL Windows package and verify PE imports.
Requires Python 3.12 and pefile==2024.8.26. Does not download anything.
"""
import argparse, hashlib, json, pathlib, shutil
import pefile

SYSTEM_DLLS = set('''advapi32 authz avicap32 avrt evr shcore bcrypt cabinet cfgmgr32 comctl32 comdlg32
crypt32 d2d1 d3d11 d3d12 d3d9 d3dcompiler_47 dbghelp dnsapi dwmapi dwrite dxgi dxva2
gdi32 imm32 iphlpapi kernel32 ksuser mf mfplat mfreadwrite mfuuid mmdevapi mpr msvcrt
msvcp_win ncrypt netapi32 normaliz ntdll ole32 oleaut32 opengl32 powrprof propsys
psapi rpcrt4 secur32 setupapi shell32 shlwapi user32 userenv usp10 ucrtbase uxtheme
version winhttp wininet winmm winspool wintrust wlanapi ws2_32 wtsapi32 dxcore
'''.split())

def imports(path):
    p = pefile.PE(str(path), fast_load=True)
    p.parse_data_directories(directories=[1,13])
    if p.FILE_HEADER.Machine != 0x8664:
        raise RuntimeError(f'Not a Windows x64 binary: {path.name}')
    result=[]
    for key in ('DIRECTORY_ENTRY_IMPORT','DIRECTORY_ENTRY_DELAY_IMPORT'):
        result += [x.dll.decode('ascii').lower() for x in getattr(p,key,[])]
    p.close()
    return sorted(set(result))

def verify_symbols(root):
    binaries=sorted(p for p in root.rglob('*') if p.suffix.lower() in ('.exe','.dll'))
    shipped={p.name.lower():p for p in binaries if p.suffix.lower()=='.dll'}
    exports={}; missing=[]; checked=0
    for path in binaries:
        pe=pefile.PE(str(path),fast_load=True)
        pe.parse_data_directories(directories=[1,13])
        for key in ('DIRECTORY_ENTRY_IMPORT','DIRECTORY_ENTRY_DELAY_IMPORT'):
            for entry in getattr(pe,key,[]):
                name=entry.dll.decode('ascii').lower()
                if name not in shipped:continue
                if name not in exports:
                    dep=pefile.PE(str(shipped[name]),fast_load=True,max_symbol_exports=100000)
                    dep.parse_data_directories(directories=[0])
                    symbols=getattr(getattr(dep,'DIRECTORY_ENTRY_EXPORT',None),'symbols',[])
                    exports[name]=({s.name for s in symbols if s.name},{s.ordinal for s in symbols})
                    dep.close()
                names,ordinals=exports[name]
                for symbol in entry.imports:
                    checked+=1
                    if (symbol.name not in names if symbol.name else symbol.ordinal not in ordinals):
                        missing.append(f'{path.name}: {name}: {symbol.name or symbol.ordinal}')
        pe.close()
    if missing:raise RuntimeError('Missing imported symbols: '+str(missing))
    return {'checked':checked,'missing':missing}

def main():
    ap=argparse.ArgumentParser()
    for key in ('build','qt','ffmpeg','mingw','output'):ap.add_argument('--'+key,required=True,type=pathlib.Path)
    a=ap.parse_args();root=pathlib.Path(__file__).resolve().parents[1];out=a.output.resolve()
    if out.exists(): raise RuntimeError('Use a fresh package directory; existing output is never erased.')
    out.mkdir(parents=True)
    shutil.copy2(a.build/'PerplexMovieStudio.exe',out)
    for tool in ('ffmpeg.exe','ffprobe.exe'):shutil.copy2(a.ffmpeg/tool,out)
    available={p.name.lower():p for p in a.qt.joinpath('bin').glob('*.dll')}
    available.update({p.name.lower():p for p in a.mingw.rglob('*.dll')})
    # Always prefer our reviewed LGPL FFmpeg build over the Qt SDK's bundled FFmpeg.
    available.update({p.name.lower():p for p in a.ffmpeg.glob('*.dll')})
    pending=list(out.glob('*.exe'))
    for plugin in ('platforms/qwindows.dll','multimedia/ffmpegmediaplugin.dll','multimedia/windowsmediaplugin.dll','imageformats/qjpeg.dll'):
        src=a.qt/'plugins'/plugin;dst=out/'plugins'/plugin;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(src,dst);pending.append(dst)
    seen=set();report={}
    while pending:
        binary=pending.pop();key=binary.name.lower()
        if key in seen:continue
        seen.add(key);report[str(binary.relative_to(out))]=imports(binary)
        for name in report[str(binary.relative_to(out))]:
            if name.startswith(('api-ms-win-','ext-ms-win-')) or pathlib.Path(name).stem in SYSTEM_DLLS:continue
            if name not in available:raise RuntimeError(f'Missing dependency {name} required by {binary.name}')
            dst=out/available[name].name
            if not dst.exists():shutil.copy2(available[name],dst)
            pending.append(dst)
    (out/'translations').mkdir();shutil.copy2(a.qt/'translations/qtbase_de.qm',out/'translations')
    (out/'qt.conf').write_text('[Paths]\nPrefix=.\nPlugins=plugins\nTranslations=translations\n',encoding='utf-8')
    for file in ('LICENSE','README.md','THIRD_PARTY_NOTICES.md','CHANGELOG.md'):shutil.copy2(root/file,out)
    shutil.copytree(root/'docs',out/'docs',ignore=shutil.ignore_patterns('specification'))
    shutil.copytree(root/'third-party/licenses',out/'licenses')
    if (root/'examples').exists():shutil.copytree(root/'examples',out/'examples')
    (out/'dependency-check.json').write_text(json.dumps({'architecture':'x86_64','missing':[],'imports':report,'symbols':verify_symbols(out)},indent=2),encoding='utf-8')
    (out/'SHA256SUMS.txt').write_text('\n'.join(f'{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.relative_to(out).as_posix()}' for p in sorted(out.rglob('*')) if p.is_file())+'\n')
    print(json.dumps({'directory':str(out),'verified_pe_files':len(seen),'files':sum(p.is_file() for p in out.rglob('*'))}))

if __name__=='__main__':main()
