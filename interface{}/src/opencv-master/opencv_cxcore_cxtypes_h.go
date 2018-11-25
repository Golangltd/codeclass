// Copyright 2014 <chaishushan{AT}gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package opencv

/*
#include <cxtypes.h>

static int CV_IS_IMAGE_HDR_(void* img) {
	return CV_IS_IMAGE_HDR(img);
}
static int CV_IS_IMAGE_(void* img) {
	return CV_IS_IMAGE(img);
}
*/
import "C"
import (
	"unsafe"
)

type CvArr C.CvArr

type Cv32suf C.Cv32suf
type Cv64suf C.Cv64suf

/****************************************************************************************\
*                             Common macros and inline functions                         *
\****************************************************************************************/

const (
	CV_PI   = 3.1415926535897932384626433832795
	CV_LOG2 = 0.69314718055994530941723212145818
)

func Round(value float64) int {
	rv := C.cvRound(C.double(value))
	return int(rv)
}
func Floor(value float64) int {
	rv := C.cvFloor(C.double(value))
	return int(rv)
}
func Ceil(value float64) int {
	rv := C.cvCeil(C.double(value))
	return int(rv)
}

func IsNaN(value float64) int {
	rv := C.cvIsNaN(C.double(value))
	return int(rv)
}
func IsInf(value float64) int {
	rv := C.cvIsInf(C.double(value))
	return int(rv)
}

/*************** Random number generation *******************/

type RNG C.CvRNG

func NewRNG(seed int64) RNG {
	rv := C.cvRNG(C.int64(seed))
	return RNG(rv)
}
func (rng *RNG) RandInt() uint32 {
	rv := C.cvRandInt((*C.CvRNG)(rng))
	return uint32(rv)
}
func (rng *RNG) RandReal() float64 {
	rv := C.cvRandReal((*C.CvRNG)(rng))
	return float64(rv)
}

/*****************************************************************************\
*                            Image type (IplImage)                            *
\*****************************************************************************/

/*
 * The following definitions (until #endif)
 * is an extract from IPL headers.
 * Copyright (c) 1995 Intel Corporation.
 */

const (
	IPL_DEPTH_SIGN = C.IPL_DEPTH_SIGN

	IPL_DEPTH_1U  = C.IPL_DEPTH_1U
	IPL_DEPTH_8U  = C.IPL_DEPTH_8U
	IPL_DEPTH_16U = C.IPL_DEPTH_16U
	IPL_DEPTH_32F = C.IPL_DEPTH_32F

	IPL_DEPTH_8S  = C.IPL_DEPTH_8S
	IPL_DEPTH_16S = C.IPL_DEPTH_16S
	IPL_DEPTH_32S = C.IPL_DEPTH_32S

	IPL_DATA_ORDER_PIXEL = C.IPL_DATA_ORDER_PIXEL
	IPL_DATA_ORDER_PLANE = C.IPL_DATA_ORDER_PLANE

	IPL_ORIGIN_TL = C.IPL_ORIGIN_TL
	IPL_ORIGIN_BL = C.IPL_ORIGIN_BL

	IPL_ALIGN_4BYTES  = C.IPL_ALIGN_4BYTES
	IPL_ALIGN_8BYTES  = C.IPL_ALIGN_8BYTES
	IPL_ALIGN_16BYTES = C.IPL_ALIGN_16BYTES
	IPL_ALIGN_32BYTES = C.IPL_ALIGN_32BYTES

	IPL_ALIGN_DWORD = C.IPL_ALIGN_DWORD
	IPL_ALIGN_QWORD = C.IPL_ALIGN_QWORD

	IPL_BORDER_CONSTANT  = C.IPL_BORDER_CONSTANT
	IPL_BORDER_REPLICATE = C.IPL_BORDER_REPLICATE
	IPL_BORDER_REFLECT   = C.IPL_BORDER_REFLECT
	IPL_BORDER_WRAP      = C.IPL_BORDER_WRAP
)

type IplImage C.IplImage

func (p *IplImage) GetChannels() int {
	return int(p.nChannels)
}
func (p *IplImage) GetDepth() int {
	return int(p.depth)
}
func (p *IplImage) GetOrigin() int {
	return int(p.origin)
}
func (p *IplImage) GetWidth() int {
	return int(p.width)
}
func (p *IplImage) GetHeight() int {
	return int(p.height)
}
func (p *IplImage) GetWidthStep() int {
	return int(p.widthStep)
}
func (p *IplImage) GetImageSize() int {
	return int(p.imageSize)
}
func (p *IplImage) GetImageData() []byte {
	return ((*[1 << 30]byte)(unsafe.Pointer(p.imageData)))[:p.GetImageSize()]
}

type IplROI C.IplROI

func (roi *IplROI) Init(coi, xOffset, yOffset, width, height int) {
	roi_c := (*C.IplROI)(roi)
	roi_c.coi = C.int(coi)
	roi_c.xOffset = C.int(xOffset)
	roi_c.yOffset = C.int(yOffset)
	roi_c.width = C.int(width)
	roi_c.height = C.int(height)
}
func (roi *IplROI) Coi() int {
	roi_c := (*C.IplROI)(roi)
	return int(roi_c.coi)
}
func (roi *IplROI) XOffset() int {
	roi_c := (*C.IplROI)(roi)
	return int(roi_c.xOffset)
}
func (roi *IplROI) YOffset() int {
	roi_c := (*C.IplROI)(roi)
	return int(roi_c.yOffset)
}
func (roi *IplROI) Width() int {
	roi_c := (*C.IplROI)(roi)
	return int(roi_c.width)
}
func (roi *IplROI) Height() int {
	roi_c := (*C.IplROI)(roi)
	return int(roi_c.height)
}

type IplConvKernel C.IplConvKernel
type IplConvKernelFP C.IplConvKernelFP

const (
	IPL_IMAGE_HEADER = C.IPL_IMAGE_HEADER // 1
	IPL_IMAGE_DATA   = C.IPL_IMAGE_DATA   // 2
	IPL_IMAGE_ROI    = C.IPL_IMAGE_ROI    // 4
)

/* extra border mode */

var (
	IPL_IMAGE_MAGIC_VAL = C.IPL_IMAGE_MAGIC_VAL
	CV_TYPE_NAME_IMAGE  = C.CV_TYPE_NAME_IMAGE
)

func CV_IS_IMAGE_HDR(img unsafe.Pointer) bool {
	rv := C.CV_IS_IMAGE_HDR_(img)
	return (int(rv) != 0)
}
func CV_IS_IMAGE(img unsafe.Pointer) bool {
	rv := C.CV_IS_IMAGE_(img)
	return (int(rv) != 0)
}

const (
	IPL_DEPTH_64F = C.IPL_DEPTH_64F
)

/****************************************************************************************\
*                                  Matrix type (CvMat)                                   *
\****************************************************************************************/

type Mat C.CvMat

func (mat *Mat) Type() int {
	return int(mat._type)
}
func (mat *Mat) Step() int {
	return int(mat.step)
}

func (mat *Mat) Rows() int {
	return int(mat.rows)
}
func (mat *Mat) Cols() int {
	return int(mat.cols)
}

func CV_IS_MAT_HDR(mat interface{}) bool {
	return false
}
func CV_IS_MAT(mat interface{}) bool {
	return false
}
func CV_IS_MASK_ARR() bool {
	return false
}
func CV_ARE_TYPE_EQ() bool {
	return false
}

func (m *Mat) Init(rows, cols int, _type int, data unsafe.Pointer) {
	return
}
func (m *Mat) Get(row, col int) float64 {
	rv := C.cvmGet((*C.CvMat)(m), C.int(row), C.int(col))
	return float64(rv)
}
func (m *Mat) Set(row, col int, value float64) {
	C.cvmSet((*C.CvMat)(m), C.int(row), C.int(col), C.double(value))
}

func CvToIplDepth(_type int) int {
	rv := C.cvCvToIplDepth(C.int(_type))
	return int(rv)
}

/****************************************************************************************\
*                       Multi-dimensional dense array (CvMatND)                          *
\****************************************************************************************/

const (
	CV_MATND_MAGIC_VAL = C.CV_MATND_MAGIC_VAL
	CV_TYPE_NAME_MATND = C.CV_TYPE_NAME_MATND

	CV_MAX_DIM      = C.CV_MAX_DIM
	CV_MAX_DIM_HEAP = C.CV_MAX_DIM_HEAP
)

type MatND C.CvMatND

func (m *MatND) Type() int {
	rv := m._type
	return int(rv)
}
func (m *MatND) Dims() int {
	rv := m.dims
	return int(rv)
}

/****************************************************************************************\
*                      Multi-dimensional sparse array (CvSparseMat)                      *
\****************************************************************************************/

const (
	CV_SPARSE_MAT_MAGIC_VAL = C.CV_SPARSE_MAT_MAGIC_VAL
	CV_TYPE_NAME_SPARSE_MAT = C.CV_TYPE_NAME_SPARSE_MAT
)

type SparseMat C.CvSparseMat

func (m *SparseMat) Type() int {
	rv := m._type
	return int(rv)
}
func (m *SparseMat) Dims() int {
	rv := m.dims
	return int(rv)
}

/**************** iteration through a sparse array *****************/

type SparseNode C.CvSparseNode

func (node *SparseNode) HashVal() uint32 {
	rv := node.hashval
	return uint32(rv)
}
func (node *SparseNode) Next() *SparseNode {
	rv := node.next
	return (*SparseNode)(rv)
}

type SparseMatIterator C.CvSparseMatIterator

func (node *SparseMatIterator) Mat() *SparseMat {
	rv := node.mat
	return (*SparseMat)(rv)
}
func (node *SparseMatIterator) Node() *SparseNode {
	rv := node.node
	return (*SparseNode)(rv)
}
func (node *SparseMatIterator) CurIdx() int {
	rv := node.curidx
	return (int)(rv)
}

/****************************************************************************************\
*                                         Histogram                                      *
\****************************************************************************************/

type HistType C.CvHistType

const (
	CV_HIST_MAGIC_VAL    = C.CV_HIST_MAGIC_VAL
	CV_HIST_UNIFORM_FLAG = C.CV_HIST_UNIFORM_FLAG

	/* indicates whether bin ranges are set already or not */
	CV_HIST_RANGES_FLAG = C.CV_HIST_RANGES_FLAG

	CV_HIST_ARRAY  = C.CV_HIST_ARRAY
	CV_HIST_SPARSE = C.CV_HIST_SPARSE
	CV_HIST_TREE   = C.CV_HIST_TREE

	/* should be used as a parameter only,
	   it turns to CV_HIST_UNIFORM_FLAG of hist->type */
	CV_HIST_UNIFORM = C.CV_HIST_UNIFORM
)

type Histogram C.CvHistogram

func CV_IS_HIST() bool {
	return false
}
func CV_IS_UNIFORM_HIST() bool {
	return false
}
func CV_IS_SPARSE_HIST() bool {
	return false
}
func CV_HIST_HAS_RANGES() bool {
	return false
}

/****************************************************************************************\
*                      Other supplementary data type definitions                         *
\****************************************************************************************/

/*************************************** CvRect *****************************************/

type Rect C.CvRect

func (r *Rect) Init(x, y, w, h int) {
	r.x = C.int(x)
	r.y = C.int(y)
	r.width = C.int(w)
	r.height = C.int(h)
}
func (r *Rect) X() int {
	r_c := (*C.CvRect)(r)
	return int(r_c.x)
}
func (r *Rect) Y() int {
	r_c := (*C.CvRect)(r)
	return int(r_c.y)
}
func (r *Rect) Width() int {
	r_c := (*C.CvRect)(r)
	return int(r_c.width)
}
func (r *Rect) Height() int {
	r_c := (*C.CvRect)(r)
	return int(r_c.height)
}

func (r *Rect) ToROI(coi int) IplROI {
	r_c := (*C.CvRect)(r)
	return (IplROI)(C.cvRectToROI(*r_c, C.int(coi)))
}

func (roi *IplROI) ToRect() Rect {
	r := C.cvRect(
		C.int(roi.XOffset()),
		C.int(roi.YOffset()),
		C.int(roi.Width()),
		C.int(roi.Height()),
	)
	return Rect(r)
}

/*********************************** CvTermCriteria *************************************/

const (
	CV_TERMCRIT_ITER   = C.CV_TERMCRIT_ITER
	CV_TERMCRIT_NUMBER = C.CV_TERMCRIT_NUMBER
	CV_TERMCRIT_EPS    = C.CV_TERMCRIT_EPS
)

type TermCriteria C.CvTermCriteria

func (x *TermCriteria) Init(type_, max_iter int, epsilon float64) {
	rv := C.cvTermCriteria(C.int(type_), C.int(max_iter), C.double(epsilon))
	(*x) = (TermCriteria)(rv)
}

func (x *TermCriteria) Type() int {
	rv := x._type
	return int(rv)
}
func (x *TermCriteria) MaxIter() int {
	rv := x.max_iter
	return int(rv)
}
func (x *TermCriteria) Epsilon() float64 {
	rv := x.epsilon
	return float64(rv)
}

/******************************* CvPoint and variants ***********************************/

type Point struct {
	X int
	Y int
}

type Point2D32f struct {
	X float32
	Y float32
}
type Point3D32f struct {
	X float32
	Y float32
	Z float32
}

type Point2D64f struct {
	X float64
	Y float64
}
type Point3D64f struct {
	X float64
	Y float64
	Z float64
}

/******************************** CvSize's & CvBox **************************************/

type Size struct {
	Width  int
	Height int
}

type Size2D32f struct {
	Width  float32
	Height float32
}

type Box2D struct {
	center Point2D32f
	size   Size2D32f
	angle  float32
}

type LineIterator C.CvLineIterator

/************************************* CvSlice ******************************************/

type Slice C.CvSlice

const (
	CV_WHOLE_SEQ_END_INDEX = C.CV_WHOLE_SEQ_END_INDEX
)

/************************************* CvScalar *****************************************/

type Scalar C.CvScalar

func ScalarAll(val0 float64) Scalar {
	rv := C.cvScalarAll(C.double(val0))
	return (Scalar)(rv)
}

/****************************************************************************************\
*                                   Dynamic Data structures                              *
\****************************************************************************************/

/******************************** Memory storage ****************************************/

type CvMemBlock C.CvMemBlock
type CvMemStorage C.CvMemStorage
type CvMemStoragePos C.CvMemStoragePos

/*********************************** Sequence *******************************************/

type CvSeqBlock C.CvSeqBlock
type CvSeq C.CvSeq

/*************************************** Set ********************************************/

type CvSet C.CvSet

/************************************* Graph ********************************************/

type CvGraphEdge C.CvGraphEdge
type CvGraphVtx C.CvGraphVtx

type CvGraphVtx2D C.CvGraphVtx2D
type CvGraph C.CvGraph

/*********************************** Chain/Countour *************************************/

type CvChain C.CvChain
type CvContour C.CvContour

/****************************************************************************************\
*                                    Sequence types                                      *
\****************************************************************************************/

/****************************************************************************************/
/*                            Sequence writer & reader                                  */
/****************************************************************************************/

type CvSeqWriter C.CvSeqWriter
type CvSeqReader C.CvSeqReader

/****************************************************************************************/
/*                                Operations on sequences                               */
/****************************************************************************************/

/****************************************************************************************\
*             Data structures for persistence (a.k.a serialization) functionality        *
\****************************************************************************************/

/* "black box" file storage */
type CvFileStorage C.CvFileStorage

/* Storage flags: */
const (
	CV_STORAGE_READ         = C.CV_STORAGE_READ
	CV_STORAGE_WRITE        = C.CV_STORAGE_WRITE
	CV_STORAGE_WRITE_TEXT   = C.CV_STORAGE_WRITE_TEXT
	CV_STORAGE_WRITE_BINARY = C.CV_STORAGE_WRITE_BINARY
	CV_STORAGE_APPEND       = C.CV_STORAGE_APPEND
)

type CvAttrList C.CvAttrList

/*****************************************************************************\
*                                 --- END ---                                 *
\*****************************************************************************/
