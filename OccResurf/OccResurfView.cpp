// OccResurfView.cpp
#include "pch.h"
#include "framework.h"
#ifndef SHARED_HANDLERS
#include "OccResurf.h"
#endif

#include "OccResurfDoc.h"
#include "OccResurfView.h"

#ifdef _DEBUG
//#define new DEBUG_NEW
#endif

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

	myView->MustBeResized();
	myView->Update();
	pDoc->DrawSphere(6);
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

// ====================== 纯旋转代码（100%兼容，零报错）======================
void COccResurfView::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (!myView.IsNull())
	{
		myView->StartRotation(point.x, point.y);
		m_bIsRotating = true;
	}
	CView::OnLButtonDown(nFlags, point);
}

void COccResurfView::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_bIsRotating && !myView.IsNull())
	{
		myView->Rotation(point.x, point.y);
		myView->Update();
	}
	CView::OnMouseMove(nFlags, point);
}

void COccResurfView::OnLButtonUp(UINT nFlags, CPoint point)
{
	m_bIsRotating = false;
	CView::OnLButtonUp(nFlags, point);
}
// ==========================================================================

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
	myView->FitAll();

	myView->ZBufferTriedronSetup(Quantity_NOC_RED, Quantity_NOC_GREEN, Quantity_NOC_BLUE1, 0.8, 0.05, 12);
	myView->TriedronDisplay(Aspect_TOTP_LEFT_LOWER, Quantity_NOC_WHITE, 0.2, V3d_ZBUFFER);
}