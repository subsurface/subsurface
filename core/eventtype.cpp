// SPDX-License-Identifier: GPL-2.0
#include "eventtype.h"
#include "event.h"
#include "gettextfromc.h"
#include "subsurface-string.h"

#include <string>
#include <vector>
#include <algorithm>

struct event_type {
	std::string name;
	event_severity severity;
	bool plot;
	event_type(const struct event *ev) :
		name(ev->name),
		severity(ev->get_severity()),
		plot(true)
	{
	}
};

static std::vector<event_type> event_types;

static bool operator==(const event_type &en1, const event_type &en2)
{
	return en1.name == en2.name && en1.severity == en2.severity;
}

void clear_event_types()
{
	event_types.clear();
}

void remember_event_type(const struct event *ev)
{
	if (ev->name.empty())
		return;
	event_type type(ev);
	if (std::find(event_types.begin(), event_types.end(), type) != event_types.end())
		return;
	event_types.push_back(std::move(type));
}

bool is_event_type_hidden(const struct event *ev)
{
	auto it = std::find(event_types.begin(), event_types.end(), ev);
	return it != event_types.end() && !it->plot;
}

void hide_event_type(const struct event *ev)
{
	auto it = std::find(event_types.begin(), event_types.end(), ev);
	if (it != event_types.end())
		it->plot = false;
}

void show_all_event_types()
{
	for (event_type &e: event_types)
		e.plot = true;
}

void show_event_type(int idx)
{
	if (idx < 0 || idx >= (int)event_types.size())
		return;
	event_types[idx].plot = true;
}

bool any_event_types_hidden()
{
	return std::any_of(event_types.begin(), event_types.end(),
			   [] (const event_type &e) { return !e.plot; });
}

extern std::vector<int> hidden_event_types()
{
	std::vector<int> res;
	for (size_t i = 0; i < event_types.size(); ++i) {
		if (!event_types[i].plot)
			res.push_back(i);
	}
	return res;
}

static QString event_severity_name(event_severity severity)
{
	switch (severity) {
	case EVENT_SEVERITY_INFO: return gettextFromC::tr("info");
	case EVENT_SEVERITY_WARN: return gettextFromC::tr("warn");
	case EVENT_SEVERITY_ALARM: return gettextFromC::tr("alarm");
	default: return QString();
	}
}

static QString event_type_name(QString name, event_severity severity)
{
	QString severity_name = event_severity_name(severity);
	if (severity_name.isEmpty())
		return name;
	return QStringLiteral("%1 (%2)").arg(name, severity_name);
}

QString event_type_name(const event &ev)
{
	if (ev.name.empty())
		return QString();

	QString name = QString::fromStdString(ev.name);
	return event_type_name(std::move(name), ev.get_severity());
}

QString event_type_name(int idx)
{
	if (idx < 0 || idx >= (int)event_types.size())
		return QString();

	const event_type &t = event_types[idx];
	QString name = QString::fromUtf8(t.name.c_str());
	return event_type_name(std::move(name), t.severity);
}
