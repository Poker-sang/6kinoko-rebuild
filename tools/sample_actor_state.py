"""Opt-in read-only sampling of a Win32 actor and its update-thread FP state."""
import argparse
import ctypes as c
from ctypes import wintypes as w
import json
import struct
import time

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('pid',type=int);p.add_argument('tid',type=int)
p.add_argument('actor',type=lambda v:int(v,0))
p.add_argument('output');p.add_argument('--samples',type=int,default=300)
a=p.parse_args()
k=c.WinDLL('kernel32',use_last_error=True)
k.OpenProcess.argtypes=[w.DWORD,w.BOOL,w.DWORD];k.OpenProcess.restype=w.HANDLE
k.OpenThread.argtypes=[w.DWORD,w.BOOL,w.DWORD];k.OpenThread.restype=w.HANDLE
k.ReadProcessMemory.argtypes=[w.HANDLE,c.c_void_p,c.c_void_p,c.c_size_t,c.POINTER(c.c_size_t)]
k.Wow64GetThreadContext.argtypes=[w.HANDLE,c.c_void_p]
k.SuspendThread.argtypes=[w.HANDLE];k.SuspendThread.restype=w.DWORD
k.ResumeThread.argtypes=[w.HANDLE];k.ResumeThread.restype=w.DWORD
k.CloseHandle.argtypes=[w.HANDLE]
process=k.OpenProcess(0x10,False,a.pid);thread=k.OpenThread(0xA,False,a.tid)
if not process or not thread:raise c.WinError(c.get_last_error())
rows=[]
def read(address,size):
 b=c.create_string_buffer(size);n=c.c_size_t()
 if not k.ReadProcessMemory(process,address,b,size,c.byref(n)) or n.value!=size:raise c.WinError(c.get_last_error())
 return b.raw
try:
 for i in range(a.samples):
  ctx=c.create_string_buffer(716);struct.pack_into('<I',ctx,0,0x1003F)
  if k.SuspendThread(thread)==0xffffffff:raise c.WinError(c.get_last_error())
  try:
   if not k.Wow64GetThreadContext(thread,ctx):raise c.WinError(c.get_last_error())
   actor=read(a.actor,548)
   def u(offset):return struct.unpack_from('<I',actor,offset)[0]
   def f(offset):return struct.unpack_from('<f',actor,offset)[0]
   r=dict(sample=i,eip=struct.unpack_from('<I',ctx,184)[0],cw=struct.unpack_from('<I',ctx,28)[0],mxcsr=struct.unpack_from('<I',ctx,228)[0],take=u(208),x=f(240),y=f(244),oldy=f(252),vy=f(260),carry=f(268),hit=u(296),parent=u(32),top=f(444),bottom=f(452))
   rows.append(r)
  finally:
   if k.ResumeThread(thread)==0xffffffff:raise c.WinError(c.get_last_error())
  time.sleep(0.017)
finally:
 k.CloseHandle(thread);k.CloseHandle(process)
 with open(a.output,'x',encoding='utf-8') as out:json.dump(rows,out,indent=2)
print('saved',len(rows),'samples; rounding modes',sorted({(r['cw']&0xc00,r['mxcsr']&0x6000) for r in rows}),'takes',sorted({r['take'] for r in rows}))
