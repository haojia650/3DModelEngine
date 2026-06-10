from PIL import Image, ImageDraw, ImageFont
from pathlib import Path
import math

OUT = Path(r'D:\ZUOYE\jianmo\report_assets')
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

def ellipse_uc(draw, cx, cy, rx, ry, text):
    draw.ellipse((cx-rx, cy-ry, cx+rx, cy+ry), outline=(37,99,235), fill=(255,255,255), width=3)
    centered(draw, (cx, cy), text, font(20), (17,24,39))

def class_box(d, box, title, attrs, ops=None, head_fill=(219,234,254)):
    x1,y1,x2,y2 = box
    rect(d, box, fill=(255,255,255), radius=14)
    d.rounded_rectangle((x1,y1,x2,y1+52), radius=14, outline=(15,23,42), fill=head_fill, width=3)
    d.rectangle((x1,y1+38,x2,y1+52), fill=head_fill, outline=None)
    centered(d, ((x1+x2)/2, y1+30), title, font(21, True), (15,23,42))
    line(d, [(x1,y1+52),(x2,y1+52)], (15,23,42), 3)
    y = y1+80
    for a in attrs:
        d.text((x1+18,y), a, font=font(16), fill=(17,24,39))
        y += 27
    if ops:
        line(d, [(x1,y+2),(x2,y+2)], (15,23,42), 3)
        y += 22
        for op in ops:
            d.text((x1+18,y), op, font=font(16), fill=(17,24,39))
            y += 27

# Use case diagram for whole system
img = Image.new('RGB', (1800, 1200), 'white')
d = ImageDraw.Draw(img)
centered(d, (900, 44), '3DModelEngine系统用例图', font(30, True), (31,41,55))
rect(d, (280,90,1710,1110), fill=(248,250,252), radius=20)
centered(d, (995, 128), '基于OCC的三维模型建模与交互系统', font(24, True), (31,41,55))

# actor
for pts in [((120,186,188,254),),]:
    pass

d.ellipse((106,186,174,254), outline=(17,24,39), width=3)
line(d, [(140,254),(140,340)], (17,24,39), 3)
line(d, [(98,286),(182,286)], (17,24,39), 3)
line(d, [(140,340),(102,402)], (17,24,39), 3)
line(d, [(140,340),(178,402)], (17,24,39), 3)
centered(d, (140, 446), '用户', font(22), (17,24,39))

ucs = [
    (500,220,'浏览与旋转视图'),
    (500,340,'平移与缩放视图'),
    (500,470,'切换标准视角'),
    (500,610,'选择与取消选择对象'),
    (500,740,'删除/隐藏/复制对象'),
    (500,880,'切换显示模式'),
    (980,220,'移动对象'),
    (980,360,'旋转对象'),
    (980,500,'缩放对象'),
    (980,640,'轴向锁定'),
    (980,780,'重置/应用变换'),
    (1410,300,'切换编辑模式'),
    (1410,470,'顶点/边/面选择'),
    (1410,650,'挤出/倒角/细分'),
    (1410,840,'新建/打开/保存工程')
]
for x,y,t in ucs:
    ellipse_uc(d,x,y,155 if x!=1410 else 170,46,t)

# actor associations
for p2 in [(345,220),(345,340),(345,470),(345,610),(345,740),(345,880),(825,220),(825,360),(825,500),(825,780),(1250,300),(1250,470),(1250,650),(1250,840)]:
    line(d, [(180,260 if p2[1] < 500 else 300 if p2[1] < 700 else 340), p2])

# extend/include style links
for p1,p2,txt,tx,ty in [
    ((1135,615),(1135,540),'<<extend>>',1150,575),
    ((1135,755),(1135,540),'<<extend>>',1150,665),
    ((580,610),(825,220),'<<include>>',650,385),
    ((580,740),(825,220),'<<include>>',660,490),
    ((580,740),(825,360),'<<include>>',685,560),
    ((580,880),(1250,840),'<<include>>',860,880),
    ((1140,360),(1250,300),'<<include>>',1180,315),
    ((1140,500),(1250,300),'<<include>>',1180,420),
]:
    dashed(d,p1,p2)
    d.text((tx,ty), txt, font=font(16), fill=(55,65,81))

img.save(OUT / 'usecase_diagram.png')

# Class diagram for whole system
img = Image.new('RGB', (2000, 1250), 'white')
d = ImageDraw.Draw(img)
centered(d, (1000, 42), '3DModelEngine系统类图', font(30, True), (31,41,55))

class_box(d, (60,90,470,500), 'COccResurfView', [
    '- myView : V3d_View',
    '- m_currentMode : InteractionMode',
    '- m_selectMode : Integer',
    '- m_bIsRotating : bool',
    '- m_bIsTransforming : bool',
    '- m_transformStates : map',
    '- m_lastMousePoint : CPoint'
], [
    '+ PreTranslateMessage()',
    '+ OnMouseMove()',
    '+ OnLButtonDown()/Up()',
    '+ SelectModelAtPoint()',
    '+ ApplyTransform()',
    '+ ResetTransform()'
])

class_box(d, (540,90,970,470), 'COccResurfDoc', [
    '- myAISContext : AIS_InteractiveContext',
    '- myViewer : V3d_Viewer',
    '- myModelShapes : vector<AIS_Shape>',
    '- myModelCenter : gp_Pnt',
    '- m_bModelBuilt : bool'
], [
    '+ GetAISContext()',
    '+ GetViewer()',
    '+ UpdateModelCenter()',
    '+ DrawSphere()'
])

class_box(d, (1050,90,1430,380), 'AIS_InteractiveContext', [
    '+ MoveTo()',
    '+ SelectDetected()',
    '+ ClearSelected()',
    '+ Redisplay()',
    '+ UpdateCurrentViewer()'
])

class_box(d, (1510,90,1890,330), 'V3d_View', [
    '+ StartRotation()',
    '+ Rotation()',
    '+ Pan()',
    '+ Zoom()',
    '+ FitAll()',
    '+ Camera()'
])

class_box(d, (1030,470,1450,800), 'AIS_Shape', [
    '+ SetColor()',
    '+ SetLocalTransformation()',
    '+ SetShape()',
    '+ ResetTransformation()',
    '+ Shape()'
])

class_box(d, (540,560,960,850), 'TransformState', [
    '+ translation : gp_Vec',
    '+ rotationX : Real',
    '+ rotationY : Real',
    '+ rotationZ : Real',
    '+ uniformScale : Real'
])

class_box(d, (60,620,430,910), 'InteractionMode', [
    '+ MODE_NONE',
    '+ MODE_VIEW_ROTATE',
    '+ MODE_VIEW_PAN',
    '+ MODE_TRANS_MOVE',
    '+ MODE_TRANS_ROTATE',
    '+ MODE_TRANS_SCALE',
    '+ MODE_EDIT_VERTEX/EDGE/FACE'
])

class_box(d, (1040,890,1460,1160), 'Graphic3d_Camera', [
    '+ SideRight()',
    '+ OrthogonalizedUp()',
    '+ Direction()'
])

class_box(d, (1530,520,1920,860), 'TopoDS_Shape / gp_Trsf', [
    '+ 几何实体数据',
    '+ 平移/旋转/缩放矩阵',
    '+ 形体构造与变换支持'
])

# relations
arrow(d,(470,180),(540,180)); d.text((485,155),'关联',font=font(15),fill=(51,65,85))
arrow(d,(970,210),(1050,210)); d.text((985,185),'持有',font=font(15),fill=(51,65,85))
arrow(d,(1430,220),(1510,220)); d.text((1450,195),'控制',font=font(15),fill=(51,65,85))
arrow(d,(970,330),(1030,560)); d.text((980,430),'管理模型对象',font=font(15),fill=(51,65,85))
arrow(d,(470,320),(1050,540)); d.text((710,410),'选择/变换对象',font=font(15),fill=(51,65,85))
arrow(d,(470,420),(1510,220)); d.text((940,300),'更新视图',font=font(15),fill=(51,65,85))
arrow(d,(470,700),(540,700)); d.text((480,675),'使用',font=font(15),fill=(51,65,85))
arrow(d,(430,760),(470,430)); d.text((438,610),'控制模式',font=font(15),fill=(51,65,85))
arrow(d,(1250,800),(1250,890)); d.text((1265,845),'获取相机方向',font=font(15),fill=(51,65,85))
arrow(d,(1450,650),(1530,650)); d.text((1470,625),'封装几何',font=font(15),fill=(51,65,85))

dashed(d,(800,850),(1120,800)); d.text((900,810),'对象变换状态映射',font=font(15),fill=(55,65,81))

dashed(d,(1700,330),(1710,520)); d.text((1600,430),'显示/几何基础依赖',font=font(15),fill=(55,65,81))

img.save(OUT / 'class_diagram.png')
print('ok')
