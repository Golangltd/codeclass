# Copyright 2014 <chaishushan{AT}gmail.com>. All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

project(OpenCV)

#------------------------------------------------------------------------------

add_definitions(
  -DCVAPI_EXPORTS
)

include_directories(AFTER
	./cxcore/include
	./cv/include
	./cvaux/include
	./ml/include
	./highgui/include

	./cxcore/src
	./cv/src
	./cvaux/src
	./ml/src
	./highgui/src
)

set(CXCORE_SRC
	./cxcore/src/cxalloc.cpp
	./cxcore/src/cxarithm.cpp
	./cxcore/src/cxarray.cpp
	./cxcore/src/cxcmp.cpp
	./cxcore/src/cxconvert.cpp
	./cxcore/src/cxcopy.cpp
	./cxcore/src/cxdatastructs.cpp
	./cxcore/src/cxdrawing.cpp
	./cxcore/src/cxdxt.cpp
	./cxcore/src/cxerror.cpp
	./cxcore/src/cximage.cpp
	./cxcore/src/cxjacobieigens.cpp
	./cxcore/src/cxlogic.cpp
	./cxcore/src/cxlut.cpp
	./cxcore/src/cxmathfuncs.cpp
	./cxcore/src/cxmatmul.cpp
	./cxcore/src/cxmatrix.cpp
	./cxcore/src/cxmean.cpp
	./cxcore/src/cxmeansdv.cpp
	./cxcore/src/cxminmaxloc.cpp
	./cxcore/src/cxnorm.cpp
	./cxcore/src/cxouttext.cpp
	./cxcore/src/cxpersistence.cpp
	./cxcore/src/cxprecomp.cpp
	./cxcore/src/cxrand.cpp
	./cxcore/src/cxsumpixels.cpp
	./cxcore/src/cxsvd.cpp
	#./cxcore/src/cxswitcher.cpp
	./cxcore/src/cxswitcher_new.cpp
	./cxcore/src/cxtables.cpp
	./cxcore/src/cxutils.cpp
)

set(CV_SRC
	./cv/src/cvaccum.cpp
	./cv/src/cvadapthresh.cpp
	./cv/src/cvapprox.cpp
	./cv/src/cvcalccontrasthistogram.cpp
	./cv/src/cvcalcimagehomography.cpp
	./cv/src/cvcalibinit.cpp
	./cv/src/cvcalibration.cpp
	./cv/src/cvcamshift.cpp
	./cv/src/cvcanny.cpp
	./cv/src/cvcolor.cpp
	./cv/src/cvcondens.cpp
	./cv/src/cvcontours.cpp
	./cv/src/cvcontourtree.cpp
	./cv/src/cvconvhull.cpp
	./cv/src/cvcorner.cpp
	./cv/src/cvcornersubpix.cpp
	./cv/src/cvderiv.cpp
	./cv/src/cvdistransform.cpp
	./cv/src/cvdominants.cpp
	./cv/src/cvemd.cpp
	./cv/src/cvfeatureselect.cpp
	./cv/src/cvfilter.cpp
	./cv/src/cvfloodfill.cpp
	./cv/src/cvfundam.cpp
	./cv/src/cvgeometry.cpp
	./cv/src/cvhaar.cpp
	./cv/src/cvhistogram.cpp
	./cv/src/cvhough.cpp
	./cv/src/cvimgwarp.cpp
	./cv/src/cvinpaint.cpp
	./cv/src/cvkalman.cpp
	./cv/src/cvkdtree.cpp
	./cv/src/cvlinefit.cpp
	./cv/src/cvlkpyramid.cpp
	./cv/src/cvmatchcontours.cpp
	./cv/src/cvmoments.cpp
	./cv/src/cvmorph.cpp
	./cv/src/cvmotempl.cpp
	./cv/src/cvoptflowbm.cpp
	./cv/src/cvoptflowhs.cpp
	./cv/src/cvoptflowlk.cpp
	./cv/src/cvpgh.cpp
	./cv/src/cvposit.cpp
	./cv/src/cvprecomp.cpp
	./cv/src/cvpyramids.cpp
	./cv/src/cvpyrsegmentation.cpp
	./cv/src/cvrotcalipers.cpp
	./cv/src/cvsamplers.cpp
	./cv/src/cvsegmentation.cpp
	./cv/src/cvshapedescr.cpp
	./cv/src/cvsmooth.cpp
	./cv/src/cvsnakes.cpp
	./cv/src/cvstereobm.cpp
	./cv/src/cvstereogc.cpp
	./cv/src/cvsubdivision2d.cpp
	./cv/src/cvsumpixels.cpp
	./cv/src/cvsurf.cpp
	./cv/src/cvswitcher.cpp
	./cv/src/cvtables.cpp
	./cv/src/cvtemplmatch.cpp
	./cv/src/cvthresh.cpp
	./cv/src/cvundistort.cpp
	./cv/src/cvutils.cpp
)

set(CVAUX_SRC
	./cvaux/src/camshift.cpp
	./cvaux/src/cv3dtracker.cpp
	./cvaux/src/cvaux.cpp
	./cvaux/src/cvauxutils.cpp
	./cvaux/src/cvbgfg_acmmm2003.cpp
	./cvaux/src/cvbgfg_codebook.cpp
	./cvaux/src/cvbgfg_common.cpp
	./cvaux/src/cvbgfg_gaussmix.cpp
	./cvaux/src/cvcalibfilter.cpp
	./cvaux/src/cvclique.cpp
	./cvaux/src/cvcorrespond.cpp
	./cvaux/src/cvcorrimages.cpp
	./cvaux/src/cvcreatehandmask.cpp
	./cvaux/src/cvdpstereo.cpp
	./cvaux/src/cveigenobjects.cpp
	./cvaux/src/cvepilines.cpp
	./cvaux/src/cvface.cpp
	./cvaux/src/cvfacedetection.cpp
	./cvaux/src/cvfacetemplate.cpp
	./cvaux/src/cvfindface.cpp
	./cvaux/src/cvfindhandregion.cpp
	./cvaux/src/cvhmm.cpp
	./cvaux/src/cvhmm1d.cpp
	./cvaux/src/cvhmmobs.cpp
	./cvaux/src/cvlcm.cpp
	./cvaux/src/cvlee.cpp
	./cvaux/src/cvlevmar.cpp
	./cvaux/src/cvlevmarprojbandle.cpp
	./cvaux/src/cvlevmartrif.cpp
	./cvaux/src/cvlines.cpp
	./cvaux/src/cvlmeds.cpp
	./cvaux/src/cvmat.cpp
	./cvaux/src/cvmorphcontours.cpp
	./cvaux/src/cvmorphing.cpp
	./cvaux/src/cvprewarp.cpp
	./cvaux/src/cvscanlines.cpp
	./cvaux/src/cvsegment.cpp
	./cvaux/src/cvsubdiv2.cpp
	./cvaux/src/cvtexture.cpp
	./cvaux/src/cvtrifocal.cpp
	./cvaux/src/cvvecfacetracking.cpp
	./cvaux/src/cvvideo.cpp
	./cvaux/src/decomppoly.cpp
	./cvaux/src/enmin.cpp
	./cvaux/src/extendededges.cpp
	./cvaux/src/precomp.cpp
)

set(ML_SRC
	./ml/src/ml.cpp
	./ml/src/ml_inner_functions.cpp
	./ml/src/mlann_mlp.cpp
	./ml/src/mlboost.cpp
	./ml/src/mlcnn.cpp
	./ml/src/mlem.cpp
	./ml/src/mlestimate.cpp
	./ml/src/mlknearest.cpp
	./ml/src/mlnbayes.cpp
	./ml/src/mlrtrees.cpp
	./ml/src/mlsvm.cpp
	./ml/src/mltestset.cpp
	./ml/src/mltree.cpp
)

set(HIGHGUI_SRC_COMMON
	./highgui/src/EasyBMP.cpp
	./highgui/src/bitstrm.cpp
	./highgui/src/cvcap.cpp
	./highgui/src/cvcap_images.cpp
	./highgui/src/image.cpp
	./highgui/src/jpgd.cpp
	./highgui/src/jpge.cpp
	./highgui/src/loadsave_new.cpp
	./highgui/src/lodepng.cpp
	./highgui/src/precomp.cpp
	./highgui/src/utils.cpp
	./highgui/src/window.cpp
)

if(WIN32 OR MINGW)
	set(HIGHGUI_SRC ${HIGHGUI_SRC_COMMON}
		./highgui/src/windows/cvcap_dshow.cpp
		./highgui/src/windows/cvcap_vfw.cpp
		./highgui/src/windows/window_w32.cpp

		./highgui/src/windows/cap_ffmpeg.cpp
		./highgui/src/windows/videoInput_capi.cpp
		./highgui/src/windows/videoInput.cpp
	)
endif()

if(UNIX AND NOT MINGW)
	set(HIGHGUI_SRC ${HIGHGUI_SRC_COMMON}
		# todo
	)
endif()

if(APPLE)
	set(HIGHGUI_SRC ${HIGHGUI_SRC_COMMON}
		# todo
	)
endif()

#------------------------------------------------------------------------------

add_library(OpenCVLib SHARED
	${CXCORE_SRC}
	${CV_SRC}
	${CVAUX_SRC}
	${ML_SRC}
	${HIGHGUI_SRC}
)

set_target_properties(OpenCVLib
	PROPERTIES OUTPUT_NAME "opencv110"
)

target_link_libraries(OpenCVLib
	Comctl32 Vfw32 Ole32 Strmiids OleAut32 Uuid
)

install(TARGETS OpenCVLib
	RUNTIME DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}
	LIBRARY DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}
	ARCHIVE DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}
)

#------------------------------------------------------------------------------

add_executable(hello
	./examples/hello.cpp
)
target_link_libraries(hello
	OpenCVLib
)

add_executable(video
	./examples/video.cpp
)
target_link_libraries(video
	OpenCVLib
)

add_executable(cam
	./examples/cam.cpp
)
target_link_libraries(cam
	OpenCVLib
)


#------------------------------------------------------------------------------
