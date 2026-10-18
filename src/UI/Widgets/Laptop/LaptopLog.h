/*
 * LaptopLog.h
 *
 * What the laptop keeps while it is not shown (no LVGL here, so the tests can use it
 * directly):
 *
 *   ConsoleLog  typed commands and every machine reply, newest at the bottom, long
 *               replies wrapped; ages ("0m41") instead of clock times. The last 5
 *               commands are kept for the history arrows; the unsent text stays.
 *   AlertList   21 slots (3 pages of 7), newest first: "Error:" / "Warning:" lines of
 *               the replies and a few INFO events (connection, job start / end). The
 *               oldest falls out when a new one comes. A repeat of the newest only
 *               refreshes it. Unread until opened. Filter ALL / ERR / WRN / INFO; the
 *               pages count only the alerts that match the filter.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

namespace UI::LaptopLog
{
	/* "12m05", "1h02", "3d04" */
	std::string FormatAge(uint32_t ms);

	class ConsoleLog
	{
	  public:
		enum class Kind : uint8_t
		{
			Command,
			Reply,
			Warning,
			Error,
		};

		struct Line
		{
			uint32_t time;
			Kind kind;
			bool first; /* first line of a message: shows the age */
			std::string text;
		};

		static constexpr size_t MAX_LINES = 60;
		static constexpr size_t MAX_HISTORY = 5;

		void setWrap(size_t chars) { m_wrap = chars > 8 ? chars : 8; }
		void addCommand(const std::string& text, uint32_t now);
		void addReply(const std::string& text, uint32_t now);
		const std::deque<Line>& lines() const { return m_lines; } /* oldest first */

		/* history: 0 = newest; pos -1 = the draft */
		const std::vector<std::string>& history() const { return m_history; }
		std::string& draft() { return m_draft; }
		const std::string& draft() const { return m_draft; }
		/* -1 up (older), +1 down (newer); returns the text to show */
		std::string historyStep(int dir);
		bool canGoUp() const { return m_historyPos + 1 < static_cast<int>(m_history.size()); }
		bool canGoDown() const { return m_historyPos >= 0; }
		void resetHistoryPos() { m_historyPos = -1; }

	  private:
		void addMessage(Kind kind, const std::string& text, uint32_t now);

		std::deque<Line> m_lines;
		std::vector<std::string> m_history;
		std::string m_draft;
		std::string m_typedDraft; /* the draft while browsing the history */
		int m_historyPos = -1;
		size_t m_wrap = 58;
	};

	class AlertList
	{
	  public:
		enum class Level : uint8_t
		{
			Error,
			Warning,
			Info,
		};

		enum class Filter : uint8_t
		{
			All,
			Error,
			Warning,
			Info,
		};

		struct Entry
		{
			uint32_t time;
			Level level;
			bool unread;
			std::string text;
		};

		static constexpr size_t PAGE_ROWS = 7;
		static constexpr size_t PAGES = 3;
		static constexpr size_t MAX_ALERTS = PAGE_ROWS * PAGES;
		static constexpr size_t MAX_TEXT = 160; /* fits the 4 lines of the popup */

		/* A machine reply: every "Error:" / "Warning:" line is kept. isError: the reply
		 * came as an error (a line without a tag is then an error too). */
		bool collect(const std::string& reply, bool isError, uint32_t now);
		void add(Level level, std::string text, uint32_t now);

		size_t count() const { return m_alerts.size(); }
		const Entry& entry(size_t i) const { return m_alerts[i]; } /* 0 = newest */
		bool anyUnread() const;
		void markRead(size_t i);

		Filter filter() const { return m_filter; }
		void nextFilter();
		size_t page() const { return m_page; }
		size_t pageCount() const; /* at least 1 */
		void pageStep(int dir);

		/* Alert shown in a row of the current page, or -1 */
		int rowIndex(size_t row) const;

		static const char* levelName(Level level);
		static const char* filterName(Filter filter);

	  private:
		bool matches(const Entry& e) const;
		size_t matchingCount() const;

		std::deque<Entry> m_alerts; /* newest first */
		Filter m_filter = Filter::All;
		size_t m_page = 0;
	};
} // namespace UI::LaptopLog
