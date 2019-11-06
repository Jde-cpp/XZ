#pragma once
#ifdef JDE_XZ_EXPORTS
	#ifdef _MSC_VER 
		#define JDE_XZ __declspec( dllexport )
	#else
		#define JDE_XZ __attribute__((visibility("default")))
	#endif
#else 
	#ifdef _MSC_VER
		#define JDE_XZ __declspec( dllimport )
		#if NDEBUG
			#pragma comment(lib, "Jde.XZ.lib")
		#else
			#pragma comment(lib, "Jde.XZ.lib")
		#endif
	#else
		#define JDE_XZ
	#endif
#endif