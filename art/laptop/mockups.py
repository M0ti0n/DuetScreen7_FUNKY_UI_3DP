from PIL import Image, ImageDraw, ImageFont, ImageFilter
import numpy as np
import os
R=os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)),'..','..'))+'/'
S=2; W,H=1024,600
def F(sz,wt=600,mono=False):
    f=ImageFont.truetype(R+('assets/fonts/GoogleSansCode.ttf' if mono else 'assets/fonts/Montserrat.ttf'),int(sz*S))
    try: f.set_variation_by_axes([wt])
    except Exception: pass
    return f
INK=(14,13,12); BODY=(60,62,67); BODY_HI=(84,87,93); BODY_LO=(40,41,45)
LCD_T=(20,32,50); LCD_B=(10,16,27)
W1=(232,241,251); DIM=(150,170,196); ACC=(127,208,242); TILE=(26,40,62); OL=(64,88,122)
RED=(214,64,46); AMB=(240,170,60); GRN=(76,175,96)
KEY_TOP=(57,58,63); KEY_SIDE=(32,32,35); LEG=(236,236,236)

def P(v): return int(round(v*S))
def backdrop():
    sc=Image.open(R+'assets/workshop/home_idle.png').convert('RGB')
    bg=sc.crop((150,170,620,446)).resize((W*S,H*S),Image.BICUBIC).filter(ImageFilter.GaussianBlur(10*S))
    a=np.asarray(bg).astype(np.float32)*0.45
    return Image.fromarray(a.astype(np.uint8)).convert('RGBA')

def lcd_bg(w,h):
    t=np.linspace(0,1,h)[:,None,None]
    g=np.array(LCD_T)*(1-t)+np.array(LCD_B)*t
    g=np.broadcast_to(g,(h,w,3)).copy()
    yy,xx=np.mgrid[0:h,0:w].astype(np.float32)
    g+=np.clip(1-np.hypot((xx-w*0.85)/(w*0.4),(yy-h*0.0)/(h*0.6)),0,1)[...,None]*14
    return Image.fromarray(g.clip(0,255).astype(np.uint8)).convert('RGBA')

def T(d,x,y,s,f,c=W1,a='la'): d.text((P(x),P(y)),s,font=f,fill=c,anchor=a)
def tile(d,x,y,w,h,label=None,sel=False,f=None,fill=TILE,ol=OL,col=W1):
    d.rounded_rectangle([P(x),P(y),P(x+w),P(y+h)],radius=P(8),fill=fill,outline=ACC if sel else ol,width=P(2.5 if sel else 1.5))
    if label: T(d,x+w/2,y+h/2+1,label,f or F(19,700),col,'mm')

def home_icon(d,cx,cy,s,c=W1):
    # Modern UI home-axis house, without the axis letter: thin roof with overhang + open body
    lw=P(2)
    d.line([P(cx-s*1.3),P(cy-s*0.05),P(cx),P(cy-s*1.0),P(cx+s*1.3),P(cy-s*0.05)],fill=c,width=lw,joint='curve')
    d.line([P(cx-s*0.85),P(cy-s*0.38),P(cx-s*0.85),P(cy+s*0.95),P(cx+s*0.85),P(cy+s*0.95),P(cx+s*0.85),P(cy-s*0.38)],fill=c,width=lw,joint='curve')
    # door (sets the screen-home icon apart from the homing buttons)
    d.line([P(cx-s*0.28),P(cy+s*0.95),P(cx-s*0.28),P(cy+s*0.2),P(cx+s*0.28),P(cy+s*0.2),P(cx+s*0.28),P(cy+s*0.95)],fill=c,width=lw,joint='curve')
    d.ellipse([P(cx+s*0.12),P(cy+s*0.55),P(cx+s*0.2),P(cy+s*0.63)],fill=c)
def kb_icon(d,cx,cy,s,c=W1):
    d.rounded_rectangle([P(cx-s*1.4),P(cy-s*0.8),P(cx+s*1.4),P(cy+s*0.8)],radius=P(3),outline=c,width=P(2.5))
    for r in range(2):
        for i in range(5): d.rectangle([P(cx-s*1.05+i*s*0.52),P(cy-s*0.42+r*s*0.42),P(cx-s*0.85+i*s*0.52),P(cy-s*0.25+r*s*0.42)],fill=c)
    d.rectangle([P(cx-s*0.6),P(cy+s*0.38),P(cx+s*0.6),P(cy+s*0.52)],fill=c)

def side_tabs(d,x0,y0,y1,active,tw=168):
    names=['STOP','ALERTS','CONSOLE','SETTINGS','HOME']; gap=10
    th=(y1-y0-gap*4)/5
    for i,n in enumerate(names):
        y=y0+i*(th+gap)
        if n=='STOP':
            d.rounded_rectangle([P(x0),P(y),P(x0+tw),P(y+th)],radius=P(8),fill=(170,40,30),outline=(235,95,75),width=P(2))
            T(d,x0+tw/2,y+th/2+1,'STOP',F(21,800),W1,'mm')
        elif n=='HOME':
            tile(d,x0,y,tw,th); home_icon(d,x0+tw/2,y+th/2+2,20)
        else:
            tile(d,x0,y,tw,th,n,sel=(n==active),f=F(18,800),fill=(32,52,80) if n==active else TILE)
        if n=='ALERTS':
            d.ellipse([P(x0+tw-30),P(y+10),P(x0+tw-10),P(y+30)],fill=INK); d.ellipse([P(x0+tw-28),P(y+12),P(x0+tw-12),P(y+28)],fill=RED)
    return x0+tw

def tabs(d,x0,y0,x1,active):
    names=['ALERTS','CONSOLE','SETTINGS']; hw=58; gap=10
    tw=(x1-x0-hw-gap*3)/3
    for i,n in enumerate(names):
        tile(d,x0+i*(tw+gap),y0,tw,42,n,sel=(n==active),f=F(19,800),fill=(32,52,80) if n==active else TILE)
    hx=x1-hw; tile(d,hx,y0,hw,42); home_icon(d,hx+hw/2,y0+22,11)
    if active=='ALERTS':
        d.ellipse([P(x0+tw-26),P(y0+12),P(x0+tw-10),P(y0+28)],fill=RED)
        T(d,x0+tw-18,y0+20,'2',F(11,800),W1,'mm')

LOG=[('42m10','PanelDue connected to printer'),('42m09','M409 object model stream started'),('41m31','Tool 0 active'),
     ('41m30','Heater 1 set to active temperature 215C'),('40m54','Bed heater set to 60C'),('40m25','Network IP updated: 192.168.1.42'),
     ('39m20','G28'),('39m03','G1 X125 Y87.5 Z12.4 F12000'),('38m27','M32 calibration_cube.gcode'),('38m26','Print started')]
def console(d,x0,y0,x1,y1,lines,typed=None,cursor=True):
    fm=F(18,500,mono=True)
    lh=30
    while lines and y1-62-len(lines)*lh<y0: lines=lines[1:]
    y=y1-62-len(lines)*lh
    for t,m in lines:
        T(d,x0+6,y,t,fm,DIM); T(d,x0+92,y,m,fm,W1 if not m.startswith(('G','M')) else (170,220,255)); y+=lh
    # input line
    iy=y1-50
    tile(d,x0,iy,x1-x0-70,44,fill=(10,18,30))
    T(d,x0+14,iy+23,'>',F(19,700,True),ACC,'lm')
    txt=typed if typed is not None else ''
    T(d,x0+36,iy+23,txt,F(19,600,True),W1,'lm')
    if cursor:
        cx=x0+36+d.textlength(txt,font=F(19,600,True))/S+3
        d.rectangle([P(cx),P(iy+10),P(cx+11),P(iy+34)],fill=ACC)
    tile(d,x1-60,iy,60,44); kb_icon(d,x1-30,iy+22,10)

def screen_content(w,h,tab,typed=None,short=False,kb=False):
    im=lcd_bg(P(w),P(h)); d=ImageDraw.Draw(im)
    cx0=side_tabs(d,14,14,h-14,tab)+16
    if kb:  # keyboard open: column slides off the top and is inactive
        ov=Image.new('RGBA',im.size,(0,0,0,0)); ImageDraw.Draw(ov).rectangle([0,0,P(cx0-6),im.height],fill=(8,12,20,150)); im.alpha_composite(ov); d=ImageDraw.Draw(im)
    if tab=='CONSOLE':
        console(d,cx0,14,w-14,h-8,LOG[-4:] if short else LOG,typed)
    elif tab=='ALERTS':
        rows=[('ERR','0m41','Heater 1 fault: temp. rising too slowly',RED),
              ('WRN','9m30','Filament runout on T0, print paused',AMB),
              ('INFO','14m02','Print started: calibration_cube.gcode',ACC),
              ('WRN','15m30','Driver 2 warning: phase A may be disconnected',AMB),
              ('INFO','17m08','Heater 1 set to 215C',ACC),
              ('ERR','17m19','Z probe was not triggered',RED),
              ('INFO','1h02','Connected to printer (USB)',ACC)]
        # controls column: up / filter / down
        cw=74; gap=10; ch=(h-28-2*gap)/3
        lx=cx0; cx0=w-14-cw
        tile(d,cx0,14,cw,ch); up=[(cx0+cw/2-14,14+ch/2+7),(cx0+cw/2,14+ch/2-8),(cx0+cw/2+14,14+ch/2+7)]
        d.line([(P(x),P(y)) for x,y in up],fill=W1,width=P(4),joint='curve')
        fy=14+ch+gap; tile(d,cx0,fy,cw,ch,sel=True)
        T(d,cx0+cw/2,fy+ch/2-12,'ALL',F(19,800),W1,'mm')
        T(d,cx0+cw/2,fy+ch/2+16,'1/3',F(14,600),DIM,'mm')
        dy=14+2*(ch+gap); tile(d,cx0,dy,cw,ch); dn=[(cx0+cw/2-14,dy+ch/2-7),(cx0+cw/2,dy+ch/2+8),(cx0+cw/2+14,dy+ch/2-7)]
        d.line([(P(x),P(y)) for x,y in dn],fill=W1,width=P(4),joint='curve')
        rh=(h-28-6*8)/7; y=14
        for k,(lvl,t,m,c) in enumerate(rows):
            tile(d,lx,y,cx0-gap-lx,rh)
            if k<2:  # unread
                ux=cx0-gap-18; d.ellipse([P(ux-5),P(y+rh/2-5),P(ux+5),P(y+rh/2+5)],fill=ACC)
            d.rounded_rectangle([P(lx+10),P(y+rh/2-13),P(lx+70),P(y+rh/2+13)],radius=P(5),outline=c,width=P(2))
            T(d,lx+40,y+rh/2,lvl,F(15,800),c,'mm')
            T(d,lx+82,y+rh/2,t,F(16,500,True),DIM,'lm'); T(d,lx+170,y+rh/2,m,F(17,600),W1,'lm')
            y+=rh+8
    else:
        def vtile(x,y,tw,th,label,val,danger=False):
            if danger:
                tile(d,x,y,tw,th,label,f=F(18,800),fill=(150,50,36),ol=(200,80,60)); return
            tile(d,x,y,tw,th)
            fs=17
            while d.textlength(label,font=F(fs,800))/S>tw-36: fs-=1
            T(d,x+tw/2,y+th/2-12,label,F(fs,800),W1,'mm')
            T(d,x+tw/2,y+th/2+14,val,F(16,600),ACC,'mm')
        def section(y,name):
            T(d,cx0+4,y+12,name,F(15,800),DIM,'lm')
            tw_=d.textlength(name,font=F(15,800))/S
            d.line([P(cx0+tw_+16),P(y+12),P(w-14-84),P(y+12)],fill=OL,width=P(1.5))
        page=getattr(screen_content,'page',1)
        # right column: page up / page indicator / page down (same as ALERTS)
        cw=74; gp=10; ch=(h-28-2*gp)/3; rx=w-14-cw
        tile(d,rx,14,cw,ch); up=[(rx+cw/2-14,14+ch/2+7),(rx+cw/2,14+ch/2-8),(rx+cw/2+14,14+ch/2+7)]
        d.line([(P(x),P(y)) for x,y in up],fill=W1 if page==2 else (90,105,128),width=P(4),joint='curve')
        my=14+ch+gp
        T(d,rx+cw/2,my+ch/2,f'{page}/2',F(18,700),DIM,'mm')
        dy=14+2*(ch+gp); tile(d,rx,dy,cw,ch); dn=[(rx+cw/2-14,dy+ch/2-7),(rx+cw/2,dy+ch/2+8),(rx+cw/2+14,dy+ch/2-7)]
        d.line([(P(x),P(y)) for x,y in dn],fill=W1 if page==1 else (90,105,128),width=P(4),joint='curve')
        gx=14; cols=3; right=rx-gp; tw=(right-cx0-gx*(cols-1))/cols; th=92
        y=14; section(y,'SYSTEM' if page==1 else 'PERSONALIZATION')
        d.line([P(right),P(y+12),P(w-14),P(y+12)],fill=(0,0,0,0),width=1)
        y+=36
        if page==1:
            items=[('BRIGHTNESS','80%'),('SOUND','SPEAKER 2'),('SCREEN DIMMING','5 MIN'),
                   ('ORIENTATION','NORMAL'),('CONNECTION','USB'),('ABOUT','v0.1'),(None,None),(None,None),('FACTORY RESET','')]
        else:
            items=[('DRAWER INDICATOR','ON')]
        for i,(l,v) in enumerate(items):
            if l is None: continue
            vtile(cx0+(i%3)*(tw+gx),y+(i//3)*(th+16),tw,th,l,v,danger=(l=='FACTORY RESET'))
    return im

def paste_lcd(base,content,quad):
    # quad: TL TR BR BL in screen px
    w,h=content.size
    import cv2
    src=np.float32([(0,0),(w,0),(w,h),(0,h)]); dst=np.float32([(x*S,y*S) for x,y in quad])
    M=cv2.getPerspectiveTransform(src,dst)
    arr=cv2.warpPerspective(np.asarray(content),M,base.size,flags=cv2.INTER_AREA,borderMode=cv2.BORDER_CONSTANT,borderValue=(0,0,0,0))
    base.alpha_composite(Image.fromarray(arr))

def key(d,x,y,w,h,label='',f=None,icon=None,legend=LEG):
    d.rounded_rectangle([P(x),P(y),P(x+w),P(y+h)],radius=P(6),fill=INK)
    d.rounded_rectangle([P(x+1.5),P(y+1.5),P(x+w-1.5),P(y+h-1.5)],radius=P(5),fill=KEY_SIDE)
    d.rounded_rectangle([P(x+4),P(y+2.5),P(x+w-4),P(y+h-7)],radius=P(5),fill=KEY_TOP)
    d.line([P(x+7),P(y+4),P(x+w-7),P(y+4)],fill=(86,87,94),width=P(1))
    cx,cy=x+w/2,y+(h-5)/2
    if icon=='bksp':
        s=10; pts=[(cx-s*1.3,cy),(cx-s*0.6,cy-s*0.7),(cx+s*1.2,cy-s*0.7),(cx+s*1.2,cy+s*0.7),(cx-s*0.6,cy+s*0.7)]
        d.line([(P(a),P(b)) for a,b in pts+[pts[0],pts[1]]],fill=legend,width=P(2.5),joint='curve')
        d.line([P(cx-1),P(cy-4),P(cx+7),P(cy+4)],fill=legend,width=P(2.5)); d.line([P(cx-1),P(cy+4),P(cx+7),P(cy-4)],fill=legend,width=P(2.5))
    elif icon=='enter':
        d.line([P(cx+12),P(cy-9),P(cx+12),P(cy+4),P(cx-12),P(cy+4)],fill=legend,width=P(3),joint='curve')
        d.line([P(cx-12),P(cy+4),P(cx-5),P(cy-3)],fill=legend,width=P(3)); d.line([P(cx-12),P(cy+4),P(cx-5),P(cy+11)],fill=legend,width=P(3))
    elif icon=='hide':
        kb_icon(d,cx,cy-5,8,legend)
        d.line([P(cx-6),P(cy+9),P(cx),P(cy+14),P(cx+6),P(cy+9)],fill=legend,width=P(2.5),joint='curve')
    elif icon in ('up','down','left','right'):
        # arrow with a shaft
        s=10; dx,dy={'up':(0,-1),'down':(0,1),'left':(-1,0),'right':(1,0)}[icon]
        d.line([P(cx-dx*s),P(cy-dy*s),P(cx+dx*s),P(cy+dy*s)],fill=legend,width=P(3))
        px,py=-dy,dx
        hx,hy=cx+dx*s,cy+dy*s
        d.line([P(hx-dx*7+px*7),P(hy-dy*7+py*7),P(hx),P(hy),P(hx-dx*7-px*7),P(hy-dy*7-py*7)],fill=legend,width=P(3),joint='curve')
    elif icon=='shift':
        s=10; pts=[(cx,cy-s),(cx+s,cy),(cx+s*0.45,cy),(cx+s*0.45,cy+s*0.8),(cx-s*0.45,cy+s*0.8),(cx-s*0.45,cy),(cx-s,cy)]
        d.line([(P(a),P(b)) for a,b in pts+[pts[0]]],fill=legend,width=P(2.5),joint='curve')
    elif label:
        T(d,cx,cy,label,f or F(20,600),legend,'mm')

def key_l(d,a,b,legend=ACC):
    """ISO-style L-shaped ENTER: a = tall right bar (x,y,w,h), b = lower wide part"""
    layers=[(0,0,0,0,6,INK),(1.5,1.5,1.5,1.5,5,KEY_SIDE),(4,2.5,4,7,5,KEY_TOP)]
    for l,t,r,bt,rad,c in layers:
        for x,y,w,h in (a,b):
            d.rounded_rectangle([P(x+l),P(y+t),P(x+w-r),P(y+h-bt)],radius=P(rad),fill=c)
    d.line([P(a[0]+7),P(a[1]+4),P(a[0]+a[2]-7),P(a[1]+4)],fill=(86,87,94),width=P(1))
    d.line([P(b[0]+7),P(b[1]+4),P(a[0]-4),P(b[1]+4)],fill=(86,87,94),width=P(1))
    cx,cy=b[0]+b[2]*0.55,b[1]+(b[3]-5)/2
    d.line([P(cx+18),P(cy-14),P(cx+18),P(cy+5),P(cx-14),P(cy+5)],fill=legend,width=P(3),joint='curve')
    d.line([P(cx-14),P(cy+5),P(cx-6),P(cy-3)],fill=legend,width=P(3)); d.line([P(cx-14),P(cy+5),P(cx-6),P(cy+13)],fill=legend,width=P(3))

def keyboard(d,x0,y0,x1,y1,shift=False):
    """Draws the keys; returns [(kind, normal, shifted, (x,y,w,h))] for the tap zones"""
    rows=["1234567890-+","QWERTYUIOP[]","ASDFGHJKL:@","ZXCVBNM,./"]
    srows=['!"#$%^&*()_=',"qwertyuiop{}","asdfghjkl;'","zxcvbnm<>?"]
    g=7; nr=5; kh=(y1-y0-g*(nr-1))/nr
    u=(x1-x0-12*g)/13.5
    keys=[]; pos={}
    # bottom row widths: SHIFT, SPACE, HIDE, then the arrow block (3 keys)
    # bottom row: SHIFT, SPACE, HIDE, then the arrow block; arrow keys as wide as the letter keys
    aw=u; block=3*aw+2*g
    ws=[2.2,7.0,1.6]; tot=sum(ws); avail=x1-x0-3*g-block
    widths=[avail*w/tot for w in ws]+[block]
    ax0=x1-block
    for r in range(4):
        y=y0+r*(kh+g); x=x0+[0,0.5,0.75,1.0][r]*u
        for i,ch in enumerate(rows[r]):
            sh=srows[r][i]
            key(d,x,y,u,kh,sh if shift else ch); pos[ch]=(x,y)
            keys.append(('Char',ch,sh,(x,y,u,kh))); x+=u+g
        rest=x1-x
        if r==0:
            key(d,x,y,rest,kh,icon='bksp'); keys.append(('Backspace',None,None,(x,y,rest,kh)))
        elif r==1:
            enter_top=(x,y,rest,kh)
        elif r==2:
            ex=x
            a=(enter_top[0],enter_top[1],enter_top[2],y+kh-enter_top[1]); b=(ex,y,x1-ex,kh)
            key_l(d,a,b)
            keys.append(('Enter',None,None,(a[0],a[1],a[2],a[3]-kh-g)))
            keys.append(('Enter',None,None,b))
        else:
            # UP, above DOWN (bare deck beside it)
            ux=ax0+aw+g
            key(d,ux,y,aw,kh,icon='up'); keys.append(('Up',None,None,(ux,y,aw,kh)))
    y=y0+4*(kh+g); x=x0
    key(d,x,y,widths[0],kh,icon='shift'); keys.append(('Shift',None,None,(x,y,widths[0],kh))); x+=widths[0]+g
    key(d,x,y,widths[1],kh,''); keys.append(('Space',None,None,(x,y,widths[1],kh))); x+=widths[1]+g
    key(d,x,y,widths[2],kh,icon='hide'); keys.append(('Hide',None,None,(x,y,widths[2],kh)))
    for i,(k,ic) in enumerate([('Left','left'),('Down','down'),('Right','right')]):
        kx=ax0+i*(aw+g)
        key(d,kx,y,aw,kh,icon=ic); keys.append((k,None,None,(kx,y,aw,kh)))
    # red TrackPoint between G H B
    gx,gy=pos['G']; cx=gx+u+g/2; cy=gy+kh+g/2
    d.ellipse([P(cx-7),P(cy-7),P(cx+7),P(cy+7)],fill=INK); d.ellipse([P(cx-5.5),P(cy-5.5),P(cx+5.5),P(cy+5.5)],fill=(200,30,36))
    d.ellipse([P(cx-3),P(cy-4),P(cx),P(cy-1)],fill=(240,110,110))
    return keys

def deck(d,top,bottom=None):
    bottom=bottom or H+20
    d.rounded_rectangle([P(18),P(top),P(W-18),P(bottom)],radius=P(18),fill=INK)
    d.rounded_rectangle([P(21),P(top+3),P(W-21),P(bottom)],radius=P(16),fill=BODY)
    d.line([P(30),P(top+5),P(W-30),P(top+5)],fill=BODY_HI,width=P(2))
    # little speaker grilles / status LEDs (T-series vibe)
    for i in range(3): d.ellipse([P(W/2-24+i*20),P(top+9),P(W/2-20+i*20),P(top+13)],fill=(80,200,110) if i==0 else (70,72,76))

def lid(d,quad,lcd_inset):
    (tlx,tly),(trx,try_),(brx,bry),(blx,bly)=quad
    d.polygon([(P(tlx-14),P(tly-12)),(P(trx+14),P(try_-12)),(P(brx+16),P(bry+12)),(P(blx-16),P(bly+12))],fill=INK)
    d.polygon([(P(tlx-11),P(tly-9)),(P(trx+11),P(try_-9)),(P(brx+13),P(bry+9)),(P(blx-13),P(bly+9))],fill=BODY)
    d.line([P(tlx-6),P(tly-5),P(trx+6),P(try_-5)],fill=BODY_HI,width=P(2))
    # LCD inner ink rim
    d.polygon([(P(tlx-3),P(tly-3)),(P(trx+3),P(try_-3)),(P(brx+3),P(bry+3)),(P(blx-3),P(bly+3))],fill=INK)
    # brand-free logo strip on the lower bezel

def hinge(d,y):
    d.rounded_rectangle([P(60),P(y),P(W-60),P(y+16)],radius=P(6),fill=INK)
    d.rounded_rectangle([P(63),P(y+2),P(W-63),P(y+13)],radius=P(5),fill=BODY_LO)

def state_screen(tab,out):
    im=backdrop(); d=ImageDraw.Draw(im)
    deck(d,520)
    keyboard(d,60,548,W-60,548+5*60)   # top row peeks out
    quad=[(46,24),(W-46,24),(W-46,506),(46,506)]
    lid(d,quad,0)
    hinge(d,512)
    content=screen_content(quad[1][0]-quad[0][0],quad[2][1]-quad[0][1],tab)
    paste_lcd(im,content,quad)
    im.resize((W,H),Image.LANCZOS).convert('RGB').save(out)

def state_keyboard(typed,out,shift=False):
    im=backdrop(); d=ImageDraw.Draw(im)
    deck(d,236)
    keyboard(d,34,262,W-34,592,shift)
    quad=[(46,18),(W-46,18),(W-46,222),(46,222)]
    lid(d,quad,0)
    hinge(d,228)
    w=quad[2][0]-quad[3][0]; h=quad[2][1]-quad[0][1]
    full=screen_content(w,476,'CONSOLE',typed=typed,kb=True)
    content=full.crop((0,full.height-P(h),full.width,full.height))
    # keep only tabs + the bottom part (half screen visible)
    content=content.crop((0,0,content.width,content.height))
    paste_lcd(im,content,quad)
    im.resize((W,H),Image.LANCZOS).convert('RGB').save(out)

state_screen('CONSOLE','lap_console.png')
state_screen('ALERTS','lap_alerts.png')
state_screen('SETTINGS','lap_settings.png')
screen_content.page=2
state_screen('SETTINGS','lap_settings_p2.png')
screen_content.page=1
state_keyboard('G1 X125 Y87.5 F6000','lap_keyboard.png')
state_keyboard('M98 P"homeall.g"','lap_keyboard_shift.png',shift=True)
