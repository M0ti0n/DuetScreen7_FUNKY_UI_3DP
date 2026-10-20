import os
import cv2, numpy as np
from PIL import Image
O=os.path.dirname(os.path.abspath(__file__))+'/'
img=np.asarray(Image.open(O+'source/bedslinger_reference.png').convert('RGB')).astype(int)
H,W=img.shape[:2]
r,g,b=img[...,0],img[...,1],img[...,2]
neutral=(np.abs(r-g)<=7)&(np.abs(g-b)<=7)&(np.abs(r-b)<=7)&(np.maximum(np.maximum(r,g),b)>=158)
cand=neutral.astype(np.uint8)*255
bg=cv2.morphologyEx(cand,cv2.MORPH_OPEN,np.ones((5,5),np.uint8))
# grow the background over the soft fringe between squares and outline (light, neutral-ish)
fr=((np.abs(r-g)<=10)&(np.abs(g-b)<=10)&(img.max(axis=2)>=135)).astype(np.uint8)*255
for _ in range(3):
    bg=np.where((cv2.dilate(bg,np.ones((3,3),np.uint8))>0)&(fr>0),255,bg).astype(np.uint8)
alpha=(255-bg).astype(np.uint8)
n,lab,st,_=cv2.connectedComponentsWithStats(alpha)
big=1+np.argmax(st[1:,cv2.CC_STAT_AREA]); alpha=np.where(lab==big,255,0).astype(np.uint8)
inv=255-alpha; n,lab,st,_=cv2.connectedComponentsWithStats(inv)
for i in range(1,n):
    if st[i,cv2.CC_STAT_AREA]<120: alpha[lab==i]=255

rgb=img.astype(np.uint8)
# ---- scene grading: slate blacks, slightly muted orange, a little paper grain ----
hsv=cv2.cvtColor(rgb,cv2.COLOR_RGB2HSV).astype(float)
orange=(hsv[...,0]>4)&(hsv[...,0]<24)&(hsv[...,1]>120)
dark=(hsv[...,2]<90)&~orange
hsv[...,1]=np.where(orange,hsv[...,1]*0.9,hsv[...,1])
out=cv2.cvtColor(np.clip(hsv,0,255).astype(np.uint8),cv2.COLOR_HSV2RGB).astype(float)
slate=np.array([46,50,58.0])
out[dark]=out[dark]*0.75+slate*0.25
# soft top-left light, like the other stickers
yy,xx=np.mgrid[0:H,0:W]
light=1.06-0.12*(xx/W*0.4+yy/H*0.6)
out*=light[...,None]
rng=np.random.default_rng(5)
out+=rng.normal(0,2.2,(H,W,1))
out=np.clip(out,0,255).astype(np.uint8)
# ---- bolder ink: thicken the existing outlines (they get thin when scaled down) ----
ink=(img.max(axis=2)<45).astype(np.uint8)*255
ink=cv2.dilate(ink,np.ones((3,3),np.uint8))&alpha
out[ink>0]=(22,20,18)
# outer outline
ol=cv2.morphologyEx(alpha,cv2.MORPH_GRADIENT,np.ones((5,5),np.uint8))
out[(ol>0)&(alpha>0)]=(22,20,18)
SPR=Image.fromarray(np.dstack([out,alpha]),'RGBA')


# ---- the white filament tube (lost with the background), redrawn ----
from PIL import ImageDraw
spr=SPR
K=3; tb=Image.new('RGBA',(W*K,H*K),(0,0,0,0)); d=ImageDraw.Draw(tb)
pts=[]
for i in range(60):
    t=i/59; u=1-t
    p0,p1,p2,p3=(432,124),(430,40),(560,28),(772,12)
    x=u**3*p0[0]+3*u*u*t*p1[0]+3*u*t*t*p2[0]+t**3*p3[0]; y=u**3*p0[1]+3*u*u*t*p1[1]+3*u*t*t*p2[1]+t**3*p3[1]
    pts.append((x*K,y*K))
d.line(pts,fill=(22,20,18,255),width=11*K,joint='curve'); d.line(pts,fill=(232,234,236,255),width=6*K,joint='curve')
tube=tb.resize((W,H),Image.LANCZOS)
fin=Image.new('RGBA',(W,H),(0,0,0,0)); fin.alpha_composite(tube)
# head over the tube's end: paste the sprite, then tube only where the sprite is transparent or frame
a=np.asarray(spr.getchannel('A'))
head=np.zeros((H,W),bool); head[110:400,330:530]=True
s2=np.asarray(spr).copy()
fin.alpha_composite(spr)
t2=np.asarray(tube).copy(); t2[head&(a>0)]=0
fin.alpha_composite(Image.fromarray(t2,'RGBA'))
bbx=fin.getchannel('A').getbbox()
import json; json.dump(list(bbx),open(O+'source/bedslinger_crop.json','w'))
fin=fin.crop(bbx)
fin.save(O+'bedslinger.png'); print('final',fin.size)
