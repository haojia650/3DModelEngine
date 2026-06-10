
// OccResurfDoc.h: COccResurfDoc 类的接口
//


#pragma once

#include <vector>

class COccResurfDoc : public CDocument
{
protected: // 仅从序列化创建
	COccResurfDoc() noexcept;
	DECLARE_DYNCREATE(COccResurfDoc)

// 特性
public:

// 操作
public:

	Handle(AIS_InteractiveContext) myAISContext;
	Handle(V3d_Viewer) myViewer;
	Handle(V3d_Viewer) GetViewer(void) const { return myViewer; }
	Handle(AIS_InteractiveContext) GetAISContext(void) const { return myAISContext; }
	const std::vector<Handle(AIS_Shape)>& GetModelShapes() const { return myModelShapes; }
	std::vector<Handle(AIS_Shape)>& AccessModelShapes() { return myModelShapes; }
	bool HasModelShapes() const { return !myModelShapes.empty(); }
	const gp_Pnt& GetModelCenter() const { return myModelCenter; }
	void UpdateModelCenter();
	void SetModelBuilt(bool theBuilt) { m_bModelBuilt = theBuilt; }

// 重写
public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
#ifdef SHARED_HANDLERS
	virtual void InitializeSearchContent();
	virtual void OnDrawThumbnail(CDC& dc, LPRECT lprcBounds);
#endif // SHARED_HANDLERS

// 实现
public:
	virtual ~COccResurfDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	void DrawSphere(double Radius);

protected:

// 生成的消息映射函数
protected:
	DECLARE_MESSAGE_MAP()

#ifdef SHARED_HANDLERS
	// 用于为搜索处理程序设置搜索内容的 Helper 函数
	void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS

private:
	std::vector<Handle(AIS_Shape)> myModelShapes;
	gp_Pnt myModelCenter;
	bool m_bModelBuilt;
};
