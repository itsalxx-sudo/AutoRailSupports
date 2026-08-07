#pragma once

#include "CoreMinimal.h"
#include "Stats/Stats.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

//PERF PROFILING
// Off by default, toggle from the game console (no rebuild):
//   ARS.Debug.Perf             1 = time the mod's heavy scopes and report to LogARSPerf
//   ARS.Debug.PerfThresholdMs  a single scope slower than this logs immediately (default 2, -1 off)
//   ARS.Debug.PerfInterval     seconds between the rolling per-scope summary (default 3)
//   ARS.Debug.PerfDump         console command: print the summary now and reset
// `stat ARS` and the CPU trace compile out of the packaged Shipping mod, so the log-based timing is
// the only profiler that works there

// Defined once in ARSDebug.cpp so every translation unit logs under the same set
AUTORAILSUPPORTS_API DECLARE_LOG_CATEGORY_EXTERN(LogARSClient, Log, All);
AUTORAILSUPPORTS_API DECLARE_LOG_CATEGORY_EXTERN(LogARSGenerator, Log, All);
AUTORAILSUPPORTS_API DECLARE_LOG_CATEGORY_EXTERN(LogARSPerf, Log, All);

// For `stat ARS` and Unreal Insights - editor/Development only, see above
DECLARE_STATS_GROUP(TEXT("AutoRailSupports"), STATGROUP_ARS, STATCAT_Advanced);
DECLARE_CYCLE_STAT_EXTERN(TEXT("Measure blueprint"),  STAT_ARS_Measure,         STATGROUP_ARS, AUTORAILSUPPORTS_API);
DECLARE_CYCLE_STAT_EXTERN(TEXT("Plan path"),          STAT_ARS_PlanPath,        STATGROUP_ARS, AUTORAILSUPPORTS_API);
DECLARE_CYCLE_STAT_EXTERN(TEXT("Resolve settings"),   STAT_ARS_ResolveSettings, STATGROUP_ARS, AUTORAILSUPPORTS_API);
DECLARE_CYCLE_STAT_EXTERN(TEXT("Preview update"),     STAT_ARS_PreviewUpdate,   STATGROUP_ARS, AUTORAILSUPPORTS_API);
DECLARE_CYCLE_STAT_EXTERN(TEXT("Preview fill ISM"),   STAT_ARS_PreviewFill,     STATGROUP_ARS, AUTORAILSUPPORTS_API);
DECLARE_CYCLE_STAT_EXTERN(TEXT("Execute stamp job"),  STAT_ARS_ExecuteStampJob, STATGROUP_ARS, AUTORAILSUPPORTS_API);
DECLARE_CYCLE_STAT_EXTERN(TEXT("Hologram cost hook"), STAT_ARS_CostHook,        STATGROUP_ARS, AUTORAILSUPPORTS_API);

namespace ARSDebug
{
	AUTORAILSUPPORTS_API bool  Perf();
	AUTORAILSUPPORTS_API float PerfThresholdMs();
	// Feed one scope timing into the rolling report. Also logs straight away when over the threshold
	AUTORAILSUPPORTS_API void RecordPerf(const TCHAR* ScopeName, double DurationMs);
	// Call once per frame from something that ticks. Emits the summary every ARS.Debug.PerfInterval seconds
	AUTORAILSUPPORTS_API void TickPerfReport();

	// RAII timer behind ARS_SCOPE_PERF, active only while Perf() is on
	struct AUTORAILSUPPORTS_API FScopedPerf
	{
		explicit FScopedPerf(const TCHAR* InName);
		~FScopedPerf();
	private:
		const TCHAR* Name;
		double StartSeconds;
		bool bActive;
	};
}

// Profile a scope: feeds the cycle stats and the log-based report at once. Pass a STAT_ARS_* suffix,
// e.g. ARS_SCOPE_PERF(Measure)
#define ARS_SCOPE_PERF(StatSuffix) \
	SCOPE_CYCLE_COUNTER(STAT_ARS_##StatSuffix); \
	TRACE_CPUPROFILER_EVENT_SCOPE_STR("ARS_" #StatSuffix); \
	::ARSDebug::FScopedPerf ARS_ScopedPerf_##StatSuffix(TEXT(#StatSuffix))
