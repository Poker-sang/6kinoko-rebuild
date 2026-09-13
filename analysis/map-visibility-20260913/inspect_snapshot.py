import mmap,struct,bisect
from pathlib import Path
f=open('runtime-builds/enemy-reset-20260913-r1-diag/fault-20260913-085803-766-p20092-script.dmp','rb');m=mmap.mmap(f.fileno(),0,access=mmap.ACCESS_READ)
def u(off,fmt='I'):return struct.unpack_from('<'+fmt,m,off)[0]
streams={u(u(12)+12*i):(u(u(12)+12*i+4),u(u(12)+12*i+8)) for i in range(u(8))}
o=streams[9][1]; n=u(o,'Q'); raw=u(o+8,'Q');ranges=[]
for i in range(n):
 a,size=struct.unpack_from('<QQ',m,o+16+i*16); ranges.append((a,a+size,raw));raw+=size
ranges.sort();starts=[r[0] for r in ranges]
def read(a,n):
 r=ranges[bisect.bisect_right(starts,a)-1]
 if not r[0]<=a or a+n>r[1]:raise ValueError(hex(a))
 return m[r[2]+a-r[0]:r[2]+a-r[0]+n]
def word(a):return struct.unpack('<I',read(a,4))[0]
def pair(a):return struct.unpack('<II',read(a,8))
def string(a):return read(a+28,word(a+20)).decode('cp932',errors='replace') if a else 'null'
def obj(t,v):
 if t==0x08000010:return string(v)
 if t==0x05000004:return str(struct.unpack('<f',struct.pack('<I',v))[0])
 return f'{t:08x}:{v:08x}'
def field(inst,name):
 cl=word(inst+28); tb=word(cl+24);nodes=word(tb+28);count=word(tb+40)
 for i in range(count):
  node=nodes+20*i; kt,kv=pair(node+8)
  if kt==0x08000010 and string(kv)==name:
   descriptor=word(node+4);addr=inst+44+8*(descriptor&0xffffff) if descriptor&0x2000000 else word(cl+44)+16*(descriptor&0xffffff)
   return pair(addr)
 return (0,0)

base=0xd20000
def addr(a):return a-0x400000+base
count=word(addr(0x4fcd64));print('next_texture_slot',count)
total=0
for i in range(1,count):
 texture,w,h=struct.unpack('<III',read(addr(0x502360)+i*12,12));total+=w*h*4
print('estimated_texture_bytes',total)
print('device',hex(word(addr(0x4feaf0))))
vt=struct.pack('<I',addr(0x4fc464));layouts=[]
for a,b,o in ranges:
 data=m[o:o+b-a];start=0
 while True:
  k=data.find(vt,start)
  if k<0:break
  p=a+k;start=k+4
  try:
   layer=word(p+312);res=word(p+316);begin=word(p+264);end=word(p+268)
   if layer and res and begin and end>=begin and (end-begin)%32==0:
    name=read(layer+112,16).split(b'\0')[0] if word(layer+136)<16 else read(word(layer+112),word(layer+132))
    print('layout',hex(p),'name',name,'chips',(end-begin)//32,'draws',word(p+380),'alpha_scale',struct.unpack('<ff',read(p+320,8)),'visible',read(layer+140,1).hex())
  except (ValueError,struct.error,IndexError):pass
