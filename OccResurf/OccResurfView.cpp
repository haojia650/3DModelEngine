// OccResurfView.cpp
#include "pch.h"
#include "framework.h"
#ifndef SHARED_HANDLERS
#include "OccResurf.h"
#endif

#include "OccResurfDoc.h"
#include "OccResurfView.h"

#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <cmath>

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
constexpr int kClickThreshold = 3;
}

IMPLEMENT_DYNCREATE(COccResurfView, CView)

BEGIN_MESSAGE_MAP(COccResurfView, CView)
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &COccResurfView::OnFilePrintPreview)
	ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONUP()
	// 仅绑定旋转鼠标消息
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()

COccResurfView::COccResurfView() noexcept
{
	m_bIsRotating = false;
	m_bIsTransforming = false;
	m_currentMode = MODE_NONE;
	m_lockedAxis = AXIS_NONE;
	m_bHasMouseMoved = false;
	m_lastMousePoint = CPoint(0, 0);
	m_mouseDownPoint = CPoint(0, 0);
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
		const bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
		const bool alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
		Handle(AIS_InteractiveContext) ctx = GetDocument()->GetAISContext();

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
			ExitTransformMode();
			return TRUE;
		}
		if (key == VK_ESCAPE)
		{
			ExitTransformMode();
			return TRUE;
		}
		if (key == 'G')
		{
			m_currentMode = MODE_TRANS_MOVE;
			m_lockedAxis = AXIS_NONE;
			return TRUE;
		}
		if (key == 'R')
		{
			m_currentMode = MODE_TRANS_ROTATE;
			m_lockedAxis = AXIS_NONE;
			return TRUE;
		}
		if (key == 'S')
		{
			m_currentMode = MODE_TRANS_SCALE;
			m_lockedAxis = AXIS_NONE;
			return TRUE;
		}
		if ((key == 'X' || key == 'Y' || key == 'Z')
			&& m_currentMode >= MODE_TRANS_MOVE
			&& m_currentMode <= MODE_TRANS_SCALE)
		{
			int nextAxis = AXIS_NONE;
			if (key == 'X') nextAxis = AXIS_X;
			if (key == 'Y') nextAxis = AXIS_Y;
			if (key == 'Z') nextAxis = AXIS_Z;
			m_lockedAxis = (m_lockedAxis == nextAxis) ? AXIS_NONE : nextAxis;
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

	ctx->UpdateCurrentViewer();
	if (!myView.IsNull())
	{
		myView->Update();
	}
}

void COccResurfView::ResetTransform(Handle(AIS_InteractiveContext) ctx, int type)
{
	Handle(AIS_Shape) aShape = GetSelectedShape(ctx);
	if (aShape.IsNull())
	{
		return;
	}

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

	const gp_Trsf aTransform = BuildShapeTransform(aShape);
	const TopoDS_Shape aBakedShape = BRepBuilderAPI_Transform(aShape->Shape(), aTransform, Standard_True);
	aShape->SetShape(aBakedShape);
	aShape->ResetTransformation();
	ctx->Redisplay(aShape, Standard_False);
	GetTransformState(aShape) = TransformState();
	ctx->UpdateCurrentViewer();
	if (!myView.IsNull())
	{
		myView->Update();
	}
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

void COccResurfView::SelectModelAtPoint(const CPoint& point)
{
	Handle(AIS_InteractiveContext) ctx = GetDocument()->GetAISContext();
	if (ctx.IsNull() || myView.IsNull() || !HasTransformableModel())
	{
		return;
	}

	ctx->MoveTo(point.x, point.y, myView, Standard_False);
	ctx->SelectDetected(AIS_SelectionScheme_Replace);

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
		ctx->ClearSelected(Standard_True);
		return;
	}

	ctx->UpdateSelected(Standard_True);
}

void COccResurfView::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetFocus();
	m_mouseDownPoint = point;
	m_lastMousePoint = point;
	m_bHasMouseMoved = false;

	if (m_currentMode >= MODE_TRANS_MOVE
		&& m_currentMode <= MODE_TRANS_SCALE
		&& !GetSelectedShape(GetDocument()->GetAISContext()).IsNull())
	{
		m_bIsTransforming = true;
		SetCapture();
		CView::OnLButtonDown(nFlags, point);
		return;
	}

	if (!myView.IsNull())
	{
		myView->StartRotation(point.x, point.y);
		m_bIsRotating = true;
		SetCapture();
	}

	CView::OnLButtonDown(nFlags, point);
}

void COccResurfView::OnMouseMove(UINT nFlags, CPoint point)
{
	Handle(AIS_InteractiveContext) ctx = GetDocument()->GetAISContext();
	if (!ctx.IsNull() && !myView.IsNull() && !m_bIsRotating && !m_bIsTransforming)
	{
		ctx->MoveTo(point.x, point.y, myView, Standard_True);
	}

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

	if (m_bIsRotating && !myView.IsNull())
	{
		if (std::abs(point.x - m_mouseDownPoint.x) > kClickThreshold
			|| std::abs(point.y - m_mouseDownPoint.y) > kClickThreshold)
		{
			m_bHasMouseMoved = true;
			myView->Rotation(point.x, point.y);
			myView->Update();
		}
	}
	CView::OnMouseMove(nFlags, point);
}

void COccResurfView::OnLButtonUp(UINT nFlags, CPoint point)
{
	const bool wasRotating = m_bIsRotating;
	const bool wasTransforming = m_bIsTransforming;
	m_bIsRotating = false;
	m_bIsTransforming = false;

	if (GetCapture() == this)
	{
		ReleaseCapture();
	}

	if (wasRotating && !m_bHasMouseMoved)
	{
		SelectModelAtPoint(point);
	}
	else if (wasTransforming)
	{
		ExitTransformMode();
	}

	CView::OnLButtonUp(nFlags, point);
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

	GetDocument()->DrawSphere(6.0);
	FitAll();
}
