// OccResurfView.cpp
#include "pch.h"
#include "framework.h"
#ifndef SHARED_HANDLERS
#include "OccResurf.h"
#endif

#include "OccResurfDoc.h"
#include "MainFrm.h"
#include "OccResurfView.h"

#include <algorithm>
#include <atlconv.h>
#include <fstream>
#include <sstream>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepTools_ReShape.hxx>
#include <BinTools.hxx>
#include <Bnd_Box.hxx>
#include <StdSelect_BRepOwner.hxx>
#include <TopAbs.hxx>
#include <cmath>

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#ifdef _DEBUG
//#define new DEBUG_NEW
#endif

namespace
{
constexpr Standard_Real kMoveStepPerPixel = 0.35;
constexpr Standard_Real kRotateStepPerPixel = 0.01;
constexpr Standard_Real kScaleStepPerPixel = 0.01;
constexpr Standard_Real kMinUniformScale = 0.1;
constexpr Standard_Real kMaxUniformScale = 10.0;
constexpr Standard_Real kStepRotationRadians = 15.0 * 3.14159265358979323846 / 180.0;
constexpr int kClickThreshold = 3;

// Team member 4: edit mode selection activation helper for OCCT in this branch.
void SetShapeSelectionMode(Handle(AIS_InteractiveContext) ctx, const Standard_Integer theSelectionMode)
{
	if (ctx.IsNull())
	{
		return;
	}

	AIS_ListOfInteractive aDisplayedObjects;
	ctx->DisplayedObjects(aDisplayedObjects);
	for (AIS_ListIteratorOfListOfInteractive it(aDisplayedObjects); it.More(); it.Next())
	{
		const Handle(AIS_Shape) aShape = Handle(AIS_Shape)::DownCast(it.Value());
		if (aShape.IsNull())
		{
			continue;
		}

		ctx->Deactivate(aShape);
		ctx->Activate(aShape, theSelectionMode, Standard_True);
	}
}
}

IMPLEMENT_DYNCREATE(COccResurfView, CView)

BEGIN_MESSAGE_MAP(COccResurfView, CView)
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &COccResurfView::OnFilePrintPreview)
	ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_MBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_MBUTTONUP()
	ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()

COccResurfView::COccResurfView() noexcept
{
	m_bIsRotating = false;
	m_bIsTransforming = false;
	m_currentMode = MODE_NONE;
	m_navigationMode = NAV_NONE;
	m_isEditMode = false;
	m_selectMode = 0;
	m_lockedAxis = AXIS_NONE;
	m_isOrthoView = true;
	m_isWireframeOnly = false;
	m_shadeMode = 0;
	m_bHasMouseMoved = false;
	m_hasSelectionAnchor = false;
	m_hasDragOverlay = false;
	m_lastMousePoint = CPoint(0, 0);
	m_mouseDownPoint = CPoint(0, 0);
	m_overlayLastPoint = CPoint(0, 0);
}

COccResurfView::~COccResurfView()
{
}

BOOL COccResurfView::PreCreateWindow(CREATESTRUCT& cs)
{
	return CView::PreCreateWindow(cs);
}

void COccResurfView::OnDraw(CDC* /*pDC*/)
{
	COccResurfDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc) return;
	if (myView.IsNull()) return;

	myView->MustBeResized();
	myView->Update();
}

void COccResurfView::OnFilePrintPreview()
{
#ifndef SHARED_HANDLERS
	AFXPrintPreview(this);
#endif
}

BOOL COccResurfView::OnPreparePrinting(CPrintInfo* pInfo)
{
	return DoPreparePrinting(pInfo);
}

BOOL COccResurfView::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg != nullptr && (pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN))
	{
		const UINT key = static_cast<UINT>(pMsg->wParam);
		const BOOL ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
		const BOOL shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
		const BOOL alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
		const bool isTransformModeActive = m_currentMode >= MODE_TRANS_MOVE
			&& m_currentMode <= MODE_TRANS_SCALE;
		Handle(AIS_InteractiveContext) ctx = GetDocument()->GetAISContext();

		if (isTransformModeActive && (key == 'X' || key == 'Y' || key == 'Z'))
		{
			int nextAxis = AXIS_NONE;
			if (key == 'X') nextAxis = AXIS_X;
			if (key == 'Y') nextAxis = AXIS_Y;
			if (key == 'Z') nextAxis = AXIS_Z;
			m_lockedAxis = (m_lockedAxis == nextAxis) ? AXIS_NONE : nextAxis;
			return TRUE;
		}

		// Team member 5: Global document commands and display settings.
		if (ctrl && shift && key == 'Z')
		{
			RedoLastAction();
			return TRUE;
		}
		if (ctrl && !shift && key == 'Z')
		{
			UndoLastAction();
			return TRUE;
		}
		if (ctrl && key == 'S')
		{
			AfxGetApp()->OnCmdMsg(ID_FILE_SAVE, 0, nullptr, nullptr);
			return TRUE;
		}
		if (ctrl && key == 'N')
		{
			AfxGetApp()->OnCmdMsg(ID_FILE_NEW, 0, nullptr, nullptr);
			return TRUE;
		}
		if (ctrl && key == 'O')
		{
			AfxGetApp()->OnCmdMsg(ID_FILE_OPEN, 0, nullptr, nullptr);
			return TRUE;
		}
		if (!isTransformModeActive && !ctrl && shift && key == 'Z')
		{
			m_isWireframeOnly = !m_isWireframeOnly;
			if (!myView.IsNull())
			{
				myView->SetComputedMode(Standard_False);
				myView->SetShadingModel(m_isWireframeOnly
					? Graphic3d_TypeOfShadingModel_Unlit
					: Graphic3d_TypeOfShadingModel_Gouraud);
			}
			if (!ctx.IsNull())
			{
				ctx->SetDisplayMode(m_isWireframeOnly ? AIS_WireFrame : AIS_Shaded, Standard_False);
				ctx->Redisplay(AIS_KOI_Shape, 0, Standard_False);
			}
			UpdateViewer();
			return TRUE;
		}
		if (!isTransformModeActive && !ctrl && !shift && key == 'Z')
		{
			m_isWireframeOnly = false;
			m_shadeMode = (m_shadeMode + 1) % 3;
			if (!ctx.IsNull())
			{
				switch (m_shadeMode)
				{
				case 0:
					ctx->SetDisplayMode(AIS_Shaded, Standard_False);
					myView->SetComputedMode(Standard_False);
					myView->SetShadingModel(Graphic3d_TypeOfShadingModel_Gouraud);
					break;
				case 1:
					ctx->SetDisplayMode(AIS_WireFrame, Standard_False);
					myView->SetComputedMode(Standard_False);
					myView->SetShadingModel(Graphic3d_TypeOfShadingModel_Unlit);
					break;
				default:
					ctx->SetDisplayMode(AIS_Shaded, Standard_False);
					myView->SetComputedMode(Standard_True);
					myView->SetShadingModel(Graphic3d_TypeOfShadingModel_Gouraud);
					break;
				}
				ctx->Redisplay(AIS_KOI_Shape, 0, Standard_False);
			}
			UpdateViewer();
			return TRUE;
		}
		if (!ctrl && key == 'N')
		{
			ToggleRightSidebar();
			return TRUE;
		}
		if (!ctrl && key == 'T')
		{
			ToggleLeftToolbar();
			return TRUE;
		}

		// Team member 1: view navigation and camera shortcuts.
		if (!isTransformModeActive && !m_isEditMode && !myView.IsNull())
		{
			if (!ctrl && shift && key == 'C')
			{
				myView->Reset(Standard_False);
				FitAll();
				UpdateViewer();
				return TRUE;
			}
			if (key == VK_NUMPAD1)
			{
				myView->SetProj(ctrl ? V3d_Ypos : V3d_Yneg, Standard_False);
				FitAll();
				UpdateViewer();
				return TRUE;
			}
			if (key == VK_NUMPAD3)
			{
				myView->SetProj(ctrl ? V3d_Xneg : V3d_Xpos, Standard_False);
				FitAll();
				UpdateViewer();
				return TRUE;
			}
			if (key == VK_NUMPAD7)
			{
				myView->SetProj(ctrl ? V3d_Zneg : V3d_Zpos, Standard_False);
				FitAll();
				UpdateViewer();
				return TRUE;
			}
			if (key == VK_NUMPAD0)
			{
				myView->SetProj(V3d_XnegYnegZpos, Standard_False);
				UpdateViewer();
				return TRUE;
			}
			if (key == VK_NUMPAD5)
			{
				m_isOrthoView = !m_isOrthoView;
				if (!myView->Camera().IsNull())
				{
					myView->Camera()->SetProjectionType(m_isOrthoView
						? Graphic3d_Camera::Projection_Orthographic
						: Graphic3d_Camera::Projection_Perspective);
				}
				UpdateViewer();
				return TRUE;
			}
			if (key == VK_NUMPAD2)
			{
				myView->Rotate(0.0, kStepRotationRadians, 0.0, Standard_True);
				UpdateViewer();
				return TRUE;
			}
			if (key == VK_NUMPAD4)
			{
				myView->Rotate(kStepRotationRadians, 0.0, 0.0, Standard_True);
				UpdateViewer();
				return TRUE;
			}
			if (key == VK_NUMPAD6)
			{
				myView->Rotate(-kStepRotationRadians, 0.0, 0.0, Standard_True);
				UpdateViewer();
				return TRUE;
			}
			if (key == VK_NUMPAD8)
			{
				myView->Rotate(0.0, -kStepRotationRadians, 0.0, Standard_True);
				UpdateViewer();
				return TRUE;
			}
		}

		// Team member 2: selection and object editing shortcuts.
		if (!isTransformModeActive && !m_isEditMode && !ctx.IsNull())
		{
			if (!alt && !ctrl && shift && key == 'A')
			{
				SelectAllDisplayedObjects(ctx);
				return TRUE;
			}
			if (alt && key == 'A')
			{
				ClearAllSelection();
				return TRUE;
			}
			if (ctrl && key == 'I')
			{
				InvertDisplayedSelection(ctx);
				return TRUE;
			}
			if (!ctrl && !shift && key == 'B')
			{
				m_navigationMode = NAV_SELECT_BOX;
				return TRUE;
			}
			if (!ctrl && !shift && key == 'C')
			{
				m_navigationMode = NAV_SELECT_CIRCLE;
				return TRUE;
			}
			if (key == 'X' || key == VK_DELETE)
			{
				DeleteSelectedObjects(ctx);
				return TRUE;
			}
			if (!alt && !shift && key == 'H')
			{
				RecordUndoSnapshot();
				ClearSelectionHighlights();
				ctx->EraseSelected(Standard_True);
				UpdateViewer();
				return TRUE;
			}
			if (alt && key == 'H')
			{
				RecordUndoSnapshot();
				ClearSelectionHighlights();
				ctx->DisplayAll(Standard_True);
				UpdateSelectionHighlights();
				UpdateViewer();
				return TRUE;
			}
			if (shift && key == 'H')
			{
				HideUnselectedObjects(ctx);
				return TRUE;
			}
			if (shift && key == 'D')
			{
				DuplicateSelectedObjects(ctx);
				return TRUE;
			}
		}

		// Team member 4: Edit mode and mesh editing shortcuts.
		if (key == VK_TAB)
		{
			m_isEditMode = !m_isEditMode;
			m_selectMode = m_isEditMode ? 3 : 0;
			SetShapeSelectionMode(ctx, m_isEditMode ? AIS_Shape::SelectionMode(TopAbs_FACE) : 0);
			if (!ctx.IsNull())
			{
				ctx->UpdateCurrentViewer();
			}
			if (!myView.IsNull())
			{
				myView->Update();
			}
			AfxMessageBox(m_isEditMode ? _T("Edit mode ON") : _T("Object mode ON"));
			return TRUE;
		}

		if (m_isEditMode && !ctx.IsNull())
		{
			if (key == '1')
			{
				m_selectMode = 1;
				SetShapeSelectionMode(ctx, AIS_Shape::SelectionMode(TopAbs_VERTEX));
				ctx->UpdateCurrentViewer();
				if (!myView.IsNull())
				{
					myView->Update();
				}
				return TRUE;
			}
			if (key == '2')
			{
				m_selectMode = 2;
				SetShapeSelectionMode(ctx, AIS_Shape::SelectionMode(TopAbs_EDGE));
				ctx->UpdateCurrentViewer();
				if (!myView.IsNull())
				{
					myView->Update();
				}
				return TRUE;
			}
			if (key == '3')
			{
				m_selectMode = 3;
				SetShapeSelectionMode(ctx, AIS_Shape::SelectionMode(TopAbs_FACE));
				ctx->UpdateCurrentViewer();
				if (!myView.IsNull())
				{
					myView->Update();
				}
				return TRUE;
			}
			if (key == 'E')
			{
				gp_Trsf aTranslation;
				aTranslation.SetTranslation(gp_Vec(0.0, 0.0, 10.0));
				ApplyTransformToSelected(ctx, aTranslation);
				return TRUE;
			}
			if (key == 'F')
			{
				std::vector<TopoDS_Shape> subShapes;
				std::vector<Handle(AIS_Shape)> parentShapes;
				CollectSelectedSubShapes(subShapes, parentShapes);
				if (subShapes.empty())
				{
					AfxMessageBox(_T("Face/Edge created"));
					return TRUE;
				}
				if (subShapes.size() == 2
					&& subShapes[0].ShapeType() == TopAbs_VERTEX
					&& subShapes[1].ShapeType() == TopAbs_VERTEX)
				{
					RecordUndoSnapshot();
					const TopoDS_Vertex v1 = TopoDS::Vertex(subShapes[0]);
					const TopoDS_Vertex v2 = TopoDS::Vertex(subShapes[1]);
					BRepBuilderAPI_MakeEdge mkEdge(v1, v2);
					if (mkEdge.IsDone())
					{
						BRepBuilderAPI_MakeWire mkWire;
						mkWire.Add(mkEdge.Edge());
						if (mkWire.IsDone())
						{
							Handle(AIS_Shape) newShape = new AIS_Shape(mkWire.Wire());
							GetDocument()->AccessModelShapes().push_back(newShape);
							ctx->Display(newShape, Standard_True);
						}
					}
				}
				else if (subShapes.size() == 1 && subShapes[0].ShapeType() == TopAbs_EDGE)
				{
					RecordUndoSnapshot();
					BRepBuilderAPI_MakeWire mkWire;
					mkWire.Add(TopoDS::Edge(subShapes[0]));
					if (mkWire.IsDone())
					{
						BRepBuilderAPI_MakeFace mkFace(mkWire.Wire(), Standard_True);
						if (mkFace.IsDone())
						{
							parentShapes[0]->SetShape(mkFace.Face());
							ctx->Redisplay(parentShapes[0], Standard_True);
						}
					}
				}
				return TRUE;
			}
			if (ctrl && key == 'B')
			{
				std::vector<TopoDS_Shape> subShapes;
				std::vector<Handle(AIS_Shape)> parentShapes;
				CollectSelectedSubShapes(subShapes, parentShapes);
				if (!subShapes.empty())
				{
					RecordUndoSnapshot();
					Handle(AIS_Shape) parentShape = parentShapes.front();
					try
					{
						BRepFilletAPI_MakeFillet mkFillet(parentShape->Shape());
						bool hasEdge = false;
						for (const auto& subShape : subShapes)
						{
							if (subShape.ShapeType() == TopAbs_EDGE)
							{
								mkFillet.Add(2.0, TopoDS::Edge(subShape));
								hasEdge = true;
							}
						}
						if (hasEdge)
						{
							mkFillet.Build();
							if (mkFillet.IsDone())
							{
								parentShape->SetShape(mkFillet.Shape());
								ctx->Redisplay(parentShape, Standard_True);
							}
						}
					}
					catch (Standard_Failure const&)
					{
						AfxMessageBox(_T("Bevel failed on the selected edge(s)"));
					}
				}
				else
				{
					AfxMessageBox(_T("Bevel mode"));
				}
				return TRUE;
			}
			if (ctrl && key == 'R')
			{
				TopoDS_Shape subShape;
				Handle(AIS_Shape) parentShape;
				if (GetSingleSelectedSubShape(subShape, parentShape) && subShape.ShapeType() == TopAbs_EDGE)
				{
					RecordUndoSnapshot();
					BRepBuilderAPI_MakeEdge mkOffsetEdge(BRep_Tool::Pnt(TopExp::FirstVertex(TopoDS::Edge(subShape))),
						BRep_Tool::Pnt(TopExp::LastVertex(TopoDS::Edge(subShape))).Translated(gp_Vec(0.0, 0.0, 5.0)));
					TopTools_SequenceOfShape edges;
					edges.Append(mkOffsetEdge.Edge());
					BRepFeat_SplitShape splitter(parentShape->Shape());
					splitter.Add(edges);
					splitter.Build();
					if (splitter.IsDone())
					{
						parentShape->SetShape(splitter.Shape());
						ctx->Redisplay(parentShape, Standard_True);
					}
				}
				else
				{
					AfxMessageBox(_T("Loop cut mode"));
				}
				return TRUE;
			}
			if (key == 'M')
			{
				std::vector<TopoDS_Shape> subShapes;
				std::vector<Handle(AIS_Shape)> parentShapes;
				CollectSelectedSubShapes(subShapes, parentShapes);
				if (subShapes.size() >= 2)
				{
					RecordUndoSnapshot();
					gp_XYZ sum(0.0, 0.0, 0.0);
					int vertexCount = 0;
					for (const auto& subShape : subShapes)
					{
						if (subShape.ShapeType() == TopAbs_VERTEX)
						{
							sum += BRep_Tool::Pnt(TopoDS::Vertex(subShape)).XYZ();
							++vertexCount;
						}
					}
					if (vertexCount == 0)
					{
						return TRUE;
					}

					const gp_Pnt mergePoint(sum / static_cast<double>(vertexCount));
					Handle(AIS_Shape) parentShape = parentShapes.front();
					BRepTools_ReShape reshaper;
					TopoDS_Vertex targetVertex;
					bool hasTargetVertex = false;
					for (const auto& subShape : subShapes)
					{
						if (subShape.ShapeType() != TopAbs_VERTEX)
						{
							continue;
						}

						const TopoDS_Vertex sourceVertex = TopoDS::Vertex(subShape);
						if (!hasTargetVertex)
						{
							targetVertex = reshaper.CopyVertex(sourceVertex, mergePoint, BRep_Tool::Tolerance(sourceVertex));
							hasTargetVertex = true;
						}
						reshaper.Replace(sourceVertex, targetVertex);
					}
					parentShape->SetShape(reshaper.Apply(parentShape->Shape()));
					ctx->Redisplay(parentShape, Standard_True);
				}
				else
				{
					AfxMessageBox(_T("Merge selected"));
				}
				return TRUE;
			}
		}

		// ============== 组员3：物体变换快捷键 ==============
		if (alt && key == 'G')
		{
			ResetTransform(ctx, 0);
			ExitTransformMode();
			return TRUE;
		}
		if (alt && key == 'R')
		{
			ResetTransform(ctx, 1);
			ExitTransformMode();
			return TRUE;
		}
		if (alt && key == 'S')
		{
			ResetTransform(ctx, 2);
			ExitTransformMode();
			return TRUE;
		}
		if (ctrl && key == 'A')
		{
			ApplyTransformPermanently(ctx);
			CommitTransformMode();
			return TRUE;
		}
		if ((key == VK_RETURN || key == VK_SPACE) && isTransformModeActive)
		{
			CommitTransformMode();
			return TRUE;
		}
		if (key == VK_ESCAPE)
		{
			ExitTransformMode();
			return TRUE;
		}
		if (key == 'G')
		{
			if (m_currentMode == MODE_TRANS_MOVE)
			{
				CommitTransformMode();
				return TRUE;
			}
			if (GetSelectedShape(ctx).IsNull())
			{
				return TRUE;
			}
			m_currentMode = MODE_TRANS_MOVE;
			m_lockedAxis = AXIS_NONE;
			return TRUE;
		}
		if (key == 'R')
		{
			if (m_currentMode == MODE_TRANS_ROTATE)
			{
				CommitTransformMode();
				return TRUE;
			}
			if (GetSelectedShape(ctx).IsNull())
			{
				return TRUE;
			}
			m_currentMode = MODE_TRANS_ROTATE;
			m_lockedAxis = AXIS_NONE;
			return TRUE;
		}
		if (key == 'S')
		{
			if (m_currentMode == MODE_TRANS_SCALE)
			{
				CommitTransformMode();
				return TRUE;
			}
			if (GetSelectedShape(ctx).IsNull())
			{
				return TRUE;
			}
			m_currentMode = MODE_TRANS_SCALE;
			m_lockedAxis = AXIS_NONE;
			return TRUE;
		}
	}

	return CView::PreTranslateMessage(pMsg);
}

void COccResurfView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/) {}
void COccResurfView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/) {}

void COccResurfView::OnRButtonUp(UINT /* nFlags */, CPoint point)
{
	ClientToScreen(&point);
	OnContextMenu(this, point);
}

void COccResurfView::OnContextMenu(CWnd* /* pWnd */, CPoint point)
{
#ifndef SHARED_HANDLERS
	theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EDIT, point.x, point.y, this, TRUE);
#endif
}

void COccResurfView::ExitTransformMode()
{
	m_currentMode = MODE_NONE;
	m_lockedAxis = AXIS_NONE;
}

void COccResurfView::CommitTransformMode()
{
	m_lockedAxis = AXIS_NONE;
	m_currentMode = MODE_NONE;
}

void COccResurfView::ClearAllSelection()
{
	Handle(AIS_InteractiveContext) ctx = GetDocument()->GetAISContext();
	if (ctx.IsNull())
	{
		return;
	}

	ctx->ClearSelected(Standard_False);
	ctx->UnhilightSelected(Standard_False);
	ctx->ClearDetected(Standard_False);
	ctx->UpdateSelected(Standard_True);
	m_hasSelectionAnchor = false;
	ClearSelectionHighlights();
	UpdateViewer();
}

void COccResurfView::ClearSelectionHighlights()
{
	Handle(AIS_InteractiveContext) ctx = GetDocument()->GetAISContext();
	if (!ctx.IsNull())
	{
		for (const auto& aHighlight : m_selectionHighlights)
		{
			if (!aHighlight.IsNull())
			{
				ctx->Remove(aHighlight, Standard_False);
			}
		}
	}

	m_selectionHighlights.clear();
}

void COccResurfView::UpdateSelectionHighlights()
{
	Handle(AIS_InteractiveContext) ctx = GetDocument()->GetAISContext();
	if (ctx.IsNull())
	{
		m_selectionHighlights.clear();
		return;
	}

	std::vector<Handle(AIS_Shape)> aSelectedShapes;
	for (ctx->InitSelected(); ctx->MoreSelected(); ctx->NextSelected())
	{
		Handle(AIS_Shape) aShape = Handle(AIS_Shape)::DownCast(ctx->SelectedInteractive());
		if (!aShape.IsNull() && IsModelShape(aShape))
		{
			aSelectedShapes.push_back(aShape);
		}
	}

	ClearSelectionHighlights();
	m_selectionHighlights.reserve(aSelectedShapes.size());
	for (const auto& aShape : aSelectedShapes)
	{
		Handle(AIS_Shape) aHighlight = new AIS_Shape(aShape->Shape());
		aHighlight->SetColor(Quantity_NOC_YELLOW);
		aHighlight->SetWidth(4.0);
		aHighlight->SetTransparency(0.15);
		aHighlight->SetLocalTransformation(aShape->LocalTransformation());
		aHighlight->SetAutoHilight(Standard_False);
		ctx->Display(aHighlight, AIS_WireFrame, -1, Standard_False);
		m_selectionHighlights.push_back(aHighlight);
	}

	m_hasSelectionAnchor = !m_selectionHighlights.empty();
}

void COccResurfView::RefreshSelectionHighlights()
{
	UpdateSelectionHighlights();
	UpdateViewer();
}

bool COccResurfView::GetSingleSelectedSubShape(TopoDS_Shape& subShape, Handle(AIS_Shape)& parentShape) const
{
	Handle(AIS_InteractiveContext) ctx = GetDocument()->GetAISContext();
	if (ctx.IsNull() || ctx->NbSelected() <= 0)
	{
		return false;
	}

	ctx->InitSelected();
	if (!ctx->MoreSelected())
	{
		return false;
	}

	parentShape = Handle(AIS_Shape)::DownCast(ctx->SelectedInteractive());
	if (parentShape.IsNull())
	{
		return false;
	}

	const Handle(StdSelect_BRepOwner) aOwner = Handle(StdSelect_BRepOwner)::DownCast(ctx->SelectedOwner());
	if (aOwner.IsNull() || !aOwner->HasShape())
	{
		return false;
	}

	subShape = aOwner->Shape();
	return !subShape.IsNull();
}

void COccResurfView::CollectSelectedSubShapes(std::vector<TopoDS_Shape>& subShapes, std::vector<Handle(AIS_Shape)>& parentShapes) const
{
	subShapes.clear();
	parentShapes.clear();

	Handle(AIS_InteractiveContext) ctx = GetDocument()->GetAISContext();
	if (ctx.IsNull())
	{
		return;
	}

	for (ctx->InitSelected(); ctx->MoreSelected(); ctx->NextSelected())
	{
		const Handle(AIS_Shape) aParentShape = Handle(AIS_Shape)::DownCast(ctx->SelectedInteractive());
		const Handle(StdSelect_BRepOwner) aOwner = Handle(StdSelect_BRepOwner)::DownCast(ctx->SelectedOwner());
		if (aParentShape.IsNull() || aOwner.IsNull() || !aOwner->HasShape())
		{
			continue;
		}

		subShapes.push_back(aOwner->Shape());
		parentShapes.push_back(aParentShape);
	}
}

void COccResurfView::ToggleRightSidebar()
{
	CMainFrame* pMainFrame = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
	if (pMainFrame == nullptr)
	{
		return;
	}

	CPropertiesWnd& pane = pMainFrame->GetPropertiesPane();
	pMainFrame->ShowPane(&pane, !pane.IsWindowVisible(), FALSE, TRUE);
}

void COccResurfView::ToggleLeftToolbar()
{
	CMainFrame* pMainFrame = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
	if (pMainFrame == nullptr)
	{
		return;
	}

	CFileView& pane = pMainFrame->GetFileViewPane();
	pMainFrame->ShowPane(&pane, !pane.IsWindowVisible(), FALSE, TRUE);
}

void COccResurfView::ResetNavigationMode()
{
	m_navigationMode = NAV_NONE;
}

bool COccResurfView::HasTransformableModel() const
{
	const COccResurfDoc* pDoc = GetDocument();
	return pDoc != nullptr && pDoc->HasModelShapes();
}

bool COccResurfView::IsModelShape(const Handle(AIS_InteractiveObject)& theObject) const
{
	if (theObject.IsNull() || !HasTransformableModel())
	{
		return false;
	}

	for (const auto& aShape : GetDocument()->GetModelShapes())
	{
		if (aShape == theObject)
		{
			return true;
		}
	}

	return false;
}

COccResurfView::TransformState& COccResurfView::GetTransformState(const Handle(AIS_Shape)& theShape)
{
	return m_transformStates[theShape.get()];
}

Handle(AIS_Shape) COccResurfView::GetSelectedShape(Handle(AIS_InteractiveContext) ctx) const
{
	if (ctx.IsNull() || ctx->NbSelected() <= 0)
	{
		return Handle(AIS_Shape)();
	}

	for (ctx->InitSelected(); ctx->MoreSelected(); ctx->NextSelected())
	{
		Handle(AIS_Shape) aShape = Handle(AIS_Shape)::DownCast(ctx->SelectedInteractive());
		if (!aShape.IsNull())
		{
			return aShape;
		}
	}

	return Handle(AIS_Shape)();
}

gp_Pnt COccResurfView::GetShapeCenter(const Handle(AIS_Shape)& theShape) const
{
	if (theShape.IsNull())
	{
		return gp_Pnt(0.0, 0.0, 0.0);
	}

	Bnd_Box aBox;
	BRepBndLib::Add(theShape->Shape(), aBox);
	if (aBox.IsVoid())
	{
		return gp_Pnt(0.0, 0.0, 0.0);
	}

	Standard_Real xmin = 0.0, ymin = 0.0, zmin = 0.0;
	Standard_Real xmax = 0.0, ymax = 0.0, zmax = 0.0;
	aBox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
	return gp_Pnt((xmin + xmax) * 0.5, (ymin + ymax) * 0.5, (zmin + zmax) * 0.5);
}

gp_Trsf COccResurfView::BuildShapeTransform(const Handle(AIS_Shape)& theShape) const
{
	gp_Trsf aTransform;
	if (theShape.IsNull())
	{
		return aTransform;
	}

	TransformState aDefaultState;
	const auto it = m_transformStates.find(theShape.get());
	const TransformState& aState = it == m_transformStates.end() ? aDefaultState : it->second;
	const gp_Pnt aCenter = GetShapeCenter(theShape);

	gp_Trsf aScale;
	aScale.SetScale(aCenter, aState.uniformScale);

	gp_Trsf aRotateX;
	aRotateX.SetRotation(gp_Ax1(aCenter, gp::DX()), aState.rotationX);

	gp_Trsf aRotateY;
	aRotateY.SetRotation(gp_Ax1(aCenter, gp::DY()), aState.rotationY);

	gp_Trsf aRotateZ;
	aRotateZ.SetRotation(gp_Ax1(aCenter, gp::DZ()), aState.rotationZ);

	gp_Trsf aTranslate;
	aTranslate.SetTranslation(aState.translation);

	aTransform = aScale;
	aTransform.PreMultiply(aRotateX);
	aTransform.PreMultiply(aRotateY);
	aTransform.PreMultiply(aRotateZ);
	aTransform.PreMultiply(aTranslate);
	return aTransform;
}

void COccResurfView::ApplyTransform(Handle(AIS_InteractiveContext) ctx, const gp_Trsf& theTransform)
{
	if (ctx.IsNull())
	{
		return;
	}

	for (ctx->InitSelected(); ctx->MoreSelected(); ctx->NextSelected())
	{
		Handle(AIS_Shape) aShape = Handle(AIS_Shape)::DownCast(ctx->SelectedInteractive());
		if (!aShape.IsNull())
		{
			aShape->SetLocalTransformation(theTransform);
			ctx->Redisplay(aShape, Standard_False);
		}
	}

	UpdateSelectionHighlights();
	UpdateViewer();
}

// Team member 4: Apply transform helper
void COccResurfView::ApplyTransformToSelected(Handle(AIS_InteractiveContext) ctx, const gp_Trsf& trsf)
{
	if (ctx.IsNull())
	{
		return;
	}

	RecordUndoSnapshot();
	ClearSelectionHighlights();
	for (ctx->InitSelected(); ctx->MoreSelected(); ctx->NextSelected())
	{
		Handle(AIS_Shape) aShape = Handle(AIS_Shape)::DownCast(ctx->SelectedInteractive());
		if (aShape.IsNull())
		{
			continue;
		}

		TransformState& aState = GetTransformState(aShape);
		aState.translation += gp_Vec(trsf.TranslationPart());
		aShape->SetLocalTransformation(BuildShapeTransform(aShape));
		ctx->Redisplay(aShape, Standard_False);
	}

	ctx->UpdateCurrentViewer();
	if (!myView.IsNull())
	{
		myView->Update();
	}
}

void COccResurfView::UpdateSelectedTransforms(Handle(AIS_InteractiveContext) ctx)
{
	if (ctx.IsNull())
	{
		return;
	}

	for (ctx->InitSelected(); ctx->MoreSelected(); ctx->NextSelected())
	{
		Handle(AIS_Shape) aShape = Handle(AIS_Shape)::DownCast(ctx->SelectedInteractive());
		if (!aShape.IsNull())
		{
			aShape->SetLocalTransformation(BuildShapeTransform(aShape));
			ctx->Redisplay(aShape, Standard_False);
		}
	}

	UpdateSelectionHighlights();
	UpdateViewer();
}

void COccResurfView::ResetTransform(Handle(AIS_InteractiveContext) ctx, int type)
{
	Handle(AIS_Shape) aShape = GetSelectedShape(ctx);
	if (aShape.IsNull())
	{
		return;
	}

	RecordUndoSnapshot();
	TransformState& aState = GetTransformState(aShape);
	switch (type)
	{
	case 0:
		aState.translation = gp_Vec(0.0, 0.0, 0.0);
		break;
	case 1:
		aState.rotationX = 0.0;
		aState.rotationY = 0.0;
		aState.rotationZ = 0.0;
		break;
	case 2:
		aState.uniformScale = 1.0;
		break;
	default:
		aState = TransformState();
		break;
	}

	UpdateSelectedTransforms(ctx);
}

void COccResurfView::ApplyTransformPermanently(Handle(AIS_InteractiveContext) ctx)
{
	Handle(AIS_Shape) aShape = GetSelectedShape(ctx);
	if (ctx.IsNull() || aShape.IsNull())
	{
		return;
	}

	RecordUndoSnapshot();
	const gp_Trsf aTransform = BuildShapeTransform(aShape);
	const TopoDS_Shape aBakedShape = BRepBuilderAPI_Transform(aShape->Shape(), aTransform, Standard_True);
	aShape->SetShape(aBakedShape);
	aShape->ResetTransformation();
	ctx->Redisplay(aShape, Standard_False);
	GetTransformState(aShape) = TransformState();
	UpdateSelectionHighlights();
	UpdateViewer();
}

void COccResurfView::ApplyMouseTransform(Handle(AIS_InteractiveContext) ctx, const CPoint& delta)
{
	Handle(AIS_Shape) aShape = GetSelectedShape(ctx);
	if (ctx.IsNull() || aShape.IsNull())
	{
		return;
	}

	const Standard_Real aSignedDelta = static_cast<Standard_Real>(delta.x - delta.y);
	TransformState& aState = GetTransformState(aShape);

	switch (m_currentMode)
	{
	case MODE_TRANS_MOVE:
	{
		gp_Vec moveVec(0.0, 0.0, 0.0);
		if (!myView.IsNull() && !myView->Camera().IsNull())
		{
			const Handle(Graphic3d_Camera)& aCamera = myView->Camera();
			const gp_Vec aRight(aCamera->SideRight());
			const gp_Vec aUp(aCamera->OrthogonalizedUp());

			if (m_lockedAxis == AXIS_X)
			{
				moveVec = gp_Vec(aSignedDelta * kMoveStepPerPixel, 0.0, 0.0);
			}
			else if (m_lockedAxis == AXIS_Y)
			{
				moveVec = gp_Vec(0.0, aSignedDelta * kMoveStepPerPixel, 0.0);
			}
			else if (m_lockedAxis == AXIS_Z)
			{
				moveVec = gp_Vec(0.0, 0.0, aSignedDelta * kMoveStepPerPixel);
			}
			else
			{
				moveVec = aRight.Multiplied(static_cast<Standard_Real>(-delta.x) * kMoveStepPerPixel)
					+ aUp.Multiplied(static_cast<Standard_Real>(-delta.y) * kMoveStepPerPixel);
			}
		}

		if (m_lockedAxis == AXIS_X)
		{
			aState.translation += moveVec;
		}
		else if (m_lockedAxis == AXIS_Y)
		{
			aState.translation += moveVec;
		}
		else if (m_lockedAxis == AXIS_Z)
		{
			aState.translation += moveVec;
		}
		else
		{
			aState.translation += moveVec;
		}
		break;
	}
	case MODE_TRANS_ROTATE:
		if (m_lockedAxis == AXIS_X)
		{
			aState.rotationX += aSignedDelta * kRotateStepPerPixel;
		}
		else if (m_lockedAxis == AXIS_Y)
		{
			aState.rotationY += aSignedDelta * kRotateStepPerPixel;
		}
		else if (m_lockedAxis == AXIS_Z)
		{
			aState.rotationZ += aSignedDelta * kRotateStepPerPixel;
		}
		else
		{
			aState.rotationZ += delta.x * kRotateStepPerPixel;
			aState.rotationX += -delta.y * kRotateStepPerPixel;
		}
		break;
	case MODE_TRANS_SCALE:
	{
		Standard_Real nextScale = aState.uniformScale * (1.0 + aSignedDelta * kScaleStepPerPixel);
		if (nextScale < kMinUniformScale)
		{
			nextScale = kMinUniformScale;
		}
		if (nextScale > kMaxUniformScale)
		{
			nextScale = kMaxUniformScale;
		}
		aState.uniformScale = nextScale;
		break;
	}
	default:
		return;
	}

	UpdateSelectedTransforms(ctx);
}

COccResurfView::SceneSnapshot COccResurfView::CaptureScene() const
{
	SceneSnapshot aSnapshot;
	aSnapshot.primitiveObjects = GetDocument()->CapturePrimitiveObjectStates();
	const auto& aShapes = GetDocument()->GetModelShapes();
	aSnapshot.shapes.reserve(aShapes.size());

	for (const auto& aShape : aShapes)
	{
		if (aShape.IsNull() || GetDocument()->IsPrimitiveShape(aShape))
		{
			continue;
		}

		ShapeSnapshot aShapeSnapshot;
		aShapeSnapshot.shape = aShape->Shape();
		aShapeSnapshot.localTransform = aShape->LocalTransformation();
		aShape->Color(aShapeSnapshot.color);
		const auto it = m_transformStates.find(aShape.get());
		if (it != m_transformStates.end())
		{
			aShapeSnapshot.transformState = it->second;
		}
		aSnapshot.shapes.push_back(aShapeSnapshot);
	}

	return aSnapshot;
}

void COccResurfView::RestoreScene(const SceneSnapshot& snapshot)
{
	Handle(AIS_InteractiveContext) ctx = GetDocument()->GetAISContext();
	if (!ctx.IsNull())
	{
		ClearSelectionHighlights();
	}
	GetDocument()->RestorePrimitiveObjectStates(snapshot.primitiveObjects);

	auto& aShapes = GetDocument()->AccessModelShapes();
	for (const auto& aShapeSnapshot : snapshot.shapes)
	{
		Handle(AIS_Shape) aShape = new AIS_Shape(aShapeSnapshot.shape);
		aShape->SetColor(aShapeSnapshot.color);
		aShape->SetLocalTransformation(aShapeSnapshot.localTransform);
		aShapes.push_back(aShape);
		m_transformStates[aShape.get()] = aShapeSnapshot.transformState;
		if (!ctx.IsNull())
		{
			ctx->Display(aShape, Standard_False);
		}
	}

	GetDocument()->SetModelBuilt(!aShapes.empty());
	GetDocument()->UpdateModelCenter();
	UpdateViewer();
	CMainFrame* mainFrame = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
	if (mainFrame != nullptr)
	{
		mainFrame->RefreshObjectPanels();
	}
}

void COccResurfView::RecordUndoSnapshot()
{
	m_undoSnapshots.push_back(CaptureScene());
	m_redoSnapshots.clear();
}

bool COccResurfView::UndoLastAction()
{
	if (m_undoSnapshots.empty())
	{
		return false;
	}

	m_redoSnapshots.push_back(CaptureScene());
	const SceneSnapshot snapshot = m_undoSnapshots.back();
	m_undoSnapshots.pop_back();
	RestoreScene(snapshot);
	GetDocument()->SetModifiedFlag(TRUE);
	return true;
}

bool COccResurfView::RedoLastAction()
{
	if (m_redoSnapshots.empty())
	{
		return false;
	}

	m_undoSnapshots.push_back(CaptureScene());
	const SceneSnapshot snapshot = m_redoSnapshots.back();
	m_redoSnapshots.pop_back();
	RestoreScene(snapshot);
	GetDocument()->SetModifiedFlag(TRUE);
	return true;
}

void COccResurfView::SaveProjectToFile(const CString& filePath)
{
	USES_CONVERSION;
	std::ofstream aFile(W2A(filePath), std::ios::binary | std::ios::trunc);
	if (!aFile.is_open())
	{
		AfxMessageBox(_T("Failed to save project"));
		return;
	}

	const SceneSnapshot aSnapshot = CaptureScene();
	const int aShapeCount = static_cast<int>(aSnapshot.shapes.size());
	aFile.write(reinterpret_cast<const char*>(&aShapeCount), sizeof(aShapeCount));

	for (const auto& aShapeSnapshot : aSnapshot.shapes)
	{
		std::ostringstream aShapeStream(std::ios::binary);
		BinTools::Write(aShapeSnapshot.shape, aShapeStream);
		const std::string aBinary = aShapeStream.str();
		const int aByteCount = static_cast<int>(aBinary.size());
		aFile.write(reinterpret_cast<const char*>(&aByteCount), sizeof(aByteCount));
		if (aByteCount > 0)
		{
			aFile.write(aBinary.data(), aByteCount);
		}

		double aColor[3] =
		{
			aShapeSnapshot.color.Red(),
			aShapeSnapshot.color.Green(),
			aShapeSnapshot.color.Blue()
		};
		aFile.write(reinterpret_cast<const char*>(aColor), sizeof(aColor));

		double aMatrix[12];
		int index = 0;
		for (int row = 1; row <= 3; ++row)
		{
			for (int col = 1; col <= 4; ++col)
			{
				aMatrix[index++] = aShapeSnapshot.localTransform.Value(row, col);
			}
		}
		aFile.write(reinterpret_cast<const char*>(aMatrix), sizeof(aMatrix));

		const TransformState& aState = aShapeSnapshot.transformState;
		double aStateData[7] =
		{
			aState.translation.X(),
			aState.translation.Y(),
			aState.translation.Z(),
			aState.rotationX,
			aState.rotationY,
			aState.rotationZ,
			aState.uniformScale
		};
		aFile.write(reinterpret_cast<const char*>(aStateData), sizeof(aStateData));
	}
}

void COccResurfView::OpenProjectFromFile(const CString& filePath)
{
	USES_CONVERSION;
	std::ifstream aFile(W2A(filePath), std::ios::binary);
	if (!aFile.is_open())
	{
		AfxMessageBox(_T("project.3d not found"));
		return;
	}

	SceneSnapshot aSnapshot;
	int aShapeCount = 0;
	aFile.read(reinterpret_cast<char*>(&aShapeCount), sizeof(aShapeCount));

	for (int i = 0; i < aShapeCount; ++i)
	{
		int aByteCount = 0;
		aFile.read(reinterpret_cast<char*>(&aByteCount), sizeof(aByteCount));
		std::string aBinary(aByteCount, '\0');
		if (aByteCount > 0)
		{
			aFile.read(&aBinary[0], aByteCount);
		}

		std::istringstream aShapeStream(aBinary, std::ios::binary);
		TopoDS_Shape aShape;
		BinTools::Read(aShape, aShapeStream);

		double aColor[3] = {};
		aFile.read(reinterpret_cast<char*>(aColor), sizeof(aColor));

		double aMatrix[12] = {};
		aFile.read(reinterpret_cast<char*>(aMatrix), sizeof(aMatrix));
		gp_Trsf aTrsf;
		aTrsf.SetValues(
			aMatrix[0], aMatrix[1], aMatrix[2], aMatrix[3],
			aMatrix[4], aMatrix[5], aMatrix[6], aMatrix[7],
			aMatrix[8], aMatrix[9], aMatrix[10], aMatrix[11]);

		double aStateData[7] = {};
		aFile.read(reinterpret_cast<char*>(aStateData), sizeof(aStateData));

		ShapeSnapshot aShapeSnapshot;
		aShapeSnapshot.shape = aShape;
		aShapeSnapshot.color = Quantity_Color(aColor[0], aColor[1], aColor[2], Quantity_TOC_RGB);
		aShapeSnapshot.localTransform = aTrsf;
		aShapeSnapshot.transformState.translation = gp_Vec(aStateData[0], aStateData[1], aStateData[2]);
		aShapeSnapshot.transformState.rotationX = aStateData[3];
		aShapeSnapshot.transformState.rotationY = aStateData[4];
		aShapeSnapshot.transformState.rotationZ = aStateData[5];
		aShapeSnapshot.transformState.uniformScale = aStateData[6];
		aSnapshot.shapes.push_back(aShapeSnapshot);
	}

	RecordUndoSnapshot();
	RestoreScene(aSnapshot);
}

void COccResurfView::ClearScene()
{
	Handle(AIS_InteractiveContext) ctx = GetDocument()->GetAISContext();
	auto& aShapes = GetDocument()->AccessModelShapes();

	if (!ctx.IsNull())
	{
		ClearSelectionHighlights();
		for (const auto& aShape : aShapes)
		{
			if (!aShape.IsNull())
			{
				ctx->Remove(aShape, Standard_False);
			}
		}
		ctx->ClearSelected(Standard_False);
	}

	aShapes.clear();
	m_selectionHighlights.clear();
	m_transformStates.clear();
	m_hasSelectionAnchor = false;
	GetDocument()->ClearPrimitiveObjects();
	GetDocument()->SetModelBuilt(false);
	GetDocument()->UpdateModelCenter();
	UpdateViewer();
	CMainFrame* mainFrame = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
	if (mainFrame != nullptr)
	{
		mainFrame->RefreshObjectPanels();
	}
}

void COccResurfView::DrawDragRectangle(const CPoint& currentPoint)
{
	CClientDC dc(this);
	const int oldRop = dc.SetROP2(R2_NOTXORPEN);
	CPen pen(PS_SOLID, 2, RGB(255, 255, 0));
	CPen* oldPen = dc.SelectObject(&pen);
	CBrush* oldBrush = static_cast<CBrush*>(dc.SelectStockObject(NULL_BRUSH));

	if (m_hasDragOverlay)
	{
		dc.Rectangle(m_mouseDownPoint.x, m_mouseDownPoint.y, m_overlayLastPoint.x, m_overlayLastPoint.y);
	}

	dc.Rectangle(m_mouseDownPoint.x, m_mouseDownPoint.y, currentPoint.x, currentPoint.y);
	m_overlayLastPoint = currentPoint;
	m_hasDragOverlay = true;

	dc.SelectObject(oldBrush);
	dc.SelectObject(oldPen);
	dc.SetROP2(oldRop);
}

void COccResurfView::ClearDragRectangle()
{
	if (!m_hasDragOverlay)
	{
		return;
	}

	CClientDC dc(this);
	const int oldRop = dc.SetROP2(R2_NOTXORPEN);
	CPen pen(PS_SOLID, 2, RGB(255, 255, 0));
	CPen* oldPen = dc.SelectObject(&pen);
	CBrush* oldBrush = static_cast<CBrush*>(dc.SelectStockObject(NULL_BRUSH));
	dc.Rectangle(m_mouseDownPoint.x, m_mouseDownPoint.y, m_overlayLastPoint.x, m_overlayLastPoint.y);
	dc.SelectObject(oldBrush);
	dc.SelectObject(oldPen);
	dc.SetROP2(oldRop);
	m_hasDragOverlay = false;
}

void COccResurfView::UpdateViewer()
{
	Handle(AIS_InteractiveContext) ctx = GetDocument()->GetAISContext();
	if (!ctx.IsNull())
	{
		ctx->UpdateCurrentViewer();
	}
	if (!myView.IsNull())
	{
		myView->Update();
	}
}

bool COccResurfView::HandleObjectClick(UINT nFlags, const CPoint& point)
{
	Handle(AIS_InteractiveContext) ctx = GetDocument()->GetAISContext();
	if (ctx.IsNull() || myView.IsNull())
	{
		return false;
	}

	AIS_SelectionScheme aScheme = AIS_SelectionScheme_Replace;
	if (nFlags & MK_SHIFT)
	{
		aScheme = AIS_SelectionScheme_Add;
	}
	else if (nFlags & MK_CONTROL)
	{
		aScheme = AIS_SelectionScheme_Remove;
	}

	ctx->ClearDetected(Standard_False);
	const AIS_StatusOfDetection detectionStatus = ctx->MoveTo(point.x, point.y, myView, Standard_False);
	if (detectionStatus == AIS_SOD_Nothing)
	{
		if (aScheme == AIS_SelectionScheme_Replace)
		{
			ClearAllSelection();
		}
		return true;
	}

	Handle(AIS_InteractiveObject) detectedObject;
	for (ctx->InitDetected(); ctx->MoreDetected(); ctx->NextDetected())
	{
		const Handle(SelectMgr_EntityOwner) detectedOwner = ctx->DetectedCurrentOwner();
		if (detectedOwner.IsNull())
		{
			continue;
		}

		Handle(AIS_InteractiveObject) anObject =
			Handle(AIS_InteractiveObject)::DownCast(detectedOwner->Selectable());
		if (IsModelShape(anObject))
		{
			detectedObject = anObject;
			break;
		}
	}

	if (detectedObject.IsNull() || !IsModelShape(detectedObject))
	{
		if (aScheme == AIS_SelectionScheme_Replace)
		{
			ClearAllSelection();
		}
		return true;
	}

	if (aScheme == AIS_SelectionScheme_Replace)
	{
		ctx->ClearSelected(Standard_False);
		ctx->SetSelected(detectedObject, Standard_False);
	}
	else if (aScheme == AIS_SelectionScheme_Add)
	{
		if (!ctx->IsSelected(detectedObject))
		{
			ctx->AddOrRemoveSelected(detectedObject, Standard_False);
		}
	}
	else if (aScheme == AIS_SelectionScheme_Remove)
	{
		if (ctx->IsSelected(detectedObject))
		{
			ctx->AddOrRemoveSelected(detectedObject, Standard_False);
		}
	}

	m_hasSelectionAnchor = true;
	ctx->UpdateSelected(Standard_False);
	UpdateSelectionHighlights();
	UpdateViewer();
	return true;
}

bool COccResurfView::HandleEditClick(UINT nFlags, const CPoint& point)
{
	Handle(AIS_InteractiveContext) ctx = GetDocument()->GetAISContext();
	if (ctx.IsNull() || myView.IsNull())
	{
		return false;
	}

	AIS_SelectionScheme aScheme = AIS_SelectionScheme_Replace;
	if (nFlags & MK_SHIFT)
	{
		aScheme = AIS_SelectionScheme_Add;
	}
	else if (nFlags & MK_CONTROL)
	{
		aScheme = AIS_SelectionScheme_Remove;
	}

	const AIS_StatusOfPick aPick = ctx->SelectPoint(Graphic3d_Vec2i(point.x, point.y), myView, aScheme);
	if (aPick == AIS_SOP_NothingSelected)
	{
		if (aScheme == AIS_SelectionScheme_Replace)
		{
			ClearAllSelection();
		}
		return true;
	}

	ctx->UpdateSelected(Standard_False);
	UpdateSelectionHighlights();
	UpdateViewer();
	return true;
}

void COccResurfView::SelectModelAtPoint(const CPoint& point, const AIS_SelectionScheme theScheme)
{
	Handle(AIS_InteractiveContext) ctx = GetDocument()->GetAISContext();
	if (ctx.IsNull() || myView.IsNull() || !HasTransformableModel())
	{
		return;
	}

	ctx->MoveTo(point.x, point.y, myView, Standard_True);
	ctx->SelectPoint(Graphic3d_Vec2i(point.x, point.y), myView, theScheme);

	Handle(AIS_InteractiveObject) aPickedObject;
	if (ctx->NbSelected() > 0)
	{
		ctx->InitSelected();
		if (ctx->MoreSelected())
		{
			aPickedObject = ctx->SelectedInteractive();
		}
	}

	if (!IsModelShape(aPickedObject))
	{
		if (theScheme == AIS_SelectionScheme_Replace)
		{
			ctx->ClearSelected(Standard_True);
		}
		return;
	}

	m_hasSelectionAnchor = true;
	ctx->UpdateSelected(Standard_False);
	UpdateSelectionHighlights();
	UpdateViewer();
}

void COccResurfView::SelectAllDisplayedObjects(Handle(AIS_InteractiveContext) ctx)
{
	if (ctx.IsNull())
	{
		return;
	}

	ctx->ClearSelected(Standard_False);
	for (const auto& aShape : GetDocument()->GetModelShapes())
	{
		if (!aShape.IsNull() && ctx->IsDisplayed(aShape))
		{
			ctx->AddOrRemoveSelected(aShape, Standard_False);
		}
	}
	UpdateSelectionHighlights();
	UpdateViewer();
}

void COccResurfView::InvertDisplayedSelection(Handle(AIS_InteractiveContext) ctx)
{
	if (ctx.IsNull())
	{
		return;
	}

	for (const auto& aShape : GetDocument()->GetModelShapes())
	{
		if (aShape.IsNull() || !ctx->IsDisplayed(aShape))
		{
			continue;
		}

		ctx->AddOrRemoveSelected(aShape, Standard_False);
	}
	UpdateSelectionHighlights();
	UpdateViewer();
}

void COccResurfView::HideUnselectedObjects(Handle(AIS_InteractiveContext) ctx)
{
	if (ctx.IsNull())
	{
		return;
	}

	RecordUndoSnapshot();
	ClearSelectionHighlights();
	for (const auto& aShape : GetDocument()->GetModelShapes())
	{
		if (!aShape.IsNull() && ctx->IsDisplayed(aShape) && !ctx->IsSelected(aShape))
		{
			ctx->Erase(aShape, Standard_False);
		}
	}
	UpdateSelectionHighlights();
	UpdateViewer();
}

void COccResurfView::DuplicateSelectedObjects(Handle(AIS_InteractiveContext) ctx)
{
	if (ctx.IsNull())
	{
		return;
	}

	RecordUndoSnapshot();
	std::vector<Handle(AIS_Shape)> aDuplicates;
	for (ctx->InitSelected(); ctx->MoreSelected(); ctx->NextSelected())
	{
		Handle(AIS_Shape) aShape = Handle(AIS_Shape)::DownCast(ctx->SelectedInteractive());
		if (aShape.IsNull())
		{
			continue;
		}

		TopoDS_Shape aCopiedShape = BRepBuilderAPI_Copy(aShape->Shape(), Standard_True).Shape();
		Handle(AIS_Shape) aDuplicate = new AIS_Shape(aCopiedShape);
		Quantity_Color aColor;
		aShape->Color(aColor);
		aDuplicate->SetColor(aColor);
		aDuplicate->SetLocalTransformation(aShape->LocalTransformation());
		const auto it = m_transformStates.find(aShape.get());
		if (it != m_transformStates.end())
		{
			m_transformStates[aDuplicate.get()] = it->second;
		}
		aDuplicates.push_back(aDuplicate);
	}

	auto& aShapes = const_cast<std::vector<Handle(AIS_Shape)>&>(GetDocument()->GetModelShapes());
	ctx->ClearSelected(Standard_False);
	for (const auto& aDuplicate : aDuplicates)
	{
		aShapes.push_back(aDuplicate);
		ctx->Display(aDuplicate, Standard_False);
		ctx->SetSelected(aDuplicate, Standard_False);
	}
	GetDocument()->UpdateModelCenter();
	UpdateSelectionHighlights();
	UpdateViewer();
}

void COccResurfView::DeleteSelectedObjects(Handle(AIS_InteractiveContext) ctx)
{
	if (ctx.IsNull())
	{
		return;
	}

	RecordUndoSnapshot();
	ClearSelectionHighlights();
	std::vector<const AIS_InteractiveObject*> aToDelete;
	for (ctx->InitSelected(); ctx->MoreSelected(); ctx->NextSelected())
	{
		Handle(AIS_InteractiveObject) anObject = ctx->SelectedInteractive();
		if (!anObject.IsNull())
		{
			aToDelete.push_back(anObject.get());
		}
	}

	GetDocument()->DeleteSelectedModelObjects();
	for (const auto* anObjectPtr : aToDelete)
	{
		for (auto& aShape : const_cast<std::vector<Handle(AIS_Shape)>&>(GetDocument()->GetModelShapes()))
		{
			if (!aShape.IsNull() && aShape.get() == anObjectPtr)
			{
				ctx->Remove(aShape, Standard_False);
				m_transformStates.erase(aShape.get());
				aShape.Nullify();
				break;
			}
		}
	}

	auto& aShapes = const_cast<std::vector<Handle(AIS_Shape)>&>(GetDocument()->GetModelShapes());
	ctx->ClearSelected(Standard_False);
	aShapes.erase(std::remove_if(aShapes.begin(), aShapes.end(),
		[](const Handle(AIS_Shape)& theShape) { return theShape.IsNull(); }),
		aShapes.end());
	GetDocument()->UpdateModelCenter();
	UpdateSelectionHighlights();
	UpdateViewer();
	CMainFrame* mainFrame = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
	if (mainFrame != nullptr)
	{
		mainFrame->RefreshObjectPanels();
	}
}

void COccResurfView::ApplySelectionRectangle(Handle(AIS_InteractiveContext) ctx, const CPoint& point)
{
	if (ctx.IsNull() || myView.IsNull())
	{
		return;
	}

	const Graphic3d_Vec2i aMin(std::min(m_mouseDownPoint.x, point.x), std::min(m_mouseDownPoint.y, point.y));
	const Graphic3d_Vec2i aMax(std::max(m_mouseDownPoint.x, point.x), std::max(m_mouseDownPoint.y, point.y));
	ClearSelectionHighlights();
	ctx->SelectRectangle(aMin, aMax, myView, AIS_SelectionScheme_Replace);
	ctx->UpdateSelected(Standard_False);
	UpdateSelectionHighlights();
	UpdateViewer();
}

void COccResurfView::ApplyCircleSelection(Handle(AIS_InteractiveContext) ctx, const CPoint& point)
{
	if (ctx.IsNull() || myView.IsNull())
	{
		return;
	}

	const double aRadius = std::max(8.0, std::sqrt(static_cast<double>(
		(point.x - m_mouseDownPoint.x) * (point.x - m_mouseDownPoint.x) +
		(point.y - m_mouseDownPoint.y) * (point.y - m_mouseDownPoint.y))));
	const int aSegments = 24;
	TColgp_Array1OfPnt2d aPolyline(1, aSegments);
	for (int i = 0; i < aSegments; ++i)
	{
		const double aAngle = 2.0 * 3.14159265358979323846 * static_cast<double>(i) / static_cast<double>(aSegments);
		aPolyline.SetValue(i + 1, gp_Pnt2d(
			m_mouseDownPoint.x + aRadius * std::cos(aAngle),
			m_mouseDownPoint.y + aRadius * std::sin(aAngle)));
	}

	ClearSelectionHighlights();
	ctx->SelectPolygon(aPolyline, myView, AIS_SelectionScheme_Replace);
	ctx->UpdateSelected(Standard_False);
	UpdateSelectionHighlights();
	UpdateViewer();
}

void COccResurfView::ApplyBoxZoom(const CPoint& point)
{
	if (myView.IsNull())
	{
		return;
	}

	if (std::abs(point.x - m_mouseDownPoint.x) > kClickThreshold
		&& std::abs(point.y - m_mouseDownPoint.y) > kClickThreshold)
	{
		myView->WindowFit(
			std::min(m_mouseDownPoint.x, point.x),
			std::min(m_mouseDownPoint.y, point.y),
			std::max(m_mouseDownPoint.x, point.x),
			std::max(m_mouseDownPoint.y, point.y));
		UpdateViewer();
	}
}

void COccResurfView::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetFocus();
	m_mouseDownPoint = point;
	m_lastMousePoint = point;
	m_bHasMouseMoved = false;
	m_overlayLastPoint = point;

	if (m_currentMode >= MODE_TRANS_MOVE
		&& m_currentMode <= MODE_TRANS_SCALE
		&& !GetSelectedShape(GetDocument()->GetAISContext()).IsNull())
	{
		m_bIsTransforming = true;
		SetCapture();
		CView::OnLButtonDown(nFlags, point);
		return;
	}

	if (m_navigationMode == NAV_SELECT_BOX
		|| m_navigationMode == NAV_SELECT_CIRCLE
		|| m_navigationMode == NAV_VIEW_BOX_ZOOM)
	{
		SetCapture();
		CView::OnLButtonDown(nFlags, point);
		return;
	}

	CView::OnLButtonDown(nFlags, point);
}

void COccResurfView::OnMButtonDown(UINT nFlags, CPoint point)
{
	SetFocus();
	m_mouseDownPoint = point;
	m_lastMousePoint = point;
	m_bHasMouseMoved = false;

	if (!myView.IsNull())
	{
		m_navigationMode = (nFlags & MK_SHIFT) ? NAV_VIEW_PAN : NAV_VIEW_ROTATE;
		m_bIsRotating = true;
		if (m_navigationMode == NAV_VIEW_ROTATE)
		{
			myView->StartRotation(point.x, point.y);
		}
		else
		{
			myView->Pan(0, 0, 1.0, Standard_True);
		}
		SetCapture();
	}

	CView::OnMButtonDown(nFlags, point);
}

void COccResurfView::OnMouseMove(UINT nFlags, CPoint point)
{
	Handle(AIS_InteractiveContext) ctx = GetDocument()->GetAISContext();
	if (m_bIsTransforming)
	{
		const CPoint delta(point.x - m_lastMousePoint.x, point.y - m_lastMousePoint.y);
		if (delta.x != 0 || delta.y != 0)
		{
			m_bHasMouseMoved = true;
			ApplyMouseTransform(ctx, delta);
			m_lastMousePoint = point;
		}
		CView::OnMouseMove(nFlags, point);
		return;
	}

	if ((m_navigationMode == NAV_SELECT_BOX
		|| m_navigationMode == NAV_SELECT_CIRCLE
		|| m_navigationMode == NAV_VIEW_BOX_ZOOM)
		&& (GetCapture() == this))
	{
		if (std::abs(point.x - m_mouseDownPoint.x) > kClickThreshold
			|| std::abs(point.y - m_mouseDownPoint.y) > kClickThreshold)
		{
			m_bHasMouseMoved = true;
			DrawDragRectangle(point);
		}
		CView::OnMouseMove(nFlags, point);
		return;
	}

	if (m_bIsRotating && !myView.IsNull())
	{
		if (std::abs(point.x - m_mouseDownPoint.x) > kClickThreshold
			|| std::abs(point.y - m_mouseDownPoint.y) > kClickThreshold)
		{
			m_bHasMouseMoved = true;
			if (m_navigationMode == NAV_VIEW_ROTATE)
			{
				myView->Rotation(point.x, point.y);
				UpdateViewer();
			}
			else if (m_navigationMode == NAV_VIEW_PAN)
			{
				myView->Pan(point.x - m_mouseDownPoint.x, point.y - m_mouseDownPoint.y, 1.0, Standard_False);
				UpdateViewer();
			}
			m_lastMousePoint = point;
		}
	}
	CView::OnMouseMove(nFlags, point);
}

void COccResurfView::OnLButtonUp(UINT nFlags, CPoint point)
{
	const bool wasTransforming = m_bIsTransforming;
	m_bIsTransforming = false;
	ClearDragRectangle();

	if (GetCapture() == this)
	{
		ReleaseCapture();
	}

	if (wasTransforming)
	{
		CommitTransformMode();
	}
	else if (m_navigationMode == NAV_SELECT_BOX)
	{
		if (m_bHasMouseMoved)
		{
			ApplySelectionRectangle(GetDocument()->GetAISContext(), point);
			ResetNavigationMode();
			CView::OnLButtonUp(nFlags, point);
			return;
		}
		ResetNavigationMode();
		HandleObjectClick(nFlags, point);
		CView::OnLButtonUp(nFlags, point);
		return;
	}
	else if (m_navigationMode == NAV_SELECT_CIRCLE)
	{
		if (m_bHasMouseMoved)
		{
			ApplyCircleSelection(GetDocument()->GetAISContext(), point);
			ResetNavigationMode();
			CView::OnLButtonUp(nFlags, point);
			return;
		}
		ResetNavigationMode();
		HandleObjectClick(nFlags, point);
		CView::OnLButtonUp(nFlags, point);
		return;
	}
	else if (m_navigationMode == NAV_VIEW_BOX_ZOOM)
	{
		ApplyBoxZoom(point);
		ResetNavigationMode();
	}
	else if (m_isEditMode)
	{
		if (!m_bHasMouseMoved)
		{
			HandleEditClick(nFlags, point);
		}
	}
	else if (!m_isEditMode)
	{
		if (!m_bHasMouseMoved)
		{
			HandleObjectClick(nFlags, point);
		}
	}

	CView::OnLButtonUp(nFlags, point);
}

void COccResurfView::OnMButtonUp(UINT nFlags, CPoint point)
{
	(void)nFlags;
	(void)point;
	m_bIsRotating = false;
	ClearDragRectangle();
	if (GetCapture() == this)
	{
		ReleaseCapture();
	}
	ResetNavigationMode();
	CView::OnMButtonUp(nFlags, point);
}

BOOL COccResurfView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	(void)pt;
	if (!myView.IsNull())
	{
		const Standard_Real aScale = (nFlags & MK_CONTROL) ? 1.3 : 1.15;
		const Handle(Graphic3d_Camera)& aCamera = myView->Camera();
		if (!aCamera.IsNull())
		{
			const Standard_Real aCurrentScale = aCamera->Scale();
			aCamera->SetScale(zDelta > 0 ? aCurrentScale / aScale : aCurrentScale * aScale);
		}
		UpdateViewer();
	}
	return TRUE;
}

#ifdef _DEBUG
void COccResurfView::AssertValid() const { CView::AssertValid(); }
void COccResurfView::Dump(CDumpContext& dc) const { CView::Dump(dc); }

COccResurfDoc* COccResurfView::GetDocument() const
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(COccResurfDoc)));
	return (COccResurfDoc*)m_pDocument;
}
#endif

void COccResurfView::OnInitialUpdate()
{
	CView::OnInitialUpdate();

	myView = GetDocument()->GetViewer()->CreateView();
	myView->SetShadingModel(V3d_GOURAUD);
	Handle(WNT_Window) aWntWindow = new WNT_Window(GetSafeHwnd());
	myView->SetWindow(aWntWindow);
	if (!aWntWindow->IsMapped()) aWntWindow->Map();

	Standard_Integer w = 100, h = 100;
	aWntWindow->Size(w, h);
	::PostMessage(GetSafeHwnd(), WM_SIZE, SIZE_RESTORED, w + h * 65536);

	myView->ZBufferTriedronSetup(Quantity_NOC_RED, Quantity_NOC_GREEN, Quantity_NOC_BLUE1, 0.8, 0.05, 12);
	myView->TriedronDisplay(Aspect_TOTP_LEFT_LOWER, Quantity_NOC_WHITE, 0.2, V3d_ZBUFFER);

	GetDocument()->GetAISContext()->SetAutomaticHilight(Standard_False);
	GetDocument()->GetAISContext()->SetPixelTolerance(1);
	const Handle(Prs3d_Drawer)& selectionStyle = GetDocument()->GetAISContext()->SelectionStyle();
	selectionStyle->SetOwnLineAspects();
	selectionStyle->LineAspect()->SetWidth(5.0);
	selectionStyle->WireAspect()->SetWidth(5.0);
	selectionStyle->SeenLineAspect()->SetWidth(5.0);
	selectionStyle->FaceBoundaryAspect()->SetWidth(5.0);
	GetDocument()->DrawSphere(6.0);
	FitAll();
	CMainFrame* mainFrame = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
	if (mainFrame != nullptr)
	{
		mainFrame->RefreshObjectPanels();
	}
}
