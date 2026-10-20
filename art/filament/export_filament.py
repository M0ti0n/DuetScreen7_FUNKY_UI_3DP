"""Writes assets/workshop/filament/: the filament page (the MMU box on the desk) and its sprites,
and layout.json (positions for src/UI/Widgets/FilamentBox/FilamentLayout.h).

  background.png        desk, drawers, fire bell, house picture, PTFE tubes, the box with four
                        empty bays and blank screens
  bay<N>_spool.png      the spool of bay N (its filament area is see-through)
  bay<N>_fil.png        the filament area in white: the app tints it with the slot's colour
  bay<N>_shade.png      the filament's shading, laid over the tint
  bay<N>_empty.png      an empty spool (cardboard core) in the filament area
  bay<N>_glow.png       the red glow around the screen (no filament)
  plate.png             "< 1 / 2 >" plate on the top rail (the app writes the page)
  arrow_next.png, arrow_back.png   wooden arrows hanging under the desk
  icon_*.png            white icons, tinted by the app
"""
import os, math, json
import numpy as np, cv2
from PIL import Image, ImageDraw, ImageFont, ImageFilter
HERE=os.path.dirname(os.path.abspath(__file__))+'/'
R=os.path.abspath(HERE+'../..')+'/'
ART=R+'art/'; FONTS=R+'assets/fonts/'
OUT=R+'assets/workshop/filament/'; os.makedirs(OUT,exist_ok=True)
INK=(22,20,18,255)
MW,MH=1336,784
S1024=1024/MW
LAYOUT={}

def font(path,size,weight=None):
    f=ImageFont.truetype(path,size)
    if weight:
        try: f.set_variation_by_axes([weight])
        except Exception: pass
    return f

# ---------- the scene ----------
base=Image.open(ART+'01_clean_desk_5_drawers.png').convert('RGBA')
def house_picture():
    K=4; w,h=118,100
    im=Image.new('RGBA',(w*K,h*K),(0,0,0,0)); d=ImageDraw.Draw(im)
    def P(x,y): return (x*K,y*K)
    def poly(pts,fill,wd=2.6):
        pp=[P(*p) for p in pts]; d.polygon(pp,fill=fill); d.line(pp+[pp[0]],fill=INK,width=int(wd*K),joint='curve')
    poly([(2,6),(116,4),(116,96),(3,97)],(150,96,52,255),3)
    poly([(11,14),(107,13),(107,88),(11,89)],(170,212,236,255),2)
    d.rectangle([P(12,62),P(106,88)],fill=(126,186,84,255))
    d.ellipse([P(80,18),P(96,34)],fill=(250,212,84,255),outline=INK,width=K*2)
    poly([(34,48),(78,48),(78,78),(34,78)],(238,226,196,255))
    poly([(28,50),(56,26),(84,50)],(198,72,52,255))
    poly([(70,36),(76,36),(76,44),(70,40)],(150,96,70,255),2)
    poly([(50,60),(60,60),(60,78),(50,78)],(150,96,52,255),2)
    poly([(38,56),(46,56),(46,64),(38,64)],(170,212,236,255),2)
    poly([(64,56),(72,56),(72,64),(64,64)],(170,212,236,255),2)
    d.line([P(12,78),P(106,78)],fill=INK,width=K*2)
    d.line([P(59,-0),P(20,6)],fill=INK,width=K*2); d.line([P(59,0),P(98,5)],fill=INK,width=K*2)
    im=im.resize((w,h),Image.LANCZOS)
    pic=Image.new('RGBA',(w+20,h+20),(0,0,0,0))
    sh=Image.new('L',pic.size,0); ImageDraw.Draw(sh).rectangle([8,12,w+6,h+10],fill=90); sh=sh.filter(ImageFilter.GaussianBlur(4))
    s=Image.new('RGBA',pic.size,(0,0,0,0)); s.putalpha(sh); pic.alpha_composite(s); pic.alpha_composite(im,(4,4))
    return pic.rotate(3,resample=Image.BICUBIC,expand=True)
pic=house_picture()
base.alpha_composite(pic.resize((round(pic.width*1.1),round(pic.height*1.1)),Image.LANCZOS),(1190,30))

# ---------- the MMU box ----------
src=np.asarray(Image.open(HERE+'source/mmu_reference.png').convert('RGB'))
m=cv2.imread(HERE+'source/mmu_mask.png',0)
hsv=cv2.cvtColor(src,cv2.COLOR_RGB2HSV)
wall=(hsv[...,1]<40)&(hsv[...,2]>150); hm=np.zeros_like(m,bool); hm[12:48,575:775]=True
m[hm&wall]=0
img=src.astype(np.float32)
SLOTS=[262,475,686,894]
def screen_rect(i): x=SLOTS[i]; return (x,328,x+184,543)
SPOOLS=[(284,446),(493,645),(690,850),(893,1055)]
def fil_rect(i): a,b=SPOOLS[i]; return (a,124,b,314)
def bay_rect(i): a,b=SPOOLS[i]; return (a-10,104,b+10,316)
def clean(mm):
    mm=cv2.morphologyEx(mm.astype(np.uint8)*255,cv2.MORPH_OPEN,np.ones((3,3),np.uint8))
    mm=cv2.morphologyEx(mm,cv2.MORPH_CLOSE,np.ones((7,7),np.uint8))
    n,lab,st,_=cv2.connectedComponentsWithStats(mm); big=1+np.argmax(st[1:,cv2.CC_STAT_AREA])
    mm=np.where(lab==big,255,0).astype(np.uint8)
    ff=mm.copy(); mk=np.zeros((mm.shape[0]+2,mm.shape[1]+2),np.uint8); cv2.floodFill(ff,mk,(0,0),255); return (mm|(255-ff))>0
x0,y0,x1,y1=fil_rect(1); h=hsv[y0:y1,x0:x1]
ROWS=np.where(clean((h[...,0]>100)&(h[...,0]<135)&(h[...,1]>110)).any(axis=1))[0]
def span_mask(i):
    x0,y0,x1,y1=fil_rect(i); h=hsv[y0:y1,x0:x1]
    wood=(h[...,0]>5)&(h[...,0]<28)&(h[...,1]>70)&(h[...,2]>80)
    w=wood.shape[1]; mm=np.zeros(wood.shape,bool)
    for r in ROWS:
        cols=np.where(wood[r])[0]
        lc=cols[cols<w*0.35]; rc=cols[cols>w*0.65]
        if len(lc)==0 or len(rc)==0: continue
        mm[r,lc.max()+1:rc.min()]=True
    mm=cv2.morphologyEx(mm.astype(np.uint8),cv2.MORPH_CLOSE,np.ones((5,1),np.uint8))>0
    return mm&~wood
MASKS={i:span_mask(i) for i in range(4)}
ORIG=img.copy()
def no_spool(i):
    x0,y0,x1,y1=bay_rect(i)
    t=np.linspace(0,1,y1-y0)[:,None,None]
    img[y0:y1,x0:x1]=np.array([10,13,15],np.float32)*(1-t)+np.array([15,18,20],np.float32)*t
    for rx in (x0+8,x1-14):
        img[y0:y1,rx:rx+6]=np.array([20,23,26],np.float32); img[y0:y1,rx:rx+1]=4
    ay=226; img[ay-11:ay+11,x0:x1]=np.array([24,25,27],np.float32)
    img[ay-11:ay-7,x0:x1]=np.array([70,72,76],np.float32); img[ay-12:ay-11,x0:x1]=3; img[ay+11:ay+12,x0:x1]=3
def blank_screen(i):
    x0,y0,x1,y1=screen_rect(i)
    t=np.linspace(0,1,y1-y0)[:,None,None]
    img[y0:y1,x0:x1]=np.array((10,26,48),np.float32)*(1.1-0.4*t)+np.array([4,8,14],np.float32)
for i in range(4):
    no_spool(i); blank_screen(i)

bb=Image.fromarray(m).getbbox()
K_BOX=632/(bb[3]-bb[1]); BW=round((bb[2]-bb[0])*K_BOX)
BX,BY=(MW-BW)//2,8
def to_screen(x,y):
    """box picture coordinates -> 1024 x 600 screen"""
    return ((BX+(x-bb[0])*K_BOX)*S1024,(BY+(y-bb[1])*K_BOX)*S1024)
def rect_to_screen(r):
    a=to_screen(r[0],r[1]); b=to_screen(r[2],r[3])
    return dict(x=round(a[0]),y=round(a[1]),w=round(b[0]-a[0]),h=round(b[1]-a[1]))
def box_layer(rgba,name,key):
    """an RGBA layer in box picture coordinates -> a sprite on the screen (cropped), like the box"""
    lay=Image.fromarray(rgba,'RGBA').crop(bb).resize((BW,632),Image.LANCZOS)
    c=Image.new('RGBA',(MW,MH),(0,0,0,0)); c.alpha_composite(lay,(BX,BY))
    s=c.resize((1024,600),Image.LANCZOS)
    box_=s.getchannel('A').point(lambda v:255 if v>2 else 0).getbbox()
    s.crop(box_).save(OUT+name,optimize=True)
    LAYOUT[key]=dict(file='filament/'+name,x=box_[0],y=box_[1],w=box_[2]-box_[0],h=box_[3]-box_[1])
    return box_

box=Image.fromarray(np.clip(img,0,255).astype(np.uint8)).convert('RGBA')
box.putalpha(Image.fromarray(m))
box=box.crop(bb).resize((BW,632),Image.LANCZOS)
sh=Image.new('L',(MW,MH),0); ImageDraw.Draw(sh).rectangle([BX+20,BY+30,BX+BW+12,BY+632+8],fill=120)
sh=sh.filter(ImageFilter.GaussianBlur(14)); s=Image.new('RGBA',(MW,MH),(0,0,0,0)); s.putalpha(sh); base.alpha_composite(s)
# PTFE tubes from behind the box
K=4; tl=Image.new('RGBA',(MW*K//2,MH*K//2),(0,0,0,0)); td=ImageDraw.Draw(tl)
def tube(y0,amp,ph,col):
    pts=[]
    for i in range(80):
        t=i/79; x=1110+t*240; y=y0+amp*math.sin(t*math.pi*2.2+ph)+t*10
        pts.append((x*K/2,y*K/2))
    td.line(pts,fill=(22,20,18,255),width=int(7*K/2),joint='curve')
    td.line(pts,fill=col,width=int(4*K/2),joint='curve')
    td.line([(x,y-1.2*K/2) for x,y in pts],fill=(255,255,255,200),width=int(1*K/2),joint='curve')
for (y0,amp,ph) in ((548,6,0.0),(560,7,1.3),(572,5,2.4),(584,6,0.7)):
    tube(y0,amp,ph,(214,222,226,235))
tl=tl.resize((MW,MH),Image.LANCZOS)
sh4=Image.new('L',(MW,MH),0); sh4.paste(tl.getchannel('A').point(lambda v:int(v*0.4)),(3,6)); s4=Image.new('RGBA',(MW,MH),(0,0,0,0)); s4.putalpha(sh4.filter(ImageFilter.GaussianBlur(3)))
base.alpha_composite(s4); base.alpha_composite(tl)
base.alpha_composite(box,(BX,BY))

out=base.convert('RGB').resize((1024,600),Image.LANCZOS)
hb=Image.open(R+'assets/workshop/home_base.png').convert('RGB')
out.paste(hb.crop((0,0,98,90)),(0,0))
out.paste(out.crop((93,74,128,116)),(93,29))
o=out.convert('RGBA')
fh=font(FONTS+'PlaypenSans.ttf',19,600); INKN=(34,32,30,235)
nt=Image.new('RGBA',o.size,(0,0,0,0)); nd=ImageDraw.Draw(nt)
for k,(t,dx) in enumerate((('EMER-',8),('GENCY',4),('CALL',14),('M 112',10))):
    nd.text((14+dx,124+k*22),t,font=fh,fill=INKN)
nd.line([(48,118),(46,100),(40,88)],fill=INKN,width=3,joint='curve')
nd.line([(34,96),(40,86),(47,95)],fill=INKN,width=3,joint='curve')
o.alpha_composite(nt.rotate(4,center=(50,160),resample=Image.BICUBIC))
nt=Image.new('RGBA',o.size,(0,0,0,0)); nd=ImageDraw.Draw(nt)
for k,(t,dx) in enumerate((('HOME',10),('SWEET',4),('HOME',14))):
    nd.text((916+dx,138+k*22),t,font=fh,fill=INKN)
nd.line([(950,134),(952,118),(958,106)],fill=INKN,width=3,joint='curve')
nd.line([(951,112),(958,104),(963,113)],fill=INKN,width=3,joint='curve')
o.alpha_composite(nt.rotate(-4,center=(960,170),resample=Image.BICUBIC))
o.convert('RGB').save(OUT+'background.png',optimize=True)

# ---------- per bay ----------
H,W=img.shape[:2]
for i in range(4):
    bx0,by0,bx1,by1=bay_rect(i); fx0,fy0,fx1,fy1=fil_rect(i)
    fm=np.zeros((H,W),bool); fm[fy0:fy1,fx0:fx1]=MASKS[i]
    # spool: the original bay, filament area see-through
    a=np.zeros((H,W,4),np.uint8); a[by0:by1,bx0:bx1,:3]=ORIG[by0:by1,bx0:bx1]; a[by0:by1,bx0:bx1,3]=255; a[fm,3]=0
    a[...,3]=np.minimum(a[...,3],m)
    box_layer(a,f'bay{i}_spool.png',f'BAY{i}_SPOOL')
    # filament, white
    a=np.zeros((H,W,4),np.uint8); a[fm]=(255,255,255,255); box_layer(a,f'bay{i}_fil.png',f'BAY{i}_FIL')
    # shading: dark where the original is dark, white highlights
    L=ORIG[...,:3].mean(axis=2); vals=L[fm]; lo,hi=np.percentile(vals,5),np.percentile(vals,97)
    Ln=np.clip((L-lo)/(hi-lo+1e-3),0,1)
    a=np.zeros((H,W,4),np.uint8)
    dark=(0.72*(1-Ln)*255).astype(np.uint8); hl=(np.clip(Ln-0.85,0,1)/0.15*150).astype(np.uint8)
    a[...,3]=np.where(hl>0,hl,dark); a[...,:3]=np.where((hl>0)[...,None],255,0)
    a[~fm]=0
    box_layer(a,f'bay{i}_shade.png',f'BAY{i}_SHADE')
    # empty spool: dark inside, cardboard core
    a=np.zeros((H,W,4),np.uint8)
    hh=fy1-fy0; yy=np.arange(hh)[:,None]; cy=hh/2
    reg=np.broadcast_to(np.array([24,26,30],np.float32),(hh,fx1-fx0,3)).copy()
    core=np.abs(yy-cy)<hh*0.17
    shade=(1.15-0.5*np.abs(yy-cy)/(hh*0.17))
    corec=np.array([176,134,86],np.float32)*np.clip(shade,0.5,1.2)[...,None]
    reg=np.where(core[...,None],np.broadcast_to(corec,reg.shape),reg)
    edge=np.abs(np.abs(yy-cy)-hh*0.17)<1.5
    reg=np.where(edge[...,None],np.array([22,20,18],np.float32),reg)
    a[fy0:fy1,fx0:fx1,:3]=reg.astype(np.uint8); a[fm,3]=255; a[~fm,3]=0
    box_layer(a,f'bay{i}_empty.png',f'BAY{i}_EMPTY')
    # red glow around the screen
    x0,y0,x1,y1=screen_rect(i)
    gl=Image.new('L',(W,H),0); ImageDraw.Draw(gl).rounded_rectangle([x0-6,y0-6,x1+6,y1+6],radius=10,outline=255,width=10)
    gl=np.asarray(gl.filter(ImageFilter.GaussianBlur(8))).astype(np.float32)*0.8
    a=np.zeros((H,W,4),np.uint8); a[...,0]=255; a[...,1]=60; a[...,2]=40; a[...,3]=np.minimum(gl,m).astype(np.uint8)
    box_layer(a,f'bay{i}_glow.png',f'BAY{i}_GLOW')
    LAYOUT[f'SCREEN{i}']=rect_to_screen(screen_rect(i))

# rows on a screen (box picture units, relative to the screen's corner) -> screen pixels
def rel(r):
    sx0,sy0=to_screen(SLOTS[0],328); a=to_screen(SLOTS[0]+r[0],328+r[1]); b=to_screen(SLOTS[0]+r[2],328+r[3])
    return dict(x=round(a[0]-sx0),y=round(a[1]-sy0),w=round(b[0]-a[0]),h=round(b[1]-a[1]))
LAYOUT['ROW_TOP']=rel((0,8,184,60))
LAYOUT['ROW_MATERIAL']=rel((10,70,174,126))
LAYOUT['ROW_BUTTON']=rel((10,146,174,205))

# ---------- icons (white, tinted by the app), drawn at 4x then scaled to the screen ----------
F=K_BOX*S1024
def icon(name,w,h,draw):
    K=6; im=Image.new('RGBA',(round(w*K),round(h*K)),(0,0,0,0)); d=ImageDraw.Draw(im); draw(d,K)
    im.resize((max(1,round(w*F)),max(1,round(h*F))),Image.LANCZOS).save(OUT+'icon_'+name+'.png',optimize=True)
W_=(255,255,255,255)
def thermo(d,K):
    d.rounded_rectangle([8*K,2*K,16*K,24*K],radius=4*K,outline=W_,width=2*K); d.ellipse([4*K,18*K,20*K,34*K],outline=W_,width=2*K)
    d.ellipse([8*K,22*K,16*K,30*K],fill=W_); d.line([12*K,10*K,12*K,24*K],fill=W_,width=3*K)
icon('thermo',24,36,thermo)
def change(d,K):
    d.arc([3*K,3*K,25*K,25*K],30,330,fill=W_,width=3*K); d.polygon([(25*K,2*K),(27*K,12*K),(18*K,10*K)],fill=W_)
icon('change',28,28,change)
def load(d,K):
    d.polygon([(4*K,2*K),(20*K,2*K),(12*K,14*K)],fill=W_); d.rectangle([2*K,18*K,22*K,22*K],fill=W_)
icon('load',24,24,load)
def extrude(d,K):
    d.polygon([(1*K,10*K),(23*K,10*K),(12*K,25*K)],fill=W_); d.rectangle([9*K,1*K,15*K,10*K],fill=W_)
icon('extrude',24,26,extrude)
def retract(d,K):
    d.polygon([(1*K,16*K),(23*K,16*K),(12*K,1*K)],fill=W_); d.rectangle([9*K,16*K,15*K,25*K],fill=W_)
icon('retract',24,26,retract)
def idle(d,K): d.rectangle([0,0,16*K,16*K],fill=W_)
icon('idle',16,16,idle)
def warn(d,K):
    tri=[(18*K,2*K),(34*K,30*K),(2*K,30*K)]; d.line(tri+[tri[0]],fill=W_,width=3*K,joint='curve')
    d.rectangle([16.5*K,11*K,19.5*K,21*K],fill=W_); d.ellipse([16*K,23.5*K,20*K,27.5*K],fill=W_)
icon('warn',36,32,warn)
def chevron(d,K): d.line([(2*K,4*K),(8*K,10*K),(14*K,4*K)],fill=W_,width=2*K,joint='curve')
icon('chevron',16,14,chevron)

# ---------- page plate on the top rail ----------
K=4; pw,ph=250,40
pl=Image.new('RGBA',(pw*K,ph*K),(0,0,0,0)); pd=ImageDraw.Draw(pl)
pd.rounded_rectangle([2*K,2*K,(pw-2)*K,(ph-2)*K],radius=6*K,fill=(196,200,206,255),outline=INK,width=3*K)
pd.rounded_rectangle([7*K,7*K,(pw-7)*K,(ph-7)*K],radius=4*K,outline=(140,146,154,255),width=K)
for rx in (14,pw-14):
    pd.ellipse([(rx-4)*K,(ph/2-4)*K,(rx+4)*K,(ph/2+4)*K],fill=(120,124,130,255),outline=INK,width=K)
for sx,dirn in ((36,-1),(pw-36,1)):
    pd.polygon([((sx+dirn*6)*K,ph/2*K),((sx-dirn*5)*K,(ph/2-8)*K),((sx-dirn*5)*K,(ph/2+8)*K)],fill=(34,36,40,255))
pl=pl.resize((pw,ph),Image.LANCZOS)
plx=BX+BW//2-pw//2; ply=BY+30
c=Image.new('RGBA',(MW,MH),(0,0,0,0))
sh2=Image.new('L',(MW,MH),0); ImageDraw.Draw(sh2).rounded_rectangle([plx+3,ply+5,plx+pw+3,ply+ph+5],radius=6,fill=110)
s2=Image.new('RGBA',(MW,MH),(0,0,0,0)); s2.putalpha(sh2.filter(ImageFilter.GaussianBlur(3))); c.alpha_composite(s2); c.alpha_composite(pl,(plx,ply))
c=c.resize((1024,600),Image.LANCZOS); b_=c.getchannel('A').point(lambda v:255 if v>2 else 0).getbbox(); c.crop(b_).save(OUT+'plate.png',optimize=True)
LAYOUT['PLATE']=dict(file='filament/plate.png',x=b_[0],y=b_[1],w=b_[2]-b_[0],h=b_[3]-b_[1])
LAYOUT['PLATE_FACE']=dict(x=round(plx*S1024),y=round(ply*S1024),w=round(pw*S1024),h=round(ph*S1024))

# ---------- wooden arrows hanging under the desk ----------
def wood_arrow(left,label):
    K=4; w,h=98,62
    face=o.crop((238,522,258,570)).resize((w*K,h*K),Image.BICUBIC)
    face=Image.eval(face,lambda v:min(255,int(v*1.35)))
    im=Image.new('RGBA',(w*K,h*K),(0,0,0,0)); tip=24
    pts=[(2,h/2),(tip,2),(tip,11),(w-2,11),(w-2,h-11),(tip,h-11),(tip,h-2)]
    if not left: pts=[(w-x,y) for x,y in pts]
    pp=[(x*K,y*K) for x,y in pts]
    mm=Image.new('L',im.size,0); ImageDraw.Draw(mm).polygon(pp,fill=255); im.paste(face,(0,0),mm)
    d=ImageDraw.Draw(im); d.line(pp+[pp[0]],fill=(18,14,10,255),width=3*K,joint='curve')
    lx0=(tip+3) if left else 6; lx1=lx0+(w-2-tip-8)
    d.rounded_rectangle([lx0*K,18*K,lx1*K,(h-18)*K],radius=2*K,fill=(150,152,156,255),outline=(30,30,32,255),width=K)
    d.rounded_rectangle([(lx0+3)*K,21*K,(lx1-3)*K,(h-21)*K],radius=K,fill=(238,228,196,255))
    d.text(((lx0+lx1)/2*K,h/2*K+K//2),label,font=font(FONTS+'Montserrat.ttf',15*K,800),fill=(24,22,20,255),anchor='mm')
    return im.resize((w,h),Image.LANCZOS)
def hung(sign,x,y,top,name,key):
    lay=Image.new('RGBA',(1024*4,600*4),(0,0,0,0)); ld=ImageDraw.Draw(lay)
    for cx in (x+sign.width*0.35,x+sign.width*0.72):
        ld.line([(cx*4,top*4),(cx*4,(y+14)*4)],fill=(200,186,150,255),width=6)
        ld.line([(cx*4+3,top*4),(cx*4+3,(y+14)*4)],fill=(60,50,36,255),width=2)
        ld.ellipse([(cx-2.5)*4,(top-1)*4,(cx+2.5)*4,(top+4)*4],fill=(160,160,166,255),outline=(20,20,22,255),width=3)
        ld.ellipse([(cx-1.6)*4,(y+12.4)*4,(cx+1.6)*4,(y+15.6)*4],fill=(30,26,20,255))
    c=lay.resize((1024,600),Image.LANCZOS)
    shs=Image.new('L',(1024,600),0); shs.paste(sign.getchannel('A').point(lambda v:int(v*0.5)),(x+3,y+5))
    ss=Image.new('RGBA',(1024,600),(0,0,0,0)); ss.putalpha(shs.filter(ImageFilter.GaussianBlur(3))); c.alpha_composite(ss)
    c.alpha_composite(sign,(x,y))
    b_=c.getchannel('A').point(lambda v:255 if v>2 else 0).getbbox(); c.crop(b_).save(OUT+name,optimize=True)
    LAYOUT[key]=dict(file='filament/'+name,x=b_[0],y=b_[1],w=b_[2]-b_[0],h=b_[3]-b_[1])
    LAYOUT[key+'_TAP']=dict(x=x,y=y,w=sign.width,h=sign.height)
la=wood_arrow(True,'NEXT'); ra=wood_arrow(False,'BACK')
hung(la,4,522,513,'arrow_next.png','ARROW_NEXT'); hung(ra,1024-4-ra.width,522,513,'arrow_back.png','ARROW_BACK')

json.dump(LAYOUT,open(HERE+'layout.json','w'),indent=1)
print(json.dumps({k:v for k,v in LAYOUT.items()},indent=0)[:3000])
