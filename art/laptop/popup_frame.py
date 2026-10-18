from PIL import Image, ImageDraw, ImageFont, ImageFilter
import numpy as np
import os
R=os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)),'..','..'))+'/'
FONT=R+'assets/fonts/Montserrat.ttf'
S=3  # supersample
K=1.43  # popup scale (1.3 x 1.1)
def F(sz,wt=700):
    f=ImageFont.truetype(FONT,sz*S)
    try: f.set_variation_by_axes([wt])
    except Exception: pass
    return f
INK=(16,14,13); BEZ=(38,41,46); BEZ_HI=(70,75,82); BEZ_LO=(22,24,27)
SCR_T=(36,62,98); SCR_B=(20,36,60)
W1=(232,241,251); DIM=(159,183,212); ACC=(127,208,242)
RED=(214,64,46); GRN=(76,175,96); BLU=(52,128,214)

def scene():
    return Image.open(R+'assets/workshop/home_idle.png').convert('RGB')

def frame(w,h):
    """Printer-display style popup: bezel + blue glass. Returns RGBA image and glass rect."""
    pad=18
    W,H=(w+2*pad)*S,(h+2*pad)*S
    im=Image.new('RGBA',(W,H),(0,0,0,0)); d=ImageDraw.Draw(im)
    ox,oy=pad*S,pad*S; x1,y1=ox+w*S,oy+h*S
    r=18*S
    d.rounded_rectangle([ox,oy,x1,y1],radius=r,fill=INK)                       # ink outline
    d.rounded_rectangle([ox+3*S,oy+3*S,x1-3*S,y1-3*S],radius=r-3*S,fill=BEZ)   # bezel
    d.rounded_rectangle([ox+5*S,oy+5*S,x1-5*S,y1-5*S],radius=r-5*S,outline=BEZ_HI,width=S)  # top bevel light
    # glass
    g=(ox+16*S,oy+16*S,x1-16*S,y1-22*S)
    d.rounded_rectangle([g[0]-3*S,g[1]-3*S,g[2]+3*S,g[3]+3*S],radius=9*S,fill=INK)
    gw,gh=g[2]-g[0],g[3]-g[1]
    grad=np.zeros((gh,gw,3),np.float32)
    t=np.linspace(0,1,gh)[:,None,None]
    grad[:]=np.array(SCR_T)*(1-t)+np.array(SCR_B)*t
    yy,xx=np.mgrid[0:gh,0:gw].astype(np.float32)
    glare=np.clip(1-np.hypot((xx-gw*0.82)/(gw*0.35),(yy-gh*0.05)/(gh*0.45)),0,1)*16
    grad+=glare[...,None]
    gimg=Image.fromarray(grad.clip(0,255).astype(np.uint8))
    m=Image.new('L',(gw,gh),0); ImageDraw.Draw(m).rounded_rectangle([0,0,gw-1,gh-1],radius=7*S,fill=255)
    im.paste(gimg,(g[0],g[1]),m)
    # diagonal glare streak like the printer screen
    gl=Image.new('L',(gw,gh),0); dg=ImageDraw.Draw(gl)
    dg.polygon([(gw*0.62,0),(gw*0.70,0),(gw*0.52,gh),(gw*0.44,gh)],fill=14)
    gl=gl.filter(ImageFilter.GaussianBlur(6*S))
    im.paste(Image.new('RGB',(gw,gh),(255,255,255)),(g[0],g[1]),Image.fromarray((np.asarray(gl)*(np.asarray(m)>0)).astype(np.uint8)))
    # lower bezel lip with two screws, like a device front
    for sx in (ox+28*S, x1-28*S):
        cy=y1-11*S
        d.ellipse([sx-4*S,cy-4*S,sx+4*S,cy+4*S],fill=INK)
        d.ellipse([sx-3*S,cy-3*S,sx+3*S,cy+3*S],fill=(120,124,130))
        d.line([sx-2*S,cy+2*S,sx+2*S,cy-2*S],fill=(60,62,66),width=S)
    # hinge bracket on top (as on the printer)
    hx=(ox+x1)//2
    d.rounded_rectangle([hx-46*S,oy-10*S,hx+46*S,oy+6*S],radius=4*S,fill=INK)
    d.rounded_rectangle([hx-43*S,oy-7*S,hx+43*S,oy+4*S],radius=3*S,fill=BEZ_LO)
    for sx in (hx-30*S,hx+30*S):
        d.ellipse([sx-3*S,oy-4*S,sx+3*S,oy+2*S],fill=(110,114,120))
    return im,(g[0]//S-pad,g[1]//S-pad,g[2]//S-pad,g[3]//S-pad),pad

def btn(d,x,y,w,h,label,color,icon=None,font=None,outline=False,fg=W1):
    x,y,w,h=[v*S for v in (x,y,w,h)]
    if outline:
        d.rounded_rectangle([x,y,x+w,y+h],radius=8*S,outline=color,width=2*S,fill=(30,52,82))
    else:
        d.rounded_rectangle([x,y,x+w,y+h],radius=8*S,fill=color)
        d.rounded_rectangle([x,y,x+w,y+h//2],radius=8*S,fill=tuple(min(255,c+14) for c in color))
        d.rectangle([x,y+h//2-8*S,x+w,y+h//2],fill=tuple(min(255,c+14) for c in color))
    cx,cy=x+w//2,y+h//2
    font=font or F(20)
    tw=d.textlength(label,font=font) if label else 0
    iw=0
    if icon:
        s=int(h*0.30)   # icons 25 % smaller than full, strokes ~3x thicker
        lw=max(3*S,int(h*0.11))
        iw=2*s+(10*S if label else 0)
        ix=cx-(iw+tw)//2+s; iy=cy
        if icon=='x':
            d.line([ix-s*0.75,iy-s*0.75,ix+s*0.75,iy+s*0.75],fill=fg,width=lw)
            d.line([ix-s*0.75,iy+s*0.75,ix+s*0.75,iy-s*0.75],fill=fg,width=lw)
        elif icon=='trash':
            # classic outline bin: lid with handle, tapered body, three ribs
            t=int(lw*0.8)
            top=iy-s*0.55; bot=iy+s*0.95; wt=s*0.72; wb=s*0.56
            d.rounded_rectangle([ix-s*0.32,top-s*0.36,ix+s*0.32,top+t*0.2],radius=int(s*0.12),outline=W1,width=t)   # handle
            d.rounded_rectangle([ix-s*0.98,top-t*0.5,ix+s*0.98,top+t*0.6],radius=int(t*0.5),fill=fg)               # lid
            body=[(ix-wt,top+s*0.25),(ix+wt,top+s*0.25),(ix+wb,bot),(ix-wb,bot)]
            d.line(body+[body[0]],fill=fg,width=t,joint='curve')
            for k in (-0.34,0,0.34):
                d.line([ix+k*wt*1.25,top+s*0.5,ix+k*wb*1.25,bot-s*0.25],fill=fg,width=int(t*0.75))
        else:
            d.line([ix-s*0.85,iy+s*0.05,ix-s*0.25,iy+s*0.65,ix+s*0.9,iy-s*0.6],fill=fg,width=lw,joint='curve')
    if label:
        tx=cx-(iw+tw)//2+iw
        bb=d.textbbox((0,0),label,font=font)
        d.text((tx,cy-(bb[1]+bb[3])/2),label,font=font,fill=fg)

def text(d,x,y,s,font,fill=W1,anchor='la'):
    d.text((x*S,y*S),s,font=font,fill=fill,anchor=anchor)

def compose(w,h,draw_content,name):
    bg=scene().convert('RGBA')
    dim=Image.new('RGBA',bg.size,(8,6,5,125)); bg=Image.alpha_composite(bg,dim)
    fr,g,pad=frame(w,h)
    d=ImageDraw.Draw(fr)
    gx0,gy0,gx1,gy1=g[0]+pad,g[1]+pad,g[2]+pad,g[3]+pad
    draw_content(d,gx0,gy0,gx1,gy1)
    fr=fr.resize((round(fr.width/S*K),round(fr.height/S*K)),Image.LANCZOS)
    px,py=(1024-fr.width)//2,max(4,(600-fr.height)//2+4)
    sh=Image.new('L',bg.size,0); a=fr.split()[3].point(lambda v:150 if v>0 else 0)
    sh.paste(a,(px+8,py+12)); sh=sh.filter(ImageFilter.GaussianBlur(10))
    bg=Image.composite(Image.new('RGBA',bg.size,(0,0,0,255)),bg,sh)
    bg.alpha_composite(fr,(px,py))
    bg.convert('RGB').save(name)
    return bg

# 1 ── confirm
def c_confirm(d,x0,y0,x1,y1):
    text(d,x0+24,y0+20,'CANCEL PRINT?',F(26,800),W1)
    text(d,x1-24,y0+26,'T0',F(16,600),DIM,'ra')
    d.line([(x0+24)*S,(y0+62)*S,(x1-24)*S,(y0+62)*S],fill=(70,98,136),width=S)
    f=F(19,500)
    for i,l in enumerate(['engine_block_v8.gcode  ·  42 %','The print stops and the part stays on the bed.','Heaters keep their temperatures.']):
        text(d,x0+24,y0+84+i*32,l,f,W1 if i==0 else DIM)
    bw=(x1-x0-24*3)//2
    btn(d,x0+24,y1-86,bw,62,'CANCEL',RED,'x')
    btn(d,x0+48+bw,y1-86,bw,62,'STOP PRINT',GRN,'check')
compose(620,330,c_confirm,'popup_confirm.png')

# 2 ── number entry (landscape so it fits at 1.3x)
def c_num(d,x0,y0,x1,y1):
    text(d,x0+24,y0+18,'NOZZLE T0',F(22,800),W1)
    text(d,x0+24,y0+46,'ACTIVE  ·  now 24 °C',F(15,600),DIM)
    lw=250
    d.rounded_rectangle([(x0+24)*S,(y0+80)*S,(x0+24+lw)*S,(y0+150)*S],radius=8*S,fill=(10,20,36),outline=ACC,width=2*S)
    text(d,x0+24+lw-56,y0+115,'215',F(40,800),W1,'rm')
    text(d,x0+24+lw-14,y0+117,'°C',F(20,600),DIM,'rm')
    text(d,x0+26,y0+168,'range 0 – 300 °C',F(14,600),DIM)
    keys=['7','8','9','4','5','6','1','2','3','DEL','0','.']
    kx0=x0+24+lw+20; kw=(x1-24-kx0-2*8)//3; kh=40
    for i,k in enumerate(keys):
        btn(d,kx0+(i%3)*(kw+8),y0+18+(i//3)*(kh+8),kw,kh,k,(44,72,110),font=F(20,700))
    bw=(x1-x0-24*3)//2
    btn(d,x0+24,y1-78,bw,56,'CANCEL',RED,'x')
    btn(d,x0+48+bw,y1-78,bw,56,'SET',GRN,'check')
compose(600,345,c_num,'popup_number.png')

# 3 ── filament picker (paged 3x2, blue UNLOAD)
def c_fil(d,x0,y0,x1,y1):
    text(d,x0+24,y0+18,'T0 FILAMENT',F(24,800),W1)
    text(d,x1-24,y0+24,'PAGE 1 / 4',F(16,600),DIM,'ra')
    mats=['PLA','PETG','ABS','ASA','TPU','PA']
    gx=x0+70; gw=x1-x0-140
    cw=(gw-2*14)//3; ch=70
    for i,mname in enumerate(mats):
        cx=gx+(i%3)*(cw+14); cy=y0+66+(i//3)*(ch+14)
        sel=(i==1)
        btn(d,cx,cy,cw,ch,mname,(44,72,110) if not sel else (32,110,150),font=F(24,800))
        if sel:
            d.rounded_rectangle([cx*S-3*S,cy*S-3*S,(cx+cw)*S+3*S,(cy+ch)*S+3*S],radius=10*S,outline=ACC,width=3*S)
    # page arrows
    for side,xx in (('<',x0+24),('>',x1-60)):
        btn(d,xx,y0+66,36,154,side,(30,52,82),font=F(26,800))
    bw=(x1-x0-24*4)//3
    btn(d,x0+24,y1-84,bw,62,'UNLOAD',BLU,font=F(21,800))
    btn(d,x0+48+bw,y1-84,bw,62,'',RED,'x')
    btn(d,x0+72+2*bw,y1-84,bw,62,'',GRN,'check')
compose(640,360,c_fil,'popup_filament.png')

# 4 ── firmware message (M291)
def c_msg(d,x0,y0,x1,y1):
    text(d,x0+24,y0+18,'PROBE CHECK',F(24,800),W1)
    text(d,x1-24,y0+24,'M291',F(16,600),DIM,'ra')
    f=F(19,500)
    for i,l in enumerate(['Fit the Z probe and jog the nozzle until','it just touches a sheet of paper.']):
        text(d,x0+24,y0+70+i*30,l,f,W1)
    labels=['−0.05','−0.01','+0.01','+0.05']
    jw=(x1-x0-24*2-10*3)//4
    for i,l in enumerate(labels):
        btn(d,x0+24+i*(jw+10),y0+148,jw,48,'Z '+l,(44,72,110),font=F(18,700))
    bw=(x1-x0-24*3)//2
    btn(d,x0+24,y1-84,bw,62,'CANCEL',RED,'x')
    btn(d,x0+48+bw,y1-84,bw,62,'OK',GRN,'check')
compose(620,330,c_msg,'popup_message.png')

ims=[Image.open(n) for n in ['popup_confirm.png','popup_number.png','popup_filament.png','popup_message.png']]
sheet=Image.new('RGB',(2048,1200))
for i,im in enumerate(ims): sheet.paste(im,((i%2)*1024,(i//2)*600))
sheet.save('popup_sheet.png')
