import json, os, sys
HERE=os.path.dirname(os.path.abspath(__file__))+'/'
__file__=HERE+'mockups.py'
src=open(HERE+'mockups.py').read().split("state_screen('CONSOLE'")[0]
exec(src)
OUT=R+'assets/workshop/laptop/'
os.makedirs(OUT,exist_ok=True)

LCD_UP=(46,24,978,506)      # x0,y0,x1,y1
LCD_KB=(46,18,978,222)
KB=(34,262,990,592)

REC=None
_key=key
def key(d,x,y,w,h,label='',f=None,icon=None,legend=LEG):
    if REC is not None: REC.append(dict(x=x,y=y,w=w,h=h,label=label,icon=icon))
    _key(d,x,y,w,h,label,f,icon,legend)

def lcd_full():
    w=LCD_UP[2]-LCD_UP[0]; h=LCD_UP[3]-LCD_UP[1]
    return lcd_bg(P(w),P(h))

def bg_screen_up():
    im=backdrop(); d=ImageDraw.Draw(im)
    deck(d,520)
    keyboard(d,60,548,W-60,548+5*60)
    quad=[(LCD_UP[0],LCD_UP[1]),(LCD_UP[2],LCD_UP[1]),(LCD_UP[2],LCD_UP[3]),(LCD_UP[0],LCD_UP[3])]
    lid(d,quad,0); hinge(d,512)
    im.alpha_composite(lcd_full(),(P(LCD_UP[0]),P(LCD_UP[1])))
    return im.resize((W,H),Image.LANCZOS).convert('RGB')

def bg_keyboard(shift):
    global REC
    im=backdrop(); d=ImageDraw.Draw(im)
    deck(d,236)
    REC=[] if not shift else None
    keyboard(d,*KB,shift)
    rec=REC; REC=None
    quad=[(LCD_KB[0],LCD_KB[1]),(LCD_KB[2],LCD_KB[1]),(LCD_KB[2],LCD_KB[3]),(LCD_KB[0],LCD_KB[3])]
    lid(d,quad,0); hinge(d,228)
    full=lcd_full(); hk=P(LCD_KB[3]-LCD_KB[1])
    im.alpha_composite(full.crop((0,full.height-hk,full.width,full.height)),(P(LCD_KB[0]),P(LCD_KB[1])))
    return im.resize((W,H),Image.LANCZOS).convert('RGB'),rec

up=bg_screen_up(); up.save(OUT+'screen_up.png',optimize=True)
kb,rec=bg_keyboard(False); kb.save(OUT+'keyboard.png',optimize=True)
kbs,_=bg_keyboard(True)
kbs.crop(KB).save(OUT+'keys_shift.png',optimize=True)

rows=["1234567890-+","QWERTYUIOP[]","ASDFGHJKL:@","ZXCVBNM,./"]
srows=['!"#$%^&*()_=',"qwertyuiop{}","asdfghjkl;'","zxcvbnm<>?"]
norm=''.join(rows); shf=''.join(srows)
keys=[]; ci=0
specials={('bksp',None):'Backspace',('up',None):'Up',('down',None):'Down',('shift',None):'Shift',('hide',None):'Hide',('enter',None):'Enter'}
for k in rec:
    r=[int(round(k['x'])),int(round(k['y'])),int(round(k['w'])),int(round(k['h']))]
    if k['label']:
        keys.append(('Char',norm[ci],shf[ci],r)); ci+=1
    elif k['icon']:
        keys.append(({'bksp':'Backspace','up':'Up','down':'Down','shift':'Shift','hide':'Hide','enter':'Enter'}[k['icon']],None,None,r))
    else:
        keys.append(('Space' if r[2]>300 else 'Blank',None,None,r))
json.dump(keys,open(HERE+'keys.json','w'))  # -> LaptopLayout.h KEYS
print(len(keys), [k for k in keys if k[0]!='Char'])
