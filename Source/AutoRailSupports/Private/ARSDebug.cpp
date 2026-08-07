#include "ARSDebug.h"

#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/ScopeLock.h"

DEFINE_LOG_CATEGORY(LogARSClient);
DEFINE_LOG_CATEGORY(LogARSGenerator);
DEFINE_LOG_CATEGORY(LogARSPerf);

DEFINE_STAT(STAT_ARS_Measure);
DEFINE_STAT(STAT_ARS_PlanPath);
DEFINE_STAT(STAT_ARS_ResolveSettings);
DEFINE_STAT(STAT_ARS_PreviewUpdate);
DEFINE_STAT(STAT_ARS_PreviewFill);
DEFINE_STAT(STAT_ARS_ExecuteStampJob);
DEFINE_STAT(STAT_ARS_CostHook);

static TAutoConsoleVariable<int32> CVarARSDebugPerf(TEXT("ARS.Debug.Perf"), 0,
	TEXT("Time the mod's heavy scopes and report them to LogARSPerf. Works in the packaged game."), ECVF_Default);
static TAutoConsoleVariable<float> CVarARSDebugPerfThreshold(TEXT("ARS.Debug.PerfThresholdMs"), 2.0f,
	TEXT("A single profiled scope slower than this (ms) logs immediately. -1 disables the immediate logs."), ECVF_Default);
static TAutoConsoleVariable<float> CVarARSDebugPerfInterval(TEXT("ARS.Debug.PerfInterval"), 3.0f,
	TEXT("Seconds between the rolling per-scope perf summary while ARS.Debug.Perf is on."), ECVF_Default);

// Rolling perf aggregation. Game thread in practice, but locked anyway
namespace
{
	struct FPerfAccum
	{
		int32 Calls = 0;
		double TotalMs = 0.0;
		double MaxMs = 0.0;
	};

	FCriticalSection GPerfLock;
	TMap<FString, FPerfAccum> GPerfStats;
	double GLastReportSeconds = 0.0;

	void DumpPerfReport()
	{
		FScopeLock Lock(&GPerfLock);
		if (GPerfStats.Num() == 0)
		{
			UE_LOG(LogARSPerf, Display, TEXT("[perf] no ARS scopes ran since the last report."));
			return;
		}

		// Heaviest first, since that's the optimization target
		TArray<TPair<FString, FPerfAccum>> Rows;
		Rows.Reserve(GPerfStats.Num());
		for (const TPair<FString, FPerfAccum>& Pair : GPerfStats)
		{
			Rows.Add(Pair);
		}
		Rows.Sort([](const TPair<FString, FPerfAccum>& A, const TPair<FString, FPerfAccum>& B)
		{
			return A.Value.TotalMs > B.Value.TotalMs;
		});

		UE_LOG(LogARSPerf, Display, TEXT("---- ARS perf summary (heaviest first) ----"));
		for (const TPair<FString, FPerfAccum>& Row : Rows)
		{
			const double Avg = Row.Value.TotalMs / FMath::Max(1, Row.Value.Calls);
			UE_LOG(LogARSPerf, Display, TEXT("  %-18s calls %6d  total %9.2f ms  avg %7.3f ms  max %8.2f ms"),
				*Row.Key, Row.Value.Calls, Row.Value.TotalMs, Avg, Row.Value.MaxMs);
		}
		GPerfStats.Reset();
	}
}

namespace ARSDebug
{
	bool  Perf()            { return CVarARSDebugPerf.GetValueOnAnyThread() != 0; }
	float PerfThresholdMs() { return CVarARSDebugPerfThreshold.GetValueOnAnyThread(); }

	void RecordPerf(const TCHAR* ScopeName, double DurationMs)
	{
		const float Threshold = PerfThresholdMs();
		if (Threshold >= 0.0f && DurationMs >= Threshold)
		{
			UE_LOG(LogARSPerf, Display, TEXT("[perf] %-18s %8.2f ms  (>= %.1f ms threshold)"), ScopeName, DurationMs, Threshold);
		}

		FScopeLock Lock(&GPerfLock);
		FPerfAccum& Accum = GPerfStats.FindOrAdd(ScopeName);
		Accum.Calls++;
		Accum.TotalMs += DurationMs;
		Accum.MaxMs = FMath::Max(Accum.MaxMs, DurationMs);
	}

	void TickPerfReport()
	{
		if (!Perf())
		{
			GLastReportSeconds = 0.0;
			return;
		}
		const double Now = FPlatformTime::Seconds();
		if (GLastReportSeconds <= 0.0)
		{
			GLastReportSeconds = Now;
			return;
		}
		const float Interval = FMath::Max(0.25f, CVarARSDebugPerfInterval.GetValueOnAnyThread());
		if (Now - GLastReportSeconds >= Interval)
		{
			DumpPerfReport();
			GLastReportSeconds = Now;
		}
	}

	FScopedPerf::FScopedPerf(const TCHAR* InName)
		: Name(InName)
		, StartSeconds(0.0)
		, bActive(Perf())
	{
		if (bActive)
		{
			StartSeconds = FPlatformTime::Seconds();
		}
	}

	FScopedPerf::~FScopedPerf()
	{
		if (bActive)
		{
			RecordPerf(Name, (FPlatformTime::Seconds() - StartSeconds) * 1000.0);
		}
	}
}

static FAutoConsoleCommand GARSPerfDumpCommand(
	TEXT("ARS.Debug.PerfDump"),
	TEXT("Print the AutoRailSupports performance summary accumulated so far and reset it."),
	FConsoleCommandDelegate::CreateStatic(&DumpPerfReport));
