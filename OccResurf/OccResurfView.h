// OccResurfView.h: COccResurfView 类的接口
//

#pragma once

#include <vector>
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
	void RefreshSelectionHighlights();
	bool UndoLastAction();
	bool RedoLastAction();
	bool CanUndo() const { return !m_undoSnapshots.empty(); }
	bool CanRedo() const { return !m_redoSnapshots.empty(); }
	void RecordUndoSnapshot();

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

	enum NavigationMode
	{
		NAV_NONE = 0,
		NAV_VIEW_ROTATE = 1,
		NAV_VIEW_PAN = 2,
		NAV_VIEW_BOX_ZOOM = 3,
		NAV_SELECT_BOX = 4,
		NAV_SELECT_CIRCLE = 5
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

	struct ShapeSnapshot
	{
		TopoDS_Shape shape;
		Quantity_Color color;
		gp_Trsf localTransform;
		TransformState transformState;
	};

	struct SceneSnapshot
	{
		std::vector<ShapeSnapshot> shapes;
		std::vector<COccResurfDoc::PrimitiveObjectState> primitiveObjects;
	};

	void ApplyTransform(Handle(AIS_InteractiveContext) ctx, const gp_Trsf& theTransform);
	void ApplyTransformToSelected(Handle(AIS_InteractiveContext) ctx, const gp_Trsf& trsf);
	void ResetTransform(Handle(AIS_InteractiveContext) ctx, int type);
	void UpdateSelectedTransforms(Handle(AIS_InteractiveContext) ctx);
	void ApplyTransformPermanently(Handle(AIS_InteractiveContext) ctx);
	void ApplyMouseTransform(Handle(AIS_InteractiveContext) ctx, const CPoint& delta);
	void ExitTransformMode();
	void ResetNavigationMode();
	void SelectModelAtPoint(const CPoint& point, const AIS_SelectionScheme theScheme = AIS_SelectionScheme_Replace);
	void SelectAllDisplayedObjects(Handle(AIS_InteractiveContext) ctx);
	void InvertDisplayedSelection(Handle(AIS_InteractiveContext) ctx);
	void HideUnselectedObjects(Handle(AIS_InteractiveContext) ctx);
	void DuplicateSelectedObjects(Handle(AIS_InteractiveContext) ctx);
	void DeleteSelectedObjects(Handle(AIS_InteractiveContext) ctx);
	void ApplySelectionRectangle(Handle(AIS_InteractiveContext) ctx, const CPoint& point);
	void ApplyCircleSelection(Handle(AIS_InteractiveContext) ctx, const CPoint& point);
	void ApplyBoxZoom(const CPoint& point);
	bool HandleObjectClick(UINT nFlags, const CPoint& point);
	bool HandleEditClick(UINT nFlags, const CPoint& point);
	void UpdateViewer();
	SceneSnapshot CaptureScene() const;
	void RestoreScene(const SceneSnapshot& snapshot);
	void SaveProjectToFile(const CString& filePath);
	void OpenProjectFromFile(const CString& filePath);
	void ClearScene();
	void DrawDragRectangle(const CPoint& currentPoint);
	void ClearDragRectangle();
	void CommitTransformMode();
	void ClearAllSelection();
	void ClearSelectionHighlights();
	void UpdateSelectionHighlights();
	bool GetSingleSelectedSubShape(TopoDS_Shape& subShape, Handle(AIS_Shape)& parentShape) const;
	void CollectSelectedSubShapes(std::vector<TopoDS_Shape>& subShapes, std::vector<Handle(AIS_Shape)>& parentShapes) const;
	void ToggleRightSidebar();
	void ToggleLeftToolbar();
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
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);

	int m_currentMode;
	int m_navigationMode;
	bool m_isEditMode;
	Standard_Integer m_selectMode;
	int m_lockedAxis;
	bool m_isOrthoView;
	bool m_isWireframeOnly;
	int m_shadeMode;
	bool m_bHasMouseMoved;
	bool m_hasSelectionAnchor;
	bool m_hasDragOverlay;
	CPoint m_lastMousePoint;
	CPoint m_mouseDownPoint;
	CPoint m_overlayLastPoint;
	std::vector<Handle(AIS_Shape)> m_selectionHighlights;
	std::unordered_map<const AIS_InteractiveObject*, TransformState> m_transformStates;
	std::vector<SceneSnapshot> m_undoSnapshots;
	std::vector<SceneSnapshot> m_redoSnapshots;

	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG
inline COccResurfDoc* COccResurfView::GetDocument() const
{
	return reinterpret_cast<COccResurfDoc*>(m_pDocument);
}
#endif
