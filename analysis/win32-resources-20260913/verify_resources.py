from pathlib import Path
import ctypes as c,hashlib,json,pefile

def resources(path):
 p=pefile.PE(str(path));out={}
 for t in p.DIRECTORY_ENTRY_RESOURCE.entries:
  for n in t.directory.entries:
   for l in n.directory.entries:out[(t.id,n.id,l.id)]=p.get_data(l.data.struct.OffsetToData,l.data.struct.Size)
 return out
original=resources('../6kinoko/6kinoko.exe')
previous=resources('runtime-builds/orange-platform-20260913-r2-quiet/kinoko_retdec_rebuild.exe')
k=c.WinDLL('kernel32',use_last_error=True);u=c.WinDLL('user32',use_last_error=True)
k.LoadLibraryExW.argtypes=[c.c_wchar_p,c.c_void_p,c.c_uint32];k.LoadLibraryExW.restype=c.c_void_p
k.FreeLibrary.argtypes=[c.c_void_p];k.FreeLibrary.restype=c.c_int
u.LoadIconW.argtypes=[c.c_void_p,c.c_void_p];u.LoadIconW.restype=c.c_void_p
u.LoadImageW.argtypes=[c.c_void_p,c.c_void_p,c.c_uint,c.c_int,c.c_int,c.c_uint];u.LoadImageW.restype=c.c_void_p
u.DestroyIcon.argtypes=[c.c_void_p];u.DestroyIcon.restype=c.c_int
results=[]
for mode in ['quiet','diag']:
 path=Path(f'runtime-builds/win32-resources-20260913-{mode}/kinoko_retdec_rebuild.exe').resolve()
 if not path.exists():continue
 actual=resources(path)
 for key,value in original.items():assert actual[key]==value,(mode,key)
 assert actual[(24,1,1033)]==previous[(24,1,1033)]
 assert set(actual)==set(original)|set(previous)
 module=k.LoadLibraryExW(str(path),None,2);assert module,c.get_last_error()
 try:
  assert u.LoadIconW(module,c.c_void_p(101)),c.get_last_error()
  for size in [16,32]:
   icon=u.LoadImageW(module,c.c_void_p(101),1,size,size,0);assert icon,c.get_last_error();assert u.DestroyIcon(icon)
 finally:k.FreeLibrary(module)
 results.append({'mode':mode,'original_icon_payloads_identical':True,'original_group_id':101,'original_language':1041,'manifest_unchanged':True,'windows_LoadIcon_succeeded':True,'windows_LoadImage_sizes':[16,32],'exe_sha256':hashlib.sha256(path.read_bytes()).hexdigest()})
print(json.dumps(results,indent=2))
