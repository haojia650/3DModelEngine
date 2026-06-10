from PIL import Image, ImageDraw, ImageFont
from pathlib import Path
import math

OUT = Path(r'D:\ZUOYE\jianmo\report_assets')
OUT.mkdir(parents=True, exist_ok=True)
FONT = 'msyh.ttc'
BOLD = 'msyhbd.ttc'

def font(size, bold=False):
    try:
        return ImageFont.truetype(BOLD if bold else FONT, size)
    except Exception:
        return ImageFont.load_default()

def centered(draw, xy, text, ft, fill=(0,0,0)):
    bbox = draw.textbbox((0,0), text, font=ft)
    w = bbox[2]-bbox[0]
    h = bbox[3]-bbox[1]
    draw.text((xy[0]-w/2, xy[1]-h/2), text, font=ft, fill=fill)

def rect(draw, box, outline=(15,23,42), fill=(255,255,255), width=3, radius=16):
    draw.rounded_rectangle(box, radius=radius, outline=outline, fill=fill, width=width)

def line(draw, pts, fill=(71,85,105), width=3):
    draw.line(pts, fill=fill, width=width)

def arrow(draw, p1, p2, fill=(51,65,85), width=3):
    draw.line((p1, p2), fill=fill, width=width)
    ang = math.atan2(p2[1]-p1[1], p2[0]-p1[0])
    l = 14
    a1 = ang + math.pi*0.85
    a2 = ang - math.pi*0.85
    p3 = (p2[0] + l*math.cos(a1), p2[1] + l*math.sin(a1))
    p4 = (p2[0] + l*math.cos(a2), p2[1] + l*math.sin(a2))
    draw.polygon([p2, p3, p4], fill=fill)

def dashed(draw, p1, p2, dash=10, gap=8, fill=(100,116,139), width=2):
    dx = p2[0]-p1[0]
    dy = p2[1]-p1[1]
    dist = (dx*dx+dy*dy)**0.5
    if dist == 0:
        return
    ux, uy = dx/dist, dy/dist
    cur = 0
    while cur < dist:
        s = cur
        e = min(cur+dash, dist)
        draw.line((p1[0]+ux*s, p1[1]+uy*s, p1[0]+ux*e, p1[1]+uy*e), fill=fill, width=width)
        cur += dash + gap

def ellipse_usecase(draw, cx, cy, rx, ry, text):
    draw.ellipse((cx-rx, cy-ry, cx+rx, cy+ry), outline=(37,99,235), fill=(255,255,255), width=3)
    centered(draw, (cx, cy), text, font(22), (17,24,39))

def usecase_png(path):
    img = Image.new('RGB', (1400, 900), 'white')
    d = ImageDraw.Draw(img)
    centered(d, (700, 45), '3D模型交互系统用例图', font(30, True), (31,41,55))
    rect(d, (270,100,1320,830), fill=(248,250,252), radius=18)
    centered(d, (795, 135), '基于OCC的三维模型交互系统', font(26, True), (31,41,55))

    d.ellipse((86,186,154,254), outline=(17,24,39), width=3)
    line(d, [(120,254),(120,340)], (17,24,39), 3)
    line(d, [(78,286),(162,286)], (17,24,39), 3)
    line(d, [(120,340),(82,400)], (17,24,39), 3)
    line(d, [(120,340),(158,400)], (17,24,39), 3)
    centered(d, (120, 440), '用户', font(22), (17,24,39))

    for x,y,t in [
        (540,210,'浏览三维场景'),
        (540,330,'选择单个物体'),
        (540,460,'移动物体'),
        (540,590,'旋转物体'),
        (540,720,'缩放物体'),
        (970,280,'轴向锁定（X/Y/Z）'),
        (970,470,'重置变换'),
        (970,650,'应用变换'),
    ]:
        ellipse_usecase(d,x,y,150 if x==970 else 140,44,t)

    assoc = [((160,220),(400,210)),((160,250),(400,325)),((160,280),(400,455)),((160,300),(400,585)),((150,320),(400,715)),((160,300),(820,465)),((150,330),(820,645))]
    for a,b in assoc:
        line(d,[a,b])

    dashed(d,(682,445),(820,300)); d.text((735,350),'<<extend>>', font=font(18), fill=(55,65,81))
    dashed(d,(682,580),(820,300)); d.text((742,430),'<<extend>>', font=font(18), fill=(55,65,81))
    dashed(d,(682,705),(820,300)); d.text((750,545),'<<extend>>', font=font(18), fill=(55,65,81))
    img.save(path)

def class_box(d, box, title, attrs, ops=None, head_fill=(219,234,254)):
    x1,y1,x2,y2 = box
    rect(d, box, fill=(255,255,255), radius=14)
    d.rounded_rectangle((x1,y1,x2,y1+52), radius=14, outline=(15,23,42), fill=head_fill, width=3)
    d.rectangle((x1,y1+38,x2,y1+52), fill=head_fill, outline=None)
    centered(d, ((x1+x2)/2, y1+30), title, font(22, True), (15,23,42))
    line(d, [(x1,y1+52),(x2,y1+52)], (15,23,42), 3)
    y = y1+82
    for a in attrs:
        d.text((x1+20,y), a, font=font(18), fill=(17,24,39))
        y += 30
    if ops:
        line(d, [(x1,y+4),(x2,y+4)], (15,23,42), 3)
        y += 28
        for op in ops:
            d.text((x1+20,y), op, font=font(18), fill=(17,24,39))
            y += 30

def class_png(path):
    img = Image.new('RGB', (1600, 980), 'white')
    d = ImageDraw.Draw(img)
    centered(d, (800, 45), '3D模型交互系统类图', font(30, True), (31,41,55))

    class_box(d, (80,120,510,620), 'COccResurfView', [
        '- myView : V3d_View',
        '- m_currentMode : int',
        '- m_lockedAxis : int',
        '- m_bIsRotating : bool',
        '- m_bIsTransforming : bool',
        '- m_transformStates : map'
    ], [
        '+ PreTranslateMessage()',
        '+ SelectModelAtPoint()',
        '+ ApplyTransform()',
        '+ ResetTransform()',
        '+ ApplyTransformPermanently()',
        '+ ExitTransformMode()'
    ])

    class_box(d, (615,120,1025,520), 'COccResurfDoc', [
        '- myAISContext : AIS_InteractiveContext',
        '- myViewer : V3d_Viewer',
        '- myModelShapes : vector<AIS_Shape>',
        '- myModelCenter : gp_Pnt',
        '- m_bModelBuilt : bool'
    ], [
        '+ GetAISContext()',
        '+ GetViewer()',
        '+ DrawSphere()'
    ])

    class_box(d, (1100,120,1440,390), 'TransformState', [
        '+ translation : gp_Vec',
        '+ rotationX : Real',
        '+ rotationY : Real',
        '+ rotationZ : Real',
        '+ uniformScale : Real'
    ])

    class_box(d, (610,560,970,800), 'AIS_InteractiveContext', [
        '+ MoveTo()',
        '+ SelectDetected()',
        '+ Redisplay()',
        '+ UpdateCurrentViewer()'
    ])

    class_box(d, (90,600,410,840), 'V3d_View', [
        '+ StartRotation()',
        '+ Rotation()',
        '+ FitAll()',
        '+ Camera()'
    ])

    class_box(d, (1080,560,1420,820), 'AIS_Shape', [
        '+ SetLocalTransformation()',
        '+ SetShape()',
        '+ ResetTransformation()',
        '+ Shape()'
    ])

    arrow(d, (510,230), (615,230)); d.text((540,205), '关联', font=font(16), fill=(51,65,85))
    line(d, [(420,620),(420,430)], (51,65,85), 3); arrow(d, (420,430), (510,430)); d.text((430,515), '控制视图', font=font(16), fill=(51,65,85))
    line(d, [(790,560),(790,520)], (51,65,85), 3); arrow(d, (790,520), (790,560)); d.text((805,510), '提供', font=font(16), fill=(51,65,85))
    arrow(d, (1080,690), (970,690)); d.text((995,665), '被管理', font=font(16), fill=(51,65,85))
    arrow(d, (1025,280), (1100,280)); d.text((1035,255), '组合', font=font(16), fill=(51,65,85))
    dashed(d, (510,320), (1100,320)); arrow(d, (1090,320), (1100,320)); d.text((760,295), '维护对象变换状态', font=font(16), fill=(51,65,85))

    img.save(path)

usecase_png(OUT / 'usecase_diagram.png')
class_png(OUT / 'class_diagram.png')
print('done')
