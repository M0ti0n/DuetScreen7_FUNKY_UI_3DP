/*
 * ObjectFolderPresenter.h
 *
 * Feeds the object cancel page from the object model (job objects, current object,
 * axis limits, job name).
 */

#pragma once

#include "ObjectModel/PrinterStatus.h"
#include "UI/Core/Presenter.h"
#include <string>

namespace UI
{
	class ObjectFolder;

	class ObjectFolderPresenter : public Presenter<ObjectFolder>
	{
	  public:
		PRESENTER_CONSTRUCTOR(ObjectFolderPresenter, ObjectFolder);

		void refresh();
		void cancelObject(size_t index);

	  protected:
		void onInit() override;
		void onActivate() override { refresh(); }
		void onConnect() override { refresh(); }
		void onDisconnect() override { refresh(); }
	};
} // namespace UI
