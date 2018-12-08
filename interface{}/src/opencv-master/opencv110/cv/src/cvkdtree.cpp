/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                        Intel License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2008, Xavier Delacour, all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of Intel Corporation may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

// 2008-05-13, Xavier Delacour <xavier.delacour@gmail.com>

#include "_cv.h"

#if !defined _MSC_VER || defined __ICL || _MSC_VER >= 1400
#include "_cvkdtree.hpp"

// * write up some docs

// * removing __valuetype parameter from CvKDTree and using virtuals instead 
// * of void* data here could simplify things.

struct CvFeatureTree {

  template <class __scalartype, int __cvtype>
  struct deref {
    typedef __scalartype scalar_type;
    typedef double accum_type;

    CvMat* mat;
    deref(CvMat* _mat) : mat(_mat) {
      assert(CV_ELEM_SIZE1(__cvtype) == sizeof(__scalartype));
    }
    scalar_type operator() (int i, int j) const {
      return *((scalar_type*)(mat->data.ptr + i * mat->step) + j);
    }
  };

#define dispatch_cvtype(mat, c) \
    switch (CV_MAT_DEPTH((mat)->type)) { \
    case CV_32F: \
      { typedef CvKDTree<int, deref<float, CV_32F> > tree_type; c; break; } \
    case CV_64F: \
      { typedef CvKDTree<int, deref<double, CV_64F> > tree_type; c; break; } \
    default: assert(0); \
    }

  CvMat* mat;
  void* data;

  template <class __treetype>
  void find_nn(CvMat* d, int k, int emax, CvMat* results, CvMat* dist) {
    __treetype* tr = (__treetype*) data;
    uchar* dptr = d->data.ptr;
    uchar* resultsptr = results->data.ptr;
    uchar* distptr = dist->data.ptr;
    typename __treetype::bbf_nn_pqueue nn;

    assert(d->cols == tr->dims());
    assert(results->rows == d->rows);
    assert(results->rows == dist->rows);
    assert(results->cols == k);
    assert(dist->cols == k);

    for (int j = 0; j < d->rows; ++j) {
      typename __treetype::scalar_type* dj = (typename __treetype::scalar_type*) dptr;

      int* resultsj = (int*) resultsptr;
      double* distj = (double*) distptr;
      tr->find_nn_bbf(dj, k, emax, nn);

      assert((int)nn.size() <= k);
      for (unsigned int j = 0; j < nn.size(); ++j) {
	*resultsj++ = *nn[j].p;
	*distj++ = nn[j].dist;
      }
      std::fill(resultsj, resultsj + k - nn.size(), -1);
      std::fill(distj, distj + k - nn.size(), 0);

      dptr += d->step;
      resultsptr += results->step;
      distptr += dist->step;
    }
  }

  template <class __treetype>
  int find_ortho_range(CvMat* bounds_min, CvMat* bounds_max,
		       CvMat* results) {
    int rn = results->rows * results->cols;
    std::vector<int> inbounds;
    dispatch_cvtype(mat, ((__treetype*)data)->
		    find_ortho_range((typename __treetype::scalar_type*)bounds_min->data.ptr, 
				     (typename __treetype::scalar_type*)bounds_max->data.ptr, 
				     inbounds));
    std::copy(inbounds.begin(),
	      inbounds.begin() + std::min((int)inbounds.size(), rn),
	      (int*) results->data.ptr);
    return inbounds.size();
  }

  CvFeatureTree(const CvFeatureTree& x);
  CvFeatureTree& operator= (const CvFeatureTree& rhs);
public:
  CvFeatureTree(CvMat* _mat) : mat(_mat) {
    // * a flag parameter should tell us whether
    // * (a) user ensures *mat outlives *this and is unchanged, 
    // * (b) we take reference and user ensures mat is unchanged,
    // * (c) we copy data, (d) we own and release data.

    std::vector<int> tmp(mat->rows);
    for (unsigned int j = 0; j < tmp.size(); ++j)
      tmp[j] = j;

    dispatch_cvtype(mat, data = new tree_type
		    (&tmp[0], &tmp[0] + tmp.size(), mat->cols,
		     tree_type::deref_type(mat)));
  }
  ~CvFeatureTree() {
    dispatch_cvtype(mat, delete (tree_type*) data);
  }

  int dims() {
    int d = 0;
    dispatch_cvtype(mat, d = ((tree_type*) data)->dims());
    return d;
  }
  int type() {
    return mat->type;
  }

  void find_nn(CvMat* d, int k, int emax, CvMat* results, CvMat* dist) {
    assert(CV_MAT_TYPE(d->type) == CV_MAT_TYPE(mat->type));
    assert(CV_MAT_TYPE(dist->type) == CV_64FC1);
    assert(CV_MAT_TYPE(results->type) == CV_32SC1);

    dispatch_cvtype(mat, find_nn<tree_type>
		    (d, k, emax, results, dist));
  }
  int find_ortho_range(CvMat* bounds_min, CvMat* bounds_max,
			CvMat* results) {
    assert(CV_MAT_TYPE(bounds_min->type) == CV_MAT_TYPE(mat->type));
    assert(CV_MAT_TYPE(bounds_min->type) == CV_MAT_TYPE(bounds_max->type));
    assert(bounds_min->rows * bounds_min->cols == dims());
    assert(bounds_max->rows * bounds_max->cols == dims());

    int count = 0;
    dispatch_cvtype(mat, count = find_ortho_range<tree_type>
		    (bounds_min, bounds_max,results));
    return count;
  }
};



CvFeatureTree* cvCreateFeatureTree(CvMat* desc) {
  __BEGIN__;
  CV_FUNCNAME("cvCreateFeatureTree");

  if (CV_MAT_TYPE(desc->type) != CV_32FC1 &&
      CV_MAT_TYPE(desc->type) != CV_64FC1)
    CV_ERROR(CV_StsUnsupportedFormat, "descriptors must be either CV_32FC1 or CV_64FC1");

  return new CvFeatureTree(desc);
  __END__;

  return 0;
}

void cvReleaseFeatureTree(CvFeatureTree* tr) {
  delete tr;
}

// desc is m x d set of candidate points.
// results is m x k set of row indices of matching points.
// dist is m x k distance to matching points.
void cvFindFeatures(CvFeatureTree* tr, CvMat* desc,
		    CvMat* results, CvMat* dist, int k, int emax) {
  bool free_desc = false;
  int dims = tr->dims();
  int type = tr->type();

  __BEGIN__;
  CV_FUNCNAME("cvFindFeatures");
  
  if (desc->cols != dims)
    CV_ERROR(CV_StsUnmatchedSizes, "desc columns be equal feature dimensions");
  if (results->rows != desc->rows && results->cols != k)
    CV_ERROR(CV_StsUnmatchedSizes, "results and desc must be same height");
  if (dist->rows != desc->rows && dist->cols != k)
    CV_ERROR(CV_StsUnmatchedSizes, "dist and desc must be same height");
  if (CV_MAT_TYPE(results->type) != CV_32SC1)
    CV_ERROR(CV_StsUnsupportedFormat, "results must be CV_32SC1");
  if (CV_MAT_TYPE(dist->type) != CV_64FC1)
    CV_ERROR(CV_StsUnsupportedFormat, "dist must be CV_64FC1");

  if (CV_MAT_TYPE(type) != CV_MAT_TYPE(desc->type)) {
    CvMat* old_desc = desc;
    desc = cvCreateMat(desc->rows, desc->cols, type);
    cvConvert(old_desc, desc);
    free_desc = true;
  }

  tr->find_nn(desc, k, emax, results, dist);

  __END__;

  if (free_desc)
    cvReleaseMat(&desc);
}

int cvFindFeaturesBoxed(CvFeatureTree* tr,
			CvMat* bounds_min, CvMat* bounds_max,
			CvMat* results) {
  int nr = -1;
  bool free_bounds = false;
  int dims = tr->dims();
  int type = tr->type();

  __BEGIN__;
  CV_FUNCNAME("cvFindFeaturesBoxed");

  if (bounds_min->cols * bounds_min->rows != dims ||
      bounds_max->cols * bounds_max->rows != dims)
    CV_ERROR(CV_StsUnmatchedSizes, "bounds_{min,max} must 1 x dims or dims x 1");
  if (CV_MAT_TYPE(bounds_min->type) != CV_MAT_TYPE(bounds_max->type))
    CV_ERROR(CV_StsUnmatchedFormats, "bounds_{min,max} must have same type");
  if (CV_MAT_TYPE(results->type) != CV_32SC1)
    CV_ERROR(CV_StsUnsupportedFormat, "results must be CV_32SC1");

  if (CV_MAT_TYPE(bounds_min->type) != CV_MAT_TYPE(type)) {
    free_bounds = true;

    CvMat* old_bounds_min = bounds_min;
    bounds_min = cvCreateMat(bounds_min->rows, bounds_min->cols, type);
    cvConvert(old_bounds_min, bounds_min);

    CvMat* old_bounds_max = bounds_max;
    bounds_max = cvCreateMat(bounds_max->rows, bounds_max->cols, type);
    cvConvert(old_bounds_max, bounds_max);
  }

  nr = tr->find_ortho_range(bounds_min, bounds_max, results);

  __END__;
  if (free_bounds) {
    cvReleaseMat(&bounds_min);
    cvReleaseMat(&bounds_max);
  }

  return nr;
}
#endif
