/*
 * LaptopLog.cpp
 */

#include "LaptopLog.h"
#include <fmt/format.h>

namespace UI::LaptopLog
{
	std::string FormatAge(uint32_t ms)
	{
		uint32_t age = ms / 1000;
		if (age < 60 * 60)
		{
			return fmt::format("{:d}m{:02d}", age / 60, age % 60);
		}
		age /= 60;
		if (age < 24 * 60)
		{
			return fmt::format("{:d}h{:02d}", age / 60, age % 60);
		}
		age /= 60;
		return fmt::format("{:d}d{:02d}", age / 24, age % 24);
	}

	/* Cut at a whole UTF-8 character */
	static size_t charBoundary(const std::string& s, size_t pos)
	{
		while (pos > 0 && pos < s.size() && (static_cast<unsigned char>(s[pos]) & 0xC0) == 0x80)
		{
			--pos;
		}
		return pos;
	}

	static std::string trim(const std::string& s)
	{
		const size_t b = s.find_first_not_of(" \t\r");
		if (b == std::string::npos)
		{
			return std::string();
		}
		const size_t e = s.find_last_not_of(" \t\r");
		return s.substr(b, e - b + 1);
	}

	/* ---------------- console ---------------- */

	void ConsoleLog::addCommand(const std::string& text, uint32_t now)
	{
		const std::string cmd = trim(text);
		if (cmd.empty())
		{
			return;
		}
		addMessage(Kind::Command, cmd, now);
		if (m_history.empty() || m_history.front() != cmd)
		{
			m_history.insert(m_history.begin(), cmd);
			if (m_history.size() > MAX_HISTORY)
			{
				m_history.pop_back();
			}
		}
		m_historyPos = -1;
		m_typedDraft.clear();
	}

	void ConsoleLog::addReply(const std::string& text, uint32_t now)
	{
		/* One message per line of the reply, coloured by its tag */
		size_t start = 0;
		while (start <= text.size())
		{
			size_t end = text.find('\n', start);
			if (end == std::string::npos)
			{
				end = text.size();
			}
			const std::string line = trim(text.substr(start, end - start));
			if (!line.empty())
			{
				const Kind kind = line.find("Error:") != std::string::npos	   ? Kind::Error
								  : line.find("Warning:") != std::string::npos ? Kind::Warning
																			   : Kind::Reply;
				addMessage(kind, line, now);
			}
			start = end + 1;
		}
	}

	void ConsoleLog::addMessage(Kind kind, const std::string& text, uint32_t now)
	{
		/* Wrap at word boundaries */
		bool first = true;
		std::string rest = text;
		while (!rest.empty())
		{
			std::string part;
			if (rest.size() <= m_wrap)
			{
				part = rest;
				rest.clear();
			}
			else
			{
				size_t cut = rest.rfind(' ', m_wrap);
				if (cut == std::string::npos || cut < m_wrap / 2)
				{
					cut = charBoundary(rest, m_wrap);
					if (cut == 0)
					{
						cut = m_wrap;
					}
				}
				part = rest.substr(0, cut);
				rest = trim(rest.substr(cut));
			}
			m_lines.push_back({now, kind, first, part});
			first = false;
		}
		while (m_lines.size() > MAX_LINES)
		{
			m_lines.pop_front();
		}
	}

	std::string ConsoleLog::historyStep(int dir)
	{
		if (dir < 0 && canGoUp())
		{
			if (m_historyPos < 0)
			{
				m_typedDraft = m_draft;
			}
			++m_historyPos;
		}
		else if (dir > 0 && canGoDown())
		{
			--m_historyPos;
		}
		return m_historyPos < 0 ? m_typedDraft : m_history[static_cast<size_t>(m_historyPos)];
	}

	/* ---------------- alerts ---------------- */

	bool AlertList::collect(const std::string& reply, bool isError, uint32_t now)
	{
		bool changed = false;
		size_t start = 0;
		while (start < reply.size())
		{
			size_t end = reply.find_first_of("\r\n", start);
			if (end == std::string::npos)
			{
				end = reply.size();
			}
			const std::string line = reply.substr(start, end - start);
			const size_t err = line.find("Error:");
			const size_t warn = line.find("Warning:");
			if (err != std::string::npos)
			{
				add(Level::Error, line.substr(err + 6), now);
				changed = true;
			}
			else if (warn != std::string::npos)
			{
				add(Level::Warning, line.substr(warn + 8), now);
				changed = true;
			}
			else if (isError && !trim(line).empty())
			{
				add(Level::Error, line, now);
				changed = true;
			}
			start = end + 1;
		}
		return changed;
	}

	void AlertList::add(Level level, std::string text, uint32_t now)
	{
		for (char& c : text)
		{
			if (c == '\t')
			{
				c = ' ';
			}
		}
		text = trim(text);
		if (text.size() > MAX_TEXT)
		{
			text.resize(charBoundary(text, MAX_TEXT));
		}
		if (text.empty())
		{
			text = "(no text)";
		}
		if (!m_alerts.empty() && m_alerts.front().level == level && m_alerts.front().text == text)
		{
			m_alerts.front().time = now; /* the same again: refresh it */
			m_alerts.front().unread = true;
			return;
		}
		m_alerts.push_front({now, level, true, std::move(text)});
		if (m_alerts.size() > MAX_ALERTS)
		{
			m_alerts.pop_back(); /* the oldest falls out */
		}
		if (m_page >= pageCount())
		{
			m_page = pageCount() - 1;
		}
	}

	bool AlertList::anyUnread() const
	{
		for (const Entry& e : m_alerts)
		{
			if (e.unread)
			{
				return true;
			}
		}
		return false;
	}

	void AlertList::markRead(size_t i)
	{
		if (i < m_alerts.size())
		{
			m_alerts[i].unread = false;
		}
	}

	bool AlertList::matches(const Entry& e) const
	{
		switch (m_filter)
		{
		case Filter::All:
			return true;
		case Filter::Error:
			return e.level == Level::Error;
		case Filter::Warning:
			return e.level == Level::Warning;
		case Filter::Info:
			return e.level == Level::Info;
		}
		return true;
	}

	size_t AlertList::matchingCount() const
	{
		size_t n = 0;
		for (const Entry& e : m_alerts)
		{
			if (matches(e))
			{
				++n;
			}
		}
		return n;
	}

	void AlertList::nextFilter()
	{
		m_filter = static_cast<Filter>((static_cast<uint8_t>(m_filter) + 1) % 4);
		m_page = 0;
	}

	size_t AlertList::pageCount() const
	{
		const size_t n = matchingCount();
		return n == 0 ? 1 : (n + PAGE_ROWS - 1) / PAGE_ROWS;
	}

	void AlertList::pageStep(int dir)
	{
		if (dir < 0 && m_page > 0)
		{
			--m_page;
		}
		else if (dir > 0 && m_page + 1 < pageCount())
		{
			++m_page;
		}
	}

	int AlertList::rowIndex(size_t row) const
	{
		const size_t wanted = m_page * PAGE_ROWS + row;
		size_t n = 0;
		for (size_t i = 0; i < m_alerts.size(); i++)
		{
			if (matches(m_alerts[i]))
			{
				if (n == wanted)
				{
					return static_cast<int>(i);
				}
				++n;
			}
		}
		return -1;
	}

	const char* AlertList::levelName(Level level)
	{
		switch (level)
		{
		case Level::Error:
			return "ERR";
		case Level::Warning:
			return "WRN";
		case Level::Info:
			return "INFO";
		}
		return "";
	}

	const char* AlertList::filterName(Filter filter)
	{
		switch (filter)
		{
		case Filter::All:
			return "ALL";
		case Filter::Error:
			return "ERR";
		case Filter::Warning:
			return "WRN";
		case Filter::Info:
			return "INFO";
		}
		return "";
	}
} // namespace UI::LaptopLog
