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
// Copyright (C) 2000, Intel Corporation, all rights reserved.
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


/****************************************************************************************/
/*                         Dynamic detection and loading of IPP modules                 */
/****************************************************************************************/

#include "_cxcore.h"

#if defined _MSC_VER && _MSC_VER >= 1200
#pragma warning( disable: 4115 )        /* type definition in () */
#endif

#if defined _MSC_VER && defined WIN64 && !defined EM64T
#pragma optimize( "", off )
#endif

#if defined WIN32 || defined WIN64
#include <windows.h>
#else
#include <dlfcn.h>
#include <sys/time.h>
#endif

#include <string.h>
#include <stdio.h>
#include <ctype.h>

#define CV_PROC_GENERIC             0
#define CV_PROC_SHIFT               10
#define CV_PROC_ARCH_MASK           ((1 << CV_PROC_SHIFT) - 1)
#define CV_PROC_IA32_GENERIC        1
#define CV_PROC_IA32_WITH_MMX       (CV_PROC_IA32_GENERIC|(2 << CV_PROC_SHIFT))
#define CV_PROC_IA32_WITH_SSE       (CV_PROC_IA32_GENERIC|(3 << CV_PROC_SHIFT))
#define CV_PROC_IA32_WITH_SSE2      (CV_PROC_IA32_GENERIC|(4 << CV_PROC_SHIFT))
#define CV_PROC_IA64                2
#define CV_PROC_EM64T               3
#define CV_GET_PROC_ARCH(model)     ((model) & CV_PROC_ARCH_MASK)

typedef struct CvProcessorInfo
{
    int model;
    int count;
    double frequency; // clocks per microsecond
}
CvProcessorInfo;

#undef MASM_INLINE_ASSEMBLY

#if defined WIN32 && !defined  WIN64

#if defined _MSC_VER
#define MASM_INLINE_ASSEMBLY 1
#elif defined __BORLANDC__

#if __BORLANDC__ >= 0x560
#define MASM_INLINE_ASSEMBLY 1
#endif

#endif

#endif

/*
   determine processor type
*/
static void
icvInitProcessorInfo( CvProcessorInfo* cpu_info )
{
    memset( cpu_info, 0, sizeof(*cpu_info) );
    cpu_info->model = CV_PROC_GENERIC;

#if defined WIN32 || defined WIN64

#ifndef PROCESSOR_ARCHITECTURE_AMD64
#define PROCESSOR_ARCHITECTURE_AMD64 9
#endif

#ifndef PROCESSOR_ARCHITECTURE_IA32_ON_WIN64
#define PROCESSOR_ARCHITECTURE_IA32_ON_WIN64 10
#endif

    SYSTEM_INFO sys;
    LARGE_INTEGER freq;

    GetSystemInfo( &sys );

    if( sys.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL &&
        sys.dwProcessorType == PROCESSOR_INTEL_PENTIUM && sys.wProcessorLevel >= 6 )
    {
        int version = 0, features = 0, family = 0;
        int id = 0;
        HKEY key = 0;

        cpu_info->count = (int)sys.dwNumberOfProcessors;
        unsigned long val = 0, sz = sizeof(val);

        if( RegOpenKeyEx( HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\SYSTEM\\CentralProcessor\\0\\",
            0, KEY_QUERY_VALUE, &key ) >= 0 )
        {
            if( RegQueryValueEx( key, "~MHz", 0, 0, (uchar*)&val, &sz ) >= 0 )
                cpu_info->frequency = (double)val;
            RegCloseKey( key );
        }

#ifdef MASM_INLINE_ASSEMBLY
        __asm
        {
            /* use CPUID to determine the features supported */
            pushfd
            mov   eax, 1
            push  ebx
            push  esi
            push  edi
#ifdef __BORLANDC__
            db 0fh
            db 0a2h
#else
            _emit 0x0f
            _emit 0xa2
#endif
            pop   edi
            pop   esi
            pop   ebx
            mov   version, eax
            mov   features, edx
            popfd
        }
#elif defined WIN32 && __GNUC__ > 2
        asm volatile
        (
            "movl $1,%%eax\n\t"
            ".byte 0x0f; .byte 0xa2\n\t"
            "movl %%eax, %0\n\t"
            "movl %%edx, %1\n\t"
            : "=r"(version), "=r" (features)
            :
            : "%ebx", "%esi", "%edi"
        );
#else
        {
            static const char cpuid_code[] =
                "\x53\x56\x57\xb8\x01\x00\x00\x00\x0f\xa2\x5f\x5e\x5b\xc3";
            typedef int64 (CV_CDECL * func_ptr)(void);
            func_ptr cpuid = (func_ptr)(void*)cpuid_code;
            int64 cpuid_val = cpuid();
            version = (int)cpuid_val;
            features = (int)(cpuid_val >> 32);
        }
#endif

        #define ICV_CPUID_M6     ((1<<15)|(1<<23))  /* cmov + MMX */
        #define ICV_CPUID_A6     ((1<<25)|ICV_CPUID_M6) /* <all above> + SSE */
        #define ICV_CPUID_W7     ((1<<26)|ICV_CPUID_A6) /* <all above> + SSE2 */

        family = (version >> 8) & 15;
        if( family >= 6 && (features & ICV_CPUID_M6) != 0 ) /* Pentium II or higher */
            id = features & ICV_CPUID_W7;

        cpu_info->model = id == ICV_CPUID_W7 ? CV_PROC_IA32_WITH_SSE2 :
                          id == ICV_CPUID_A6 ? CV_PROC_IA32_WITH_SSE :
                          id == ICV_CPUID_M6 ? CV_PROC_IA32_WITH_MMX :
                          CV_PROC_IA32_GENERIC;
    }
    else
    {
#if defined EM64T
        if( sys.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 )
            cpu_info->model = CV_PROC_EM64T;
#elif defined WIN64
        if( sys.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64 )
            cpu_info->model = CV_PROC_IA64;
#endif
        if( QueryPerformanceFrequency( &freq ) )
            cpu_info->frequency = (double)freq.QuadPart;
    }
#else
    cpu_info->frequency = 1;

#ifdef __x86_64__
    cpu_info->model = CV_PROC_EM64T;
#elif defined __ia64__
    cpu_info->model = CV_PROC_IA64;
#elif !defined __i386__
    cpu_info->model = CV_PROC_GENERIC;
#else
    cpu_info->model = CV_PROC_IA32_GENERIC;
    
    // reading /proc/cpuinfo file (proc file system must be supported)
    FILE *file = fopen( "/proc/cpuinfo", "r" );

    if( file )
    {
        char buffer[1024];
        int max_size = sizeof(buffer)-1;

        for(;;)
        {
            const char* ptr = fgets( buffer, max_size, file );
            if( !ptr )
                break;
            if( strncmp( buffer, "flags", 5 ) == 0 )
            {
                if( strstr( buffer, "mmx" ) && strstr( buffer, "cmov" ))
                {
                    cpu_info->model = CV_PROC_IA32_WITH_MMX;
                    if( strstr( buffer, "xmm" ) || strstr( buffer, "sse" ))
                    {
                        cpu_info->model = CV_PROC_IA32_WITH_SSE;
                        if( strstr( buffer, "emm" ))
                            cpu_info->model = CV_PROC_IA32_WITH_SSE2;
                    }
                }
            }
            else if( strncmp( buffer, "cpu MHz", 7 ) == 0 )
            {
                char* pos = strchr( buffer, ':' );
                if( pos )
                    cpu_info->frequency = strtod( pos + 1, &pos );
            }
        }

        fclose( file );
        if( CV_GET_PROC_ARCH(cpu_info->model) != CV_PROC_IA32_GENERIC )
            cpu_info->frequency = 1;
        else
            assert( cpu_info->frequency > 1 );
    }
#endif
#endif
}


CV_INLINE const CvProcessorInfo*
icvGetProcessorInfo()
{
    static CvProcessorInfo cpu_info;
    static int init_cpu_info = 0;
    if( !init_cpu_info )
    {
        icvInitProcessorInfo( &cpu_info );
        init_cpu_info = 1;
    }
    return &cpu_info;
}


/****************************************************************************************/
/*                               Make functions descriptions                            */
/****************************************************************************************/

#undef IPCVAPI_EX
#define IPCVAPI_EX(type,func_name,names,modules,arg) \
    { (void**)&func_name##_p, (void*)(size_t)-1, names, modules, 0 },

#undef IPCVAPI_C_EX
#define IPCVAPI_C_EX(type,func_name,names,modules,arg) \
    { (void**)&func_name##_p, (void*)(size_t)-1, names, modules, 0 },

static CvPluginFuncInfo cxcore_ipp_tab[] =
{
#undef _CXCORE_IPP_H_
#include "_cxipp.h"
#undef _CXCORE_IPP_H_
    {0, 0, 0, 0, 0}
};


/*
   determine processor type, load appropriate dll and
   initialize all function pointers
*/
#if defined WIN32 || defined WIN64
#define DLL_PREFIX ""
#define DLL_SUFFIX ".dll"
#else
#define DLL_PREFIX "lib"
#define DLL_SUFFIX ".so"
#define LoadLibrary(name) dlopen(name, RTLD_LAZY)
#define FreeLibrary(name) dlclose(name)
#define GetProcAddress dlsym
typedef void* HMODULE;
#endif

#if 0 /*def _DEBUG*/
#define DLL_DEBUG_FLAG "d"
#else
#define DLL_DEBUG_FLAG ""
#endif

#define VERBOSE_LOADING 0

#if VERBOSE_LOADING
#define ICV_PRINTF(args)  printf args; fflush(stdout)
#else
#define ICV_PRINTF(args)
#endif

typedef struct CvPluginInfo
{
    const char* basename;
    HMODULE handle;
    char name[100];
}
CvPluginInfo;

static CvPluginInfo plugins[CV_PLUGIN_MAX];
static CvModuleInfo cxcore_info = { 0, "cxcore", CV_VERSION, cxcore_ipp_tab };

CvModuleInfo *CvModule::first = 0, *CvModule::last = 0;

CvModule::CvModule( CvModuleInfo* _info )
{
    cvRegisterModule( _info );
    info = last;
}

CvModule::~CvModule()
{
    if( info )
    {
        CvModuleInfo* p = first;
        for( ; p != 0 && p->next != info; p = p->next )
            ;
        if( p )
            p->next = info->next;
        if( first == info )
            first = info->next;
        if( last == info )
            last = p;
        cvFree( &info );
        info = 0;
    }
}

static int
icvUpdatePluginFuncTab( CvPluginFuncInfo* func_tab )
{
    int i, loaded_functions = 0;

    // 1. reset pointers
    for( i = 0; func_tab[i].func_addr != 0; i++ )
    {
        if( func_tab[i].default_func_addr == (void*)(size_t)-1 )
            func_tab[i].default_func_addr = *func_tab[i].func_addr;
        else
            *func_tab[i].func_addr = func_tab[i].default_func_addr;
        func_tab[i].loaded_from = 0;
    }

    // ippopencv substitutes all the other IPP modules
    if( plugins[CV_PLUGIN_OPTCV].handle != 0 )
    {
        for( i = 2; i < CV_PLUGIN_MKL; i++ )
        {
            assert( plugins[i].handle == 0 );
            plugins[i].handle = plugins[CV_PLUGIN_OPTCV].handle;
        }
    }

    // 2. try to find corresponding functions in ipp* and reassign pointers to them
    for( i = 0; func_tab[i].func_addr != 0; i++ )
    {
    #if defined _MSC_VER && _MSC_VER >= 1200
        #pragma warning( disable: 4054 4055 ) /* converting pointers to code<->data */
    #endif
        char name[100];
        int j = 0, idx = 0;

        assert( func_tab[i].loaded_from == 0 );

        if( func_tab[i].search_modules )
        {
            uchar* addr = 0;
            const char* name_ptr = func_tab[i].func_names;

            for( ; j < 10 && name_ptr; j++ )
            {
                const char* name_start = name_ptr;
                const char* name_end;
                while( !isalpha(name_start[0]) && name_start[0] != '\0' )
                    name_start++;
                if( !name_start[0] )
                    name_start = 0;
                name_end = name_start ? strchr( name_start, ',' ) : 0;
                idx = (func_tab[i].search_modules / (1<<j*4)) % CV_PLUGIN_MAX;

                if( plugins[idx].handle != 0 && name_start )
                {
                    if( name_end != 0 )
                    {
                        strncpy( name, name_start, name_end - name_start );
                        name[name_end - name_start] = '\0';
                    }
                    else
                        strcpy( name, name_start );

                    addr = (uchar*)GetProcAddress( plugins[idx].handle, name );
                    if( addr )
                        break;
                }
                name_ptr = name_end;
            }

            if( addr )
            {
            /*#ifdef WIN32
                while( *addr == 0xE9 )
                    addr += 5 + *((int*)(addr + 1));
            #endif*/
                *func_tab[i].func_addr = addr;
                func_tab[i].loaded_from = idx; // store index of the module
                                                   // that contain the loaded function
                loaded_functions++;
                ICV_PRINTF(("%s: \t%s\n", name, plugins[idx].name ));
            }

            #if defined _MSC_VER && _MSC_VER >= 1200
                #pragma warning( default: 4054 4055 )
            #endif
        }
    }

#if VERBOSE_LOADING
    {
    int not_loaded = 0;
    ICV_PRINTF(("\nTotal loaded: %d\n\n", loaded_functions ));
    printf( "***************************************************\nNot loaded ...\n\n" );
    for( i = 0; func_tab[i].func_addr != 0; i++ )
        if( !func_tab[i].loaded_from )
        {
            ICV_PRINTF(( "%s\n", func_tab[i].func_names ));
            not_loaded++;
        }

    ICV_PRINTF(("\nTotal: %d\n", not_loaded ));
    }
#endif

    if( plugins[CV_PLUGIN_OPTCV].handle != 0 )
    {
        for( i = 2; i < CV_PLUGIN_MKL; i++ )
            plugins[i].handle = 0;
    }

    return loaded_functions;
}


CV_IMPL int
cvRegisterModule( const CvModuleInfo* module )
{
    CvModuleInfo* module_copy = 0;

    CV_FUNCNAME( "cvRegisterModule" );

    __BEGIN__;

    size_t name_len, version_len;

    CV_ASSERT( module != 0 && module->name != 0 && module->version != 0 );

    name_len = strlen(module->name);
    version_len = strlen(module->version);

    CV_CALL( module_copy = (CvModuleInfo*)cvAlloc( sizeof(*module_copy) +
                                        name_len + 1 + version_len + 1 ));

    *module_copy = *module;
    module_copy->name = (char*)(module_copy + 1);
    module_copy->version = (char*)(module_copy + 1) + name_len + 1;

    memcpy( (void*)module_copy->name, module->name, name_len + 1 );
    memcpy( (void*)module_copy->version, module->version, version_len + 1 );
    module_copy->next = 0;

    if( CvModule::first == 0 )
        CvModule::first = module_copy;
    else
        CvModule::last->next = module_copy;
    CvModule::last = module_copy;

    if( CvModule::first == CvModule::last )
    {
        CV_CALL( cvUseOptimized(1));
    }
    else
    {
        CV_CALL( icvUpdatePluginFuncTab( module_copy->func_tab ));
    }

    __END__;

    if( cvGetErrStatus() < 0 && module_copy )
        cvFree( &module_copy );

    return module_copy ? 0 : -1;
}


CV_IMPL int
cvUseOptimized( int load_flag )
{
    int i, loaded_modules = 0, loaded_functions = 0;
    CvModuleInfo* module;
    const CvProcessorInfo* cpu_info = icvGetProcessorInfo();
    int arch = CV_GET_PROC_ARCH(cpu_info->model);
    
    // TODO: implement some more elegant way
    // to find the latest and the greatest IPP/MKL libraries
    static const char* opencv_sfx[] = { "100", "099", "097", 0 };
    static const char* ipp_sfx_ia32[] = { "-6.1", "-6.0", "-5.3", "-5.2", "-5.1", "", 0 };
    static const char* ipp_sfx_ia64[] = { "64-6.1", "64-6.0", "64-5.3", "64-5.2", "64-5.1", "64", 0 };
    static const char* ipp_sfx_em64t[] = { "em64t-6.1", "em64t-6.0", "em64t-5.3", "em64t-5.2", "em64t-5.1", "em64t", 0 };
    static const char* mkl_sfx_ia32[] = { "p4", "p3", "def", 0 };
    static const char* mkl_sfx_ia64[] = { "i2p", "itp", 0 };
    static const char* mkl_sfx_em64t[] = { "def", 0 };
    const char** ipp_suffix = arch == CV_PROC_IA64 ? ipp_sfx_ia64 :
                              arch == CV_PROC_EM64T ? ipp_sfx_em64t : ipp_sfx_ia32;
    const char** mkl_suffix = arch == CV_PROC_IA64 ? mkl_sfx_ia64 :
                              arch == CV_PROC_EM64T ? mkl_sfx_em64t : mkl_sfx_ia32;

    for( i = 0; i < CV_PLUGIN_MAX; i++ )
        plugins[i].basename = 0;
    plugins[CV_PLUGIN_NONE].basename = 0;
    plugins[CV_PLUGIN_NONE].name[0] = '\0';
    plugins[CV_PLUGIN_OPTCV].basename = "ippopencv";
    plugins[CV_PLUGIN_IPPCV].basename = "ippcv";
    plugins[CV_PLUGIN_IPPI].basename = "ippi";
    plugins[CV_PLUGIN_IPPS].basename = "ipps";
    plugins[CV_PLUGIN_IPPVM].basename = "ippvm";
    plugins[CV_PLUGIN_IPPCC].basename = "ippcc";
    plugins[CV_PLUGIN_MKL].basename = "mkl_";

    // try to load optimized dlls
    for( i = 1; i < CV_PLUGIN_MAX; i++ )
    {
        // unload previously loaded optimized modules
        if( plugins[i].handle )
        {
            FreeLibrary( plugins[i].handle );
            plugins[i].handle = 0;
        }

        // do not load regular IPP modules if the custom merged IPP module is already found.
        if( i < CV_PLUGIN_MKL && load_flag && plugins[CV_PLUGIN_OPTCV].handle != 0 )
            continue;

        if( load_flag && plugins[i].basename &&
            (arch == CV_PROC_IA32_GENERIC || arch == CV_PROC_IA64 || arch == CV_PROC_EM64T) )
        {
            const char** suffix = i == CV_PLUGIN_OPTCV ? opencv_sfx :
                            i < CV_PLUGIN_MKL ? ipp_suffix : mkl_suffix;
            if( suffix == mkl_sfx_ia32 )
            {
                if( !(cpu_info->model & CV_PROC_IA32_WITH_SSE2) )
                    suffix++;
                if( !(cpu_info->model & CV_PROC_IA32_WITH_SSE) )
                    suffix++;
            }

            for( ; *suffix != 0; suffix++ )
            {
                sprintf( plugins[i].name, DLL_PREFIX "%s%s" DLL_DEBUG_FLAG DLL_SUFFIX,
                    plugins[i].basename, *suffix );

                ICV_PRINTF(("loading %s...\n", plugins[i].name ));
                plugins[i].handle = LoadLibrary( plugins[i].name );
                if( plugins[i].handle != 0 )
                {
                    ICV_PRINTF(("%s loaded\n", plugins[i].name ));
                    loaded_modules++;
                    break;
                }
                #ifndef WIN32
                // temporary workaround for MacOSX 
                sprintf( plugins[i].name, DLL_PREFIX "%s%s" DLL_DEBUG_FLAG ".dylib",
                    plugins[i].basename, *suffix );

                ICV_PRINTF(("loading %s...\n", plugins[i].name ));
                plugins[i].handle = LoadLibrary( plugins[i].name );
                if( plugins[i].handle != 0 )
                {
                    ICV_PRINTF(("%s loaded\n", plugins[i].name ));
                    loaded_modules++;
                    break;
                }
                #endif
            }
        }
    }

    for( module = CvModule::first; module != 0; module = module->next )
        loaded_functions += icvUpdatePluginFuncTab( module->func_tab );

    return loaded_functions;
}

CvModule cxcore_module( &cxcore_info );

CV_IMPL void
cvGetModuleInfo( const char* name, const char **version, const char **plugin_list )
{
    static char joint_verinfo[1024] = "";
    static char plugin_list_buf[1024] = "";

    CV_FUNCNAME( "cvGetLibraryInfo" );

    if( version )
        *version = 0;

    if( plugin_list )
        *plugin_list = 0;

    __BEGIN__;

    CvModuleInfo* module;

    if( version )
    {
        if( name )
        {
            size_t i, name_len = strlen(name);

            for( module = CvModule::first; module != 0; module = module->next )
            {
                if( strlen(module->name) == name_len )
                {
                    for( i = 0; i < name_len; i++ )
                    {
                        int c0 = toupper(module->name[i]), c1 = toupper(name[i]);
                        if( c0 != c1 )
                            break;
                    }
                    if( i == name_len )
                        break;
                }
            }
            if( !module )
                CV_ERROR( CV_StsObjectNotFound, "The module is not found" );

            *version = module->version;
        }
        else
        {
            char* ptr = joint_verinfo;

            for( module = CvModule::first; module != 0; module = module->next )
            {
                sprintf( ptr, "%s: %s%s", module->name, module->version, module->next ? ", " : "" );
                ptr += strlen(ptr);
            }

            *version = joint_verinfo;
        }
    }

    if( plugin_list )
    {
        char* ptr = plugin_list_buf;
        int i;

        for( i = 0; i < CV_PLUGIN_MAX; i++ )
            if( plugins[i].handle != 0 )
            {
                sprintf( ptr, "%s, ", plugins[i].name );
                ptr += strlen(ptr);
            }

        if( ptr > plugin_list_buf )
        {
            ptr[-2] = '\0';
            *plugin_list = plugin_list_buf;
        }
        else
            *plugin_list = "";
    }

    __END__;
}


typedef int64 (CV_CDECL * rdtsc_func)(void);

/* helper functions for RNG initialization and accurate time measurement */
CV_IMPL  int64  cvGetTickCount( void )
{
    const CvProcessorInfo* cpu_info = icvGetProcessorInfo();

    if( cpu_info->frequency > 1 &&
        CV_GET_PROC_ARCH(cpu_info->model) == CV_PROC_IA32_GENERIC )
    {
#ifdef MASM_INLINE_ASSEMBLY
    #ifdef __BORLANDC__
        __asm db 0fh
        __asm db 31h
    #else
        __asm _emit 0x0f;
        __asm _emit 0x31;
    #endif
#elif (defined __GNUC__ || defined CV_ICC) && defined __i386__
        int64 t;
        asm volatile (".byte 0xf; .byte 0x31" /* "rdtsc" */ : "=A" (t));
        return t;
#else
        static const char code[] = "\x0f\x31\xc3";
        rdtsc_func func = (rdtsc_func)(void*)code;
        return func();
#endif
    }
    else
    {
#if defined WIN32 || defined WIN64
        LARGE_INTEGER counter;
        QueryPerformanceCounter( &counter );
        return (int64)counter.QuadPart;
#else
        struct timeval tv;
        struct timezone tz;
        gettimeofday( &tv, &tz );
        return (int64)tv.tv_sec*1000000 + tv.tv_usec;
#endif
    }
}

CV_IMPL  double  cvGetTickFrequency()
{
    return icvGetProcessorInfo()->frequency;
}


static int icvNumThreads = 0;
static int icvNumProcs = 0;

CV_IMPL int cvGetNumThreads(void)
{
    if( !icvNumProcs )
        cvSetNumThreads(0);
    return icvNumThreads;
}

CV_IMPL void cvSetNumThreads( int threads )
{
    if( !icvNumProcs )
    {
#ifdef _OPENMP
        icvNumProcs = omp_get_num_procs();
        icvNumProcs = MIN( icvNumProcs, CV_MAX_THREADS );
#else
        icvNumProcs = 1;
#endif
    }

#ifdef _OPENMP
    if( threads <= 0 )
        threads = icvNumProcs;
    //else
    //    threads = MIN( threads, icvNumProcs );

    icvNumThreads = threads;
#else
    icvNumThreads = 1;
#endif
}


CV_IMPL int cvGetThreadNum(void)
{
#ifdef _OPENMP
    return omp_get_thread_num();
#else
    return 0;
#endif
}


/* End of file. */
