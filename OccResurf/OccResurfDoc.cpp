
// OccResurfDoc.cpp: COccResurfDoc 类的实现
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS 可以在实现预览、缩略图和搜索筛选器句柄的
// ATL 项目中进行定义，并允许与该项目共享文档代码。
#ifndef SHARED_HANDLERS
#include "OccResurf.h"
#endif

#include "OccResurfDoc.h"

#include "OccResurfView.h"

#include <propkey.h>

#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <Quantity_Color.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <gp_Ax1.hxx>
#include <gp_Dir.hxx>
#ifdef _DEBUG
//#define new DEBUG_NEW
#endif

// COccResurfDoc

IMPLEMENT_DYNCREATE(COccResurfDoc, CDocument)

BEGIN_MESSAGE_MAP(COccResurfDoc, CDocument)
END_MESSAGE_MAP()


// COccResurfDoc 构造/析构

COccResurfDoc::COccResurfDoc() noexcept
{
	// TODO: 在此添加一次性构造代码

	Handle(Graphic3d_GraphicDriver) theGraphicDriver = ((COccResurfApp*)AfxGetApp())->GetGraphicDriver();

	myViewer = new V3d_Viewer(theGraphicDriver);
	myViewer->SetDefaultLights();
	myViewer->SetLightOn();

	myAISContext = new AIS_InteractiveContext(myViewer);

	myAISContext->SetDisplayMode(AIS_Shaded, true);
	myAISContext->SetAutomaticHilight(Standard_False);
}

COccResurfDoc::~COccResurfDoc()
{
}

BOOL COccResurfDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: 在此添加重新初始化代码
	// (SDI 文档将重用该文档)

	return TRUE;
}




// COccResurfDoc 序列化

void COccResurfDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: 在此添加存储代码
	}
	else
	{
		// TODO: 在此添加加载代码
	}
}

#ifdef SHARED_HANDLERS

// 缩略图的支持
void COccResurfDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
{
	// 修改此代码以绘制文档数据
	dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));

	CString strText = _T("TODO: implement thumbnail drawing here");
	LOGFONT lf;

	CFont* pDefaultGUIFont = CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT));
	pDefaultGUIFont->GetLogFont(&lf);
	lf.lfHeight = 36;

	CFont fontDraw;
	fontDraw.CreateFontIndirect(&lf);

	CFont* pOldFont = dc.SelectObject(&fontDraw);
	dc.DrawText(strText, lprcBounds, DT_CENTER | DT_WORDBREAK);
	dc.SelectObject(pOldFont);
}

// 搜索处理程序的支持
void COccResurfDoc::InitializeSearchContent()
{
	CString strSearchContent;
	// 从文档数据设置搜索内容。
	// 内容部分应由“;”分隔

	// 例如:     strSearchContent = _T("point;rectangle;circle;ole object;")；
	SetSearchContent(strSearchContent);
}

void COccResurfDoc::SetSearchContent(const CString& value)
{
	if (value.IsEmpty())
	{
		RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
	}
	else
	{
		CMFCFilterChunkValueImpl *pChunk = nullptr;
		ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
		if (pChunk != nullptr)
		{
			pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
			SetChunkValue(pChunk);
		}
	}
}

#endif // SHARED_HANDLERS

// COccResurfDoc 诊断

#ifdef _DEBUG
void COccResurfDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void COccResurfDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// COccResurfDoc 命令

void COccResurfDoc::DrawSphere(double Radius)
{
	// ========== 经典高达配色 ==========
	const Quantity_Color COLOR_MAIN(Quantity_NOC_WHITE);      // 主体白
	const Quantity_Color COLOR_ARMOR(Quantity_NOC_BLUE4);      // 装甲蓝
	const Quantity_Color COLOR_DECO(Quantity_NOC_RED);        // 装饰红
	const Quantity_Color COLOR_DETAIL(Quantity_NOC_YELLOW);     // 细节黄
	const Quantity_Color COLOR_LEG(Quantity_NOC_GRAY75);     // 腿部灰

	// 辅助函数：简化 形状创建+变换+上色+显示 流程
	auto DrawShape = [&](const TopoDS_Shape& theShape,
		const gp_Trsf& theTrsf,
		const Quantity_Color& theColor)
		{
			TopoDS_Shape aTransShape = BRepBuilderAPI_Transform(theShape, theTrsf, Standard_True);
			Handle(AIS_Shape) anAis = new AIS_Shape(aTransShape);
			anAis->SetColor(theColor);
			myAISContext->Display(anAis, Standard_False);
		};

	// ========== 1. 头部 + 颈部（填补缝隙） ==========
	// 颈部圆柱
	TopoDS_Shape neck = BRepPrimAPI_MakeCylinder(10.0, 12.0);
	gp_Trsf neckTrsf;
	neckTrsf.SetTranslation(gp_Vec(0.0, 0.0, 82.0));
	DrawShape(neck, neckTrsf, COLOR_MAIN);

	// 头部主体（半径略小，更协调）
	TopoDS_Shape head = BRepPrimAPI_MakeSphere(24.0);
	gp_Trsf headTrsf;
	headTrsf.SetTranslation(gp_Vec(0.0, 0.0, 96.0));
	DrawShape(head, headTrsf, COLOR_MAIN);

	// 双天线
	TopoDS_Shape antenna = BRepPrimAPI_MakeCylinder(1.8, 28.0);
	gp_Trsf antLTrsf; antLTrsf.SetTranslation(gp_Vec(-6.0, 0.0, 120.0));
	gp_Trsf antRTrsf; antRTrsf.SetTranslation(gp_Vec(6.0, 0.0, 120.0));
	DrawShape(antenna, antLTrsf, COLOR_DECO);
	DrawShape(antenna, antRTrsf, COLOR_DECO);

	// 眼部传感器
	TopoDS_Shape eye = BRepPrimAPI_MakeBox(8.0, 5.0, 6.0);
	gp_Trsf eyeLTrsf; eyeLTrsf.SetTranslation(gp_Vec(-12.0, 19.0, 95.0));
	gp_Trsf eyeRTrsf; eyeRTrsf.SetTranslation(gp_Vec(8.0, 19.0, 95.0));
	DrawShape(eye, eyeLTrsf, COLOR_DETAIL);
	DrawShape(eye, eyeRTrsf, COLOR_DETAIL);

	// 头部V字天线（额外装饰）
	TopoDS_Shape vFin = BRepPrimAPI_MakeCylinder(2.0, 10.0);
	gp_Trsf vFinTrsf; vFinTrsf.SetRotation(gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 1, 0)), M_PI / 6);
	vFinTrsf.SetTranslationPart(gp_Vec(0.0, 0.0, 115.0));
	DrawShape(vFin, vFinTrsf, COLOR_DECO);

	// ========== 2. 躯干（加长并修正位置） ==========
	TopoDS_Shape body = BRepPrimAPI_MakeBox(56.0, 30.0, 50.0);
	gp_Trsf bodyTrsf;
	bodyTrsf.SetTranslation(gp_Vec(-28.0, -15.0, 32.0));
	DrawShape(body, bodyTrsf, COLOR_MAIN);

	// 胸部蓝色装甲
	TopoDS_Shape chestArmor = BRepPrimAPI_MakeBox(44.0, 24.0, 12.0);
	gp_Trsf chestTrsf; chestTrsf.SetTranslation(gp_Vec(-22.0, -12.0, 52.0));
	DrawShape(chestArmor, chestTrsf, COLOR_ARMOR);

	// 腰部（紧贴躯干下方）
	TopoDS_Shape waist = BRepPrimAPI_MakeBox(36.0, 22.0, 18.0);
	gp_Trsf waistTrsf; waistTrsf.SetTranslation(gp_Vec(-18.0, -11.0, 14.0));
	DrawShape(waist, waistTrsf, COLOR_DECO);

	// 前裙甲（遮挡腰部与腿部连接处）
	TopoDS_Shape frontSkirt = BRepPrimAPI_MakeBox(32.0, 8.0, 16.0);
	gp_Trsf skirtFTrsf; skirtFTrsf.SetTranslation(gp_Vec(-16.0, 3.0, 14.0));
	DrawShape(frontSkirt, skirtFTrsf, COLOR_ARMOR);

	// 侧裙甲
	TopoDS_Shape sideSkirt = BRepPrimAPI_MakeBox(8.0, 14.0, 16.0);
	gp_Trsf skirtLTrsf; skirtLTrsf.SetTranslation(gp_Vec(-26.0, -7.0, 14.0));
	gp_Trsf skirtRTrsf; skirtRTrsf.SetTranslation(gp_Vec(18.0, -7.0, 14.0));
	DrawShape(sideSkirt, skirtLTrsf, COLOR_ARMOR);
	DrawShape(sideSkirt, skirtRTrsf, COLOR_ARMOR);

	// ========== 3. 肩甲（包裹上臂顶端） ==========
	TopoDS_Shape shoulder = BRepPrimAPI_MakeBox(24.0, 24.0, 22.0);
	// 左肩
	gp_Trsf shoulderLTrsf; shoulderLTrsf.SetTranslation(gp_Vec(-40.0, -12.0, 72.0));
	DrawShape(shoulder, shoulderLTrsf, COLOR_ARMOR);
	// 右肩
	gp_Trsf shoulderRTrsf; shoulderRTrsf.SetTranslation(gp_Vec(16.0, -12.0, 72.0));
	DrawShape(shoulder, shoulderRTrsf, COLOR_ARMOR);

	// ========== 4. 手臂（无缝隙衔接） ==========
	// 上臂圆柱（共用）
	TopoDS_Shape armUp = BRepPrimAPI_MakeCylinder(10.0, 36.0);

	// 左臂上臂（向外旋转15度）
	gp_Trsf leftArmTrsf;
	leftArmTrsf.SetRotation(gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 1, 0)), M_PI * 0.15);
	leftArmTrsf.SetTranslationPart(gp_Vec(-38.0, -5.0, 68.0));
	DrawShape(armUp, leftArmTrsf, COLOR_MAIN);

	// 右臂上臂（对称旋转-15度）
	gp_Trsf rightArmTrsf;
	rightArmTrsf.SetRotation(gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 1, 0)), -M_PI * 0.15);
	rightArmTrsf.SetTranslationPart(gp_Vec(38.0, -5.0, 68.0));
	DrawShape(armUp, rightArmTrsf, COLOR_MAIN);

	// 左前臂（跟随上臂旋转）
	TopoDS_Shape armLowL = BRepPrimAPI_MakeBox(16.0, 16.0, 32.0);
	gp_Trsf leftLowTrsf;
	leftLowTrsf.SetRotation(gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 1, 0)), M_PI * 0.15);
	leftLowTrsf.SetTranslationPart(gp_Vec(-48.0, -8.0, 38.0));
	DrawShape(armLowL, leftLowTrsf, COLOR_MAIN);

	// 右前臂（对称旋转）
	TopoDS_Shape armLowR = BRepPrimAPI_MakeBox(16.0, 16.0, 32.0);
	gp_Trsf rightLowTrsf;
	rightLowTrsf.SetRotation(gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 1, 0)), -M_PI * 0.15);
	rightLowTrsf.SetTranslationPart(gp_Vec(48.0, -8.0, 38.0));
	DrawShape(armLowR, rightLowTrsf, COLOR_MAIN);

	// ========== 5. 腿部（大腿紧贴腰部，小腿+脚部） ==========
	// 大腿
	TopoDS_Shape thigh = BRepPrimAPI_MakeCylinder(12.0, 42.0);
	// 左大腿
	gp_Trsf leftThighTrsf; leftThighTrsf.SetTranslation(gp_Vec(-18.0, -6.0, -28.0));
	DrawShape(thigh, leftThighTrsf, COLOR_MAIN);
	// 右大腿
	gp_Trsf rightThighTrsf; rightThighTrsf.SetTranslation(gp_Vec(18.0, -6.0, -28.0));
	DrawShape(thigh, rightThighTrsf, COLOR_MAIN);

	// 小腿（圆柱，略细）
	TopoDS_Shape calf = BRepPrimAPI_MakeCylinder(10.0, 40.0);
	gp_Trsf leftCalfTrsf; leftCalfTrsf.SetTranslation(gp_Vec(-18.0, -6.0, -68.0));
	gp_Trsf rightCalfTrsf; rightCalfTrsf.SetTranslation(gp_Vec(18.0, -6.0, -68.0));
	DrawShape(calf, leftCalfTrsf, COLOR_LEG);
	DrawShape(calf, rightCalfTrsf, COLOR_LEG);

	// 脚部（加长并前移，更稳定）
	TopoDS_Shape foot = BRepPrimAPI_MakeBox(24.0, 22.0, 10.0);
	gp_Trsf footLTrsf; footLTrsf.SetTranslation(gp_Vec(-30.0, -11.0, -78.0));
	gp_Trsf footRTrsf; footRTrsf.SetTranslation(gp_Vec(6.0, -11.0, -78.0));
	DrawShape(foot, footLTrsf, COLOR_ARMOR);
	DrawShape(foot, footRTrsf, COLOR_ARMOR);

	// 脚部装甲细节（脚踝护甲）
	TopoDS_Shape ankleArmor = BRepPrimAPI_MakeBox(14.0, 10.0, 8.0);
	gp_Trsf ankleLTrsf; ankleLTrsf.SetTranslation(gp_Vec(-25.0, -5.0, -62.0));
	gp_Trsf ankleRTrsf; ankleRTrsf.SetTranslation(gp_Vec(11.0, -5.0, -62.0));
	DrawShape(ankleArmor, ankleLTrsf, COLOR_DECO);
	DrawShape(ankleArmor, ankleRTrsf, COLOR_DECO);

	// ========== 6. 武器（握持姿态调整） ==========
	TopoDS_Shape weapon = BRepPrimAPI_MakeBox(10.0, 10.0, 60.0);
	gp_Trsf weaponTrsf;
	// 与右手前臂配合，角度旋转
	weaponTrsf.SetRotation(gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(1, 0, 0)), M_PI * 0.1);
	weaponTrsf.SetTranslationPart(gp_Vec(56.0, -5.0, 12.0));
	DrawShape(weapon, weaponTrsf, Quantity_NOC_BLACK);

	// 枪口加长
	TopoDS_Shape barrel = BRepPrimAPI_MakeCylinder(3.0, 20.0);
	gp_Trsf barrelTrsf;
	barrelTrsf.SetRotation(gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(1, 0, 0)), M_PI * 0.1);
	barrelTrsf.SetTranslationPart(gp_Vec(66.0, 0.0, 17.0));
	DrawShape(barrel, barrelTrsf, Quantity_NOC_GRAY50);

	// ========== 7. 背包推进器（紧贴背部） ==========
	TopoDS_Shape backpack = BRepPrimAPI_MakeBox(30.0, 12.0, 30.0);
	gp_Trsf backTrsf; backTrsf.SetTranslation(gp_Vec(-15.0, -24.0, 40.0));
	DrawShape(backpack, backTrsf, COLOR_MAIN);

	// 推进器喷口
	TopoDS_Shape booster = BRepPrimAPI_MakeCylinder(9.0, 22.0);
	gp_Trsf boostLTrsf; boostLTrsf.SetTranslation(gp_Vec(-22.0, -26.0, 28.0));
	gp_Trsf boostRTrsf; boostRTrsf.SetTranslation(gp_Vec(13.0, -26.0, 28.0));
	DrawShape(booster, boostLTrsf, COLOR_DECO);
	DrawShape(booster, boostRTrsf, COLOR_DECO);

	// ========== 刷新视图 ==========
	myAISContext->UpdateCurrentViewer();
	CMDIFrameWnd* pFrame = (CMDIFrameWnd*)AfxGetApp()->m_pMainWnd;
	CMDIChildWnd* pChild = (CMDIChildWnd*)pFrame->GetActiveFrame();
	COccResurfView* pView = (COccResurfView*)pChild->GetActiveView();
	pView->FitAll();
}