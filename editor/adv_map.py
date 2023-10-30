import sys,os
import pipng

ADV_MAP_W=40
ADV_MAP_H=22
ADV_TILE_W=32
ADV_TILE_H=32

def wr16(src): return chr((src>>8)&0xff)+chr(src&0xff)
def rd16(src,srcp=0): return (ord(src[srcp])<<8)|ord(src[srcp+1])

class adv_map:
  def __init__(self,mapid=0,src=None,path=None):
    self.path=path
    self.mapid=mapid
    self.north=0
    self.south=0
    self.east=0
    self.west=0
    self.bgimageid=100
    self.sprimageid=102
    self.lights_out=0
    self.song=1
    self.spawnv=[]  # (sprdefid,col,row)
    self.doorv=[]   # (srccol,srcrow,dstmapid,dstcol,dstrow)
    self.switchv=[] # (col,row,tileoff,tileon,globalid,method)
    self.neighborinfo=None # populate lazily
    if src is None: self.cellv=[0]*(ADV_MAP_W*ADV_MAP_H) # background cells only; we're not supporting foreground yet
    else: self.decode(src)
    
  def save(self):
    if self.path is None: raise Exception("Can't save map#%d, no path set"%(self.mapid))
    dst="\0MP\xff"+chr(ADV_MAP_W)+chr(ADV_MAP_H)
    dst+=''.join(map(chr,self.cellv))
    dst+="\x01"+wr16(self.bgimageid)
    dst+="\x02"+wr16(self.sprimageid)
    if self.north: dst+="\x03"+wr16(self.north)
    if self.south: dst+="\x04"+wr16(self.south)
    if self.east: dst+="\x05"+wr16(self.east)
    if self.west: dst+="\x06"+wr16(self.west)
    if self.song: dst+="\x07"+wr16(self.song)
    if self.lights_out: dst+="\x08\x01"
    for spawn in self.spawnv: dst+="\x09"+chr(spawn[1])+chr(spawn[2])+wr16(spawn[0])
    for door in self.doorv: dst+="\x0a"+chr(door[0])+chr(door[1])+wr16(door[2])+chr(door[3])+chr(door[4])
    for sw in self.switchv: dst+="\x0b"+chr(sw[0])+chr(sw[1])+chr(sw[2])+chr(sw[3])+chr(sw[4])+chr(sw[5])
    open(self.path,"w").write(dst)
    
  def decode_binary(self,src):
    if not src.startswith("\x00MP\xff"): raise Exception("map#%d: signature mismatch"%(self.mapid))
    if len(src)<6: raise Exception("map#%d: unexpected end of file"%(self.mapid))
    if src[4]!=chr(ADV_MAP_W) or src[5]!=chr(ADV_MAP_H):
      raise Exception("map#%d: file is %dx%d, we require maps to be %dx%d"%(self.mapid,ord(src[4]),ord(src[5]),ADV_MAP_W,ADV_MAP_H))
    srcp=6
    if srcp>len(src)-ADV_MAP_W*ADV_MAP_H: raise Exception("map#%d: unexpected end of file"%(self.mapid))
    self.cellv=map(ord,src[srcp:srcp+ADV_MAP_W*ADV_MAP_H])
    srcp+=ADV_MAP_W*ADV_MAP_H
    while srcp<len(src):
      cmd=ord(src[srcp])
      srcp+=1
      if cmd==0x00: break
      elif cmd==0x01: self.bgimageid=rd16(src,srcp) ; srcp+=2
      elif cmd==0x02: self.sprimageid=rd16(src,srcp) ; srcp+=2
      elif cmd==0x03: self.north=rd16(src,srcp) ; srcp+=2
      elif cmd==0x04: self.south=rd16(src,srcp) ; srcp+=2
      elif cmd==0x05: self.east=rd16(src,srcp) ; srcp+=2
      elif cmd==0x06: self.west=rd16(src,srcp) ; srcp+=2
      elif cmd==0x07: self.song=rd16(src,srcp) ; srcp+=2
      elif cmd==0x08: self.lights_out=ord(src[srcp]) ; srcp+=1
      elif cmd==0x09:
        col=ord(src[srcp]) ; srcp+=1
        row=ord(src[srcp]) ; srcp+=1
        sprdefid=rd16(src,srcp) ; srcp+=2
        self.spawnv.append((sprdefid,col,row))
      elif cmd==0x0a:
        srccol=ord(src[srcp]) ; srcp+=1
        srcrow=ord(src[srcp]) ; srcp+=1
        dstmapid=rd16(src,srcp) ; srcp+=2
        dstcol=ord(src[srcp]) ; srcp+=1
        dstrow=ord(src[srcp]) ; srcp+=1
        self.doorv.append((srccol,srcrow,dstmapid,dstcol,dstrow))
      elif cmd==0x0b:
        col=ord(src[srcp]) ; srcp+=1
        row=ord(src[srcp]) ; srcp+=1
        tileoff=ord(src[srcp]) ; srcp+=1
        tileon=ord(src[srcp]) ; srcp+=1
        globalid=ord(src[srcp]) ; srcp+=1
        method=ord(src[srcp]) ; srcp+=1
        self.switchv.append((col,row,tileoff,tileon,globalid,method))
      else: raise Exception("map#%d: unexpected field 0x%02x"%(self.mapid,cmd))
    
  def save_text(self):
    #XXX Old format.
    if self.path is None: raise Exception("Can't save map#%d, no path set"%(self.mapid))
    #TODO Use a binary format for maps. No need for text now that we have an editor.
    dst=""
    dst+="bg %d\n"%(self.bgimageid)
    #dst+="fg %d\n"%(self.fgimageid)
    dst+="spr %d\n"%(self.sprimageid)
    dst+="song %d\n"%(self.song)
    if self.north: dst+="north %d\n"%(self.north)
    if self.south: dst+="south %d\n"%(self.south)
    if self.east: dst+="east %d\n"%(self.east)
    if self.west: dst+="west %d\n"%(self.west)
    if self.lights_out: dst+="lights_out 1\n"
    for spawn in self.spawnv: dst+="sprite %d %d %d\n"%(spawn[0],spawn[1],spawn[2])
    for door in self.doorv: dst+="door %d %d %d %d %d\n"%(door[0],door[1],door[2],door[3],door[4])
    for sw in self.switchv: dst+="switch %d %d %02x %02x %d %d\n"%(sw[0],sw[1],sw[2],sw[3],sw[4],sw[5])
    macroed=set()
    for tile in self.cellv:
      if tile in macroed: continue
      macroed.add(tile)
      dst+="tile '%02x' %02x 00\n"%(tile,tile)
    cellp=0
    for row in xrange(ADV_MAP_H):
      for col in xrange(ADV_MAP_W):
        dst+="%02x"%(self.cellv[cellp])
        cellp+=1
      dst+="\n"
    open(self.path,"w").write(dst)
    
  def decode(self,src):
    if src.startswith("\x00MP\xff"): return self.decode_binary(src)
    else: return self.decode_text(src)
    
  def decode_text(self,src):
    """This format is not used anymore. But it should do no harm to leave it available."""
    bodyc=((ADV_MAP_W<<1)+1)*ADV_MAP_H
    if len(src)<bodyc: raise Exception("%d: map too small"%(self.mapid))
    body=src[-bodyc:]
    macrov={}
    for lineno,line in enumerate(src[:-bodyc].split('\n')):
      cmtp=line.find('#')
      if cmtp>=0: line=line[:cmtp]
      line=line.strip()
      if len(line)<1: continue
      spcp=line.find(' ')
      if spcp<0: raise Exception("map#%d:%d: bad line (no separator)"%(self.mapid,lineno+1))
      kw=line[:spcp]
      arg=line[spcp:].strip()
      if kw=="tile":
        k=arg[1:3]
        macrov[k]=int(arg[5:7],16)
      elif kw=="sprite":
        spriteid,col,row=map(int,arg.split())
        self.spawnv.append((spriteid,col,row))
      elif kw=="door":
        srccol,srcrow,mapid,dstcol,dstrow=map(int,arg.split())
        self.doorv.append((srccol,srcrow,mapid,dstcol,dstrow))
      elif kw=="switch":
        col,row,tileoff,tileon,globalid,method=arg.split()
        col,row,globalid,method=map(int,(col,row,globalid,method))
        tileoff=int(tileoff,16)
        tileon=int(tileon,16)
        self.switchv.append((col,row,tileoff,tileon,globalid,method))
      elif kw=="bg": self.bgimageid=int(arg)
      elif kw=="fg": self.fgimageid=int(arg)
      elif kw=="spr": self.sprimageid=int(arg)
      elif kw=="north": self.north=int(arg)
      elif kw=="south": self.south=int(arg)
      elif kw=="east": self.east=int(arg)
      elif kw=="west": self.west=int(arg)
      elif kw=="lights_out": self.lights_out=int(arg)
      elif kw=="song": self.song=int(arg)
      else: raise Exception("map#%d:%d: unexpected keyword %r"%(self.mapid,lineno+1,kw))
    self.cellv=[]
    bodyp=0
    for row in xrange(ADV_MAP_H):
      for col in xrange(ADV_MAP_W):
        text=body[bodyp:bodyp+2]
        if not (text in macrov): raise Exception("map#%d: undefined tile macro %r at (%d,%d)"%(self.mapid,text,col,row))
        self.cellv.append(macrov[text])
        bodyp+=2
      bodyp+=1
      
  def populate_neighbor_info(self):
    self.neighborinfo={} # key is any tileid, value is the top-left tile of its neighbor set
    for tileid,prop in enumerate(self.r.tsprop[self.bgimageid]):
      if prop&0x08:
        for dcol in xrange(5):
          for drow in xrange(3):
            self.neighborinfo[tileid+dcol+drow*16]=tileid
            
  def _check_neighbors(self,col,row):
    if col<0 or row<0 or col>=ADV_MAP_W or row>=ADV_MAP_H: return
    tileid=self.cellv[row*ADV_MAP_W+col]
    if not tileid in self.neighborinfo: return
    basetileid=self.neighborinfo[tileid]
    neighbors=0x00
    def neighbormask(dx,dy):
      if dx==0:
        if dy==0: return 0x80
        if dy==1: return 0x10
        if dy==2: return 0x04
      if dx==1:
        if dy==0: return 0x40
        if dy==1: return 0x00
        if dy==2: return 0x02
      if dx==2:
        if dy==0: return 0x20
        if dy==1: return 0x08
        if dy==2: return 0x01
      return 0x00
    for dx in xrange(3):
      ncol=col+dx-1
      if ncol<0: ncol=0
      elif ncol>=ADV_MAP_W: ncol=ADV_MAP_W-1
      for dy in xrange(3):
        nrow=row+dy-1
        if nrow<0: nrow=0
        elif nrow>=ADV_MAP_H: nrow=ADV_MAP_H-1
        ntileid=self.cellv[nrow*ADV_MAP_W+ncol]
        if not ntileid in self.neighborinfo: continue
        if self.neighborinfo[ntileid]==basetileid: neighbors|=neighbormask(dx,dy)
        
    if   neighbors==0xff: tileid=basetileid+0x13
    elif neighbors==0xfe: tileid=basetileid+0x10
    elif neighbors==0xfb: tileid=basetileid+0x11
    elif neighbors==0xdf: tileid=basetileid+0x20
    elif neighbors==0x7f: tileid=basetileid+0x21
    elif neighbors&0x1f==0x1f: tileid=basetileid+0x03
    elif neighbors&0x6b==0x6b: tileid=basetileid+0x12
    elif neighbors&0xd6==0xd6: tileid=basetileid+0x14
    elif neighbors&0xf8==0xf8: tileid=basetileid+0x23
    elif neighbors&0x0b==0x0b: tileid=basetileid+0x02
    elif neighbors&0x16==0x16: tileid=basetileid+0x04
    elif neighbors&0x68==0x68: tileid=basetileid+0x22
    elif neighbors&0xd0==0xd0: tileid=basetileid+0x24
    else: tileid=basetileid
    self.cellv[row*ADV_MAP_W+col]=tileid
      
  def check_neighbors(self,col,row):
    if self.neighborinfo is None: self.populate_neighbor_info()
    if len(self.neighborinfo)<1: return
    for dx in xrange(-1,2):
      for dy in xrange(-1,2):
        self._check_neighbors(col+dx,row+dy)
        
  def assert_neighbor(self,m,dx,dy):
    """Ensure that (north,south,east,east) in both self and m agree with m being +(dx,dy) of self.
       NB: This is not related to cell "neighbors".
    """
    if dx==1 and dy==0:
      if self.east!=m.mapid:
        self.east=m.mapid
        self.r.dirty.add(self)
      if m.west!=self.mapid:
        m.west=self.mapid
        self.r.dirty.add(m)
    elif dx==-1 and dy==0:
      if self.west!=m.mapid:
        self.west=m.mapid
        self.r.dirty.add(self)
      if m.east!=self.mapid:
        m.east=self.mapid
        self.r.dirty.add(m)
    elif dx==0 and dy==1:
      if self.south!=m.mapid:
        self.south=m.mapid
        self.r.dirty.add(self)
      if m.north!=self.mapid:
        m.north=self.mapid
        self.r.dirty.add(m)
    elif dx==0 and dy==-1:
      if self.north!=m.mapid:
        self.north=m.mapid
        self.r.dirty.add(self)
      if m.south!=self.mapid:
        m.south=self.mapid
        self.r.dirty.add(m)
