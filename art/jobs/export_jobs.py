"""Writes assets/workshop/jobs/: the jobs folder background and its sprites."""
import os, math
HERE=os.path.dirname(os.path.abspath(__file__))+'/'
__file__=HERE+'mockups.py'
src=open(HERE+"mockups.py").read().split("\nif __name__==")[0]
exec(src)
OUT=R+'assets/workshop/jobs/'
os.makedirs(OUT,exist_ok=True)

def background():
    base=desk(); folder(base)
    paper(base,40,30,452,462,11)
    paper(base,476,34,884,464,12)
    d=ImageDraw.Draw(base)
    jx0,jy0,jx1,jy1=496,50,864,94
    poly(d,wobble([(jx0,jy0),(jx1,jy0),(jx1,jy1),(jx0,jy1)],0.7,40),fill=(255,255,255),w=2.5)
    T(d,jx0+14,(jy0+jy1)/2,'JOBS:',HAND(17,700),INK,'lm')
    estop(base,3,3); clip(ImageDraw.Draw(base),84,-4,0.95)
    home_note(base,914,10)
    card(base,924,112,82,140,70); d=ImageDraw.Draw(base); hand_arrow(d,965,226,138,INK)
    card(base,924,262,82,64,71); d=ImageDraw.Draw(base); T(d,965,280,'SORT',HAND(13,700),PENCIL,'mm')
    card(base,924,336,82,128,72); d=ImageDraw.Draw(base); hand_arrow(d,965,350,448,INK)
    return base.resize((W,H),Image.LANCZOS).convert('RGB')

def sprite(w,h,draw):
    im=Image.new('RGBA',(P(w),P(h)),(0,0,0,0)); draw(im)
    return im.resize((w,h),Image.LANCZOS)

# Photo frame (placed at 87,10): white border, ink outline, shadow, a see-through window for the
# thumbnail and two tape strips at 45 degrees over the upper corners
PX,PY,PS=34,34,250          # frame corner inside the sprite
def photo(im):
    d=ImageDraw.Draw(im)
    sh=Image.new('L',im.size,0); ImageDraw.Draw(sh).rectangle([P(PX+4),P(PY+6),P(PX+PS+4),P(PY+PS+6)],fill=100)
    sh=sh.filter(ImageFilter.GaussianBlur(P(5)))
    black=Image.new('RGBA',im.size,(0,0,0,0)); black.putalpha(sh); im.alpha_composite(black)
    d=ImageDraw.Draw(im)
    d.rectangle([P(PX),P(PY),P(PX+PS),P(PY+PS)],fill=(255,255,255,255),outline=INK,width=P(2))
    d.rectangle([P(PX+8),P(PY+8),P(PX+PS-8),P(PY+PS-8)],fill=(0,0,0,0))
    tape=Image.new('RGBA',im.size,(0,0,0,0)); td=ImageDraw.Draw(tape)
    for cx,cy,ux,uy in ((PX+4,PY+4,1,-1),(PX+PS-4,PY+4,1,1)):
        ux,uy=ux/math.sqrt(2),uy/math.sqrt(2); px,py=-uy,ux; L,Wt=34,10
        pts=[(cx-ux*L-px*Wt,cy-uy*L-py*Wt),(cx+ux*L-px*Wt,cy+uy*L-py*Wt),(cx+ux*L+px*Wt,cy+uy*L+py*Wt),(cx-ux*L+px*Wt,cy-uy*L+py*Wt)]
        td.polygon([(P(a),P(b)) for a,b in pts],fill=(236,230,210,215))
    im.alpha_composite(tape)

def stamp(im):
    sx,sy,k=6,8,1.2
    st=Image.new('RGBA',im.size,(0,0,0,0)); sd=ImageDraw.Draw(st)
    sd.rounded_rectangle([P(sx),P(sy),P(sx+104*k),P(sy+50*k)],radius=P(9),outline=(206,52,38,235),width=P(4.5))
    sd.polygon([(P(sx+16*k),P(sy+14*k)),(P(sx+16*k),P(sy+36*k)),(P(sx+34*k),P(sy+25*k))],fill=(206,52,38,235))
    sd.text((P(sx+68*k),P(sy+25*k)),'PRINT',font=F(18*k,800),fill=(206,52,38,235),anchor='mm')
    im.alpha_composite(st.rotate(-6,center=(P(sx+52*k),P(sy+25*k)),resample=Image.BICUBIC))

def row(sel):
    def f(im):
        d=ImageDraw.Draw(im)
        if sel: d.rectangle([0,0,im.width,im.height],fill=BLUE_L+(150,))
        poly(d,wobble([(6,4),(374,4),(374,42),(6,42)],0.6,70),fill=None,ol=BLUE if sel else INK,w=3 if sel else 2.2)
    return f

def arrow(up,on):
    def f(im):
        d=ImageDraw.Draw(im); cx,cy=24,18
        if on: poly(d,wobble([(cx-22,cy-17),(cx+22,cy-17),(cx+22,cy+17),(cx-22,cy+17)],0.5,90),fill=BLUE_L,ol=BLUE,w=2.5)
        col=BLUE if on else (176,176,180); sg=-1 if up else 1
        d.line([P(cx),P(cy-9*sg),P(cx),P(cy+9*sg)],fill=col,width=P(3.5))
        d.line([P(cx-10),P(cy+9*sg-8*sg),P(cx),P(cy+9*sg),P(cx+10),P(cy+9*sg-8*sg)],fill=col,width=P(3.5),joint='curve')
    return f

def folder_icon(im):
    d=ImageDraw.Draw(im); fx,fy=2,2
    d.polygon([(P(fx),P(fy)),(P(fx+8),P(fy)),(P(fx+11),P(fy+3)),(P(fx+22),P(fy+3)),(P(fx+22),P(fy+17)),(P(fx),P(fy+17))],fill=MANILA,outline=INK,width=P(1.5))

def nopic(im):
    d=ImageDraw.Draw(im)
    for x,y,x2,y2 in ((60,90,170,120),(60,90,95,70),(170,120,200,98),(95,70,200,98)):
        pass
    # a pencil sketch of a box
    pts=[(80,110),(140,80),(200,110),(140,140)]
    for a,b in zip(pts,pts[1:]+pts[:1]): d.line([P(a[0]),P(a[1]),P(b[0]),P(b[1])],fill=PENCIL,width=P(2))
    for p in (pts[0],pts[2],pts[3]): d.line([P(p[0]),P(p[1]),P(p[0]),P(p[1]+40)],fill=PENCIL,width=P(2))
    d.line([P(80),P(150),P(140),P(180),P(200),P(150)],fill=PENCIL,width=P(2),joint='curve')

background().save(OUT+'background.png',optimize=True)
sprite(318,304,photo).save(OUT+'photo.png',optimize=True)
sprite(140,90,stamp).save(OUT+'stamp.png',optimize=True)
sprite(380,46,row(False)).save(OUT+'row.png',optimize=True)
sprite(380,46,row(True)).save(OUT+'row_sel.png',optimize=True)
for up in (True,False):
    for on in (True,False):
        sprite(48,36,arrow(up,on)).save(OUT+f'sort_{"up" if up else "down"}_{"on" if on else "off"}.png',optimize=True)
sprite(26,22,folder_icon).save(OUT+'folder.png',optimize=True)
sprite(234,234,nopic).save(OUT+'no_picture.png',optimize=True)
print(sorted(os.listdir(OUT)))
