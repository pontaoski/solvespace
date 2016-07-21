//-----------------------------------------------------------------------------
// All declarations not grouped specially elsewhere.
//
// Copyright 2008-2013 Jonathan Westhues.
//-----------------------------------------------------------------------------

#ifndef __SOLVESPACE_H
#define __SOLVESPACE_H

#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <math.h>
#include <limits.h>
#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <locale>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include <chrono>

// We declare these in advance instead of simply using FT_Library
// (defined as typedef FT_LibraryRec_* FT_Library) because including
// freetype.h invokes indescribable horrors and we would like to avoid
// doing that every time we include solvespace.h.
struct FT_LibraryRec_;
struct FT_FaceRec_;

// The few floating-point equality comparisons in SolveSpace have been
// carefully considered, so we disable the -Wfloat-equal warning for them
#ifdef __clang__
#   define EXACT(expr) \
        (_Pragma("clang diagnostic push") \
         _Pragma("clang diagnostic ignored \"-Wfloat-equal\"") \
         (expr) \
         _Pragma("clang diagnostic pop"))
#else
#   define EXACT(expr) (expr)
#endif

// Debugging functions
#if defined(__GNUC__)
#define ssassert(condition, message) \
    do { \
        if(__builtin_expect((condition), true) == false) { \
            SolveSpace::assert_failure(__FILE__, __LINE__, __func__, #condition, message); \
            __builtin_unreachable(); \
        } \
    } while(0)
#else
#define ssassert(condition, message) \
    do { \
        if((condition) == false) { \
            SolveSpace::assert_failure(__FILE__, __LINE__, __func__, #condition, message); \
            abort(); \
        } \
    } while(0)
#endif

#ifndef isnan
#   define isnan(x) (((x) != (x)) || (x > 1e11) || (x < -1e11))
#endif

namespace SolveSpace {

using std::min;
using std::max;
using std::swap;

#if defined(__GNUC__)
__attribute__((noreturn))
#endif
void assert_failure(const char *file, unsigned line, const char *function,
                    const char *condition, const char *message);

#if defined(__GNUC__)
__attribute__((__format__ (__printf__, 1, 2)))
#endif
std::string ssprintf(const char *fmt, ...);

inline int WRAP(int v, int n) {
    // Clamp it to the range [0, n)
    while(v >= n) v -= n;
    while(v < 0) v += n;
    return v;
}
inline double WRAP_NOT_0(double v, double n) {
    // Clamp it to the range (0, n]
    while(v > n) v -= n;
    while(v <= 0) v += n;
    return v;
}
inline double WRAP_SYMMETRIC(double v, double n) {
    // Clamp it to the range (-n/2, n/2]
    while(v >   n/2) v -= n;
    while(v <= -n/2) v += n;
    return v;
}

// Why is this faster than the library function?
inline double ffabs(double v) { return (v > 0) ? v : (-v); }

#define CO(v) (v).x, (v).y, (v).z

#define ANGLE_COS_EPS   (1e-6)
#define LENGTH_EPS      (1e-6)
#define VERY_POSITIVE   (1e10)
#define VERY_NEGATIVE   (-1e10)

#define isforname(c) (isalnum(c) || (c) == '_' || (c) == '-' || (c) == '#')

#if defined(WIN32)
std::string Narrow(const wchar_t *s);
std::wstring Widen(const char *s);
std::string Narrow(const std::wstring &s);
std::wstring Widen(const std::string &s);
#endif

inline double Random(double vmax) {
    return (vmax*rand()) / RAND_MAX;
}

class Expr;
class ExprVector;
class ExprQuaternion;
class RgbaColor;
enum class Command : uint32_t;
enum class ContextCommand : uint32_t;

//================
// From the platform-specific code.
#if defined(WIN32)
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif

extern const bool FLIP_FRAMEBUFFER;

bool PathEqual(const std::string &a, const std::string &b);
FILE *ssfopen(const std::string &filename, const char *mode);
void ssremove(const std::string &filename);

const size_t MAX_RECENT = 8;
extern std::string RecentFile[MAX_RECENT];
void RefreshRecentMenus();

enum DialogChoice { DIALOG_YES = 1, DIALOG_NO = -1, DIALOG_CANCEL = 0 };
DialogChoice SaveFileYesNoCancel();
DialogChoice LoadAutosaveYesNo();
DialogChoice LocateImportedFileYesNoCancel(const std::string &filename,
                                           bool canCancel);

#define AUTOSAVE_SUFFIX "~"

struct FileFilter {
    const char *name;
    const char *patterns[3];
};

// SolveSpace native file format
const FileFilter SlvsFileFilter[] = {
    { "SolveSpace models",          { "slvs" } },
    { NULL, {} }
};
// PNG format bitmap
const FileFilter PngFileFilter[] = {
    { "PNG",                        { "png" } },
    { NULL, {} }
};
// Triangle mesh
const FileFilter MeshFileFilter[] = {
    { "STL mesh",                   { "stl" } },
    { "Wavefront OBJ mesh",         { "obj" } },
    { "Three.js-compatible mesh, with viewer",  { "html" } },
    { "Three.js-compatible mesh, mesh only",    { "js" } },
    { NULL, {} }
};
// NURBS surfaces
const FileFilter SurfaceFileFilter[] = {
    { "STEP file",                  { "step", "stp" } },
    { NULL, {} }
};
// 2d vector (lines and curves) format
const FileFilter VectorFileFilter[] = {
    { "PDF file",                   { "pdf" } },
    { "Encapsulated PostScript",    { "eps",  "ps" } },
    { "Scalable Vector Graphics",   { "svg" } },
    { "STEP file",                  { "step", "stp" } },
    { "DXF file (AutoCAD 2007)",    { "dxf" } },
    { "HPGL file",                  { "plt",  "hpgl" } },
    { "G Code",                     { "ngc",  "txt" } },
    { NULL, {} }
};
// 3d vector (wireframe lines and curves) format
const FileFilter Vector3dFileFilter[] = {
    { "STEP file",                  { "step", "stp" } },
    { "DXF file (AutoCAD 2007)",    { "dxf" } },
    { NULL, {} }
};
// All Importable formats
const FileFilter ImportableFileFilter[] = {
    { "AutoCAD DXF and DWG files",  { "dxf", "dwg" } },
    { NULL, {} }
};
// Comma-separated value, like a spreadsheet would use
const FileFilter CsvFileFilter[] = {
    { "CSV",                        { "csv" } },
    { NULL, {} }
};

enum class Unit : uint32_t {
    MM = 0,
    INCHES
};

bool GetSaveFile(std::string *filename, const std::string &defExtension,
                 const FileFilter filters[]);
bool GetOpenFile(std::string *filename, const std::string &defExtension,
                 const FileFilter filters[]);
std::vector<std::string> GetFontFiles();

void OpenWebsite(const char *url);

void CheckMenuByCmd(Command id, bool checked);
void RadioMenuByCmd(Command id, bool selected);
void EnableMenuByCmd(Command id, bool enabled);

void ShowGraphicsEditControl(int x, int y, int fontHeight, int minWidthChars,
                             const std::string &str);
void HideGraphicsEditControl();
bool GraphicsEditControlIsVisible();
void ShowTextEditControl(int x, int y, const std::string &str);
void HideTextEditControl();
bool TextEditControlIsVisible();
void MoveTextScrollbarTo(int pos, int maxPos, int page);

void AddContextMenuItem(const char *legend, ContextCommand id);
void CreateContextSubmenu();
ContextCommand ShowContextMenu();

void ToggleMenuBar();
bool MenuBarIsVisible();
void ShowTextWindow(bool visible);
void InvalidateText();
void InvalidateGraphics();
void PaintGraphics();
void ToggleFullScreen();
bool FullScreenIsActive();
void GetGraphicsWindowSize(int *w, int *h);
void GetTextWindowSize(int *w, int *h);
int64_t GetMilliseconds();

void dbp(const char *str, ...);
#define DBPTRI(tri) \
    dbp("tri: (%.3f %.3f %.3f) (%.3f %.3f %.3f) (%.3f %.3f %.3f)", \
        CO((tri).a), CO((tri).b), CO((tri).c))

void SetCurrentFilename(const std::string &filename);
void SetMousePointerToHand(bool yes);
void DoMessageBox(const char *str, int rows, int cols, bool error);
void SetTimerFor(int milliseconds);
void SetAutosaveTimerFor(int minutes);
void ScheduleLater();
void ExitNow();

void CnfFreezeInt(uint32_t val, const std::string &name);
void CnfFreezeFloat(float val, const std::string &name);
void CnfFreezeString(const std::string &val, const std::string &name);
std::string CnfThawString(const std::string &val, const std::string &name);
uint32_t CnfThawInt(uint32_t val, const std::string &name);
float CnfThawFloat(float val, const std::string &name);

void *AllocTemporary(size_t n);
void FreeTemporary(void *p);
void FreeAllTemporary();
void *MemAlloc(size_t n);
void MemFree(void *p);
void InitHeaps();
void vl(); // debug function to validate heaps

#include "resource.h"

// End of platform-specific functions
//================

template<class T>
struct CompareHandle {
    bool operator()(T lhs, T rhs) const { return lhs.v < rhs.v; }
};

template<class Key, class T>
using handle_map = std::map<Key, T, CompareHandle<Key>>;

class Group;
class SSurface;
#include "dsc.h"
#include "polygon.h"
#include "srf/surface.h"
#include "render/render.h"

class Entity;
class hEntity;
class Param;
class hParam;
typedef IdList<Entity,hEntity> EntityList;
typedef IdList<Param,hParam> ParamList;

enum class SolveResult : uint32_t {
    OKAY                     = 0,
    DIDNT_CONVERGE           = 10,
    REDUNDANT_OKAY           = 11,
    REDUNDANT_DIDNT_CONVERGE = 12,
    TOO_MANY_UNKNOWNS        = 20
};


#include "sketch.h"
#include "ui.h"
#include "expr.h"


// Utility functions that are provided in the platform-independent code.
class utf8_iterator : std::iterator<std::forward_iterator_tag, char32_t> {
    const char *p, *n;
public:
    utf8_iterator(const char *p) : p(p), n(NULL) {}
    bool           operator==(const utf8_iterator &i) const { return p==i.p; }
    bool           operator!=(const utf8_iterator &i) const { return p!=i.p; }
    ptrdiff_t      operator- (const utf8_iterator &i) const { return p -i.p; }
    utf8_iterator& operator++()    { **this; p=n; n=NULL; return *this; }
    utf8_iterator  operator++(int) { utf8_iterator t(*this); operator++(); return t; }
    char32_t       operator*();
};
class ReadUTF8 {
    const std::string &str;
public:
    ReadUTF8(const std::string &str) : str(str) {}
    utf8_iterator begin() const { return utf8_iterator(&str[0]); }
    utf8_iterator end()   const { return utf8_iterator(&str[str.length()]); }
};


#define arraylen(x) (sizeof((x))/sizeof((x)[0]))
#define PI (3.1415926535897931)
void MakeMatrix(double *mat, double a11, double a12, double a13, double a14,
                             double a21, double a22, double a23, double a24,
                             double a31, double a32, double a33, double a34,
                             double a41, double a42, double a43, double a44);
std::string MakeAcceleratorLabel(int accel);
bool FilenameHasExtension(const std::string &str, const char *ext);
void Message(const char *str, ...);
void Error(const char *str, ...);
void CnfFreezeBool(bool v, const std::string &name);
void CnfFreezeColor(RgbaColor v, const std::string &name);
bool CnfThawBool(bool v, const std::string &name);
RgbaColor CnfThawColor(RgbaColor v, const std::string &name);

class System {
public:
    enum { MAX_UNKNOWNS = 1024 };

    EntityList                      entity;
    ParamList                       param;
    IdList<Equation,hEquation>      eq;

    // A list of parameters that are being dragged; these are the ones that
    // we should put as close as possible to their initial positions.
    List<hParam>                    dragged;

    enum {
        // In general, the tag indicates the subsys that a variable/equation
        // has been assigned to; these are exceptions for variables:
        VAR_SUBSTITUTED      = 10000,
        VAR_DOF_TEST         = 10001,
        // and for equations:
        EQ_SUBSTITUTED       = 20000
    };

    // The system Jacobian matrix
    struct {
        // The corresponding equation for each row
        hEquation   eq[MAX_UNKNOWNS];

        // The corresponding parameter for each column
        hParam      param[MAX_UNKNOWNS];

        // We're solving AX = B
        int m, n;
        struct {
            Expr        *sym[MAX_UNKNOWNS][MAX_UNKNOWNS];
            double       num[MAX_UNKNOWNS][MAX_UNKNOWNS];
        }           A;

        double      scale[MAX_UNKNOWNS];

        // Some helpers for the least squares solve
        double AAt[MAX_UNKNOWNS][MAX_UNKNOWNS];
        double Z[MAX_UNKNOWNS];

        double      X[MAX_UNKNOWNS];

        struct {
            Expr        *sym[MAX_UNKNOWNS];
            double       num[MAX_UNKNOWNS];
        }           B;
    } mat;

    static const double RANK_MAG_TOLERANCE, CONVERGE_TOLERANCE;
    int CalculateRank();
    bool TestRank();
    static bool SolveLinearSystem(double X[], double A[][MAX_UNKNOWNS],
                                  double B[], int N);
    bool SolveLeastSquares();

    bool WriteJacobian(int tag);
    void EvalJacobian();

    void WriteEquationsExceptFor(hConstraint hc, Group *g);
    void FindWhichToRemoveToFixJacobian(Group *g, List<hConstraint> *bad);
    void SolveBySubstitution();

    bool IsDragged(hParam p);

    bool NewtonSolve(int tag);

    SolveResult Solve(Group *g, int *dof, List<hConstraint> *bad,
                bool andFindBad, bool andFindFree);

    void Clear();
};

#include "ttf.h"

class StepFileWriter {
public:
    void ExportSurfacesTo(const std::string &filename);
    void WriteHeader();
	void WriteProductHeader();
    int ExportCurve(SBezier *sb);
    int ExportCurveLoop(SBezierLoop *loop, bool inner);
    void ExportSurface(SSurface *ss, SBezierList *sbl);
    void WriteWireframe();
    void WriteFooter();

    List<int> curves;
    List<int> advancedFaces;
    FILE *f;
    int id;
};

class VectorFileWriter {
protected:
    Vector u, v, n, origin;
    double cameraTan, scale;

public:
    FILE *f;
    std::string filename;
    Vector ptMin, ptMax;

    static double MmToPts(double mm);

    static VectorFileWriter *ForFile(const std::string &filename);

    void SetModelviewProjection(const Vector &u, const Vector &v, const Vector &n,
                                const Vector &origin, double cameraTan, double scale);
    Vector Transform(Vector &pos) const;

    void OutputLinesAndMesh(SBezierLoopSetSet *sblss, SMesh *sm);

    void BezierAsPwl(SBezier *sb);
    void BezierAsNonrationalCubic(SBezier *sb, int depth=0);

    virtual void StartPath(RgbaColor strokeRgb, double lineWidth,
                            bool filled, RgbaColor fillRgb, hStyle hs) = 0;
    virtual void FinishPath(RgbaColor strokeRgb, double lineWidth,
                            bool filled, RgbaColor fillRgb, hStyle hs) = 0;
    virtual void Bezier(SBezier *sb) = 0;
    virtual void Triangle(STriangle *tr) = 0;
    virtual bool OutputConstraints(IdList<Constraint,hConstraint> *) { return false; }
    virtual void StartFile() = 0;
    virtual void FinishAndCloseFile() = 0;
    virtual bool HasCanvasSize() const = 0;
    virtual bool CanOutputMesh() const = 0;
};
class DxfFileWriter : public VectorFileWriter {
public:
    struct BezierPath {
        std::vector<SBezier *> beziers;
    };

    std::vector<BezierPath>         paths;
    IdList<Constraint,hConstraint> *constraint;

    static const char *lineTypeName(StipplePattern stippleType);

    bool OutputConstraints(IdList<Constraint,hConstraint> *constraint) override;

    void StartPath( RgbaColor strokeRgb, double lineWidth,
                    bool filled, RgbaColor fillRgb, hStyle hs) override;
    void FinishPath(RgbaColor strokeRgb, double lineWidth,
                    bool filled, RgbaColor fillRgb, hStyle hs) override;
    void Triangle(STriangle *tr) override;
    void Bezier(SBezier *sb) override;
    void StartFile() override;
    void FinishAndCloseFile() override;
    bool HasCanvasSize() const override { return false; }
    bool CanOutputMesh() const override { return false; }
    bool NeedToOutput(Constraint *c);
};
class EpsFileWriter : public VectorFileWriter {
public:
    Vector prevPt;
    void MaybeMoveTo(Vector s, Vector f);

    void StartPath( RgbaColor strokeRgb, double lineWidth,
                    bool filled, RgbaColor fillRgb, hStyle hs) override;
    void FinishPath(RgbaColor strokeRgb, double lineWidth,
                    bool filled, RgbaColor fillRgb, hStyle hs) override;
    void Triangle(STriangle *tr) override;
    void Bezier(SBezier *sb) override;
    void StartFile() override;
    void FinishAndCloseFile() override;
    bool HasCanvasSize() const override { return true; }
    bool CanOutputMesh() const override { return true; }
};
class PdfFileWriter : public VectorFileWriter {
public:
    uint32_t xref[10];
    uint32_t bodyStart;
    Vector prevPt;
    void MaybeMoveTo(Vector s, Vector f);

    void StartPath( RgbaColor strokeRgb, double lineWidth,
                    bool filled, RgbaColor fillRgb, hStyle hs) override;
    void FinishPath(RgbaColor strokeRgb, double lineWidth,
                    bool filled, RgbaColor fillRgb, hStyle hs) override;
    void Triangle(STriangle *tr) override;
    void Bezier(SBezier *sb) override;
    void StartFile() override;
    void FinishAndCloseFile() override;
    bool HasCanvasSize() const override { return true; }
    bool CanOutputMesh() const override { return true; }
};
class SvgFileWriter : public VectorFileWriter {
public:
    Vector prevPt;
    void MaybeMoveTo(Vector s, Vector f);

    void StartPath( RgbaColor strokeRgb, double lineWidth,
                    bool filled, RgbaColor fillRgb, hStyle hs) override;
    void FinishPath(RgbaColor strokeRgb, double lineWidth,
                    bool filled, RgbaColor fillRgb, hStyle hs) override;
    void Triangle(STriangle *tr) override;
    void Bezier(SBezier *sb) override;
    void StartFile() override;
    void FinishAndCloseFile() override;
    bool HasCanvasSize() const override { return true; }
    bool CanOutputMesh() const override { return true; }
};
class HpglFileWriter : public VectorFileWriter {
public:
    static double MmToHpglUnits(double mm);
    void StartPath( RgbaColor strokeRgb, double lineWidth,
                    bool filled, RgbaColor fillRgb, hStyle hs) override;
    void FinishPath(RgbaColor strokeRgb, double lineWidth,
                    bool filled, RgbaColor fillRgb, hStyle hs) override;
    void Triangle(STriangle *tr) override;
    void Bezier(SBezier *sb) override;
    void StartFile() override;
    void FinishAndCloseFile() override;
    bool HasCanvasSize() const override { return false; }
    bool CanOutputMesh() const override { return false; }
};
class Step2dFileWriter : public VectorFileWriter {
    StepFileWriter sfw;
    void StartPath( RgbaColor strokeRgb, double lineWidth,
                    bool filled, RgbaColor fillRgb, hStyle hs) override;
    void FinishPath(RgbaColor strokeRgb, double lineWidth,
                    bool filled, RgbaColor fillRgb, hStyle hs) override;
    void Triangle(STriangle *tr) override;
    void Bezier(SBezier *sb) override;
    void StartFile() override;
    void FinishAndCloseFile() override;
    bool HasCanvasSize() const override { return false; }
    bool CanOutputMesh() const override { return false; }
};
class GCodeFileWriter : public VectorFileWriter {
public:
    SEdgeList sel;
    void StartPath( RgbaColor strokeRgb, double lineWidth,
                    bool filled, RgbaColor fillRgb, hStyle hs) override;
    void FinishPath(RgbaColor strokeRgb, double lineWidth,
                    bool filled, RgbaColor fillRgb, hStyle hs) override;
    void Triangle(STriangle *tr) override;
    void Bezier(SBezier *sb) override;
    void StartFile() override;
    void FinishAndCloseFile() override;
    bool HasCanvasSize() const override { return false; }
    bool CanOutputMesh() const override { return false; }
};

#ifdef LIBRARY
#   define ENTITY EntityBase
#   define CONSTRAINT ConstraintBase
#else
#   define ENTITY Entity
#   define CONSTRAINT Constraint
#endif
class Sketch {
public:
    // These are user-editable, and define the sketch.
    IdList<Group,hGroup>            group;
    List<hGroup>                    groupOrder;
    IdList<CONSTRAINT,hConstraint>  constraint;
    IdList<Request,hRequest>        request;
    IdList<Style,hStyle>            style;

    // These are generated from the above.
    IdList<ENTITY,hEntity>          entity;
    IdList<Param,hParam>            param;

    inline CONSTRAINT *GetConstraint(hConstraint h)
        { return constraint.FindById(h); }
    inline ENTITY  *GetEntity (hEntity  h) { return entity. FindById(h); }
    inline Param   *GetParam  (hParam   h) { return param.  FindById(h); }
    inline Request *GetRequest(hRequest h) { return request.FindById(h); }
    inline Group   *GetGroup  (hGroup   h) { return group.  FindById(h); }
    // Styles are handled a bit differently.

    void Clear();

    BBox CalculateEntityBBox(bool includingInvisible);
    Group *GetRunningMeshGroupFor(hGroup h);
};
#undef ENTITY
#undef CONSTRAINT

class SolveSpaceUI {
public:
    TextWindow                 *pTW;
    TextWindow                 &TW;
    GraphicsWindow              GW;

    // The state for undo/redo
    typedef struct {
        IdList<Group,hGroup>            group;
        List<hGroup>                    groupOrder;
        IdList<Request,hRequest>        request;
        IdList<Constraint,hConstraint>  constraint;
        IdList<Param,hParam>            param;
        IdList<Style,hStyle>            style;
        hGroup                          activeGroup;

        void Clear() {
            group.Clear();
            request.Clear();
            constraint.Clear();
            param.Clear();
            style.Clear();
        }
    } UndoState;
    enum { MAX_UNDO = 16 };
    typedef struct {
        UndoState   d[MAX_UNDO];
        int         cnt;
        int         write;
    } UndoStack;
    UndoStack   undo;
    UndoStack   redo;
    void UndoEnableMenus();
    void UndoRemember();
    void UndoUndo();
    void UndoRedo();
    void PushFromCurrentOnto(UndoStack *uk);
    void PopOntoCurrentFrom(UndoStack *uk);
    void UndoClearState(UndoState *ut);
    void UndoClearStack(UndoStack *uk);

    // Little bits of extra configuration state
    enum { MODEL_COLORS = 8 };
    RgbaColor modelColor[MODEL_COLORS];
    Vector   lightDir[2];
    double   lightIntensity[2];
    double   ambientIntensity;
    double   chordTol;
    double   chordTolCalculated;
    int      maxSegments;
    double   exportChordTol;
    int      exportMaxSegments;
    double   cameraTangent;
    float    gridSpacing;
    float    exportScale;
    float    exportOffset;
    bool     fixExportColors;
    bool     drawBackFaces;
    bool     checkClosedContour;
    bool     showToolbar;
    RgbaColor backgroundColor;
    bool     exportShadedTriangles;
    bool     exportPwlCurves;
    bool     exportCanvasSizeAuto;
    bool     exportMode;
    struct {
        float   left;
        float   right;
        float   bottom;
        float   top;
    }        exportMargin;
    struct {
        float   width;
        float   height;
        float   dx;
        float   dy;
    }        exportCanvas;
    struct {
        float   depth;
        int     passes;
        float   feed;
        float   plungeFeed;
    }        gCode;

    Unit     viewUnits;
    int      afterDecimalMm;
    int      afterDecimalInch;
    int      autosaveInterval; // in minutes

    std::string MmToString(double v);
    double ExprToMm(Expr *e);
    double StringToMm(const std::string &s);
    const char *UnitName();
    double MmPerUnit();
    int UnitDigitsAfterDecimal();
    void SetUnitDigitsAfterDecimal(int v);
    double ChordTolMm();
    double ExportChordTolMm();
    int GetMaxSegments();
    bool usePerspectiveProj;
    double CameraTangent();

    // Some stuff relating to the tangent arcs created non-parametrically
    // as special requests.
    double tangentArcRadius;
    bool tangentArcManual;
    bool tangentArcDeleteOld;

    // The platform-dependent code calls this before entering the msg loop
    void Init();
    bool OpenFile(const std::string &filename);
    void Exit();

    // File load/save routines, including the additional files that get
    // loaded when we have link groups.
    FILE        *fh;
    void AfterNewFile();
    static void RemoveFromRecentList(const std::string &filename);
    static void AddToRecentList(const std::string &filename);
    std::string saveFile;
    bool        fileLoadError;
    bool        unsaved;
    typedef struct {
        char        type;
        const char *desc;
        char        fmt;
        void       *ptr;
    } SaveTable;
    static const SaveTable SAVED[];
    void SaveUsingTable(int type);
    void LoadUsingTable(char *key, char *val);
    struct {
        Group        g;
        Request      r;
        Entity       e;
        Param        p;
        Constraint   c;
        Style        s;
    } sv;
    static void MenuFile(Command id);
	bool Autosave();
    void RemoveAutosave();
    bool GetFilenameAndSave(bool saveAs);
    bool OkayToStartNewFile();
    hGroup CreateDefaultDrawingGroup();
    void UpdateWindowTitle();
    void ClearExisting();
    void NewFile();
    bool SaveToFile(const std::string &filename);
    bool LoadAutosaveFor(const std::string &filename);
    bool LoadFromFile(const std::string &filename);
    bool LoadEntitiesFromFile(const std::string &filename, EntityList *le,
                              SMesh *m, SShell *sh);
    bool ReloadAllImported(bool canCancel=false);
    // And the various export options
    void ExportAsPngTo(const std::string &filename);
    void ExportMeshTo(const std::string &filename);
    void ExportMeshAsStlTo(FILE *f, SMesh *sm);
    void ExportMeshAsObjTo(FILE *f, SMesh *sm);
    void ExportMeshAsThreeJsTo(FILE *f, const std::string &filename,
                               SMesh *sm, SOutlineList *sol);
    void ExportViewOrWireframeTo(const std::string &filename, bool exportWireframe);
    void ExportSectionTo(const std::string &filename);
    void ExportWireframeCurves(SEdgeList *sel, SBezierList *sbl,
                               VectorFileWriter *out);
    void ExportLinesAndMesh(SEdgeList *sel, SBezierList *sbl, SMesh *sm,
                            Vector u, Vector v,
                            Vector n, Vector origin,
                            double cameraTan,
                            VectorFileWriter *out);

    static void MenuAnalyze(Command id);

    // Additional display stuff
    struct {
        SContour    path;
        hEntity     point;
    } traced;
    SEdgeList nakedEdges;
    struct {
        bool        draw;
        Vector      ptA;
        Vector      ptB;
    } extraLine;
    struct {
        std::shared_ptr<Pixmap> pixmap;
        double      scale; // pixels per mm
        Vector      origin;
    } bgImage;
    struct {
        bool        draw, showOrigin;
        Vector      pt, u, v;
    } justExportedInfo;

    class Clipboard {
    public:
        List<ClipboardRequest>  r;
        List<Constraint>        c;

        void Clear();
        bool ContainsEntity(hEntity old);
        hEntity NewEntityFor(hEntity old);
    };
    Clipboard clipboard;

    void MarkGroupDirty(hGroup hg);
    void MarkGroupDirtyByEntity(hEntity he);

    // Consistency checking on the sketch: stuff with missing dependencies
    // will get deleted automatically.
    struct {
        int     requests;
        int     groups;
        int     constraints;
        int     nonTrivialConstraints;
    } deleted;
    bool GroupExists(hGroup hg);
    bool PruneOrphans();
    bool EntityExists(hEntity he);
    bool GroupsInOrder(hGroup before, hGroup after);
    bool PruneGroups(hGroup hg);
    bool PruneRequests(hGroup hg);
    bool PruneConstraints(hGroup hg);

    enum class Generate : uint32_t {
        DIRTY,
        ALL,
        REGEN,
        UNTIL_ACTIVE,
    };

    void GenerateAll(Generate type = Generate::DIRTY, bool andFindFree = false,
                     bool genForBBox = false);
    void SolveGroup(hGroup hg, bool andFindFree);
    void MarkDraggedParams();
    void ForceReferences();

    bool ActiveGroupsOkay();

    // The system to be solved.
    System     *pSys;
    System     &sys;

    // All the TrueType fonts in memory
    TtfFontList fonts;

    // Everything has been pruned, so we know there's no dangling references
    // to entities that don't exist. Before that, we mustn't try to display
    // the sketch!
    bool allConsistent;

    struct {
        bool    scheduled;
        bool    showTW;
        bool    generateAll;
    } later;
    void ScheduleShowTW();
    void ScheduleGenerateAll();
    void DoLater();

    static void MenuHelp(Command id);

    void Clear();

    // We allocate TW and sys on the heap to work around an MSVC problem
    // where it puts zero-initialized global data in the binary (~30M of zeroes)
    // in release builds.
    SolveSpaceUI()
        : pTW(new TextWindow({})), TW(*pTW),
          pSys(new System({})), sys(*pSys) {}

    ~SolveSpaceUI() {
        delete pTW;
        delete pSys;
    }
};

void ImportDxf(const std::string &file);
void ImportDwg(const std::string &file);

extern SolveSpaceUI SS;
extern Sketch SK;

}

#ifndef __OBJC__
using namespace SolveSpace;
#endif

#endif
