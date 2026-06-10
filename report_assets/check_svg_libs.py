import importlib.util, sys
mods = ['cairosvg','svglib','reportlab','PIL']
for m in mods:
    print(m, bool(importlib.util.find_spec(m)))
print(sys.version)
