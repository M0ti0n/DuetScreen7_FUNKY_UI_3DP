import os
R=os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)),'..','..'))+'/'
src=open(R+'art/objects/mockups.py').read().split("if __name__==")[0]
g={'__file__':R+'art/objects/mockups.py'}; exec(src,g); globals().update({k:v for k,v in g.items() if not k.startswith('__')})

FILES=[('engine_block_PETG.gcode','2h ago'),('calibration_cube_PLA.gcode','5h ago'),('benchy_0p2_PLA.gcode','1d ago'),
       ('first_layer_test_PLA.gcode','2d ago'),('temp_tower_PETG.gcode','3d ago'),('badge_dual_ASA.gcode','5d ago'),
       ('drawer_handle_PLA.gcode','1w ago'),('cable_clips_TPU.gcode','2w ago')]
MATS=('PLA','PETG','ABS','ASA','TPU','PA','PC','PVA','HIPS')
def material(n):
    t=n.rsplit('.',1)[0].rsplit('_',1)[-1].upper()
    return t if t in MATS else ''

def thumb(base,x,y,s=250):
    shadow(base,lambda d:d.rectangle([P(x),P(y),P(x+s),P(y+s)],fill=255),(4,6),5,100)
    d=ImageDraw.Draw(base)
    d.rectangle([P(x),P(y),P(x+s),P(y+s)],fill=(255,255,255),outline=INK,width=P(2))
    ix,iy,iw=x+8,y+8,s-16
    im=Image.new('RGB',(P(iw),P(iw)),(148,143,134)); dd=ImageDraw.Draw(im)   # old-photo grey behind the thumbnail
    cx,cy=iw/2,iw/2+10
    def q(pts,c): dd.polygon([(P(a),P(b)) for a,b in pts],fill=c)
    w,t=78,40
    q([(cx,cy-60-t),(cx+w,cy-30-t),(cx,cy-t),(cx-w,cy-30-t)],(244,150,52))
    q([(cx-w,cy-30-t),(cx,cy-t),(cx,cy),(cx-w,cy-30)],(200,108,30))
    q([(cx+w,cy-30-t),(cx,cy-t),(cx,cy),(cx+w,cy-30)],(168,86,22))
    dd.ellipse([P(cx-22),P(cy-30-t-12),P(cx+22),P(cy-30-t+12)],fill=(120,62,16))
    for k in range(-4,5): dd.line([P(cx+k*22-60),P(cy+40),P(cx+k*22+60),P(cy+70)],fill=(70,78,88),width=P(1))
    base.paste(im,(P(ix),P(iy)))
    d=ImageDraw.Draw(base)
    # tape strips at 45 degrees over the two upper corners
    import math
    tape=Image.new('RGBA',base.size,(0,0,0,0)); td=ImageDraw.Draw(tape)
    for cx,cy,ux,uy in ((x+4,y+4,1,-1),(x+s-4,y+4,1,1)):
        ux,uy=ux/math.sqrt(2),uy/math.sqrt(2); px,py=-uy,ux; L,Wt=34,10
        pts=[(cx-ux*L-px*Wt,cy-uy*L-py*Wt),(cx+ux*L-px*Wt,cy+uy*L-py*Wt),(cx+ux*L+px*Wt,cy+uy*L+py*Wt),(cx-ux*L+px*Wt,cy-uy*L+py*Wt)]
        td.polygon([(P(a),P(b)) for a,b in pts],fill=(236,230,210,215))
    base.alpha_composite(tape)

def details_page(base,f,empty=False):
    paper(base,40,30,452,462,11)
    if empty:
        d=ImageDraw.Draw(base); T(d,118,44,'just in case!',HAND(15,500),PENCIL,'la')
        T(d,246,240,'pick a job from the list',HAND(20,500),PENCIL,'mm'); return
    thumb(base,121,44)
    d=ImageDraw.Draw(base)
    T(d,246,312,f,TYPE(17,700),INK,'mm')
    d.line([P(70),P(330),P(422),P(330)],fill=PENCIL,width=P(1.4))
    rows=[('print time','2h 14m'),('filament',(material(f) or '-')+', 5.62 m'),('layer height','0.20 mm'),('height','42.0 mm'),('slicer','OrcaSlicer'),('saved','18 Oct, 14:02')]
    y=342
    for k,v in rows:
        T(d,70,y,k,HAND(15,500),PENCIL,'la'); T(d,172,y,v,TYPE(15,600),INK,'la'); y+=19
    sx,sy,k=306,354,1.2
    st=Image.new('RGBA',base.size,(0,0,0,0)); sd=ImageDraw.Draw(st)
    sd.rounded_rectangle([P(sx),P(sy),P(sx+104*k),P(sy+50*k)],radius=P(9),outline=(206,52,38,235),width=P(4.5))
    sd.polygon([(P(sx+16*k),P(sy+14*k)),(P(sx+16*k),P(sy+36*k)),(P(sx+34*k),P(sy+25*k))],fill=(206,52,38,235))
    sd.text((P(sx+68*k),P(sy+25*k)),'PRINT',font=F(18*k,800),fill=(206,52,38,235),anchor='mm')
    base.alpha_composite(st.rotate(-6,center=(P(sx+52*k),P(sy+25*k)),resample=Image.BICUBIC))

def list_page(base,files,sel,page,pages,sortmode,ascending):
    paper(base,476,34,884,464,12)
    d=ImageDraw.Draw(base)
    jx0,jy0,jx1,jy1=496,50,864,94
    poly(d,wobble([(jx0,jy0),(jx1,jy0),(jx1,jy1),(jx0,jy1)],0.7,40),fill=(255,255,255),w=2.5)
    T(d,jx0+14,(jy0+jy1)/2,'JOBS:',HAND(17,700),INK,'lm')
    T(d,jx1-14,(jy0+jy1)/2,f'{page}/{pages} pages',HAND(16,500),PENCIL,'rm')
    cx=660; cy=(jy0+jy1)/2
    for k,(dx,up) in enumerate(((-26,True),(26,False))):
        on=(up==ascending); bx=cx+dx
        if on: poly(d,wobble([(bx-22,cy-17),(bx+22,cy-17),(bx+22,cy+17),(bx-22,cy+17)],0.5,90+k),fill=BLUE_L,ol=BLUE,w=2.5)
        col=BLUE if on else (176,176,180); sg=-1 if up else 1
        d.line([P(bx),P(cy-9*sg),P(bx),P(cy+9*sg)],fill=col,width=P(3.5))
        d.line([P(bx-10),P(cy+9*sg-8*sg),P(bx),P(cy+9*sg),P(bx+10),P(cy+9*sg-8*sg)],fill=col,width=P(3.5),joint='curve')
    y=108
    for i,(n,age) in enumerate(files):
        rh=38
        b=wobble([(496,y),(864,y),(864,y+rh),(496,y+rh)],0.6,70+i)
        if i==sel:
            hl=Image.new('RGBA',base.size,(0,0,0,0)); ImageDraw.Draw(hl).rectangle([P(490),P(y-2),P(870),P(y+rh+2)],fill=BLUE_L+(150,))
            base.alpha_composite(hl); d=ImageDraw.Draw(base)
        poly(d,b,fill=None,ol=BLUE if i==sel else INK,w=3 if i==sel else 2.2)
        if n.endswith('/'):
            fx,fy=508,y+11
            d.polygon([(P(fx),P(fy)),(P(fx+8),P(fy)),(P(fx+11),P(fy+3)),(P(fx+22),P(fy+3)),(P(fx+22),P(fy+17)),(P(fx),P(fy+17))],fill=MANILA,outline=INK)
            T(d,540,y+rh/2+1,n,TYPE(17,600),INK,'lm')
        else:
            T(d,510,y+rh/2+1,n,TYPE(16,500),INK,'lm')
        m=material(n)
        if sortmode=='FILA' and m:
            tw=d.textlength(m,font=HAND(13,700))/S+14
            poly(d,wobble([(854-tw,y+8),(854,y+8),(854,y+rh-8),(854-tw,y+rh-8)],0.5,99+i),fill=(250,222,92),ol=INK,w=1.8)
            T(d,854-tw/2,y+rh/2+1,m,HAND(13,700),INK,'mm')
        elif age:
            T(d,854,y+rh/2+1,age,HAND(14,500),PENCIL,'rm')
        y+=rh+5

def side(base,sortmode,up,down):
    card(base,924,112,82,140,70); d=ImageDraw.Draw(base)
    hand_arrow(d,965,226,138,INK if up else (176,176,180))
    card(base,924,262,82,64,71); d=ImageDraw.Draw(base)
    T(d,965,280,'SORT',HAND(13,700),PENCIL,'mm'); T(d,965,304,sortmode,TYPE(18,700),INK,'mm')
    card(base,924,336,82,128,72); d=ImageDraw.Draw(base)
    hand_arrow(d,965,350,448,INK if down else (176,176,180))

def scene(sel=0,sortmode='NEW',files=FILES,page=1,pages=3,empty=False,ascending=False):
    base=desk(); folder(base)
    details_page(base,files[sel][0] if not empty else '',empty)
    list_page(base,files,sel if not empty else -1,page,pages,sortmode,ascending)
    estop(base,3,3); clip(ImageDraw.Draw(base),84,-4,0.95)
    home_note(base,914,10)
    side(base,sortmode,page>1,page<pages)
    return base

if __name__=='__main__':
  order=['PLA','PETG','ABS','ASA','TPU']
  scene().resize((W,H),Image.LANCZOS).convert('RGB').save('jobs_newest.png')
  fila=sorted(FILES,key=lambda f:(order.index(material(f[0])) if material(f[0]) in order else 99,f[0]))
  scene(sel=2,sortmode='FILA',files=fila,ascending=True).resize((W,H),Image.LANCZOS).convert('RGB').save('jobs_fila.png')
  scene(sel=1,sortmode='A-Z',files=[('prototypes/','')]+sorted(FILES)[:7],ascending=True).resize((W,H),Image.LANCZOS).convert('RGB').save('jobs_az.png')

