// OccResurfView.h: COccResurfView 类的接口
//

#pragma once

class COccResurfView : public CView
{
protected: // 仅从序列化创建
	COccResurfView() noexcept;
	DECLARE_DYNCREATE(COccResurfView)

	// 特性
public:
	COccResurfDoc* GetDocument() const;
	Handle(V3d_View) myView;
	bool m_bIsRotating;

	// 操作
public:
	virtual void OnDraw(CDC* pDC);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnInitialUpdate();

protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

	// 实现
public:
	virtual ~COccResurfView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	void FitAll() { if (!myView.IsNull()) { myView->FitAll(); myView->ZFitAll(); } };

protected:
	afx_msg void OnFilePrintPreview();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);

	// 仅保留旋转声明（无任何报错API）
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);

	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG
inline COccResurfDoc* COccResurfView::GetDocument() const
{
	return reinterpret_cast<COccResurfDoc*>(m_pDocument);
}
#endif