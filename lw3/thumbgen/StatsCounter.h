#pragma once
#include <atomic>

class StatsCounter
{
public:
	void IncrementProcessed()
	{
		++m_processedFilesCount;
	}

	void IncrementFailed()
	{
		++m_failedFilesCount;
	}

	[[nodiscard]] int GetProcessedFilesCount() const
	{
		return m_processedFilesCount.load();
	}

	[[nodiscard]] int GetFailedFilesCount() const
	{
		return m_failedFilesCount.load();
	}

private:
	std::atomic<int> m_processedFilesCount{ 0 };
	std::atomic<int> m_failedFilesCount{ 0 };
};