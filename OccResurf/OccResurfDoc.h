
// OccResurfDoc.h: COccResurfDoc 类的接口
//


#pragma once

#include <vector>

#include <TDF_Label.hxx>
#include <TDocStd_Document.hxx>

class COccResurfDoc : public CDocument
{
protected: // 仅从序列化创建
	COccResurfDoc() noexcept;
	DECLARE_DYNCREATE(COccResurfDoc)

// 特性
public:

// 操作
public:
	enum class PrimitiveKind
	{
		Unknown = 0,
		Box,
		Sphere,
		Cylinder
	};

	struct PrimitiveParameter
	{
		CString name;
		double value;
	};

	struct PrimitiveObject
	{
		int id;
		PrimitiveKind kind;
		CString name;
		Handle(AIS_Shape) aisShape;
		TDF_Label label;
		std::vector<PrimitiveParameter> parameters;
	};

	struct PrimitiveObjectState
	{
		int id;
		PrimitiveKind kind;
		CString name;
		std::vector<PrimitiveParameter> parameters;
		gp_Trsf localTransformation;
	};

	Handle(AIS_InteractiveContext) myAISContext;
	Handle(V3d_Viewer) myViewer;
	Handle(TDocStd_Document) GetOcafDocument() const { return myOcafDocument; }
	Handle(V3d_Viewer) GetViewer(void) const { return myViewer; }
	Handle(AIS_InteractiveContext) GetAISContext(void) const { return myAISContext; }
	const std::vector<Handle(AIS_Shape)>& GetModelShapes() const { return myModelShapes; }
	std::vector<Handle(AIS_Shape)>& AccessModelShapes() { return myModelShapes; }
	const std::vector<PrimitiveObject>& GetDocumentModelObjects() const { return myPrimitiveObjects; }
	bool HasModelShapes() const { return !myModelShapes.empty(); }
	const gp_Pnt& GetModelCenter() const { return myModelCenter; }
	void UpdateModelCenter();
	void SetModelBuilt(bool theBuilt) { m_bModelBuilt = theBuilt; }
	PrimitiveObject* FindObjectById(int id);
	const PrimitiveObject* FindObjectById(int id) const;
	PrimitiveObject* FindObjectByShape(const Handle(AIS_Shape)& shape);
	int AddBoxObject(double length, double width, double height);
	int AddSphereObject(double radius);
	int AddCylinderObject(double radius, double height);
	bool UpdateObjectParameter(int objectId, const CString& parameterName, double value);
	bool DeleteObject(int objectId);
	int DeleteSelectedModelObjects();
	void ClearPrimitiveObjects();
	void ClearDocumentModel();
	std::vector<PrimitiveObjectState> CapturePrimitiveObjectStates() const;
	void RestorePrimitiveObjectStates(const std::vector<PrimitiveObjectState>& states);
	void RebuildPrimitiveObjectsAfterLoad();
	void SerializePrimitiveObjects(CArchive& ar);
	bool IsPrimitiveShape(const Handle(AIS_Shape)& shape) const;

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
	TopoDS_Shape BuildPrimitiveShape(const PrimitiveObject& object) const;
	void InitializeOcafDocument();
	void StoreObjectInOcaf(PrimitiveObject& object);
	void RebuildObjectShape(PrimitiveObject& object);
	CString MakePrimitiveName(PrimitiveKind kind) const;
	CString PrimitiveKindToString(PrimitiveKind kind) const;

	std::vector<Handle(AIS_Shape)> myModelShapes;
	std::vector<PrimitiveObject> myPrimitiveObjects;
	Handle(TDocStd_Document) myOcafDocument;
	gp_Pnt myModelCenter;
	bool m_bModelBuilt;
	int m_nextObjectId;
};
