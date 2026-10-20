import os
import cv2, numpy as np, math
from PIL import Image, ImageDraw, ImageFilter
O=os.path.dirname(os.path.abspath(__file__))+'/'
ART=O+'../objects/'
INK=(22,20,18,255)
S=4
def P(v): return int(round(v*S))

def wall_round():
    src=Image.open(ART+'wall_bed.png').convert('RGBA')
    W,H=src.size
    a=np.array(src)
    hsv=cv2.cvtColor(a[...,:3],cv2.COLOR_RGB2HSV)
    yel=((hsv[...,0]>15)&(hsv[...,0]<40)&(hsv[...,1]>70)&(hsv[...,2]>140)).astype(np.uint8)
    cnts,_=cv2.findContours(yel,cv2.RETR_EXTERNAL,cv2.CHAIN_APPROX_SIMPLE)
    c=max(cnts,key=cv2.contourArea); hull=cv2.convexHull(c)
    note=np.zeros((H,W),np.uint8); cv2.fillConvexPoly(note,hull,255)
    note=cv2.dilate(note,np.ones((3,3),np.uint8))
    # cable: everything right of the plate
    cable=np.zeros((H,W),np.uint8); cable[:, 238:]=255
    cx,cy,r=142,106,104
    out=Image.new('RGBA',(W,H),(0,0,0,0))
    # drop shadow
    sh=Image.new('L',(W,H),0); ImageDraw.Draw(sh).ellipse([cx-r+3,cy-r+5,cx+r+3,cy+r+5],fill=110)
    sh=sh.filter(ImageFilter.GaussianBlur(4)); s=Image.new('RGBA',(W,H),(0,0,0,0)); s.putalpha(sh); out.alpha_composite(s)
    # plate texture from the square plate
    g=np.zeros((2*r,2*r,4),np.uint8)
    t=np.linspace(0,1,2*r)[:,None]
    top=np.array([88,92,92]); bot=np.array([66,69,69])
    col=top*(1-t)+bot*t
    rng=np.random.default_rng(3)
    g[...,:3]=np.clip(col[:,None,:]+rng.normal(0,1.6,(2*r,2*r,1)),0,255).astype(np.uint8)
    g[...,3]=255
    tex=Image.fromarray(g)
    big=Image.new('RGBA',(W*S,H*S),(0,0,0,0)); d=ImageDraw.Draw(big)
    m=Image.new('L',(W,H),0); ImageDraw.Draw(m).ellipse([cx-r,cy-r,cx+r,cy+r],fill=255)
    plate=Image.new('RGBA',(W,H),(0,0,0,0)); plate.paste(tex,(cx-r,cy-r)); plate.putalpha(m)
    out.alpha_composite(plate)
    # rings like a delta bed, rim, screws
    d.ellipse([P(cx-r),P(cy-r),P(cx+r),P(cy+r)],outline=INK,width=P(3))
    d.ellipse([P(cx-r+5),P(cy-r+5),P(cx+r-5),P(cy+r-5)],outline=(112,116,122,255),width=P(1.2))
    for rr in (35,70):
        d.ellipse([P(cx-rr),P(cy-rr),P(cx+rr),P(cy+rr)],outline=(98,100,104,255),width=P(1))
    d.line([P(cx-r+12),P(cy),P(cx+r-12),P(cy)],fill=(98,100,104,255),width=P(1))
    d.line([P(cx),P(cy-r+12),P(cx),P(cy+r-12)],fill=(98,100,104,255),width=P(1))
    for k in range(3):
        ang=math.radians(-150+120*k); sx=cx+(r-13)*math.cos(ang); sy=cy+(r-13)*math.sin(ang)
        d.ellipse([P(sx-7),P(sy-7),P(sx+7),P(sy+7)],fill=(150,152,156,255),outline=INK,width=P(1.5))
        d.line([P(sx-4),P(sy-4),P(sx+4),P(sy+4)],fill=(70,70,74,255),width=P(1.5))
    out.alpha_composite(big.resize((W,H),Image.LANCZOS))
    # cable and note back on top
    cab=src.copy(); cab.putalpha(Image.fromarray(np.minimum(a[...,3],cable))); out.alpha_composite(cab)
    nt=src.copy(); nt.putalpha(Image.fromarray(np.minimum(a[...,3],note)))
    out.alpha_composite(nt)
    return out

def desk_round():
    W,H=282,98
    big=Image.new('RGBA',(W*S,H*S),(0,0,0,0)); d=ImageDraw.Draw(big)
    cx,cy,rx,ry=141,44,134,38
    # thickness
    d.ellipse([P(cx-rx),P(cy-ry+7),P(cx+rx),P(cy+ry+7)],fill=(30,31,34,255),outline=INK,width=P(2.5))
    d.ellipse([P(cx-rx),P(cy-ry),P(cx+rx),P(cy+ry)],fill=(58,60,64,255),outline=INK,width=P(2.5))
    for k in range(10):
        t=k/9; e=(cx-rx+6+ t*0, cy-ry+4)
    # subtle radial shading
    for i in range(14):
        f=1-i/14
        c=(int(58+i*1.6),int(60+i*1.6),int(64+i*1.6),255)
        d.ellipse([P(cx-rx*f*0.96),P(cy-ry*f*0.92),P(cx+rx*f*0.96),P(cy+ry*f*0.92)],fill=c)
    for rr in (0.33,0.66):
        d.ellipse([P(cx-rx*rr),P(cy-ry*rr),P(cx+rx*rr),P(cy+ry*rr)],outline=(92,94,100,255),width=P(1))
    d.line([P(cx-rx+14),P(cy),P(cx+rx-14),P(cy)],fill=(92,94,100,255),width=P(1))
    # glare
    d.line([P(cx+20),P(cy-24),P(cx+70),P(cy-30)],fill=(150,152,158,255),width=P(2))
    d.line([P(cx+28),P(cy-19),P(cx+64),P(cy-23)],fill=(120,122,128,255),width=P(1.5))
    return big.resize((W,H),Image.LANCZOS)

wall_round().save(O+'wall_bed_round.png'); desk_round().save(O+'bed_round.png')
