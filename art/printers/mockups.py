import os
import json, cv2, numpy as np
from PIL import Image, ImageDraw, ImageFilter
ART=os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)),'..'))+'/'
O=os.path.dirname(os.path.abspath(__file__))+'/'
SRC=O+'source/'
OUT=O+'mockups/'
pos=json.load(open(ART+'objects/objects.json'))
INK=(22,20,18)

def base_scene(with_printer_shadow, round_beds=False):
    im=Image.open(ART+'01_clean_desk_5_drawers.png').convert('RGBA')
    sh=Image.open(ART+'objects/shadows.png').convert('RGBA')
    if not with_printer_shadow:
        a=np.array(sh).astype(float)
        keep=Image.new('L',sh.size,255); kd=ImageDraw.Draw(keep)
        kd.polygon([(648,466),(1150,466),(1150,540),(1062,540),(1062,610),(648,610)],fill=0)
        keep=np.asarray(keep.filter(ImageFilter.GaussianBlur(6))).astype(float)/255
        a[...,3]*=keep; sh=Image.fromarray(a.astype(np.uint8))
    if round_beds:
        a=np.array(sh).astype(float)
        keep=Image.new('L',sh.size,255); kd=ImageDraw.Draw(keep)
        kd.rectangle([330,22,632,302],fill=0); kd.rectangle([1046,558,1336,652],fill=0)
        keep=np.asarray(keep.filter(ImageFilter.GaussianBlur(4))).astype(float)/255
        a[...,3]*=keep; sh=Image.fromarray(a.astype(np.uint8))
        m=Image.new('L',sh.size,0); ImageDraw.Draw(m).ellipse([1062,560,1336,646],fill=140)
        m=m.filter(ImageFilter.GaussianBlur(7)); bs=Image.new('RGBA',sh.size,(0,0,0,0)); bs.putalpha(m)
        sh.alpha_composite(bs)
    im.alpha_composite(sh)
    wb=Image.open(O+'wall_bed_round.png') if round_beds else Image.open(ART+'objects/wall_bed.png').convert('RGBA')
    im.alpha_composite(wb,(338,29))
    global ROUND; ROUND=round_beds
    return im

def finish(im, printer=None):
    for k in ['writing','spools','holder']:
        p=pos[k]; im.alpha_composite(Image.open(ART+f'objects/{k}.png').convert('RGBA'),(p['x'],p['y']))
    if printer: printer(im)
    for k in (['rack','cables','bed','laptop'] if ROUND else ['rack','bed','cables','laptop']):
        p=pos[k]
        if k=='cables' and ROUND:
            draw_cable(im); continue
        src=O+'bed_round.png' if (k=='bed' and ROUND) else ART+f'objects/{k}.png'
        im.alpha_composite(Image.open(src).convert('RGBA'),(p['x'],p['y']))
    return im

def draw_cable(im):
    import math
    K=4; x0,y0,w,h=1110,500,190,70
    big=Image.new('RGBA',(w*K,h*K),(0,0,0,0)); d=ImageDraw.Draw(big)
    def pt(x,y): return (int((x-x0)*K),int((y-y0)*K))
    # bezier from behind the rack to the bed rim
    a=(1124,517); b=(1170,510); c=(1215,530); e=(1240,560)
    path=[]
    for i in range(41):
        t=i/40; u=1-t
        x=u**3*a[0]+3*u*u*t*b[0]+3*u*t*t*c[0]+t**3*e[0]; y=u**3*a[1]+3*u*u*t*b[1]+3*u*t*t*c[1]+t**3*e[1]
        path.append((x,y))
    wires=[(-4.5,(40,40,42)),(-1.5,(196,46,38)),(1.5,(236,236,230)),(4.5,(120,122,128))]
    def offset(o):
        pts=[]
        for i,(x,y) in enumerate(path):
            j=min(i+1,len(path)-1); k=max(i-1,0)
            dx=path[j][0]-path[k][0]; dy=path[j][1]-path[k][1]; L=math.hypot(dx,dy) or 1
            pts.append(pt(x-dy/L*o,y+dx/L*o))
        return pts
    d.line(offset(0),fill=(22,20,18,255),width=int(13*K),joint='curve')
    for o,colr in wires: d.line(offset(o),fill=colr+(255,),width=int(2.4*K),joint='curve')
    # zip tie
    zx,zy=path[16]; d.line([pt(zx-2,zy-7),pt(zx+2,zy+7)],fill=(240,240,240,255),width=3*K)
    # connector at the bed rim
    ex,ey=path[-1]
    im.alpha_composite(big.resize((w,h),Image.LANCZOS),(x0,y0))

def contact_shadow(im, poly, blur=10):
    m=Image.new('L',im.size,0); ImageDraw.Draw(m).polygon(poly,fill=140)
    m=m.filter(ImageFilter.GaussianBlur(blur))
    s=Image.new('RGBA',im.size,(0,0,0,0)); s.putalpha(m); im.alpha_composite(s)

# ---------- CoreXY (current) ----------
def corexy(im):
    p=pos['printer']; im.alpha_composite(Image.open(ART+'objects/printer.png').convert('RGBA'),(p['x'],p['y']))

# ---------- Delta, cut from the reference picture ----------
def delta_sprite():
    src=cv2.imread(SRC+'delta_reference.png')
    vm=cv2.imread(SRC+'vase_mask.png',0)
    vm=cv2.dilate(vm,np.ones((9,9),np.uint8),iterations=2)
    vm[858:,:]=0   # keep the bed edge
    # fill each row between the colours either side of the vase, then soften
    f=src.astype(float)
    for y in range(src.shape[0]):
        xs=np.where(vm[y]>0)[0]
        if len(xs)==0: continue
        x0,x1=max(xs.min()-1,0),min(xs.max()+1,src.shape[1]-1)
        l=f[y,max(x0-6,0):x0+1].mean(axis=0); r=f[y,x1:x1+7].mean(axis=0)
        t=np.linspace(0,1,x1-x0+1)[:,None]
        f[y,x0:x1+1]=l*(1-t)+r*t
    out=f.astype(np.uint8)
    blur=cv2.GaussianBlur(out,(0,0),5)
    mm=cv2.GaussianBlur(vm,(0,0),3)[...,None]/255.0
    src=(out*(1-mm)+blur*mm).astype(np.uint8)
    rgb=Image.fromarray(cv2.cvtColor(src,cv2.COLOR_BGR2RGB)).convert('RGBA')
    # blank the display (the app draws on it)
    quad=[(413,997),(780,993),(773,1205),(382,1189)]   # inside the frame
    g=Image.new('RGBA',rgb.size,(0,0,0,0)); gd=ImageDraw.Draw(g)
    for i in range(60):
        t=i/59; y0=994+t*212
        c=(int(32-8*t),int(55-13*t),int(88-20*t),255)
        gd.line([(360,y0),(820,y0)],fill=c,width=5)
    # the glass: blue pixels of the screen, as one convex shape (dark corners included)
    gm=cv2.imread(SRC+'glass_mask.png',0)
    cnts,_=cv2.findContours(gm,cv2.RETR_EXTERNAL,cv2.CHAIN_APPROX_NONE)
    hull=cv2.convexHull(max(cnts,key=cv2.contourArea))
    glass=np.zeros_like(gm); cv2.fillConvexPoly(glass,hull,255)
    m=Image.fromarray(glass)
    rgb.paste(g,(0,0),m)
    mask=Image.open(SRC+'delta_mask.png').convert('L')
    rgb.putalpha(mask)
    bb=mask.getbbox(); spr=rgb.crop(bb)
    return spr

def delta(im):
    spr=delta_sprite()
    H=548; W=round(spr.width*H/spr.height)
    spr=spr.resize((W,H),Image.LANCZOS)
    x=890-W//2; y=588-H
    contact_shadow(im,[(x+20,y+H-95),(x+W-10,y+H-95),(x+W+5,y+H-2),(x+5,y+H-2)],12)
    im.alpha_composite(spr,(x,y))
    print('delta at',x,y,W,H)

# ---------- Bed slinger (bedslinger.py makes bedslinger.png), with the CoreXY's display ----------
def cxy_display():
    src=Image.open(ART+'objects/printer.png').convert('RGBA')
    m=Image.new('L',src.size,0); ImageDraw.Draw(m).polygon([(131,422),(301,424),(293,515),(114,515)],fill=255)
    d=src.copy(); d.putalpha(Image.fromarray(np.minimum(np.asarray(src.getchannel('A')),np.asarray(m))))
    return d.crop((112,420,304,517))

def bedslinger(im):
    spr=Image.open(O+'bedslinger.png')
    H=470; W=round(spr.width*H/spr.height)
    spr=spr.resize((W,H),Image.LANCZOS)
    x=900-W//2; y=590-H
    a=np.asarray(spr.getchannel('A')).copy(); a[:int(H*0.7)]=0
    m=Image.new('L',im.size,0); m.paste(Image.fromarray((a*0.5).astype(np.uint8)),(x+10,y+10))
    m=m.filter(ImageFilter.GaussianBlur(9)); s=Image.new('RGBA',im.size,(0,0,0,0)); s.putalpha(m); im.alpha_composite(s)
    im.alpha_composite(spr,(x,y))
    dsp=cxy_display()
    dx=x+int(W*0.33)-dsp.width//2; dy=y+H-dsp.height-2
    sh=Image.new('L',im.size,0); sh.paste(dsp.getchannel('A'),(dx+6,dy+8)); sh=sh.filter(ImageFilter.GaussianBlur(6))
    sh=Image.fromarray((np.asarray(sh)*0.55).astype(np.uint8)); ss=Image.new('RGBA',im.size,(0,0,0,0)); ss.putalpha(sh); im.alpha_composite(ss)
    im.alpha_composite(dsp,(dx,dy))
    print('bed slinger at',x,y,W,H,'display',dx,dy)

scenes=[('no printer',lambda: finish(base_scene(False))),
        ('CoreXY (current)',lambda: finish(base_scene(True),corexy)),
        ('Delta',lambda: finish(base_scene(False,True),delta)),
        ('Bed slinger',lambda: finish(base_scene(False),bedslinger))]
from PIL import ImageFont
f=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf',22)
tiles=[]
for name,fn in scenes:
    im=fn().convert('RGB').resize((1024,600),Image.LANCZOS)
    im.save(OUT+'home_'+{'no':'no_printer','corexy':'corexy','delta':'delta','bed':'bedslinger'}[name.split()[0].lower().split('(')[0]]+'.png')
    t=im.resize((512,300),Image.LANCZOS); tiles.append((name,t))
grid=Image.new('RGB',(512*2+30,(300+34)*2+20),(30,30,32)); gd=ImageDraw.Draw(grid)
for i,(name,t) in enumerate(tiles):
    x=10+(i%2)*(512+10); y=10+(i//2)*(334+10)
    gd.text((x,y),name,fill=(240,240,240),font=f); grid.paste(t,(x,y+30))
grid.save(OUT+'printers_grid.png')
