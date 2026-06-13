# MFC Command And Document Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a first MFC-native command line, model object registry, object tree, and parameter panel to the existing OccResurf OCCT app.

**Architecture:** Keep the current MFC SDI app and OCCT viewer. Store editable primitives in `COccResurfDoc`, mirror their metadata into an OCAF `TDocStd_Document`, and let `MainFrm` coordinate dock-pane refreshes.

**Tech Stack:** C++17, MFC, OCCT AIS/BRep/OCAF, Visual Studio MSBuild.

---

### Task 1: Regression Harness

**Files:**
- Create: `C:/Users/Lenovo/Documents/Codex/2026-06-10/d-zuoye-vs-shift-shift-a/work/command_document_regression_check.ps1`

- [x] Create a static regression script that checks for the new command widget, document object APIs, OCAF attributes, tree refresh, property refresh, and project-file entries.
- [x] Run it before implementation and verify it fails because `CommandEdit.h` is missing.

### Task 2: Command Input Widget

**Files:**
- Create: `D:/ZUOYE/新建文件夹/OccResurf/CommandEdit.h`
- Create: `D:/ZUOYE/新建文件夹/OccResurf/CommandEdit.cpp`
- Modify: `D:/ZUOYE/新建文件夹/OccResurf/OccResurf.vcxproj`
- Modify: `D:/ZUOYE/新建文件夹/OccResurf/OccResurf.vcxproj.filters`

- [ ] Implement a `CCommandEdit` subclass of `CEdit`.
- [ ] Support Enter dispatch via `WM_OCC_COMMAND_ENTERED`.
- [ ] Support Up/Down history recall and Tab completion for `box`, `sphere`, `cylinder`, `move`, `rotate`, `delete`, `undo`.
- [ ] Keep history capped at 50 entries.

### Task 3: Document Object Registry

**Files:**
- Modify: `D:/ZUOYE/新建文件夹/OccResurf/OccResurfDoc.h`
- Modify: `D:/ZUOYE/新建文件夹/OccResurf/OccResurfDoc.cpp`

- [ ] Add `PrimitiveKind`, `PrimitiveObject`, and `PrimitiveParameter`.
- [ ] Add OCAF document creation.
- [ ] Implement `AddBoxObject`, `AddSphereObject`, `AddCylinderObject`, `DeleteSelectedModelObjects`, and `UpdateObjectParameter`.
- [ ] Store name, shape, and real parameters in OCAF labels using `TDataStd_Name`, `TNaming_Builder`, and `TDataStd_Real`.

### Task 4: Dock Pane Integration

**Files:**
- Modify: `D:/ZUOYE/新建文件夹/OccResurf/OutputWnd.h`
- Modify: `D:/ZUOYE/新建文件夹/OccResurf/OutputWnd.cpp`
- Modify: `D:/ZUOYE/新建文件夹/OccResurf/FileView.h`
- Modify: `D:/ZUOYE/新建文件夹/OccResurf/FileView.cpp`
- Modify: `D:/ZUOYE/新建文件夹/OccResurf/PropertiesWnd.h`
- Modify: `D:/ZUOYE/新建文件夹/OccResurf/PropertiesWnd.cpp`
- Modify: `D:/ZUOYE/新建文件夹/OccResurf/MainFrm.h`
- Modify: `D:/ZUOYE/新建文件夹/OccResurf/MainFrm.cpp`

- [ ] Replace fake file-tree content with model objects.
- [ ] Add a command tab with output history and a command edit line.
- [ ] Dispatch `box`, `sphere`, `cylinder`, `delete`, and `undo` commands to the document/view.
- [ ] Display selected object parameters in the existing MFC property grid.
- [ ] On property changes, update the document object and redisplay the 3D model.

### Task 5: Verification

**Files:**
- Read: `D:/ZUOYE/新建文件夹/OccResurf.sln`

- [ ] Run `work/command_document_regression_check.ps1` and expect pass.
- [ ] Run `work/selection_regression_check.ps1` and expect pass.
- [ ] Build `Debug|x64` with MSBuild and expect exit code 0.
