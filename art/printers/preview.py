"""Composes the baked layers the way the app does (for checking the bake)."""
import os, json, sys
from PIL import Image, ImageDraw
HERE=os.path.dirname(os.path.abspath(__file__))+'/'
A=os.path.abspath(HERE+'../../assets/workshop')+'/'
L=json.load(open(HERE+'layout.json'))
PARTS=[('bed_boat',869,418),('bed_spinner',921,427),('bed_bar',941,437),('bed_cube',965,419)]
def put(im,key):
    v=L[key]; im.alpha_composite(Image.open(A+v['file']).convert('RGBA'),(v['x'],v['y']))
def scene(style,printing=True,pct=42,show_display=True):
    rnd=style=='delta'
    im=Image.open(A+'home_base.png').convert('RGBA')
    put(im,'WALL_BED_ROUND' if rnd else 'WALL_BED_SQUARE')
    put(im,'PRINTER_'+style.upper())
    s=L['styles'][style]; p=s['part']
    if printing:
        stage=min(10,pct//10); h=round(p['h']*stage/10)
        ghost=Image.open(A+f'printers/{style}_ghost.png').convert('RGBA')
        if h<p['h']: im.alpha_composite(ghost.crop((0,0,p['w'],p['h']-h)),(p['x'],p['y']))
        if h>0:
            part=Image.open(A+f'printers/{style}_part.png').convert('RGBA')
            im.alpha_composite(part.crop((0,p['h']-h,p['w'],p['h'])),(p['x'],p['y']+p['h']-h))
        im.alpha_composite(Image.open(A+f'printers/{style}_outline.png').convert('RGBA'),(p['x'],p['y']))
    if show_display:
        d=s['display']; ImageDraw.Draw(im).rectangle([d['x'],d['y'],d['x']+d['w'],d['y']+d['h']],outline=(255,0,255,255))
    put(im,'SPARE_BED_ROUND' if rnd else 'SPARE_BED_SQUARE')
    if printing:
        put(im,'FOLDER_ROUND' if rnd else 'FOLDER_SQUARE')
        for n,x,y in PARTS: im.alpha_composite(Image.open(A+f'layers/{n}.png').convert('RGBA'),(x,y))
    return im.convert('RGB')
if __name__=='__main__':
    for st in ('corexy','delta','bedslinger'):
        scene(st).save(sys.argv[1]+f'prev_{st}.png')
