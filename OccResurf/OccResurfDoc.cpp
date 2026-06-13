
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

#include "MainFrm.h"
#include "OccResurfView.h"

#include <propkey.h>

#include <algorithm>
#include <sstream>
#include <string>
#include <BinTools.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <Quantity_Color.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <gp_Ax1.hxx>
#include <gp_Dir.hxx>
#include <TCollection_ExtendedString.hxx>
#include <TDataStd_Name.hxx>
#include <TDataStd_Real.hxx>
#include <TNaming_Builder.hxx>
#ifdef _DEBUG
//#define new DEBUG_NEW
#endif

namespace
{
const UINT kOccResurfDocumentVersion = 1;
}

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
	myAISContext->SetAutomaticHilight(Standard_True);
	m_bModelBuilt = false;
	m_nextObjectId = 1;
	InitializeOcafDocument();
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
	ClearDocumentModel();
	SetModifiedFlag(FALSE);

	return TRUE;
}




// COccResurfDoc 序列化

void COccResurfDoc::Serialize(CArchive& ar)
{
	SerializePrimitiveObjects(ar);
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

void COccResurfDoc::UpdateModelCenter()
{
	if (myModelShapes.empty())
	{
		myModelCenter = gp_Pnt(0.0, 0.0, 0.0);
		return;
	}

	Bnd_Box aBox;
	for (const auto& aShape : myModelShapes)
	{
		if (!aShape.IsNull())
		{
			BRepBndLib::Add(aShape->Shape(), aBox);
		}
	}

	if (aBox.IsVoid())
	{
		myModelCenter = gp_Pnt(0.0, 0.0, 0.0);
		return;
	}

	Standard_Real xmin = 0.0, ymin = 0.0, zmin = 0.0;
	Standard_Real xmax = 0.0, ymax = 0.0, zmax = 0.0;
	aBox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
	myModelCenter.SetCoord((xmin + xmax) * 0.5, (ymin + ymax) * 0.5, (zmin + zmax) * 0.5);
}

COccResurfDoc::PrimitiveObject* COccResurfDoc::FindObjectById(int id)
{
	for (auto& object : myPrimitiveObjects)
	{
		if (object.id == id)
		{
			return &object;
		}
	}

	return nullptr;
}

const COccResurfDoc::PrimitiveObject* COccResurfDoc::FindObjectById(int id) const
{
	for (const auto& object : myPrimitiveObjects)
	{
		if (object.id == id)
		{
			return &object;
		}
	}

	return nullptr;
}

COccResurfDoc::PrimitiveObject* COccResurfDoc::FindObjectByShape(const Handle(AIS_Shape)& shape)
{
	if (shape.IsNull())
	{
		return nullptr;
	}

	for (auto& object : myPrimitiveObjects)
	{
		if (object.aisShape == shape)
		{
			return &object;
		}
	}

	return nullptr;
}

int COccResurfDoc::AddBoxObject(double length, double width, double height)
{
	PrimitiveObject object;
	object.id = m_nextObjectId++;
	object.kind = PrimitiveKind::Box;
	object.name = MakePrimitiveName(object.kind);
	object.parameters =
	{
		{ _T("length"), length },
		{ _T("width"), width },
		{ _T("height"), height }
	};
	RebuildObjectShape(object);
	myPrimitiveObjects.push_back(object);
	UpdateModelCenter();
	m_bModelBuilt = true;
	SetModifiedFlag(TRUE);
	return object.id;
}

int COccResurfDoc::AddSphereObject(double radius)
{
	PrimitiveObject object;
	object.id = m_nextObjectId++;
	object.kind = PrimitiveKind::Sphere;
	object.name = MakePrimitiveName(object.kind);
	object.parameters = { { _T("radius"), radius } };
	RebuildObjectShape(object);
	myPrimitiveObjects.push_back(object);
	UpdateModelCenter();
	m_bModelBuilt = true;
	SetModifiedFlag(TRUE);
	return object.id;
}

int COccResurfDoc::AddCylinderObject(double radius, double height)
{
	PrimitiveObject object;
	object.id = m_nextObjectId++;
	object.kind = PrimitiveKind::Cylinder;
	object.name = MakePrimitiveName(object.kind);
	object.parameters =
	{
		{ _T("radius"), radius },
		{ _T("height"), height }
	};
	RebuildObjectShape(object);
	myPrimitiveObjects.push_back(object);
	UpdateModelCenter();
	m_bModelBuilt = true;
	SetModifiedFlag(TRUE);
	return object.id;
}

bool COccResurfDoc::UpdateObjectParameter(int objectId, const CString& parameterName, double value)
{
	PrimitiveObject* object = FindObjectById(objectId);
	if (object == nullptr || value <= 0.0)
	{
		return false;
	}

	bool updated = false;
	for (auto& parameter : object->parameters)
	{
		if (parameter.name.CompareNoCase(parameterName) == 0)
		{
			parameter.value = value;
			updated = true;
			break;
		}
	}

	if (!updated)
	{
		return false;
	}

	RebuildObjectShape(*object);
	UpdateModelCenter();
	SetModifiedFlag(TRUE);
	return true;
}

bool COccResurfDoc::DeleteObject(int objectId)
{
	auto it = std::find_if(myPrimitiveObjects.begin(), myPrimitiveObjects.end(),
		[objectId](const PrimitiveObject& object) { return object.id == objectId; });
	if (it == myPrimitiveObjects.end())
	{
		return false;
	}

	if (!myAISContext.IsNull() && !it->aisShape.IsNull())
	{
		myAISContext->Remove(it->aisShape, Standard_False);
	}

	myModelShapes.erase(std::remove_if(myModelShapes.begin(), myModelShapes.end(),
		[&it](const Handle(AIS_Shape)& shape) { return shape == it->aisShape; }),
		myModelShapes.end());
	myPrimitiveObjects.erase(it);
	UpdateModelCenter();
	m_bModelBuilt = !myModelShapes.empty();
	if (!myAISContext.IsNull())
	{
		myAISContext->UpdateCurrentViewer();
	}
	SetModifiedFlag(TRUE);
	return true;
}

int COccResurfDoc::DeleteSelectedModelObjects()
{
	if (myAISContext.IsNull())
	{
		return 0;
	}

	std::vector<int> idsToDelete;
	for (myAISContext->InitSelected(); myAISContext->MoreSelected(); myAISContext->NextSelected())
	{
		Handle(AIS_Shape) shape = Handle(AIS_Shape)::DownCast(myAISContext->SelectedInteractive());
		PrimitiveObject* object = FindObjectByShape(shape);
		if (object != nullptr)
		{
			idsToDelete.push_back(object->id);
		}
	}

	int deleted = 0;
	for (const int id : idsToDelete)
	{
		if (DeleteObject(id))
		{
			++deleted;
		}
	}
	myAISContext->ClearSelected(Standard_False);
	if (deleted > 0)
	{
		SetModifiedFlag(TRUE);
	}
	return deleted;
}

void COccResurfDoc::ClearPrimitiveObjects()
{
	if (!myAISContext.IsNull())
	{
		for (const auto& object : myPrimitiveObjects)
		{
			if (!object.aisShape.IsNull())
			{
				myAISContext->Remove(object.aisShape, Standard_False);
			}
		}
	}

	myModelShapes.erase(std::remove_if(myModelShapes.begin(), myModelShapes.end(),
		[this](const Handle(AIS_Shape)& shape)
		{
			for (const auto& object : myPrimitiveObjects)
			{
				if (shape == object.aisShape)
				{
					return true;
				}
			}
			return false;
		}),
		myModelShapes.end());
	myPrimitiveObjects.clear();
	UpdateModelCenter();
	m_bModelBuilt = !myModelShapes.empty();
}

void COccResurfDoc::ClearDocumentModel()
{
	if (!myAISContext.IsNull())
	{
		for (const auto& shape : myModelShapes)
		{
			if (!shape.IsNull())
			{
				myAISContext->Remove(shape, Standard_False);
			}
		}
		myAISContext->ClearSelected(Standard_False);
	}

	myModelShapes.clear();
	myPrimitiveObjects.clear();
	InitializeOcafDocument();
	m_nextObjectId = 1;
	m_bModelBuilt = false;
	UpdateModelCenter();
}

std::vector<COccResurfDoc::PrimitiveObjectState> COccResurfDoc::CapturePrimitiveObjectStates() const
{
	std::vector<PrimitiveObjectState> states;
	states.reserve(myPrimitiveObjects.size());
	for (const auto& object : myPrimitiveObjects)
	{
		PrimitiveObjectState state;
		state.id = object.id;
		state.kind = object.kind;
		state.name = object.name;
		state.parameters = object.parameters;
		if (!object.aisShape.IsNull())
		{
			state.localTransformation = object.aisShape->LocalTransformation();
		}
		states.push_back(state);
	}
	return states;
}

void COccResurfDoc::RestorePrimitiveObjectStates(const std::vector<PrimitiveObjectState>& states)
{
	ClearDocumentModel();
	int nextId = 1;
	myPrimitiveObjects.reserve(states.size());
	for (const auto& state : states)
	{
		PrimitiveObject object;
		object.id = state.id;
		object.kind = state.kind;
		object.name = state.name;
		object.parameters = state.parameters;
		RebuildObjectShape(object);
		if (!object.aisShape.IsNull())
		{
			object.aisShape->SetLocalTransformation(state.localTransformation);
			if (!myAISContext.IsNull())
			{
				myAISContext->Redisplay(object.aisShape, Standard_False);
			}
		}
		myPrimitiveObjects.push_back(object);
		if (nextId < object.id + 1)
		{
			nextId = object.id + 1;
		}
	}

	m_nextObjectId = nextId;
	m_bModelBuilt = !myModelShapes.empty();
	UpdateModelCenter();
	if (!myAISContext.IsNull())
	{
		myAISContext->UpdateCurrentViewer();
	}
}

void COccResurfDoc::RebuildPrimitiveObjectsAfterLoad()
{
	const std::vector<PrimitiveObjectState> states = CapturePrimitiveObjectStates();
	RestorePrimitiveObjectStates(states);
}

void COccResurfDoc::SerializePrimitiveObjects(CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << kOccResurfDocumentVersion;
		const UINT count = static_cast<UINT>(myPrimitiveObjects.size());
		ar << count;
		for (const auto& object : myPrimitiveObjects)
		{
			ar << object.id;
			ar << static_cast<int>(object.kind);
			ar << object.name;
			const UINT parameterCount = static_cast<UINT>(object.parameters.size());
			ar << parameterCount;
			for (const auto& parameter : object.parameters)
			{
				ar << parameter.name;
				ar << parameter.value;
			}

			const gp_Trsf localTransformation = object.aisShape.IsNull()
				? gp_Trsf()
				: object.aisShape->LocalTransformation();
			for (int row = 1; row <= 3; ++row)
			{
				for (int col = 1; col <= 4; ++col)
				{
					ar << localTransformation.Value(row, col);
				}
			}
		}

		UINT shapeCount = 0;
		for (const auto& shape : myModelShapes)
		{
			if (!shape.IsNull() && !IsPrimitiveShape(shape))
			{
				++shapeCount;
			}
		}
		ar << shapeCount;
		for (const auto& shape : myModelShapes)
		{
			if (shape.IsNull() || IsPrimitiveShape(shape))
			{
				continue;
			}

			std::ostringstream shapeStream(std::ios::binary);
			BinTools::Write(shape->Shape(), shapeStream);
			const std::string binary = shapeStream.str();
			const UINT byteCount = static_cast<UINT>(binary.size());
			ar << byteCount;
			if (byteCount > 0)
			{
				ar.Write(binary.data(), byteCount);
			}

			Quantity_Color color;
			shape->Color(color);
			ar << color.Red() << color.Green() << color.Blue();

			const gp_Trsf localTransformation = shape->LocalTransformation();
			for (int row = 1; row <= 3; ++row)
			{
				for (int col = 1; col <= 4; ++col)
				{
					ar << localTransformation.Value(row, col);
				}
			}
		}
		return;
	}

	UINT version = 0;
	ar >> version;
	if (version != kOccResurfDocumentVersion)
	{
		AfxThrowArchiveException(CArchiveException::badSchema);
	}

	UINT count = 0;
	ar >> count;
	std::vector<PrimitiveObjectState> states;
	states.reserve(count);
	for (UINT i = 0; i < count; ++i)
	{
		PrimitiveObjectState state;
		int kind = 0;
		ar >> state.id;
		ar >> kind;
		state.kind = static_cast<PrimitiveKind>(kind);
		ar >> state.name;

		UINT parameterCount = 0;
		ar >> parameterCount;
		state.parameters.reserve(parameterCount);
		for (UINT parameterIndex = 0; parameterIndex < parameterCount; ++parameterIndex)
		{
			PrimitiveParameter parameter;
			ar >> parameter.name;
			ar >> parameter.value;
			state.parameters.push_back(parameter);
		}

		double matrix[12] = {};
		int matrixIndex = 0;
		for (int row = 1; row <= 3; ++row)
		{
			for (int col = 1; col <= 4; ++col)
			{
				ar >> matrix[matrixIndex++];
			}
		}
		state.localTransformation.SetValues(
			matrix[0], matrix[1], matrix[2], matrix[3],
			matrix[4], matrix[5], matrix[6], matrix[7],
			matrix[8], matrix[9], matrix[10], matrix[11]);
		states.push_back(state);
	}

	RestorePrimitiveObjectStates(states);
	UINT shapeCount = 0;
	ar >> shapeCount;
	for (UINT i = 0; i < shapeCount; ++i)
	{
		UINT byteCount = 0;
		ar >> byteCount;
		std::string binary(byteCount, '\0');
		if (byteCount > 0)
		{
			ar.Read(&binary[0], byteCount);
		}

		std::istringstream shapeStream(binary, std::ios::binary);
		TopoDS_Shape topoShape;
		BinTools::Read(topoShape, shapeStream);

		double red = 1.0;
		double green = 1.0;
		double blue = 1.0;
		ar >> red >> green >> blue;

		double matrix[12] = {};
		int matrixIndex = 0;
		for (int row = 1; row <= 3; ++row)
		{
			for (int col = 1; col <= 4; ++col)
			{
				ar >> matrix[matrixIndex++];
			}
		}
		gp_Trsf localTransformation;
		localTransformation.SetValues(
			matrix[0], matrix[1], matrix[2], matrix[3],
			matrix[4], matrix[5], matrix[6], matrix[7],
			matrix[8], matrix[9], matrix[10], matrix[11]);

		Handle(AIS_Shape) aisShape = new AIS_Shape(topoShape);
		aisShape->SetColor(Quantity_Color(red, green, blue, Quantity_TOC_RGB));
		aisShape->SetLocalTransformation(localTransformation);
		myModelShapes.push_back(aisShape);
		if (!myAISContext.IsNull())
		{
			myAISContext->Display(aisShape, Standard_False);
		}
	}
	m_bModelBuilt = !myModelShapes.empty();
	UpdateModelCenter();
	if (!myAISContext.IsNull())
	{
		myAISContext->UpdateCurrentViewer();
	}
	SetModifiedFlag(FALSE);
	UpdateAllViews(nullptr);
	CMainFrame* mainFrame = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
	if (mainFrame != nullptr)
	{
		mainFrame->RefreshObjectPanels();
	}
}

TopoDS_Shape COccResurfDoc::BuildPrimitiveShape(const PrimitiveObject& object) const
{
	auto param = [&object](LPCTSTR name, double fallback)
		{
			for (const auto& parameter : object.parameters)
			{
				if (parameter.name.CompareNoCase(name) == 0)
				{
					return parameter.value;
				}
			}
			return fallback;
		};

	switch (object.kind)
	{
	case PrimitiveKind::Box:
		return BRepPrimAPI_MakeBox(param(_T("length"), 10.0), param(_T("width"), 10.0), param(_T("height"), 10.0)).Shape();
	case PrimitiveKind::Sphere:
		return BRepPrimAPI_MakeSphere(param(_T("radius"), 10.0)).Shape();
	case PrimitiveKind::Cylinder:
		return BRepPrimAPI_MakeCylinder(param(_T("radius"), 5.0), param(_T("height"), 10.0)).Shape();
	default:
		return TopoDS_Shape();
	}
}

void COccResurfDoc::InitializeOcafDocument()
{
	myOcafDocument = new TDocStd_Document(TCollection_ExtendedString("OccResurf"));
}

void COccResurfDoc::StoreObjectInOcaf(PrimitiveObject& object)
{
	if (myOcafDocument.IsNull())
	{
		InitializeOcafDocument();
	}

	if (object.label.IsNull())
	{
		object.label = myOcafDocument->Main().NewChild();
	}

	USES_CONVERSION;
	TDataStd_Name::Set(object.label, TCollection_ExtendedString(T2A(object.name)));
	TDataStd_Name::Set(object.label.FindChild(1, Standard_True), TCollection_ExtendedString(T2A(PrimitiveKindToString(object.kind))));
	if (!object.aisShape.IsNull())
	{
		TNaming_Builder builder(object.label);
		builder.Generated(object.aisShape->Shape());
	}

	int childIndex = 10;
	for (const auto& parameter : object.parameters)
	{
		TDF_Label parameterLabel = object.label.FindChild(childIndex++, Standard_True);
		TDataStd_Name::Set(parameterLabel, TCollection_ExtendedString(T2A(parameter.name)));
		TDataStd_Real::Set(parameterLabel, parameter.value);
	}
}

void COccResurfDoc::RebuildObjectShape(PrimitiveObject& object)
{
	const TopoDS_Shape shape = BuildPrimitiveShape(object);
	if (shape.IsNull())
	{
		return;
	}

	if (object.aisShape.IsNull())
	{
		object.aisShape = new AIS_Shape(shape);
		switch (object.kind)
		{
		case PrimitiveKind::Box:
			object.aisShape->SetColor(Quantity_NOC_STEELBLUE);
			break;
		case PrimitiveKind::Sphere:
			object.aisShape->SetColor(Quantity_NOC_SEAGREEN);
			break;
		case PrimitiveKind::Cylinder:
			object.aisShape->SetColor(Quantity_NOC_GOLDENROD);
			break;
		default:
			break;
		}
		myModelShapes.push_back(object.aisShape);
		if (!myAISContext.IsNull())
		{
			myAISContext->Display(object.aisShape, Standard_False);
		}
	}
	else
	{
		object.aisShape->SetShape(shape);
		if (!myAISContext.IsNull())
		{
			myAISContext->Redisplay(object.aisShape, Standard_False);
		}
	}

	StoreObjectInOcaf(object);
	if (!myAISContext.IsNull())
	{
		myAISContext->UpdateCurrentViewer();
	}
}

bool COccResurfDoc::IsPrimitiveShape(const Handle(AIS_Shape)& shape) const
{
	if (shape.IsNull())
	{
		return false;
	}

	for (const auto& object : myPrimitiveObjects)
	{
		if (shape == object.aisShape)
		{
			return true;
		}
	}
	return false;
}

CString COccResurfDoc::MakePrimitiveName(PrimitiveKind kind) const
{
	CString name;
	name.Format(_T("%s_%d"), static_cast<LPCTSTR>(PrimitiveKindToString(kind)), m_nextObjectId - 1);
	return name;
}

CString COccResurfDoc::PrimitiveKindToString(PrimitiveKind kind) const
{
	switch (kind)
	{
	case PrimitiveKind::Box:
		return _T("Box");
	case PrimitiveKind::Sphere:
		return _T("Sphere");
	case PrimitiveKind::Cylinder:
		return _T("Cylinder");
	default:
		return _T("Object");
	}
}

void COccResurfDoc::DrawSphere(double Radius)
{
	(void)Radius;
	if (m_bModelBuilt)
	{
		return;
	}

	myModelShapes.clear();

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
			myModelShapes.push_back(anAis);
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
	UpdateModelCenter();
	m_bModelBuilt = true;
	myAISContext->UpdateCurrentViewer();
}
