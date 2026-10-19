/*
 * JobsFolderPresenter.cpp
 */

#include "JobsFolderPresenter.h"
#include "Comm/FileInfo.h"
#include "Comm/Thumbnail.h"
#include "Debug.h"
#include "Hardware/Duet.h"
#include "JobsFolder.h"
#include "ObjectModel/Directories.h"
#include "ObjectModel/Files.h"
#include "ObjectModel/Job.h"
#include "ObjectModel/PrinterStatus.h"
#include "UI/Core/Navigation.h"
#include <ctime>
#include <filesystem>
#include <numeric>

namespace UI
{
	static std::string gcodesRoot()
	{
		std::string root(OM::Directories::GetGcodesDirectory());
		if (!root.empty() && root.back() == '/')
		{
			root.pop_back();
		}
		return root;
	}

	void JobsFolderPresenter::onInit()
	{
		ZoneScoped;
		registerEventListener<EventType::ThumbnailData>(this, &JobsFolderPresenter::newThumbnailData);
		registerEventListener<EventType::Status>([this](const OM::PrinterStatus&) { refreshPrinting(); });
		registerEventListener<EventType::JobFileName>([this](const std::string&) { refreshPrinting(); });
		registerEventListener<EventType::PrinterUniqueId>(
			[this]()
			{
				m_folder.clear();
				requestFiles();
			});
	}

	void JobsFolderPresenter::onActivate()
	{
		ZoneScoped;
		if (!OM::GetPrinterUniqueId().empty())
		{
			requestFiles();
		}
		refreshDetails();
		refreshPrinting();
	}

	void JobsFolderPresenter::refreshPrinting()
	{
		JobsFolder* view = getView();
		if (view == nullptr)
		{
			return;
		}
		bool printing = false, paused = false;
		std::string job;
		{
			MODEL_LOCK();
			const OM::PrinterStatus status = OM::GetStatus();
			printing = OM::IsPrintingStatus(status);
			paused = status == OM::PrinterStatus::paused || status == OM::PrinterStatus::pausing;
			job = OM::GetJobName();
		}
		view->setPrinting(printing, paused, job);
	}

	void JobsFolderPresenter::onDisconnect()
	{
		if (JobsFolder* view = getView(); view != nullptr)
		{
			view->setFiles(m_folder, {});
			view->setPrinting(false, false, "");
		}
	}

	void JobsFolderPresenter::requestFiles()
	{
		ZoneScoped;
		UI_LOCK();
		OM::FileSystem::RequestFiles(
			OM::Directories::DirectoryType::GCODES,
			m_folder,
			[this](OM::FileSystem::ItemList items)
			{
				ZoneScoped;
				UI_LOCK();
				JobsFolder* view = getView();
				if (view == nullptr)
				{
					return;
				}
				std::vector<JobsList::Entry> entries;
				if (!m_folder.empty())
				{
					JobsList::Entry up;
					up.name = "..";
					up.folder = true;
					up.up = true;
					entries.push_back(up);
				}
				const std::time_t now = std::time(nullptr);
				for (const auto& item : items)
				{
					if (item == nullptr)
					{
						continue;
					}
					JobsList::Entry e;
					e.name = item->GetName();
					e.path = item->GetPath();
					e.folder = item->GetType() == OM::FileSystem::FileSystemItemType::folder;
					e.date = item->GetDate();
					e.age = e.folder ? std::string() : JobsList::AgeText(e.date, now);
					entries.push_back(std::move(e));
				}
				view->setFiles(m_folder, entries);
			},
			true);
	}

	void JobsFolderPresenter::openFolder(const std::string& path)
	{
		const std::string root = gcodesRoot();
		std::string rel = path.starts_with(root) ? path.substr(root.size()) : path;
		while (!rel.empty() && rel.front() == '/')
		{
			rel.erase(rel.begin());
		}
		if (!rel.empty() && rel.back() != '/')
		{
			rel += '/';
		}
		m_folder = rel;
		requestFiles();
	}

	void JobsFolderPresenter::folderUp()
	{
		if (m_folder.empty())
		{
			return;
		}
		std::string rel = m_folder.substr(0, m_folder.size() - 1);
		const size_t slash = rel.rfind('/');
		m_folder = slash == std::string::npos ? std::string() : rel.substr(0, slash + 1);
		requestFiles();
	}

	void JobsFolderPresenter::select(const std::string& path)
	{
		m_selected = path;
		if (!path.empty())
		{
			FILEINFO_CACHE->QueueLargeThumbnailRequest(path);
		}
		refreshDetails();
	}

	void JobsFolderPresenter::refreshDetails()
	{
		ZoneScoped;
		JobsFolder* view = getView();
		if (view == nullptr)
		{
			return;
		}
		JobsFolder::Details d;
		if (!m_selected.empty())
		{
			d.valid = true;
			d.path = m_selected;
			d.name = std::filesystem::path(m_selected).filename().string();
			if (Comm::FileInfoPtr info = FILEINFO_CACHE->GetFileInfo(m_selected); info != nullptr)
			{
				d.infoKnown = true;
				d.printTime = info->printTime != 0 ? info->printTime : info->simulatedTime;
				d.filamentMm = std::accumulate(info->filament.begin(), info->filament.end(), 0.0f);
				d.layerHeight = info->layerHeight;
				d.height = info->height;
				d.slicer = info->generatedBy.c_str();
				d.saved = info->lastModified.c_str();
			}
			if (IsThumbnailCached(m_selected))
			{
				d.thumbnail = GetThumbnailPath(m_selected).string();
			}
			else if (d.infoKnown)
			{
				FILEINFO_CACHE->QueueLargeThumbnailRequest(m_selected); /* the info came first */
			}
		}
		view->setDetails(d);
	}

	void JobsFolderPresenter::newThumbnailData(const std::string& filename)
	{
		if (filename == m_selected)
		{
			refreshDetails();
		}
	}

	void JobsFolderPresenter::startPrint(const std::string& path)
	{
		ZoneScoped;
		LOG_INFO("Jobs: start print {:s}", path);
		OM::FileSystem::StartPrint(path);
		home();
	}

	void JobsFolderPresenter::deleteFile(const std::string& path)
	{
		ZoneScoped;
		LOG_INFO("Jobs: delete {:s}", path);
		if (!Comm::DUET.DeleteFile(path))
		{
			LOG_WARN("Jobs: failed to delete {:s}", path);
		}
		if (path == m_selected)
		{
			select("");
		}
		requestFiles();
	}
} // namespace UI
