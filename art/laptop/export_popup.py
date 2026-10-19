import os
HERE=os.path.dirname(os.path.abspath(__file__))+'/'
__file__=HERE+'popup_frame.py'
src=open(HERE+'popup_frame.py').read().split("# 1 ── confirm")[0]
exec(src)
OUT=R+'assets/workshop/popup/'
os.makedirs(OUT,exist_ok=True)
fr,g,pad=frame(660,386)
fw,fh=round(fr.width/S*K),round(fr.height/S*K)
fr=fr.resize((fw,fh),Image.LANCZOS)
px,py=(1024-fw)//2,max(4,(600-fh)//2+4)
fr.save(OUT+'frame.png',optimize=True)
print('frame',fw,fh,'at',px,py)
gx0,gy0,gx1,gy1=[v+pad for v in g]
print('glass prescale',gx0,gy0,gx1,gy1)
fx=(gx1-gx0-20)/620; fy=(gy1-gy0-12)/400
print('fx fy',fx,fy)
bw,bh=110*fx,78*fy
ORG=(232,120,36); GREY=(96,104,116)
for name,col,ic,fg in [('btn_x',RED,'x',W1),('btn_check',GRN,'check',W1),('btn_trash',ORG,'trash',W1),('btn_off',GREY,None,W1)]:
    im=Image.new('RGBA',(int(bw*S)+2,int(bh*S)+2),(0,0,0,0)); d=ImageDraw.Draw(im)
    btn(d,0,0,bw,bh,'',col,ic,fg=fg)
    im=im.resize((round(bw*K),round(bh*K)),Image.LANCZOS); im.save(OUT+name+'.png',optimize=True)
    print(name,im.size)

# ---- cardboard variant (object cancel and jobs folders only) ----
SCR_T=(226,188,128); SCR_B=(204,160,100)
fr,g,pad=frame(660,386)
fr=fr.resize((fw,fh),Image.LANCZOS); fr.save(OUT+'frame_paper.png',optimize=True)
W1=INK   # icons (and the bin handle) in ink
for name,col,ic in [('btn_x_paper',RED,'x'),('btn_check_paper',GRN,'check'),('btn_trash_paper',ORG,'trash')]:
    im=Image.new('RGBA',(int(bw*S)+2,int(bh*S)+2),(0,0,0,0)); d=ImageDraw.Draw(im)
    btn(d,0,0,bw,bh,'',col,ic,fg=INK)
    d.rounded_rectangle([0,0,int(bw*S),int(bh*S)],radius=8*S,outline=INK,width=int(1.5*S))
    im=im.resize((round(bw*K),round(bh*K)),Image.LANCZOS); im.save(OUT+name+'.png',optimize=True)
    print(name,im.size)
