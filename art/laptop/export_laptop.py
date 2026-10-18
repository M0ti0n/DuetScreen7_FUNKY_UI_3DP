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
    im=backdrop(); d=ImageDraw.Draw(im)
    deck(d,236)
    rec=keyboard(d,*KB,shift)
    quad=[(LCD_KB[0],LCD_KB[1]),(LCD_KB[2],LCD_KB[1]),(LCD_KB[2],LCD_KB[3]),(LCD_KB[0],LCD_KB[3])]
    lid(d,quad,0); hinge(d,228)
    full=lcd_full(); hk=P(LCD_KB[3]-LCD_KB[1])
    im.alpha_composite(full.crop((0,full.height-hk,full.width,full.height)),(P(LCD_KB[0]),P(LCD_KB[1])))
    return im.resize((W,H),Image.LANCZOS).convert('RGB'),rec

up=bg_screen_up(); up.save(OUT+'screen_up.png',optimize=True)
kb,rec=bg_keyboard(False); kb.save(OUT+'keyboard.png',optimize=True)
kbs,_=bg_keyboard(True)
kbs.crop(KB).save(OUT+'keys_shift.png',optimize=True)

keys=[(k,n,sh,[int(round(v)) for v in r]) for k,n,sh,r in rec]
json.dump(keys,open(HERE+'keys.json','w'))  # -> LaptopLayout.h KEYS
print(len(keys), [k for k in keys if k[0]!='Char'])
