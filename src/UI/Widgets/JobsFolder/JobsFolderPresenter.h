/*
 * JobsFolderPresenter.h
 *
 * Feeds the jobs folder: the G-code files of the current folder (M20), the details and the
 * thumbnail of the selected file (M36, from DuetScreen's file info cache), and starts or
 * deletes a print.
 */

#pragma once

#include "UI/Core/Presenter.h"
#include <cstdint>
#include <string>

namespace UI
{
	class JobsFolder;

	class JobsFolderPresenter : public Presenter<JobsFolder>
	{
	  public:
		PRESENTER_CONSTRUCTOR(JobsFolderPresenter, JobsFolder);

		void requestFiles();
		void openFolder(const std::string& path); /* full path, or "" for the gcodes root */
		void folderUp();
		void select(const std::string& path); /* "" = none */
		void refreshDetails();
		void startPrint(const std::string& path);
		void deleteFile(const std::string& path);

	  protected:
		void onInit() override;
		void onActivate() override;
		void onDisconnect() override;

	  private:
		void newThumbnailData(const std::string& filename);
		void refreshPrinting();

		std::string m_folder; /* relative to the gcodes folder, "" or "sub/" */
		std::string m_selected;
		bool m_largeQueued = false; /* large thumbnail asked for the selected file */
		bool m_waiting = false;		/* the selected file's info has not arrived yet */
		uint32_t m_thumbnailVersion = 0;
	};
} // namespace UI
