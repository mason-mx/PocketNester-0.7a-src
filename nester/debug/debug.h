// debug for WindowsCE

#ifndef _DEBUG_H_
#define _DEBUG_H_

#pragma warning(disable:4005)

// manual override

#if 0 //__INTEL_COMPILER

#define THROW_EXCEPTION throw("error")
#define PN_TRY try
#define PN_CATCH catch(...)

#else

#define THROW_EXCEPTION RaiseException(0, EXCEPTION_NONCONTINUABLE, 0, NULL);
#define PN_TRY __try
#define PN_CATCH __except(EXCEPTION_EXECUTE_HANDLER)

#endif

inline void errorlog(char* sz)
{
	FILE* fp = fopen("\\nesterlog.txt", "a+");
	fprintf(fp, sz);
	fclose(fp);
}

#undef ASSERT
#define ASSERT(EXPR)
#define LOG(MSG)

#define IFDEBUG(X)

#endif
