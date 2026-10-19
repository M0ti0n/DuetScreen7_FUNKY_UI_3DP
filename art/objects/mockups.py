from PIL import Image, ImageDraw, ImageFont, ImageFilter
import numpy as np, random, math
import os
R=os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)),'..','..'))+'/'
S=2; W,H=1024,600
def P(v): return int(round(v*S))
def F(sz,wt=600,face='Montserrat'):
    f=ImageFont.truetype(R+'assets/fonts/'+face+'.ttf',int(sz*S))
    try: f.set_variation_by_axes([wt])
    except Exception: pass
    return f
HAND=lambda sz,wt=500: F(sz,wt,'PlaypenSans')
TYPE=lambda sz,wt=500: F(sz,wt,'GoogleSansCode')
INK=(22,20,18); PAPER=(247,244,235); PAPER2=(238,233,220)
MANILA=(224,182,118); MANILA_D=(198,152,90); BLUE=(38,104,196); BLUE_L=(178,214,246); RED=(206,52,38); PENCIL=(96,96,100)
def T(d,x,y,s,f,c=INK,a='la'): d.text((P(x),P(y)),s,font=f,fill=c,anchor=a)
def poly(d,pts,fill=None,ol=INK,w=2.5):
    pp=[(P(x),P(y)) for x,y in pts]
    if fill: d.polygon(pp,fill=fill)
    if ol: d.line(pp+[pp[0]],fill=ol,width=P(w),joint='curve')
def wobble(pts,amp=1.2,seed=1):
    random.seed(seed); return [(x+random.uniform(-amp,amp),y+random.uniform(-amp,amp)) for x,y in pts]

def desk():
    st=Image.open(R+'art/00_start_point_clean_desk.png').convert('RGB')
    k=st.width/1024
    band=st.crop((0,int(388*k),st.width,int(470*k))).resize((W*S,P(82)),Image.LANCZOS)
    im=Image.new('RGB',(W*S,H*S))
    y=0;i=0
    while y<H*S:
        b=band if i%2==0 else band.transpose(Image.FLIP_LEFT_RIGHT)
        if i%3==2: b=b.transpose(Image.FLIP_TOP_BOTTOM)
        im.paste(b,(0,y)); y+=band.height; i+=1
    d=ImageDraw.Draw(im)
    for j in range(1,8): d.line([0,j*band.height,W*S,j*band.height],fill=(70,42,22),width=P(1.5))
    home=Image.open(R+'assets/workshop/home_idle.png').convert('RGB')
    strip=home.crop((0,487,1024,600)).resize((W*S,P(113)),Image.LANCZOS)
    im.paste(strip,(0,P(487)))
    return im.convert('RGBA')

def shadow(base,shape_fn,off=(6,8),blur=8,alpha=110):
    m=Image.new('L',base.size,0); shape_fn(ImageDraw.Draw(m))
    m=m.filter(ImageFilter.GaussianBlur(P(blur)))
    sh=Image.new('RGBA',base.size,(0,0,0,0)); sh.putalpha(m.point(lambda v:v*alpha//255))
    base.alpha_composite(sh,(P(off[0]),P(off[1])))

def paper(base,x0,y0,x1,y1,seed,curl=True,fill=PAPER):
    pts=wobble([(x0,y0),(x1,y0),(x1,y1-(14 if curl else 0)),(x1-(14 if curl else 0),y1),(x0,y1)],1.0,seed)
    shadow(base,lambda d:d.polygon([(P(x),P(y)) for x,y in pts],fill=255),(4,6),6,90)
    d=ImageDraw.Draw(base)
    poly(d,pts,fill=fill,w=2.2)
    if curl:
        c=[(x1,y1-14),(x1-14,y1),(x1-11,y1-11)]
        poly(d,wobble(c,0.4,seed+3),fill=PAPER2,w=2)

def folder(base):
    # back half (larger), tab on top right
    d=ImageDraw.Draw(base)
    back=[(26,22),(600,22),(612,8),(760,8),(772,22),(902,22),(902,470),(26,470)]
    shadow(base,lambda dd:dd.polygon([(P(x),P(y)) for x,y in back],fill=255),(7,9),9,120)
    d=ImageDraw.Draw(base)
    poly(d,wobble(back,0.8,4),fill=MANILA,w=3)
    # fold line in the middle
    d.line([P(462),P(26),P(462),P(466)],fill=MANILA_D,width=P(3))
    d.line([P(466),P(26),P(466),P(466)],fill=(236,200,140),width=P(1.5))

def clip(d,x,y,s=1.0):
    # paper clip: long loop + inner loop
    def loop(cx,top,bot,w):
        pts=[]
        for a in range(0,181,15): pts.append((cx+w*math.cos(math.radians(180-a)),top+w-w*math.sin(math.radians(a))))
        return pts
    col=(150,156,166); hi=(214,218,226)
    outer=[(x,y+70*s),(x,y+12*s)]+[(x+11*s-11*s*math.cos(math.radians(a)),y+12*s-11*s*math.sin(math.radians(a))) for a in range(0,181,20)]+[(x+22*s,y+12*s),(x+22*s,y+60*s)]+[(x+16*s+6*s*math.cos(math.radians(a)),y+60*s+6*s*math.sin(math.radians(a))) for a in range(0,181,20)]+[(x+10*s,y+60*s),(x+10*s,y+20*s)]
    for wdt,c in ((6,INK),(3.6,col),(1.2,hi)):
        d.line([(P(a),P(b)) for a,b in outer],fill=c,width=P(wdt),joint='curve')

def estop(base,x,y,s=92):
    d=ImageDraw.Draw(base)
    shadow(base,lambda dd:dd.rounded_rectangle([P(x),P(y),P(x+s),P(y+s)],radius=P(8),fill=255),(3,4),3,110)
    d=ImageDraw.Draw(base)
    d.rounded_rectangle([P(x),P(y),P(x+s),P(y+s)],radius=P(8),fill=(214,40,34),outline=INK,width=P(3))
    # call point: glass frame with the push button
    bx,by,bs=x+18,y+12,s-36
    d.rounded_rectangle([P(bx),P(by),P(bx+bs),P(by+bs)],radius=P(6),outline=(255,255,255),width=P(4))
    d.ellipse([P(bx+bs*0.27),P(by+bs*0.27),P(bx+bs*0.73),P(by+bs*0.73)],fill=(255,255,255))
    d.ellipse([P(bx+bs*0.36),P(by+bs*0.36),P(bx+bs*0.64),P(by+bs*0.64)],fill=(214,40,34))
    T(d,x+s/2,y+s-12,'STOP',F(15,800),(255,255,255),'mm')

def home_note(base,x,y,w=96,h=92):
    d=ImageDraw.Draw(base)
    pts=wobble([(x,y),(x+w,y+3),(x+w-2,y+h),(x+2,y+h-2)],0.8,9)
    shadow(base,lambda dd:dd.polygon([(P(a),P(b)) for a,b in pts],fill=255),(4,6),5,110)
    d=ImageDraw.Draw(base)
    poly(d,pts,fill=(250,222,92),w=2.5)
    d.rectangle([P(x+28),P(y-6),P(x+68),P(y+8)],fill=(232,226,206))   # tape
    d.line([P(x+28),P(y-6),P(x+68),P(y-6)],fill=(200,194,170),width=P(1))
    # house with a door (screen home)
    cx,cy,s=x+w/2,y+40,19
    for seg in ([(cx-s*1.3,cy-s*0.05),(cx,cy-s),(cx+s*1.3,cy-s*0.05)],
                [(cx-s*0.85,cy-s*0.38),(cx-s*0.85,cy+s*0.95),(cx+s*0.85,cy+s*0.95),(cx+s*0.85,cy-s*0.38)],
                [(cx-s*0.28,cy+s*0.95),(cx-s*0.28,cy+s*0.2),(cx+s*0.28,cy+s*0.2),(cx+s*0.28,cy+s*0.95)]):
        d.line([(P(a),P(b)) for a,b in seg],fill=INK,width=P(3),joint='curve')
    T(d,cx,y+h-14,'HOME',HAND(15,700),INK,'mm')

def card(base,x,y,w,h,seed):
    pts=wobble([(x,y),(x+w,y),(x+w,y+h),(x,y+h)],0.8,seed)
    shadow(base,lambda dd:dd.polygon([(P(a),P(b)) for a,b in pts],fill=255),(3,5),4,100)
    poly(ImageDraw.Draw(base),pts,fill=PAPER,w=2.5)

def hand_arrow(d,cx,y0,y1,col=INK):
    d.line([P(cx),P(y0),P(cx+0.8),P(y1)],fill=col,width=P(3.5))
    s=1 if y1<y0 else -1
    d.line([P(cx-12),P(y1+16*s),P(cx),P(y1),P(cx+12),P(y1+15*s)],fill=col,width=P(3.5),joint='curve')

OBJS=[('Front left clip',0.22,0.25),('Front right clip',0.72,0.27),('Rear bracket',0.55,0.68),('Cable guide',0.25,0.78),('Sensor cover',0.78,0.75),
      ('Fan duct',0.45,0.42),('Spool hub',0.86,0.48),('Knob',0.12,0.5)]

def bed_page(base,objs,selected,cancelled,empty=False,outline=True):
    paper(base,40,30,452,462,11)
    d=ImageDraw.Draw(base)
    ox,oy=74,426
    d.line([P(ox),P(oy+12),P(ox),P(108)],fill=INK,width=P(3)); d.line([P(ox-9),P(122),P(ox),P(106),P(ox+9),P(122)],fill=INK,width=P(3),joint='curve')
    d.line([P(ox-12),P(oy),P(442),P(oy)],fill=INK,width=P(3)); d.line([P(428),P(oy-9),P(444),P(oy),P(428),P(oy+9)],fill=INK,width=P(3),joint='curve')
    T(d,ox+14,92,'Y',HAND(22,600)); T(d,420,430,'X',HAND(22,600)); T(d,ox-16,oy+2,'0',HAND(15,500),PENCIL)
    # bed outline (pencil, dashed)
    bx0,by0,bx1,by1=92,118,430,414
    if not outline: objs=[]
    for x in (range(bx0,bx1,14) if outline else []): d.line([P(x),P(by0),P(min(x+8,bx1)),P(by0)],fill=PENCIL,width=P(1.6)); d.line([P(x),P(by1),P(min(x+8,bx1)),P(by1)],fill=PENCIL,width=P(1.6))
    for y in (range(by0,by1,14) if outline else []): d.line([P(bx0),P(y),P(bx0),P(min(y+8,by1))],fill=PENCIL,width=P(1.6)); d.line([P(bx1),P(y),P(bx1),P(min(y+8,by1))],fill=PENCIL,width=P(1.6))
    T(d,56,442,'RED = object canceled',HAND(14,500),RED)
    if empty:
        T(d,(bx0+bx1)/2,(by0+by1)/2,'nothing printing',HAND(20,500),PENCIL,'mm'); return
    for i,(n,fx,fy) in enumerate(objs):
        cx=bx0+fx*(bx1-bx0); cy=by1-fy*(by1-by0); s=17
        box=wobble([(cx-s,cy-s),(cx+s,cy-s),(cx+s,cy+s),(cx-s,cy+s)],0.9,30+i)
        if i in cancelled:
            poly(d,box,fill=RED,ol=INK,w=2.5); T(d,cx,cy+1,str(i+1),HAND(19,700),(255,255,255),'mm')
        elif i==selected:
            poly(d,box,fill=BLUE_L,ol=BLUE,w=3.5); T(d,cx,cy+1,str(i+1),HAND(19,700),BLUE,'mm')
        else:
            poly(d,box,fill=PAPER,ol=INK,w=2.5); T(d,cx,cy+1,str(i+1),HAND(19,700),INK,'mm')

def list_page(base,job,objs,page,selected,cancelled,empty=False):
    paper(base,476,34,884,464,12)
    d=ImageDraw.Draw(base)
    # job name label (typed)
    jx0,jy0,jx1,jy1=496,50,864,94
    poly(d,wobble([(jx0,jy0),(jx1,jy0),(jx1,jy1),(jx0,jy1)],0.7,40),fill=(255,255,255),w=2.5)
    T(d,jx0+12,(jy0+jy1)/2,'JOB',HAND(13,700),PENCIL,'lm')
    T(d,jx0+56,(jy0+jy1)/2+1,job,TYPE(17,600),INK,'lm')
    if empty:
        T(d,680,260,'no objects yet',HAND(22,500),PENCIL,'mm'); return
    rows=5; y=112
    for r in range(rows):
        i=page*rows+r
        if i>=len(objs): break
        name=objs[i][0]; rh=60
        nb=wobble([(496,y),(552,y),(552,y+rh-8),(496,y+rh-8)],0.7,50+r)
        tb=wobble([(564,y),(864,y),(864,y+rh-8),(564,y+rh-8)],0.7,60+r)
        if i==selected:
            hl=Image.new('RGBA',base.size,(0,0,0,0)); ImageDraw.Draw(hl).rectangle([P(490),P(y-4),P(870),P(y+rh-4)],fill=BLUE_L+(150,))
            base.alpha_composite(hl); d=ImageDraw.Draw(base)
        if i in cancelled:
            poly(d,nb,fill=RED,w=2.5); T(d,524,y+(rh-8)/2+1,str(i+1),HAND(21,700),(255,255,255),'mm')
        else:
            poly(d,nb,fill=None,ol=BLUE if i==selected else INK,w=3.5 if i==selected else 2.5)
            T(d,524,y+(rh-8)/2+1,str(i+1),HAND(21,700),BLUE if i==selected else INK,'mm')
        poly(d,tb,fill=None,ol=BLUE if i==selected else INK,w=3.5 if i==selected else 2.5)
        T(d,580,y+(rh-8)/2+1,name,TYPE(19,500),INK,'lm')
        if i in cancelled:
            tw=d.textlength(name,font=TYPE(19,500))/S
            d.line([P(574),P(y+(rh-8)/2+2),P(586+tw),P(y+(rh-8)/2-1)],fill=RED,width=P(3))
        y+=rh+8

def pager(base,page,pages,text=True):
    card(base,924,112,82,140,70); d=ImageDraw.Draw(base)
    hand_arrow(d,965,226,138,INK if page>0 else (176,176,180))
    card(base,924,266,82,56,71); d=ImageDraw.Draw(base)
    if text: T(d,965,295,f'{page+1}/{pages}',TYPE(22,700),INK,'mm')
    card(base,924,336,82,140-12,72); d=ImageDraw.Draw(base)
    hand_arrow(d,965,350,448,INK if page+1<pages else (176,176,180))

def scene(job='engine_block_v8.gcode',page=0,selected=2,cancelled=(3,),empty=False):
    base=desk()
    folder(base)
    objs=OBJS
    bed_page(base,objs,selected,cancelled,empty)
    list_page(base,job if not empty else '—',objs,page,selected,cancelled,empty)
    d=ImageDraw.Draw(base)
    estop(base,3,3)
    clip(ImageDraw.Draw(base),84,-4,0.95)
    T(ImageDraw.Draw(base),118,44,'just in case!',HAND(15,500),PENCIL,'la')
    home_note(base,914,10)
    pages=max(1,(len(objs)+4)//5) if not empty else 1
    pager(base,page,pages)
    return base

if __name__=='__main__':
    scene().resize((W,H),Image.LANCZOS).convert('RGB').save('obj_printing.png')
    scene(page=1,selected=6,cancelled=(3,)).resize((W,H),Image.LANCZOS).convert('RGB').save('obj_page2.png')
    scene(empty=True).resize((W,H),Image.LANCZOS).convert('RGB').save('obj_idle.png')

