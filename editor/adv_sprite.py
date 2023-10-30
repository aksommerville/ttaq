import sys,os

class adv_sprite:
  def __init__(self,spriteid=0,src=None,path=None):
    self.path=path
    self.spriteid=spriteid
    self.tileid=0
    self.sprclass=""
    if not (src is None): self.decode(src)
    
  def decode(self,src):
    for lineno,line in enumerate(src.split('\n')):
      cmtp=line.find('#')
      if cmtp>=0: line=line[:cmtp]
      wordv=line.split()
      if not len(wordv): continue
      if wordv[0]=="tile":
        self.tileid=int(wordv[1],16)
      elif wordv[0]=="class":
        self.sprclass=wordv[1]
      # Sprites may have class-specific fields now, so don't report unknown key as an error.
      #else: raise Exception("sprite#%d:%d: unexpected key %r"%(self.spriteid,lineno+1,wordv[0]))
    
  def save(self):
    if self.path is None: raise Exception("Can't save sprite#%d, path is unset"%(self.spriteid))
    #sys.stdout.write("TODO: Save sprite#%d to %r\n"%(self.spriteid,self.path))
    dst=""
    dst+="tile %02x\n"%(self.tileid)
    dst+="class %s\n"%(self.sprclass)
    open(self.path,"w").write(dst)
