/****************************************************************************
*																			*
*							cryptlib Debug Header File 						*
*						Copyright Peter Gutmann 1992-2014					*
*																			*
****************************************************************************/

#ifndef _DEBUG_DEFINED

#define _DEBUG_DEFINED

/****************************************************************************
*																			*
*								assert() Handling							*
*																			*
****************************************************************************/

/* Older WinCE environments don't support assert() because there's no 
   console and no other support for it in the runtime (the documentation
   claims there's at least an _ASSERT/_ASSERTE available, but this isn't 
   present in many systems such as PocketPC), so we use it if it's available 
   and otherwise kludge it using NKDbgPrintfW().

   Note that (in theory) the version check isn't reliable since we should be 
   checking for the development environment version rather than the target 
   OS version, however in practice compiler/SDK version == OS version unless 
   you seriously enjoy pain, and in any case it's not really possible to 
   differentiate between eVC++ 3.0 and 4.0 - the SHx, MIPS, and ARM compilers 
   at least report 120{1|2} for 3.0 and 1200 for 3.0, but the x86 compiler 
   reports 1200 for both 3.0 and 4.0 even though it's a different build, 
   0.8168 vs. 0.8807 */

#if defined( __WINCE__ ) && _WIN32_WCE < 400
  #if 0	/* Too nonportable, see comment above */
	#ifndef _ASSERTE
	  #ifdef NDEBUG
		#define _ASSERTE( x )
	  #else
		#define _ASSERTE( expr ) \
				( void )( ( expr ) || ( NKDbgPrintfW( #expr ) ) )
	  #endif /* Debug vs. non-debug builds */
	#endif /* _ASSERTE available */
	#define assert( expr )	_ASSERTE( expr )
  #else
	#ifdef NDEBUG
	  #define assert( x )
	#else
	  #define assert( x ) \
			  DEBUGMSG( !( x ), ( TEXT( "Assert failed in %s line %d: %s" ), TEXT( __FILE__ ), __LINE__, TEXT( #x ) ) )
	#endif /* Debug vs. non-debug builds */
  #endif /* 0 */
#else
  #include <assert.h>
#endif /* Systems without assert() */

/* Assertions can't be used to detect post-preprocessing compile-time 
   problems, typically using expressions involving sizeof().  Newer versions
   of the C standard added static_assert() to deal with this, since only
   recent compilers support this we have to enable it as required, in some
   cases using preprocessor kludges.
   
   In addition to the standard static_assert() we also define an alternative,
   static_assert_opt(), for cases where the compiler isn't tough enough to
   handle the standard static_assert().  This occurs with expressions like
   "foo"[ 1 ] == 'o' */

#define ASSERT_CONCAT_( a, b )	a##b
#define ASSERT_CONCAT( a, b )	ASSERT_CONCAT_( a, b )
#ifndef __COUNTER__
  /* We need a unique name for each static assertion, ideally we'd like to 
	 use a unique value for this but if it's not available (it's supported 
	 in the major platforms VC++ and gcc) then we use the next-best thing */
  #define __COUNTER__			__LINE__
#endif /* __COUNTER__ */
#if defined( _MSC_VER ) 
  #if VC_GE_2010( _MSC_VER )	
	/* Built into VC++ 2010 and up.  This is supposedly partially supported 
	   in VS 2008 as '_STATIC_ASSERT( expr )' but the support isn't complete 
	   enough to rely on it so we only enable it for VS 2010 and up, for 
	   which it's definitely present */
  #else
	#define static_assert( expr, string ) \
			{ enum { ASSERT_CONCAT( static_assert_, __COUNTER__ ) = 1 / ( !!( expr ) ) }; }
  #endif /* VC++ versions */
  #define static_assert_opt( expr, string ) \
		  assert( expr )
#elif defined( __GNUC__ ) && \
	  ( ( __GNUC__ == 4 && __GNUC_MINOR__ >= 7 && defined( static_assert ) ) || \
		( __GNUC__ >= 5 ) ) 
  /* Supposedly built into gcc 4.5 and above (as usual for new gcc features
	 this isn't really documented, but web comments indicate that it should 
	 be present in 4.5 and above), however trying this with 4.5 produces
	 assorted errors indicating that it isn't actually supported.  The GNU
	 standards-support docs indicate its presence in an even earlier version,
	 4.3, which doesn't have it either.  It is present in the <assert.h> for 
	 gcc 4.9 and has an effect there, and sometimes works in 4.7 and 4.8 as 
	 well.  To protect ourselves, we assume that it's present in 4.7 and 
	 above but also check for static_assert being defined (which maps the
	 C11 keyword _Static_assert to the C++-style static_assert()), which 
	 indicates that it really is present.  For 5.x and above we just assume 
	 that it's present */
  #define static_assert_opt( expr, string ) \
		  assert( expr )
#elif defined( __llvm__ )
  /* Supported in LLVM/clang, however it's present using the C11 
     _Static_assert name rather than the C++11 static_assert one.  Some 
	 assert.h's will map static_assert to _Static_assert, but since this 
	 depends on which assert.h is used we optionally do it ourselves here */
  #ifndef static_assert
	#define static_assert	_Static_assert
  #endif /* static_assert */
  #define static_assert_opt( expr, string ) \
		  assert( expr )
#else
  #define static_assert( expr, string ) \
		  { enum { ASSERT_CONCAT( static_assert_, __COUNTER__ ) = 1 / ( !!( expr ) ) }; }
  #define static_assert_opt( expr, string ) \
		  assert( expr )
#endif /* VC++ vs. other compilers */

/* Force an assertion failure via assert( DEBUG_WARN ) */

#define DEBUG_WARN				0

/* Under VC++ 6 assert() can randomly stop working so that only the abort() 
   portion still functions, making it impossible to find out what went wrong. 
   To deal with this we redefine the assert() macro to call our own 
   assertion handler */

#if defined( __WIN32__ ) && !defined( NDEBUG ) && VC_LE_VC6( _MSC_VER )

void vc6assert( const char *exprString, const char *fileName, 
				const int lineNo );

#undef assert
#define assert( expr ) \
		( void )( ( expr ) || ( vc6assert( #expr, __FILE__, __LINE__ ), 0 ) )

#endif /* VC++ 6.0 */

/****************************************************************************
*																			*
*						Debugging Diagnostic Functions						*
*																			*
****************************************************************************/

/* As a safeguard the following debugging functions are only enabled by 
   default in the Win32 debug version to prevent them from being 
   accidentally enabled in any release version, or for the Suite B test
   build, which is expected to dump diagnostic information to the debug
   output.  Note that the required support functions are present for non-
   Windows OSes, they're just disabled at this level for safety purposes 
   because the cl32.dll with debug options safely disabled is included with 
   the release while there's no control over which version gets built for 
   other releases.  If you know what you're doing then you can enable the 
   debug-dump options by defining the following when you build the code */

#if !defined( NDEBUG ) && \
	( defined( __WIN32__ ) || defined( CONFIG_SUITEB_TESTS ) )
  #define DEBUG_DIAGNOSTIC_ENABLE
#endif /* 0 */

/* Debugging printf() that sends its output to the debug output.  Under
   Windows and eCos this is the debugger, under Unix it's the next-best
   thing, stderr, on anything else we have to use stdout.  In addition 
   since we sometimes need to output pre-formatted strings (which may 
   contain '%' signs interpreted by printf()) we also provide an alternative 
   that just outputs a fixed text string */

#if ( defined( NDEBUG ) || defined( CONFIG_FUZZ ) ) && \
	!defined( DEBUG_DIAGNOSTIC_ENABLE ) 
  /* Debugging diagnostics are disabled for release builds and also for
	 fuzzing builds, which don't do anything with output and for which
	 lots of spew just slows things down unnecessarily */
  #define DEBUG_PRINT( x )
  #define DEBUG_PRINT_COND( x, y )
  #define DEBUG_OUT( string )
#elif defined( __WIN32__ )
  #define DEBUG_OUT( string )	OutputDebugString( string )
#elif defined( __WINCE__ )
  #define DEBUG_OUT( string )	NKDbgPrintfW( L"%s", string )
#elif defined( __ECOS__ )
  #define DEBUG_PRINT( x )		diag_printf x
  #define DEBUG_PRINT_COND( c, x )	if( c ) diag_printf x
  #define DEBUG_OUT( string )	diag_printf( "%s", string )
#elif defined( __UNIX__ )
  #define DEBUG_OUT( string )	debugPrintf( "%s", string )
#else
  #include <stdio.h>			/* Needed for printf() */
  #define DEBUG_PRINT( x )		printf x
  #define DEBUG_PRINT_COND( c, x )	if( c ) printf x
  #define DEBUG_OUT( string )	printf( "%s", string )
#endif /* OS-specific diagnostic functions */
#ifndef DEBUG_PRINT
  int debugPrintf( const char *format, ... );

  #define DEBUG_PRINT( x )		debugPrintf x
  #define DEBUG_PRINT_COND( c, x )	if( c ) debugPrintf x
#endif /* !DEBUG_PRINT */

/* Sometimes we need to add additional debugging code that's needed only in 
   the debug version, the following macro allows debug-only operations to be 
   inserted */

#if defined( NDEBUG )
  #define DEBUG_OP( x )
#else
  #define DEBUG_OP( x )			x
#endif /* Debug build */

/* Output an I-am-here to the debugging outout (see above), useful when 
   tracing errors in code without debug symbols available */

#if defined( __GNUC__ ) || ( defined( _MSC_VER ) && VC_GE_2005( _MSC_VER ) )
  /* Older versions of gcc don't support the current syntax */
  #if defined( __GNUC__ ) && ( __STDC_VERSION__ < 199901L )
	#if __GNUC__ >= 2
	  #define __FUNCTION__	__func__ 
	#else
	  #define __FUNCTION__	"<unknown>"
	#endif /* gcc 2.x or newer */
  #endif /* gcc without __FUNCTION__ support */

  #define DEBUG_ENTER()	DEBUG_PRINT(( "Enter %s:%s:%d.\n", __FILE__, __FUNCTION__, __LINE__ ))
  #define DEBUG_IN()	DEBUG_PRINT(( "In    %s:%s:%d.\n", __FILE__, __FUNCTION__, __LINE__ ))
  #define DEBUG_EXIT()	DEBUG_PRINT(( "Exit  %s:%s:%d, status %d.\n", __FILE__, __FUNCTION__, __LINE__, status ))
  #define DEBUG_EXIT_NONE()	\
						DEBUG_PRINT(( "Exit  %s:%s:%d.\n", __FILE__, __FUNCTION__, __LINE__ ))

  #define DEBUG_DIAG( x ) \
						DEBUG_PRINT(( "%s:%s:%d: ", __FILE__, __FUNCTION__, __LINE__ )); \
						DEBUG_PRINT( x ); \
						DEBUG_PRINT(( ".\n" ))
#else
  #define DEBUG_ENTER()	DEBUG_PRINT(( "Enter %s:%d.\n", __FILE__, __LINE__ ))
  #define DEBUG_IN()	DEBUG_PRINT(( "In    %s:%d.\n", __FILE__, __LINE__ ))
  #define DEBUG_EXIT()	DEBUG_PRINT(( "Exit  %s:%d, status %d.\n", __FILE__, __LINE__, status ))
  #define DEBUG_EXIT_NONE()	\
						DEBUG_PRINT(( "Exit  %s:%d.\n", __FILE__, __LINE__ ))

  #define DEBUG_DIAG( x ) \
						DEBUG_PRINT(( "%s:%d: ", __FILE__, __LINE__ )); \
						DEBUG_PRINT( x ); \
						DEBUG_PRINT(( ".\n" ))
#endif /* Compiler-specific diagnotics */

/* Dump a PDU to disk or screen.  The debug functions do the following:

	DEBUG_DUMP_FILE: Writes a block of memory to a file in $TMP, suffix 
		".der".

	DEBUG_DUMP_FILE_OPT: As DEBUG_DUMP_FILE, but only tries to dump the 
		contents if the length is non-zero.  This can happen if we get a
		zero-length from a client or server, for example what's being
		communicated is an error status and there's no content such as
		as a certificate to dump.

	DEBUG_DUMP_CERT: Writes a certificate object to a file, details as for
		DEBUG_DUMP_FILE().

	DEBUG_DUMP_HEX: Creates a hex + text dump of a block of memory along 
		with a length and checksum of the entire buffer, prepended with a 
		supplied text string.

	DEBUG_DUMP_DATA: Creates a hex + text dump of a block of memory, to be 
		used in conjunction with DEBUG_PRINT() for free-format text.

	DEBUG_DUMP_STREAM: As DEBUG_DUMP_DATA() but taking its input from a 
		stream.

	DEBUG_GET_STREAMBYTE: Support function for DEBUG_DUMP_STREAM() that 
		pulls data bytes, typically containing type information, out of a 
		stream for display */

#if defined( NDEBUG ) && !defined( DEBUG_DIAGNOSTIC_ENABLE )
  #define DEBUG_DUMP_FILE( name, data, length )
  #define DEBUG_DUMP_FILE_OPT( name, data, length )
  #define DEBUG_DUMP_CERT( name, cert )
  #define DEBUG_DUMP_HEX( dumpPrefix, dumpBuf, dumpLen )
  #define DEBUG_DUMP_DATA( dumpBuf, dumpLen )
  #define DEBUG_DUMP_STREAM( stream, position, length )
  #define DEBUG_DUMP_STACKTRACE()
  #define DEBUG_GET_STREAMBYTE( stream, position )		0
#else
  #define DEBUG_DUMP_FILE	debugDumpFile
  #define DEBUG_DUMP_FILE_OPT( name, data, length ) \
			if( ( length ) != 0 ) \
				debugDumpFile( ( name ), ( data ), ( length ) )
  #define DEBUG_DUMP_CERT	debugDumpFileCert
  #define DEBUG_DUMP_HEX	debugDumpHex
  #define DEBUG_DUMP_DATA	debugDumpData
  #define DEBUG_DUMP_STREAM	debugDumpStream
  #if defined( __WIN32__ ) && !defined( _WIN64 )
	#define DEBUG_DUMP_STACKTRACE	displayBacktrace
	void displayBacktrace( void );
  #elif defined( __UNIX__ ) && defined( __linux__ )
	#define DEBUG_DUMP_STACKTRACE	displayBacktrace
	void displayBacktrace( void );
  #else
	#define DEBUG_DUMP_STACKTRACE()
  #endif /* OS-specific stack trace display routines */
  #define DEBUG_GET_STREAMBYTE debugGetStreamByte
  STDC_NONNULL_ARG( ( 1, 2 ) ) \
  void debugDumpFile( IN_STRING const char *fileName, 
					  IN_BUFFER( dataLength ) const void *data, 
					  IN_LENGTH_SHORT const int dataLength );
  STDC_NONNULL_ARG( ( 1 ) ) \
  void debugDumpFileCert( IN_STRING const char *fileName, 
						  IN_HANDLE const CRYPT_CERTIFICATE iCryptCert );
  STDC_NONNULL_ARG( ( 1, 2 ) ) \
  void debugDumpHex( IN_STRING const char *prefixString, 
					 IN_BUFFER( dataLength ) const void *data, 
					 IN_LENGTH_SHORT const int dataLength );
  STDC_NONNULL_ARG( ( 1 ) ) \
  void debugDumpData( IN_BUFFER( dataLength ) const void *data, 
					  IN_LENGTH_SHORT const int dataLength );
  STDC_NONNULL_ARG( ( 1 ) ) \
  void debugDumpStream( INOUT /*STREAM*/ void *streamPtr, 
						IN_LENGTH const int position, 
						IN_LENGTH const int length );
  RETVAL_RANGE_NOERROR( 0, 0xFF ) STDC_NONNULL_ARG( ( 1 ) ) \
  int debugGetStreamByte( INOUT /*STREAM*/ void *streamPtr, 
						  IN_LENGTH const int position );
#endif /* Win32 debug */

/* Dump a trace of a double-linked list.  This has to be done as a macro 
   because it requires in-place substitution of code */

#if defined( NDEBUG ) && !defined( DEBUG_DIAGNOSTIC_ENABLE )
  #define DEBUG_DUMP_LIST( label, listHead, listTail, listType )
#else
  #define DEBUG_DUMP_LIST( label, listHead, listTail, listType ) \
		{ \
		listType *listPtr; \
		int count = 0; \
		\
		DEBUG_PRINT(( "%s: Walking list from %lX to %lX.\n", \
					  label, ( listHead ), ( listTail ) )); \
		for( listPtr = ( listHead ); \
			 listPtr != NULL; listPtr = listPtr->next ) \
			{ \
			DEBUG_PRINT(( "  Ptr = %lX, prev = %lX, next = %lX.\n", \
						  listPtr, listPtr->prev, listPtr->next )); \
			count++; \
			} \
		DEBUG_PRINT(( "  List has %d entr%s.\n", count, \
					  ( count != 1 ) ? "ies" : "y" )); \
		}
#endif /* NDEBUG */

/* Support functions that may be needed by the general debug functions */

#if !defined( NDEBUG ) && defined( USE_ERRMSGS ) && \
	defined( DEBUG_DIAGNOSTIC_ENABLE )

const char *getErrorInfoString( ERROR_INFO *errorInfo );

#endif /* !NDEBUG && USE_ERRMSGS && DEBUG_DIAGNOSTIC_ENABLE */

/****************************************************************************
*																			*
*							Memory Debugging Functions						*
*																			*
****************************************************************************/

/* In order to debug memory usage, we can define CONFIG_DEBUG_MALLOC to dump
   memory usage diagnostics to stdout (this would usually be done in the
   makefile).  Without this, the debug malloc just becomes a standard malloc.
   Note that crypt/osconfig.h contains its own debug-malloc() handling for
   the OpenSSL-derived code enabled via USE_BN_DEBUG_MALLOC in osconfig.h,
   and zlib also has its own allocation code (which isn't instrumented for
   diagnostic purposes).  In addition the default mapping to malloc()/free()
   may be overridden in os_spec.h/os_spec.c for embedded systems that don't
   use standard malloc/free, so we only set up the mappings if clAlloc() 
   isn't already defined.

   In addition in order to control on-demand allocation of buffers for
   larger-than-normal data items, we can define CONFIG_NO_DYNALLOC to
   disable this allocation.  This is useful in memory-constrained
   environments where we can't afford to grab chunks of memory at random */

#if defined( CONFIG_DEBUG_MALLOC )
  #undef clAlloc
  #undef clFree
  #define clAlloc( string, size ) \
		  clAllocFn( __FILE__, ( string ), __LINE__, ( size ) )
  #define clFree( string, memblock ) \
		  clFreeFn( __FILE__, ( string ), __LINE__, ( memblock ) )
  void *clAllocFn( const char *fileName, const char *fnName,
				   const int lineNo, size_t size );
  void clFreeFn( const char *fileName, const char *fnName,
				 const int lineNo, void *memblock );
#elif !defined( clAlloc )
  #define clAlloc( string, size )		malloc( size )
  #define clFree( string, memblock )	free( memblock )
#endif /* !CONFIG_DEBUG_MALLOC */
#ifdef CONFIG_NO_DYNALLOC
  #define clDynAlloc( string, size )	NULL
#else
  #define clDynAlloc( string, size )	clAlloc( string, size )
#endif /* CONFIG_NO_DYNALLOC */

/* To provide fault-injection testing capabilities we can have memory 
   allocations fail after a given count, thus exercising a large number of 
   failure code paths that are normally never taken.  The following 
   configuration define enables this fault-malloc(), with the first call 
   setting the allocation call at which failure occurs */

#ifdef CONFIG_FAULT_MALLOC
  #undef clAlloc
  #undef clFree
  #define clAlloc( string, size ) \
		  clFaultAllocFn( __FILE__, ( string ), __LINE__, ( size ) )
  #define clFree( string, memblock )	free( memblock )
  void *clFaultAllocFn( const char *fileName, const char *fnName, 
						const int lineNo, size_t size );
  void clFaultAllocSetCount( const int number );

  /* Some of the should-neven-fail functions like the kernel self-tests
	 include assertions to ensure that any failure raises an immediate 
	 alert.  Since we're explicitly forcing a failure, we don't want to 
	 be alerted to these conditions so we no-op out the assert if we're
	 doing memory fault injection */
  #define assertNoFault( x )
  #define ENSURES_NOFAULT( x )	if( !( x ) ) return( CRYPT_ERROR_MEMORY )
#else
  #define assertNoFault			assert
  #define ENSURES_NOFAULT		ENSURES
#endif /* CONFIG_FAULT_MALLOC */

/****************************************************************************
*																			*
*								Timing Functions							*
*																			*
****************************************************************************/

#if defined( __WINDOWS__ ) || defined( __UNIX__ )

/* High-resolution timing functionality, used to diagnose performance 
   issues */

#if defined( _MSC_VER )
  typedef __int64 HIRES_TIME;
  #define HIRES_FORMAT_SPECIFIER	"%lld"
#elif defined( __GNUC__ )
  typedef long long HIRES_TIME;
  #define HIRES_FORMAT_SPECIFIER	"%lld"
#else
  typedef unsigned long HIRES_TIME;
  #define HIRES_FORMAT_SPECIFIER	"%ld"
#endif /* 32/64-bit time values */

/* Timing support functions.  Call as:

	HIRES_TIME timeVal;

	timeVal = timeDiff( 0 );
	function_to_time();
	result = ( int ) timeDiff( timeVal ); */

HIRES_TIME debugTimeDiff( HIRES_TIME startTime );
int debugTimeDisplay( HIRES_TIME time );

#endif /* __WINDOWS__ || __UNIX__ */
#endif /* _DEBUG_DEFINED */
