import sys,os
import pipng
from adv_map import adv_map
from adv_sprite import adv_sprite

ADV_MAP_W=40
ADV_MAP_H=22
ADV_TILE_W=32
ADV_TILE_H=32

adv_res_shared=None

class adv_res:
  def __init__(self,root=None):
    global adv_res_shared
    adv_res_shared=self
    self.root="src/data" if root is None else root
    self.load()
    
  def load(self):
    self.tilesheet={}
    self.tsprop={}
    self.sprite={}
    self.map={}
    self.dirty=set()
    for base in os.listdir(os.path.join(self.root,"tilesheet")):
      self.load_tilesheet(os.path.join(self.root,"tilesheet",base))
    for base in os.listdir(os.path.join(self.root,"tsprop")):
      self.load_tsprop(os.path.join(self.root,"tsprop",base))
    for base in os.listdir(os.path.join(self.root,"sprite")):
      self.load_sprite(os.path.join(self.root,"sprite",base))
    for base in os.listdir(os.path.join(self.root,"map")):
      self.load_map(os.path.join(self.root,"map",base))
    for imgid in self.tilesheet.iterkeys():
      if not imgid in self.tsprop: self.tsprop[imgid]=[0]*256
      
  def save(self):
    for res in self.dirty: res.save()
    self.dirty.clear()
      
  def load_tilesheet(self,path):
    if not path.endswith(".png"): return
    base=os.path.basename(path)
    digitc=0
    while digitc<len(base) and base[digitc].isdigit(): digitc+=1
    if digitc<1: return
    imgid=int(base[:digitc])
    pixels,w,h,fmt=pipng.decode_file(path,"rgba")
    if w!=ADV_TILE_W*16 or h!=ADV_TILE_H*16:
      raise Exception("%s: bad dimensions (%d,%d), expected (%d,%d)"%(path,w,h,ADV_TILE_W*16,ADV_TILE_H*16))
    self.tilesheet[imgid]=pixels
    
  def load_tsprop(self,path):
    base=os.path.basename(path)
    digitc=0
    while digitc<len(base) and base[digitc].isdigit(): digitc+=1
    if digitc<1: return
    imgid=int(base[:digitc])
    if not imgid in self.tilesheet: return
    prop=[0]*256
    for lineno,line in enumerate(open(path,"r")):
      cmtp=line.find('#')
      if cmtp>=0: line=line[:cmtp]
      words=line.split()
      if len(words)<1: continue
      try:
        tileid=int(words[0],16)
        assert tileid>=0 and tileid<256
      except: raise Exception("%s:%d: expected tile id, found %r"%(path,lineno+1,words[0]))
      for word in words[1:]:
        if word=="hole": prop[tileid]|=0x01
        elif word=="solid": prop[tileid]|=0x02
        elif word=="latch": prop[tileid]|=0x04
        elif word=="neighbor15": prop[tileid]|=0x08
        elif word=="heal": prop[tileid]|=0x10
        else: raise Exception("%s:%d: undefined tile property %r"%(path,lineno+1,word))
    self.tsprop[imgid]=prop
    
  def load_sprite(self,path):
    #if not path.endswith(".sprite"): return
    base=os.path.basename(path)
    digitc=0
    while digitc<len(base) and base[digitc].isdigit(): digitc+=1
    if digitc<1: return
    spriteid=int(base[:digitc])
    self.sprite[spriteid]=adv_sprite(spriteid,open(path,"r").read(),path)
    
  def load_map(self,path):
    #if not path.endswith(".map"): return
    base=os.path.basename(path)
    digitc=0
    while digitc<len(base) and base[digitc].isdigit(): digitc+=1
    if digitc<1: return
    mapid=int(base[:digitc])
    self.map[mapid]=adv_map(mapid,open(path,"r").read(),path)
    self.map[mapid].r=self
    
  def _unique_mapid(self):
    if len(self.map)<1: return 1
    return max(self.map.keys())+1
    
  def get_map(self,mapid):
    if not mapid: mapid=self._unique_mapid()
    if not (mapid in self.map):
      self.map[mapid]=adv_map(mapid)
      self.map[mapid].r=self
      if self.map[mapid].path is None: self.map[mapid].path=os.path.join(self.root,"map","%d.map"%(mapid))
    return self.map[mapid]
    
  def get_entries(self,dstmapid):
    entries=[]
    for m in self.map.itervalues():
      for door in m.doorv:
        if door[2]==dstmapid:
          entries.append((door[3],door[4],m.mapid,door[0],door[1]))
    return entries
    
  def get_sprite_tile(self,spriteid):
    if not (spriteid in self.sprite): return -1
    return self.sprite[spriteid].tileid
    
  def replace_entry(self,fromdoor,todoor,mapid):
    if not fromdoor[2] in self.map: return
    for i,door in enumerate(self.map[fromdoor[2]].doorv):
      if door[0]==fromdoor[3] and door[1]==fromdoor[4] and door[3]==fromdoor[0] and door[4]==fromdoor[1]:
        self.map[fromdoor[2]].doorv[i]=(todoor[3],todoor[4],mapid,todoor[0],todoor[1])
        self.dirty.add(self.map[fromdoor[2]])
        return
    
  def remove_entry(self,door):
    if not door[2] in self.map: return
    for i,q in enumerate(self.map[door[2]].doorv):
      if q[0]==door[3] and q[1]==door[4] and q[3]==door[0] and q[4]==door[1]:
        self.map[door[2]].doorv.pop(i)
        self.dirty.add(self.map[door[2]])
        return
        
  #----------------------------------------------------------------------------
  
  class FlatMaps:
    def __init__(self,r):
      self.r=r
      self.left=0
      self.right=0
      self.top=0
      self.bottom=0
      self.content={} # key is tuple (x,y)
      
    def populate(self,m,visited,x=0,y=0):
      if m in visited: return
      visited.add(m)
      self.content[(x,y)]=m
      if x<self.left: self.left=x
      if x>self.right: self.right=x
      if y<self.top: self.top=y
      if y>self.bottom: self.bottom=y
      if m.north in self.r.map: self.populate(self.r.map[m.north],visited,x,y-1)
      if m.south in self.r.map: self.populate(self.r.map[m.south],visited,x,y+1)
      if m.west in self.r.map: self.populate(self.r.map[m.west],visited,x-1,y)
      if m.east in self.r.map: self.populate(self.r.map[m.east],visited,x+1,y)
      
    def fill_neighbors(self):
      for (x,y),m in self.content.iteritems():
        if (x-1,y) in self.content and not m.west:
          m.west=self.content[(x-1,y)].mapid
          self.r.dirty.add(m)
        if (x+1,y) in self.content and not m.east:
          m.east=self.content[(x+1,y)].mapid
          self.r.dirty.add(m)
        if (x,y-1) in self.content and not m.north:
          m.north=self.content[(x,y-1)].mapid
          self.r.dirty.add(m)
        if (x,y+1) in self.content and not m.south:
          m.south=self.content[(x,y+1)].mapid
          self.r.dirty.add(m)
  
  def fix_neighbors(self):
    """After creating a map with a neighbor, generate a full graph and fill in any missing links.
       This runs from scratch every time just to be safe.
    """
    visited=set()
    for m in self.map.itervalues():
      if m in visited: continue
      fm=self.FlatMaps(self)
      fm.populate(m,visited)
      fm.fill_neighbors()
