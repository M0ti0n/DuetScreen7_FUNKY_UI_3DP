"""Writes assets/workshop/objects/: the object cancel page background and its sprites."""
import os
HERE=os.path.dirname(os.path.abspath(__file__))+'/'
__file__=HERE+'mockups.py'
src=open(HERE+'mockups.py').read().split("if __name__=='__main__':")[0]
exec(src)
OUT=R+'assets/workshop/objects/'
os.makedirs(OUT,exist_ok=True)

def background():
    base=desk(); folder(base)
    bed_page(base,[],-1,(),False,outline=False)
    list_page(base,'',[],0,-1,(),False)
    estop(base,3,3); clip(ImageDraw.Draw(base),84,-4,0.95)
    home_note(base,914,10)
    pager(base,0,2,text=False)
    # the up arrow was drawn grey (page 0): draw it in ink, the page shows/greys it at run time
    d=ImageDraw.Draw(base); hand_arrow(d,965,226,138,INK)
    return base.resize((W,H),Image.LANCZOS).convert('RGB')

def sprite(w,h,draw):
    im=Image.new('RGBA',(P(w),P(h)),(0,0,0,0)); draw(ImageDraw.Draw(im))
    return im.resize((w,h),Image.LANCZOS)

def box(x0,y0,x1,y1,seed):
    return wobble([(x0,y0),(x1,y0),(x1,y1),(x0,y1)],0.7,seed)

background().save(OUT+'background.png',optimize=True)
looks={'normal':(PAPER,INK,2.5),'sel':(BLUE_L,BLUE,3.5),'red':(RED,INK,2.5)}
for k,(fill,ol,w) in looks.items():
    sprite(40,40,lambda d:poly(d,box(3,3,37,37,31),fill=fill,ol=ol,w=w)).save(OUT+f'marker_{k}.png',optimize=True)
    sprite(60,56,lambda d:poly(d,box(2,2,58,54,51),fill=None if k!='red' else RED,ol=ol,w=w)).save(OUT+f'num_{k}.png',optimize=True)
for k in ('normal','sel'):
    fill,ol,w=looks[k]
    sprite(304,56,lambda d:poly(d,box(2,2,302,54,61),fill=None,ol=ol,w=w)).save(OUT+f'name_{k}.png',optimize=True)
print(sorted(os.listdir(OUT)))
