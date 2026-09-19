from pathlib import Path
import re
LEX = re.compile(r'R"([A-Za-z0-9_]*)\([\s\S]*?\)\1"|//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')
def mask(s): return LEX.sub(lambda m: ''.join('\n' if c=='\n' else ' ' for c in m[0]), s)
def matching(s, pos, left='(', right=')'):
    depth=0
    for i in range(pos,len(s)):
        if s[i]==left: depth+=1
        elif s[i]==right:
            depth-=1
            if not depth:return i
    raise ValueError(pos)
def args(s,start,end,m=None):
    out=[]; depth=0; last=start; m=mask(s) if m is None else m
    for i in range(start,end):
        c=m[i]
        if c in '([{': depth+=1
        elif c in ')]}': depth-=1
        elif c==',' and not depth:out.append(s[last:i].strip()); last=i+1
    if last<end:out.append(s[last:end].strip())
    return out

def funcs(s):
    m=mask(s); depth=0; ranges=[]
    for b in re.finditer(r'[{}]',m):
        if b[0]=='{':
            if depth==0:
                endpar=b.start()-1
                while endpar>=0 and m[endpar].isspace(): endpar-=1
                if endpar>=0 and m[endpar]==')':
                    d=1; j=endpar-1
                    while d:
                        if m[j]==')':d+=1
                        elif m[j]=='(':d-=1
                        j-=1
                    prefix_start=max(0,j-160)
                    match=re.search(r'\b(\w+)\s*$',m[prefix_start:j+1])
                    if match:
                        name=match[1]; name_start=prefix_start+match.start(); start=m.rfind('\n',0,name_start)+1
                        while start and not s[start:name_start].strip():
                            start=m.rfind('\n',0,start-1)+1
                        if not m[start:name_start].strip():
                            prev=m.rfind('\n',0,start-1)+1; start=prev
                        current=(name,start,b.start(),j+1,endpar)
                    else: current=None
                else:current=None
            depth+=1
        else:
            depth-=1
            if depth==0 and current:
                name,start,body,params,endpar=current
                ranges.append(dict(name=name,start=start,body=body,end=b.end(),signature=s[start:body].strip(),lines=s.count('\n',start,b.end())+1,line=s.count('\n',0,start)+1))
    return ranges
