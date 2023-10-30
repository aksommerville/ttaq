#!/usr/bin/env python
import sys,os
import linput,pig,vg,pipng,vgaid
from adv_map import adv_map
from adv_res import adv_res

ADV_MAP_W=40
ADV_MAP_H=22
ADV_TILE_W=32
ADV_TILE_H=32
ADV_SCREEN_W=ADV_MAP_W*ADV_TILE_W
ADV_SCREEN_H=ADV_MAP_H*ADV_TILE_H

pig.init(pig.API_OPENVG)
screenw,screenh=pig.get_screen_size()
vg.Set(vg.CLEAR_COLOR,[0.0,0.0,0.0,1.0])

linput.init()

font=vg.CreateFont()
fontimg,w,h,fmt=pipng.decode_file(os.path.join(sys.path[0],"font.png"),"rgba")
if w%16 or h%6: raise Exception("font dimensions must be multiples of (16,6)")
colw,rowh=w/16,h/6
parent=vg.CreateImage(vg.sABGR_8888,w,h)
vg.ImageSubData(parent,fontimg)
for ch in xrange(95):
  col=ch&15
  row=ch>>4
  img=vg.ChildImage(parent,col*colw,row*rowh,colw,rowh)
  vg.SetGlyphToImage(font,0x20+ch,img)
  vg.DestroyImage(img)
vg.DestroyImage(parent)
query_prompt=None
query_input=""
query_callback=None

cursor_stroke_paint=vg.CreatePaint()
vg.SetColor(cursor_stroke_paint,0xffffffff)
cursor_fill_paint=vg.CreatePaint()
vg.SetColor(cursor_fill_paint,0x000000ff)
vgaid.begin()
vgaid.move_to(0,0)
vgaid.line_to(10,20)
vgaid.quad_to(5,5,20,10)
vgaid.line_to(0,0)
cursor_path=vgaid.end()
cursorx=ADV_SCREEN_W>>1
cursory=ADV_SCREEN_H>>1
pvcol=-1
pvrow=-1

selection_stroke_paint=vg.CreatePaint()
vg.SetColor(selection_stroke_paint,0xff0000ff)
selection_fill_paint=vg.CreatePaint()
vg.SetColor(selection_fill_paint,0xff000040)
vgaid.begin()
vgaid.move_to(0,0)
vgaid.line_to(0,ADV_TILE_H)
vgaid.line_to(ADV_TILE_W,ADV_TILE_H)
vgaid.line_to(ADV_TILE_W,0)
vgaid.line_to(0,0)
selection_path=vgaid.end()
selectionx=0
selectiony=0

blotter_paint=vg.CreatePaint()
vg.SetColor(blotter_paint,0x00000080)
vgaid.begin()
vgaid.move_to(0,0)
vgaid.line_to(0,ADV_SCREEN_H)
vgaid.line_to(ADV_SCREEN_W,ADV_SCREEN_H)
vgaid.line_to(ADV_SCREEN_W,0)
vgaid.line_to(0,0)
blotter_path=vgaid.end()

exit_paint=vg.CreatePaint()
vg.SetColor(exit_paint,0x000000ff)
vgaid.begin()
vgaid.move_to(ADV_TILE_W-10,0)
vgaid.line_to(ADV_TILE_W,0)
vgaid.line_to(ADV_TILE_W,10)
vgaid.move_to(ADV_TILE_W,0)
vgaid.line_to((ADV_TILE_W>>1)+5,ADV_TILE_H>>1)
exit_path=vgaid.end()

entry_paint=vg.CreatePaint()
vg.SetColor(entry_paint,0x000000ff)
vgaid.begin()
vgaid.move_to(0,0)
vgaid.line_to((ADV_TILE_W>>1)-5,ADV_TILE_H>>1)
vgaid.line_to((ADV_TILE_W>>1)-5,(ADV_TILE_H>>1)-10)
vgaid.move_to((ADV_TILE_W>>1)-5,ADV_TILE_H>>1)
vgaid.line_to((ADV_TILE_W>>1)-15,ADV_TILE_H>>1)
entry_path=vgaid.end()

switch_paint=vg.CreatePaint()
vg.SetColor(switch_paint,0x800080ff)
vgaid.begin()
vgaid.move_to(0,0)
vgaid.line_to(ADV_TILE_W,0)
vgaid.line_to(ADV_TILE_W>>1,ADV_TILE_H>>1)
vgaid.line_to(0,0)
switch_path=vgaid.end()

grid_paint=vg.CreatePaint()
vg.SetColor(grid_paint,0xff000040)
vgaid.begin()
for col in xrange(1,ADV_MAP_W):
  vgaid.move_to(col*ADV_TILE_W,0)
  vgaid.line_to(col*ADV_TILE_W,ADV_SCREEN_H)
for row in xrange(1,ADV_MAP_H):
  vgaid.move_to(0,row*ADV_TILE_H)
  vgaid.line_to(ADV_SCREEN_W,row*ADV_TILE_H)
grid_path=vgaid.end()

r=adv_res()
m=r.get_map(1)
entries=r.get_entries(m.mapid)
drag_door=None
drag_entry=None
drag_sprite=None
drag_switch=None

def load_tiles_1(imgid):
  if imgid in r.tilesheet: pixels=r.tilesheet[imgid]
  else: pixels="\0\0\0\0"*(ADV_TILE_W*ADV_TILE_H*256)
  parent=vg.CreateImage(vg.sABGR_8888,ADV_TILE_W*16,ADV_TILE_H*16)
  vg.ImageSubData(parent,pixels)
  return parent
def split_16x16(parent):
  children=[]
  for row in xrange(16):
    for col in xrange(16):
      child=vg.ChildImage(parent,col*ADV_TILE_W,row*ADV_TILE_H,ADV_TILE_W,ADV_TILE_H)
      children.append(child)
  vg.DestroyImage(parent)
  return children
def load_tiles():
  bg=load_tiles_1(m.bgimageid)
  spr=split_16x16(load_tiles_1(m.sprimageid))
  return (bg,spr)

bgimage=vg.CreateImage(vg.sABGR_8888,ADV_SCREEN_W,ADV_SCREEN_H)
def drawbg(x=0,y=0,w=ADV_MAP_W,h=ADV_MAP_H):
  if x<0: x=0
  if y<0: y=0
  if x+w>ADV_MAP_W: w=ADV_MAP_W-x
  if y+h>ADV_MAP_H: h=ADV_MAP_H-y
  for px in xrange(w):
    for py in xrange(h):
      tile=m.cellv[(y+py)*ADV_MAP_W+x+px]
      srcx=(tile&15)*ADV_TILE_W
      srcy=(tile>>4)*ADV_TILE_H
      vg.CopyImage(bgimage,(x+px)*ADV_TILE_W,(y+py)*ADV_TILE_H,bgtiles,srcx,srcy,ADV_TILE_W,ADV_TILE_H)

bgtiles,sprtiles=load_tiles()
drawbg()
show_tile_palette=False
show_doors=True
show_sprites=True
show_switches=True
show_grid=True

vg.Set(vg.MATRIX_MODE,vg.MATRIX_PATH_USER_TO_SURFACE)
vg.LoadIdentity()
vg.Translate(-(screenw>>1)+(ADV_SCREEN_W>>1),screenh-(screenh>>1)+(ADV_SCREEN_H>>1))
vg.Scale(1.0,-1.0)
mainmtx=vg.GetMatrix()

def draw_query():
  vg.Set(vg.CLEAR_COLOR,(0.5,0.6,0.5,1.0))
  vg.Clear()
  vg.Set(vg.CLEAR_COLOR,(0.0,0.0,0.0,1.0))
  vg.Set(vg.MATRIX_MODE,vg.MATRIX_GLYPH_USER_TO_SURFACE)
  vg.LoadMatrix(mainmtx)
  
  vg.Set(vg.GLYPH_ORIGIN,(100,100))
  if len(query_prompt): vg.DrawGlyphs(font,query_prompt)
  if query_input is not None:
    if len(query_input): vg.DrawGlyphs(font,query_input)
  
  pig.swap_sync()

def draw():
  if query_prompt is not None: return draw_query()
  
  vg.Clear()
  vg.Set(vg.MATRIX_MODE,vg.MATRIX_IMAGE_USER_TO_SURFACE)
  vg.LoadMatrix(mainmtx)
  
  vg.DrawImage(bgimage)
  
  if show_tile_palette:
    vg.Set(vg.MATRIX_MODE,vg.MATRIX_PATH_USER_TO_SURFACE)
    vg.LoadMatrix(mainmtx)
    vg.SetPaint(blotter_paint,vg.FILL_PATH)
    vg.DrawPath(blotter_path,vg.FILL_PATH)
    vg.Set(vg.MATRIX_MODE,vg.MATRIX_IMAGE_USER_TO_SURFACE)
    vg.Translate((ADV_SCREEN_W>>1)-(ADV_TILE_W*8),(ADV_SCREEN_H>>1)-(ADV_TILE_H*8))
    vg.DrawImage(bgtiles)
    
  if show_sprites and not show_tile_palette:
    for sprite in m.spawnv:
      tile=r.get_sprite_tile(sprite[0])
      if tile<0: continue
      x,y=sprite[1]*ADV_TILE_W,sprite[2]*ADV_TILE_H
      vg.Translate(x,y)
      vg.DrawImage(sprtiles[tile])
      vg.Translate(-x,-y)
  
  vg.Set(vg.MATRIX_MODE,vg.MATRIX_PATH_USER_TO_SURFACE)
  vg.LoadMatrix(mainmtx)
  
  if show_grid and not show_tile_palette:
    vg.SetPaint(grid_paint,vg.STROKE_PATH)
    vg.DrawPath(grid_path,vg.STROKE_PATH)
  
  if show_doors and not show_tile_palette:
    vg.SetPaint(exit_paint,vg.STROKE_PATH)
    for door in m.doorv:
      x=door[0]*ADV_TILE_W
      y=door[1]*ADV_TILE_H
      vg.Translate(x,y)
      vg.DrawPath(exit_path,vg.STROKE_PATH)
      vg.Translate(-x,-y)
    vg.SetPaint(entry_paint,vg.STROKE_PATH)
    for door in entries:
      x=door[0]*ADV_TILE_W
      y=door[1]*ADV_TILE_H
      vg.Translate(x,y)
      vg.DrawPath(entry_path,vg.STROKE_PATH)
      vg.Translate(-x,-y)
      
  if show_switches and not show_tile_palette:
    vg.SetPaint(switch_paint,vg.STROKE_PATH)
    for sw in m.switchv:
      x=sw[0]*ADV_TILE_W
      y=sw[1]*ADV_TILE_H
      vg.Translate(x,y)
      vg.DrawPath(switch_path,vg.STROKE_PATH)
      vg.Translate(-x,-y)
  
  vg.SetPaint(selection_stroke_paint,vg.STROKE_PATH)
  vg.SetPaint(selection_fill_paint,vg.FILL_PATH)
  if show_tile_palette: # Highlight current tile in palette.
    cell=m.cellv[selectiony*ADV_MAP_W+selectionx]
    x=(cell&15)*ADV_TILE_W+(ADV_SCREEN_W>>1)-(ADV_TILE_W*8)
    y=(cell>>4)*ADV_TILE_H+(ADV_SCREEN_H>>1)-(ADV_TILE_W*8)
    vg.Translate(x,y)
    vg.DrawPath(selection_path,vg.STROKE_PATH|vg.FILL_PATH)
    vg.Translate(-x,-y)
  else: # Highlight selected cell.
    vg.Translate(selectionx*ADV_TILE_W,selectiony*ADV_TILE_H)
    vg.DrawPath(selection_path,vg.STROKE_PATH|vg.FILL_PATH)
    vg.Translate(-selectionx*ADV_TILE_W,-selectiony*ADV_TILE_H)
  
  vg.SetPaint(cursor_stroke_paint,vg.STROKE_PATH)
  vg.SetPaint(cursor_fill_paint,vg.FILL_PATH)
  vg.Translate(cursorx,cursory)
  vg.DrawPath(cursor_path,vg.STROKE_PATH|vg.FILL_PATH)
  vg.Translate(-cursorx,-cursory)
  
  pig.swap_sync()
  
def update_drawing():
  global pvcol,pvrow
  col,row=cursorx/ADV_TILE_W,cursory/ADV_TILE_H
  if col<0 or row<0 or col>=ADV_MAP_W or row>=ADV_MAP_H: return
  if drag_door is not None or drag_entry is not None:
    if drag_door is not None:
      door=m.doorv[drag_door]
      if col!=door[0] or row!=door[1]:
        m.doorv[drag_door]=(col,row,door[2],door[3],door[4])
        r.dirty.add(m)
    if drag_entry is not None:
      door=entries[drag_entry]
      if col!=door[0] or row!=door[1]:
        entries[drag_entry]=(col,row,door[2],door[3],door[4])
        r.replace_entry(door,entries[drag_entry],m.mapid)
  elif drag_sprite is not None:
    if col==m.spawnv[drag_sprite][1] and row==m.spawnv[drag_sprite][2]: return
    m.spawnv[drag_sprite]=(m.spawnv[drag_sprite][0],col,row)
    r.dirty.add(m)
  elif drag_switch is not None:
    if col==m.switchv[drag_switch][0] and row==m.switchv[drag_switch][1]: return
    m.switchv[drag_switch]=(col,row)+m.switchv[drag_switch][2:]
    r.dirty.add(m)
  else:
    if selectionx<0 or selectiony<0 or selectionx>=ADV_MAP_W or selectiony>=ADV_MAP_H: return
    if col==pvcol and row==pvrow: return
    pvcol=col
    pvrow=row
    srcp=selectiony*ADV_MAP_W+selectionx
    dstp=row*ADV_MAP_W+col
    m.cellv[dstp]=m.cellv[srcp]
    m.check_neighbors(col,row)
    drawbg(col-1,row-1,3,3)
    r.dirty.add(m)
  
def adjust_selected_cell(d):
  col,row=selectionx,selectiony
  if col<0 or row<0 or col>=ADV_MAP_W or row>=ADV_MAP_H: return
  p=row*ADV_MAP_W+col
  m.cellv[p]+=d
  if m.cellv[p]<0: m.cellv[p]+=256
  elif m.cellv[p]>=256: m.cellv[p]-=256
  drawbg(col,row,1,1)
  r.dirty.add(m)
  
def toggle_tile_palette():
  global show_tile_palette
  show_tile_palette=not show_tile_palette
  
def choose_tile_palette():
  global show_tile_palette
  basex=(ADV_SCREEN_W>>1)-(ADV_TILE_W*8)
  basey=(ADV_SCREEN_H>>1)-(ADV_TILE_W*8)
  x,y=cursorx-basex,cursory-basey
  if x>=0 and y>=0 and x<ADV_TILE_W*16 and y<ADV_TILE_H*16:
    x/=ADV_TILE_W
    y/=ADV_TILE_H
    cell=(y<<4)|x
    col,row=selectionx,selectiony
    if col>=0 and row>=0 and col<ADV_MAP_W and row<ADV_MAP_H:
      m.cellv[row*ADV_MAP_W+col]=cell
      drawbg(col,row,1,1)
      r.dirty.add(m)
  show_tile_palette=False
  
def mouse_down():
  """tile palette already handled. set (drawing) or begin drag of sprite or door"""
  global drag_door,drag_entry,drag_sprite,drawing,drag_switch
  drag_door=None
  drag_entry=None
  drag_sprite=None
  drag_switch=None
  drawing=False
  if cursorx<0 or cursory<0 or cursorx>=ADV_SCREEN_W or cursory>=ADV_SCREEN_H: return
  col,row=cursorx/ADV_TILE_W,cursory/ADV_TILE_H
  if show_doors or show_switches:
    ymod=cursory%ADV_TILE_H
    if ymod<ADV_TILE_H>>1: # check doors and switches
      for i,door in enumerate(m.doorv):
        if col==door[0] and row==door[1]:
          drag_door=i
          break
      for i,door in enumerate(entries):
        if col==door[0] and row==door[1]:
          drag_entry=i
          break
      if not (drag_door is None) or not (drag_entry is None): return
      for i,sw in enumerate(m.switchv):
        if col==sw[0] and row==sw[1]:
          drag_switch=i
          return
  if show_sprites:
    for i,sprite in enumerate(m.spawnv):
      if col==sprite[1] and row==sprite[2]:
        drag_sprite=i
        return
  drawing=True
  
def right_mouse_down():
  global selectionx,selectiony
  if cursorx<0 or cursory<0 or cursorx>=ADV_SCREEN_W or cursory>=ADV_SCREEN_H: return
  col,row=cursorx/ADV_TILE_W,cursory/ADV_TILE_H
  if show_doors or show_switches:
    ymod=cursory%ADV_TILE_H
    if ymod<ADV_TILE_H>>1: # check doors and switches
      for i,door in enumerate(m.doorv):
        if col==door[0] and row==door[1]: return navigate(door[2])
      for i,door in enumerate(entries):
        if col==door[0] and row==door[1]: return navigate(door[2])
      for i,sw in enumerate(m.switchv):
        if col==sw[0] and row==sw[1]: return edit_switch(i)
  selectionx,selectiony=col,row
  
def navigate(mapid,create=False,dx=0,dy=0):
  global m,entries,bgtiles,sprtiles
  if not mapid and not create: return show_message("No map over there. Hold SHIFT while navigating to create one.")
  oldmap=m
  m=r.get_map(mapid)
  entries=r.get_entries(mapid)
  vg.DestroyImage(bgtiles)
  for img in sprtiles: vg.DestroyImage(img)
  bgtiles,sprtiles=load_tiles()
  drawbg()
  
  # Update neighbor references, if we created this map fresh.
  if dx or dy: 
    oldmap.assert_neighbor(m,dx,dy)
    r.fix_neighbors()
    
def delete():
  global drag_sprite,drag_door,drag_entry,drag_switch
  if drag_sprite is not None:
    m.spawnv.pop(drag_sprite)
    drag_sprite=None
    r.dirty.add(m)
  if drag_door is not None:
    m.doorv.pop(drag_door)
    drag_door=None
    r.dirty.add(m)
  if drag_entry is not None:
    r.remove_entry(entries[drag_entry])
    entries.pop(drag_entry)
    drag_entry=None
  if drag_switch is not None:
    m.switchv.pop(drag_switch)
    drag_switch=None
    r.dirty.add(m)
    
def show_message(msg):
  global query_prompt,query_input,query_callback
  query_prompt=msg
  query_input=None
  query_callback=None
    
def begin_query(prompt,callback):
  global query_prompt,query_input,query_callback
  query_prompt=prompt+" "
  query_input=""
  query_callback=callback
  
def end_query():
  global query_prompt,query_input,query_callback
  try: result=int(query_input)
  except: valid=False
  else: valid=True
  query_prompt=None
  query_input=""
  cb=query_callback
  query_callback=None
  if valid and cb: cb(result)
  
def create_door_id(mapid):
  global entries
  if not mapid in r.map:
    nm=r.get_map(mapid)
    mapid=nm.mapid
    show_message("Created map ID %d"%(mapid))
  dstx,dsty=ADV_MAP_W>>1,ADV_MAP_H>>1
  m.doorv.append((selectionx,selectiony,mapid,dstx,dsty))
  entries.append((selectionx,selectiony,mapid,dstx,dsty))
  r.map[mapid].doorv.append((dstx,dsty,m.mapid,selectionx,selectiony))
  r.dirty.add(m)
  r.dirty.add(r.map[mapid])
  
def create_door():
  begin_query("Create door to map ID (0 for new):",create_door_id)
  
def create_sprite_id(spriteid):
  if not spriteid in r.sprite: return show_message("No such sprite")
  m.spawnv.append((spriteid,selectionx,selectiony))
  r.dirty.add(m)
  
def create_sprite():
  begin_query("Add sprite ID:",create_sprite_id)
  
switch_global_id=0
switch_edit_p=0

def create_switch_method(method):
  global switch_global_id
  tileoff=m.cellv[selectiony*ADV_MAP_W+selectionx]
  tileon=tileoff+1
  m.switchv.append((selectionx,selectiony,tileoff,tileon,switch_global_id,method))
  r.dirty.add(m)
  switch_global_id=0
  
def create_switch_global(globalid):
  global switch_global_id
  switch_global_id=globalid
  begin_query("(switch) method:",create_switch_method)
  
def create_switch():
  begin_query("(switch) global ID:",create_switch_global)
  
def edit_switch_method(method):
  m.switchv[switch_edit_p]=m.switchv[switch_edit_p][:5]+(method,)
  r.dirty.add(m)
  
def edit_switch_global(globalid):
  m.switchv[switch_edit_p]=m.switchv[switch_edit_p][:4]+(globalid,)+m.switchv[switch_edit_p][5:]
  r.dirty.add(m)
  begin_query("(switch) method [%d]:"%(m.switchv[switch_edit_p][5]),edit_switch_method)
  
def edit_switch(p):
  global switch_edit_p
  switch_edit_p=p
  begin_query("(switch) global ID [%d]:"%(m.switchv[p][4]),edit_switch_global)
  
def cb_bgimageid(imgid):
  global bgtiles,sprtiles
  if not imgid in r.tilesheet: return show_message("no such tilesheet")
  m.bgimageid=imgid
  r.dirty.add(m)
  vg.DestroyImage(bgtiles)
  for img in sprtiles: vg.DestroyImage(img)
  bgtiles,sprtiles=load_tiles()
  drawbg()
  
def cb_sprimageid(imgid):
  global bgtiles,sprtiles
  if not imgid in r.tilesheet: return show_message("no such tilesheet")
  m.sprimageid=imgid
  r.dirty.add(m)
  vg.DestroyImage(bgtiles)
  for img in sprtiles: vg.DestroyImage(img)
  bgtiles,sprtiles=load_tiles()
  drawbg()
  
def cb_lights_out(flag):
  m.lights_out=1 if flag else 0
  r.dirty.add(m)
  
def cb_song(song):
  if song<0 or song>99: return show_message("illegal song id")
  m.song=song
  r.dirty.add(m)
    
def char_from_keycode(code):
  """Luckily, for now at least, we only accept decimal integers as input."""
  if code==linput.KEY_0: return "0"
  if code==linput.KEY_1: return "1"
  if code==linput.KEY_2: return "2"
  if code==linput.KEY_3: return "3"
  if code==linput.KEY_4: return "4"
  if code==linput.KEY_5: return "5"
  if code==linput.KEY_6: return "6"
  if code==linput.KEY_7: return "7"
  if code==linput.KEY_8: return "8"
  if code==linput.KEY_9: return "9"
  return None

def update_query():
  global quit,query_input
  for devid,ev,code,value in linput.update(1.0):
    if ev==linput.EV_KEY and value:
      if code==linput.KEY_ESC: quit=True
      elif code==linput.KEY_ENTER: end_query()
      elif code==linput.KEY_BACKSPACE and query_input is not None:
        if len(query_input)>0: query_input=query_input[:-1]
      elif query_input is not None:
        ch=char_from_keycode(code)
        if ch is not None: query_input+=ch

def update_main():
  global quit,drawing,cursorx,cursory,show_grid,show_sprites,show_doors,drag_sprite,drag_door,drag_entry,shift,show_switches,drag_switch
  mousemoved=False
  for devid,ev,code,value in linput.update(1.0):
    if ev==linput.EV_KEY:
      if code==linput.KEY_ESC: quit=True
      elif code==linput.KEY_SPACE:
        if value: toggle_tile_palette()
      elif code==linput.KEY_F1 and value: show_grid=not show_grid
      elif code==linput.KEY_F2 and value: show_sprites=not show_sprites
      elif code==linput.KEY_F3 and value: show_doors=not show_doors
      elif code==linput.KEY_F4 and value: show_switches=not show_switches
      elif code==linput.KEY_F5 and value: begin_query("Background tilesheet [%d]:"%(m.bgimageid),cb_bgimageid)
      elif code==linput.KEY_F7 and value: begin_query("Sprite tilesheet [%d]:"%(m.sprimageid),cb_sprimageid)
      elif code==linput.KEY_F8 and value: begin_query("Lights-out effect [%d]:"%(m.lights_out),cb_lights_out)
      elif code==linput.KEY_F9 and value: begin_query("Song [%d]:"%(m.song),cb_song)
      elif code==linput.KEY_F10 and value: show_message("This is map ID %d"%(m.mapid))
      elif code==linput.KEY_S and value: r.save()
      elif code==linput.KEY_UP and value: navigate(m.north,shift,0,-1)
      elif code==linput.KEY_DOWN and value: navigate(m.south,shift,0,1)
      elif code==linput.KEY_LEFT and value: navigate(m.west,shift,-1,0)
      elif code==linput.KEY_RIGHT and value: navigate(m.east,shift,1,0)
      elif code==linput.KEY_LEFTSHIFT: shift=bool(value)
      elif code==linput.KEY_BACKSPACE and value: delete()
      elif code==linput.KEY_D and value and show_doors: create_door()
      elif code==linput.KEY_ENTER and value and show_sprites: create_sprite()
      elif code==linput.KEY_W and value and show_switches: create_switch()
      elif code==linput.BTN_RIGHT:
        if value:
          if show_tile_palette: choose_tile_palette()
          else: right_mouse_down()
      elif code==linput.BTN_LEFT:
        if show_tile_palette:
          if value: choose_tile_palette()
        else:
          if value: mouse_down()
          else:
            drawing=False
            drag_sprite=drag_door=drag_entry=drag_switch=None
          if drawing: update_drawing()
    elif ev==linput.EV_REL:
      if code==linput.REL_X: 
        cursorx+=value
        mousemoved=True
      elif code==linput.REL_Y: 
        cursory+=value
        mousemoved=True
      elif code==linput.REL_WHEEL: adjust_selected_cell(16 if value<0 else -16)
      elif code==linput.REL_HWHEEL: adjust_selected_cell(-1 if value<0 else 1)
  if (drawing or drag_sprite is not None or drag_door is not None or drag_entry is not None or drag_switch is not None) and mousemoved: update_drawing()

quit=False
drawing=False
shift=False
try:
  while not quit:
    if query_prompt is not None: update_query()
    else: update_main()
    draw()
except KeyboardInterrupt:
  pass
  
r.save()
pig.quit()
linput.quit()
