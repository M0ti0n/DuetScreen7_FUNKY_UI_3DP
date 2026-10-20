"""Bakes the home scene runtime art into assets/workshop/ (1024 x 600):

  home_base.png                 the scene without printer, wall bed and spare bed; the laptop
                                shows the console
  scene/wall_bed_{square,round}.png, scene/spare_bed_{square,round}.png
  printers/<style>.png          printer stickers with their own shadow (corexy, delta, bedslinger)
  printers/<style>_part.png, _ghost.png, _outline.png   the print on the printer's bed
  layers/folder_{square,round}.png, layers/bed_*.png (coloured prints)
and prints the positions for src/UI/Widgets/Workshop/WorkshopLayout.h.
"""
import os, math, json
import numpy as np, cv2
from PIL import Image, ImageDraw, ImageFilter
HERE=os.path.dirname(os.path.abspath(__file__))+'/'
src=open(HERE+'mockups.py').read().split("scenes=[")[0]
g={'__file__':HERE+'mockups.py'}; exec(src,g)
ART=g['ART']; pos=g['pos']; O=HERE; SRC=HERE+'source/'
R=os.path.abspath(ART+'..')+'/'
A=R+'assets/workshop/'
for d in ('scene','printers','layers'): os.makedirs(A+d,exist_ok=True)
MW,MH=1336,784; SC=MW/1024
INK=(22,20,18,255)

def canvas(): return Image.new('RGBA',(MW,MH),(0,0,0,0))
def obj(name): return Image.open(ART+f'objects/{name}.png').convert('RGBA')
def place(c,name):
    p=pos[name]; c.alpha_composite(obj(name),(p['x'],p['y']))
def alpha_of(c): return np.asarray(c.getchannel('A')).astype(np.float32)/255
def shadow_layer(alpha):
    s=Image.new('RGBA',(MW,MH),(0,0,0,0)); s.putalpha(Image.fromarray(np.clip(alpha*255,0,255).astype(np.uint8))); return s
def holes(c,*covers):
    a=np.asarray(c).copy().astype(np.float32)
    for cov in covers: a[...,3]*=1-alpha_of(cov)
    return Image.fromarray(a.astype(np.uint8),'RGBA')
def small(c): return c.resize((1024,600),Image.LANCZOS)
LAYOUT={}
def save_layer(c,path,key):
    s=small(c); bb=s.getchannel('A').point(lambda v:255 if v>2 else 0).getbbox()
    s.crop(bb).save(A+path,optimize=True)
    LAYOUT[key]=dict(file=path,x=bb[0],y=bb[1],w=bb[2]-bb[0],h=bb[3]-bb[1])
    return bb

# ---- the shadow layer, split per object ----
S=Image.open(ART+'objects/shadows.png').convert('RGBA'); Sa=alpha_of(S)
def region(poly_or_rect,blur):
    m=Image.new('L',(MW,MH),0); d=ImageDraw.Draw(m)
    if len(poly_or_rect)==4 and not isinstance(poly_or_rect[0],tuple): d.rectangle(poly_or_rect,fill=255)
    else: d.polygon(poly_or_rect,fill=255)
    return np.asarray(m.filter(ImageFilter.GaussianBlur(blur))).astype(np.float32)/255
r_wall=region([330,22,632,302],4)
r_spare=region([1046,505,1336,652],4)
r_printer=region([(648,466),(1150,466),(1150,540),(1062,540),(1062,610),(648,610)],6)*(1-r_spare)
sh_wall=Sa*r_wall; sh_spare=Sa*r_spare; sh_printer=Sa*r_printer
sh_rest=Sa*(1-r_wall)*(1-r_spare)*(1-r_printer)

# ---- objects in front of the printer (they cut holes in the stickers) ----
front=canvas(); place(front,'laptop'); place(front,'rack')
writing=canvas(); place(writing,'writing')

# ---- base: everything but printer and beds ----
base=Image.open(ART+'01_clean_desk_5_drawers.png').convert('RGBA')
base.alpha_composite(shadow_layer(sh_rest))
for k in ['writing','spools','holder','rack','laptop']: place(base,k)
removed=np.maximum.reduce([sh_wall,sh_spare,sh_printer])
rm=canvas(); rm.alpha_composite(obj('wall_bed'),(338,29)); place(rm,'printer'); place(rm,'bed'); place(rm,'cables')
mask=np.maximum(removed,alpha_of(rm))
mask=cv2.dilate((mask>0.01).astype(np.uint8)*255,np.ones((5,5),np.uint8))
mask=cv2.resize(mask,(1024,600),interpolation=cv2.INTER_AREA).astype(np.float32)/255
mask=np.clip(mask*1.5,0,1)[...,None]
idle=np.asarray(Image.open(A+'home_idle.png').convert('RGB')).astype(np.float32)
red=np.asarray(small(base).convert('RGB')).astype(np.float32)
b=idle*(1-mask)+red*mask

# laptop screen: the console, fitted inside the frame (left frame kept even)
con=np.asarray(Image.open(O+'source/laptop_console.png').convert('RGB')).astype(np.float32)
h,w=con.shape[:2]
quad=(np.float32([[151,15],[368,15],[343,169],[121,155]])+np.float32([pos['laptop']['x'],pos['laptop']['y']]))/SC
K=4
M=cv2.getPerspectiveTransform(np.float32([[0,0],[w,0],[w,h],[0,h]]),quad*K)
big=cv2.resize(cv2.warpPerspective(con,M,(1024*K,600*K),flags=cv2.INTER_AREA),(1024,600),interpolation=cv2.INTER_AREA)
m=cv2.resize(cv2.warpPerspective(np.ones((h,w),np.float32),M,(1024*K,600*K)),(1024,600),interpolation=cv2.INTER_AREA)[...,None]
b=b*(1-m)+(big*np.linspace(1.0,0.88,600)[:,None,None])*m
edge=np.zeros((600*K,1024*K),np.uint8); cv2.polylines(edge,[(quad*K).astype(np.int32)],True,255,int(1.2*K))
e=cv2.resize(edge.astype(np.float32)/255,(1024,600),interpolation=cv2.INTER_AREA)[...,None]
b=b*(1-e*0.85)+np.array([18,18,20])*e*0.85
Image.fromarray(np.clip(b,0,255).astype(np.uint8)).save(A+'home_base.png',optimize=True)

# ---- wall beds ----
c=canvas(); c.alpha_composite(shadow_layer(sh_wall)); c.alpha_composite(obj('wall_bed'),(338,29))
save_layer(holes(c,writing),'scene/wall_bed_square.png','WALL_BED_SQUARE')
c=canvas(); c.alpha_composite(Image.open(O+'wall_bed_round.png').convert('RGBA'),(338,29))
save_layer(holes(c,writing),'scene/wall_bed_round.png','WALL_BED_ROUND')

# ---- spare beds ----
c=canvas(); c.alpha_composite(shadow_layer(sh_spare)); place(c,'bed'); place(c,'cables')
save_layer(c,'scene/spare_bed_square.png','SPARE_BED_SQUARE')
c=canvas()
m=Image.new('L',(MW,MH),0); ImageDraw.Draw(m).ellipse([1062,560,1336,646],fill=140)
c.alpha_composite(shadow_layer(np.asarray(m.filter(ImageFilter.GaussianBlur(7))).astype(np.float32)/255))
g['draw_cable'](c); c.alpha_composite(Image.open(O+'bed_round.png').convert('RGBA'),(pos['bed']['x'],pos['bed']['y']))
save_layer(c,'scene/spare_bed_round.png','SPARE_BED_ROUND')

# ---- printers ----
DSP=g['cxy_display']()                 # the CoreXY's display, shared by all printers
def blue_glass(rgba):
    """The blue glass of a display picture, as a filled convex shape (alpha 0/255)"""
    a=np.asarray(rgba).astype(int); r,g_,b=a[...,0],a[...,1],a[...,2]
    m=((b-r>18)&(b>40)&(a[...,3]>200)).astype(np.uint8)*255
    m=cv2.morphologyEx(m,cv2.MORPH_OPEN,np.ones((3,3),np.uint8))
    cnts,_=cv2.findContours(m,cv2.RETR_EXTERNAL,cv2.CHAIN_APPROX_NONE)
    out=np.zeros(m.shape,np.uint8)
    if cnts: cv2.fillConvexPoly(out,cv2.convexHull(max(cnts,key=cv2.contourArea)),255)
    return out
GLASS=blue_glass(DSP)
def glass_layer(mask_master,key,path):
    """mask in master coordinates (full canvas) -> white glass sprite for the app to tint"""
    c=canvas(); w=np.zeros((MH,MW,4),np.uint8); w[...,:3]=255; w[...,3]=mask_master; c=Image.fromarray(w,'RGBA')
    return save_layer(c,path,key)
def dsp_glass_at(dx,dy):
    m=np.zeros((MH,MW),np.uint8); h,w=GLASS.shape; m[dy:dy+h,dx:dx+w]=GLASS; return m
# its glass in the crop (master px): the app draws the screen there
GL=(589*SC-639-112, 382*SC-66-420, 118*SC, 54*SC)
def display_rect(dx,dy,k):
    x=(dx+GL[0]*k)/SC; y=(dy+GL[1]*k)/SC
    return dict(x=round(x),y=round(y),w=round(118*k),h=round(54*k))
def paste_display(c,dx,dy,k=1.0,shadow=True):
    d=DSP.resize((round(DSP.width*k),round(DSP.height*k)),Image.LANCZOS)
    if shadow:
        sh=Image.new('L',(MW,MH),0); sh.paste(d.getchannel('A'),(dx+6,dy+8)); sh=sh.filter(ImageFilter.GaussianBlur(6))
        c.alpha_composite(shadow_layer(np.asarray(sh).astype(np.float32)/255*0.55))
    c.alpha_composite(d,(dx,dy))
STYLES={}
# CoreXY
c=canvas(); c.alpha_composite(shadow_layer(sh_printer)); place(c,'printer')
save_layer(holes(c,front),'printers/corexy.png','PRINTER_COREXY')
glass_layer(dsp_glass_at(pos['printer']['x']+112,pos['printer']['y']+420),'GLASS_COREXY','printers/corexy_glass.png')
STYLES['corexy']=dict(display=dict(x=589,y=382,w=118,h=54),part=dict(x=576,y=223,w=197,h=127))
# Delta
spr=g['delta_sprite'](); f=548/spr.height; W=round(spr.width*f)
spr=spr.resize((W,548),Image.LANCZOS); x0,y0=890-W//2,588-548
c=canvas()
def cs(poly,blur):
    m=Image.new('L',(MW,MH),0); ImageDraw.Draw(m).polygon(poly,fill=140)
    c.alpha_composite(shadow_layer(np.asarray(m.filter(ImageFilter.GaussianBlur(blur))).astype(np.float32)/255))
cs([(x0+20,y0+548-95),(x0+W-10,y0+548-95),(x0+W+5,y0+548-2),(x0+5,y0+548-2)],12)
c.alpha_composite(spr,(x0,y0))
save_layer(holes(c,front),'printers/delta.png','PRINTER_DELTA')
gm=cv2.imread(SRC+'glass_mask.png',0)
cnts,_=cv2.findContours(gm,cv2.RETR_EXTERNAL,cv2.CHAIN_APPROX_NONE)
hull=np.zeros_like(gm); cv2.fillConvexPoly(hull,cv2.convexHull(max(cnts,key=cv2.contourArea)),255)
bbm=Image.open(SRC+'delta_mask.png').getbbox()
hm=Image.fromarray(hull).crop(bbm).resize((W,548),Image.LANCZOS)
m=np.zeros((MH,MW),np.uint8); m[y0:y0+548,x0:x0+W]=np.where(np.asarray(hm)>127,255,0)
glass_layer(m,'GLASS_DELTA','printers/delta_glass.png')
# its own display: the app fills the glass (inside the frame) and centres the screen in it
gx0,gy0,gx1,gy1=x0+(410-78)*f, y0+(1003-5)*f, x0+(770-78)*f, y0+(1196-5)*f
bedc=(x0+(620-78)*f, y0+(848-5)*f)                     # centre of the round bed (master)
pw=150; ph=pw*127/197
STYLES['delta']=dict(display=dict(x=round(gx0/SC),y=round(gy0/SC),w=round((gx1-gx0)/SC),h=round((gy1-gy0)/SC)),part=dict(x=round((bedc[0]-pw/2)/SC),y=round((bedc[1]-ph)/SC),w=round(pw/SC),h=round(ph/SC)))
# Bed slinger
spr=Image.open(O+'bedslinger.png'); f=470/spr.height; W=round(spr.width*f)
spr=spr.resize((W,470),Image.LANCZOS); x0,y0=900-W//2,590-470
c=canvas()
a=np.asarray(spr.getchannel('A')).copy(); a[:int(470*0.7)]=0
m=Image.new('L',(MW,MH),0); m.paste(Image.fromarray((a*0.5).astype(np.uint8)),(x0+10,y0+10))
c.alpha_composite(shadow_layer(np.asarray(m.filter(ImageFilter.GaussianBlur(9))).astype(np.float32)/255))
c.alpha_composite(spr,(x0,y0))
dx=x0+int(W*0.33)-DSP.width//2; dy=y0+470-DSP.height-2
paste_display(c,dx,dy)
save_layer(holes(c,front),'printers/bedslinger.png','PRINTER_BEDSLINGER')
glass_layer(dsp_glass_at(dx,dy),'GLASS_BEDSLINGER','printers/bedslinger_glass.png')
# bed of the slinger in its picture: centre (420, 646) of the 1152 x 912 drawing, cropped to the sprite
bb=json.load(open(O+'source/bedslinger_crop.json'))
bx,by=x0+(420-bb[0])*f, y0+(646-bb[1])*f
pw=170; ph=pw*127/197
STYLES['bedslinger']=dict(display=display_rect(dx,dy,1.0),part=dict(x=round((bx-pw/2)/SC),y=round((by-ph)/SC),w=round(pw/SC),h=round(ph/SC)))

# ---- the print on the printer's bed: part, faint red "still to print", red outline ----
RED=(206,52,38)
part=obj('print_part')
for st,v in STYLES.items():
    p=v['part']; im=part.resize((p['w'],p['h']),Image.LANCZOS); im.save(A+f'printers/{st}_part.png',optimize=True)
    pa=np.asarray(im.getchannel('A'))
    ghost=np.zeros((p['h'],p['w'],4),np.uint8); ghost[pa>100]=RED+(38,)
    Image.fromarray(ghost,'RGBA').save(A+f'printers/{st}_ghost.png',optimize=True)
    ring=cv2.morphologyEx((pa>100).astype(np.uint8)*255,cv2.MORPH_GRADIENT,np.ones((3,3),np.uint8))
    o=np.zeros((p['h'],p['w'],4),np.uint8); o[ring>0]=RED+(255,)
    Image.fromarray(o,'RGBA').save(A+f'printers/{st}_outline.png',optimize=True)

# ---- folder on the spare bed, coloured prints ----
fb=open(HERE+'folder.py').read(); fg={'__file__':HERE+'folder.py'}; exec(fb,fg)
bp=pos['bed']
SQ=np.float32([[100,4],[282,10],[226,88],[3,80]])+np.float32([bp['x'],bp['y']])
cx,cy,rx,ry=bp['x']+141,bp['y']+44,134,38
RQ=np.float32([[cx-0.62*rx,cy-0.70*ry],[cx+0.62*rx,cy-0.70*ry],[cx+0.74*rx,cy+0.72*ry],[cx-0.74*rx,cy+0.72*ry]])
for name,q in (('square',SQ),('round',RQ)):
    save_layer(fg['folder_on'](q,(MW,MH)),f'layers/folder_{name}.png','FOLDER_'+name.upper())
COLOURS={'bed_boat':(52,118,222),'bed_spinner':(146,210,48),'bed_bar':(244,132,40),'bed_cube':(200,64,64)}
for n,col in COLOURS.items():
    im=obj(n); a=np.asarray(im).astype(float)
    lum=(0.3*a[...,0]+0.59*a[...,1]+0.11*a[...,2])/255.0
    cc=np.array(col,float)
    rgb=np.where(lum[...,None]<0.22, a[...,:3]*0.8, cc*np.clip(0.55+0.75*(lum[...,None]-0.22)/0.6,0,1.25))
    out=Image.fromarray(np.dstack([np.clip(rgb,0,255),a[...,3]]).astype(np.uint8),'RGBA')
    size=Image.open(A+f'layers/{n}.png').size
    out.resize(size,Image.LANCZOS).save(A+f'layers/{n}.png',optimize=True)

LAYOUT['styles']=STYLES
json.dump(LAYOUT,open(HERE+'layout.json','w'),indent=1)
print(json.dumps(LAYOUT,indent=1))
