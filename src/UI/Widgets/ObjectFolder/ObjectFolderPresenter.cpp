/*
 * ObjectFolderPresenter.cpp
 */

#include "ObjectFolderPresenter.h"
#include "Debug.h"
#include "ObjectFolder.h"
#include "ObjectModel/Axis.h"
#include "ObjectModel/Job.h"
#include "ObjectModel/Tool.h"
#include <filesystem>

namespace UI
{
	void ObjectFolderPresenter::onInit()
	{
		ZoneScoped;
		registerEventListener<EventType::JobBuild>([this]() { refresh(); });
		registerEventListener<EventType::JobCurrentObject>([this]() { refresh(); });
		registerEventListener<EventType::JobObjectData>([this]() { refresh(); });
		registerEventListener<EventType::AxesData>([this]() { refresh(); });
		registerEventListener<EventType::Status>([this](const OM::PrinterStatus&) { refresh(); });
		registerEventListener<EventType::JobFileName>([this](const std::string&) { refresh(); });
	}

	void ObjectFolderPresenter::refresh()
	{
		ZoneScoped;
		ObjectFolder* view = getView();
		if (view == nullptr)
		{
			return;
		}
		ObjectFolder::State s;
		{
			MODEL_LOCK();
			s.printing = OM::IsPrintingStatus(OM::GetStatus());
			s.job = std::filesystem::path(OM::GetJobName()).filename().string();
			s.current = OM::GetCurrentJobObjectIndex();

			auto axisX = OM::Move::GetAxisByLetter('X');
			auto axisY = OM::Move::GetAxisByLetter('Y');
			if (axisX && axisY)
			{
				s.xMin = axisX->minPosition;
				s.xMax = axisX->maxPosition;
				s.yMin = axisY->minPosition;
				s.yMax = axisY->maxPosition;
				s.hasBed = s.xMax > s.xMin && s.yMax > s.yMin;
			}

			OM::IterateJobObjectsWhile(
				[&s](std::shared_ptr<OM::JobObject> obj, size_t)
				{
					if (obj->index >= s.objects.size())
					{
						s.objects.resize(obj->index + 1);
					}
					ObjectFolder::Object& o = s.objects[obj->index];
					o.present = true;
					o.name = obj->name;
					o.cancelled = obj->cancelled;
					o.hasPosition = obj->bounds.x[1] > obj->bounds.x[0] || obj->bounds.y[1] > obj->bounds.y[0];
					o.x = static_cast<float>(obj->bounds.x[0] + obj->bounds.x[1]) * 0.5f;
					o.y = static_cast<float>(obj->bounds.y[0] + obj->bounds.y[1]) * 0.5f;
					return true;
				});

			/* No axis limits: the objects with a margin */
			if (!s.hasBed)
			{
				bool any = false;
				for (const auto& o : s.objects)
				{
					if (!o.present || !o.hasPosition)
					{
						continue;
					}
					if (!any)
					{
						s.xMin = s.xMax = o.x;
						s.yMin = s.yMax = o.y;
						any = true;
					}
					s.xMin = std::min(s.xMin, o.x);
					s.xMax = std::max(s.xMax, o.x);
					s.yMin = std::min(s.yMin, o.y);
					s.yMax = std::max(s.yMax, o.y);
				}
				const float mx = std::max(10.0f, (s.xMax - s.xMin) * 0.15f);
				const float my = std::max(10.0f, (s.yMax - s.yMin) * 0.15f);
				s.xMin -= mx;
				s.xMax += mx;
				s.yMin -= my;
				s.yMax += my;
				s.hasBed = any;
			}
		}
		view->update(s);
	}

	void ObjectFolderPresenter::cancelObject(size_t index)
	{
		ZoneScoped;
		LOG_INFO("ObjectFolder: cancel object {:d}", index);
		OM::SetJobObjectActive(index, false); /* M486 P<index> */
	}
} // namespace UI
