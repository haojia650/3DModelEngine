// OccResurfView.h: COccResurfView 类的接口
//

#pragma once

#include <unordered_map>

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
	bool m_bIsTransforming;

	// 操作
public:
	virtual void OnDraw(CDC* pDC);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
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
	enum TransformMode
	{
		MODE_NONE = 0,
		MODE_TRANS_MOVE = 1,
		MODE_TRANS_ROTATE = 2,
		MODE_TRANS_SCALE = 3
	};

	enum TransformAxis
	{
		AXIS_NONE = -1,
		AXIS_X = 0,
		AXIS_Y = 1,
		AXIS_Z = 2
	};

	struct TransformState
	{
		gp_Vec translation;
		Standard_Real rotationX;
		Standard_Real rotationY;
		Standard_Real rotationZ;
		Standard_Real uniformScale;

		TransformState()
			: translation(0.0, 0.0, 0.0),
			rotationX(0.0),
			rotationY(0.0),
			rotationZ(0.0),
			uniformScale(1.0)
		{
		}
	};

	void ApplyTransform(Handle(AIS_InteractiveContext) ctx, const gp_Trsf& theTransform);
	void ResetTransform(Handle(AIS_InteractiveContext) ctx, int type);
	void UpdateSelectedTransforms(Handle(AIS_InteractiveContext) ctx);
	void ApplyTransformPermanently(Handle(AIS_InteractiveContext) ctx);
	void ApplyMouseTransform(Handle(AIS_InteractiveContext) ctx, const CPoint& delta);
	void ExitTransformMode();
	void SelectModelAtPoint(const CPoint& point);
	gp_Trsf BuildShapeTransform(const Handle(AIS_Shape)& theShape) const;
	gp_Pnt GetShapeCenter(const Handle(AIS_Shape)& theShape) const;
	Handle(AIS_Shape) GetSelectedShape(Handle(AIS_InteractiveContext) ctx) const;
	TransformState& GetTransformState(const Handle(AIS_Shape)& theShape);
	bool HasTransformableModel() const;
	bool IsModelShape(const Handle(AIS_InteractiveObject)& theObject) const;

	afx_msg void OnFilePrintPreview();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);

	// 鼠标交互：旋转视图 + 变换模型
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);

	int m_currentMode;
	int m_lockedAxis;
	bool m_bHasMouseMoved;
	CPoint m_lastMousePoint;
	CPoint m_mouseDownPoint;
	std::unordered_map<const AIS_InteractiveObject*, TransformState> m_transformStates;

	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG
inline COccResurfDoc* COccResurfView::GetDocument() const
{
	return reinterpret_cast<COccResurfDoc*>(m_pDocument);
}
#endif
