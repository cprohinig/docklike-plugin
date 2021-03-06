/*
 * Docklike Taskbar - A modern, minimalist taskbar for XFCE
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * gnu.org/licenses/gpl-3.0
 */

#include "GroupWindow.hpp"

GroupWindow::GroupWindow(WnckWindow* wnckWindow)
{
	mWnckWindow = wnckWindow;
	mGroupMenuItem = new GroupMenuItem(this);

	std::string groupName = Wnck::getGroupName(this); // check here for exotic group association (like libreoffice)
	AppInfo* appInfo = AppInfos::search(groupName);

	mGroup = Dock::prepareGroup(appInfo);

	/*std::cout << "SEARCHING GROUPNAME:" << groupName << std::endl;
	if (appInfo == NULL)
		std::cout << "NO MATCH:" << 0 << std::endl;
	else
	{
		std::cout << "> APPINFO NAME:" << appInfo->name << std::endl;
		std::cout << "> APPINFO PATH:" << appInfo->path << std::endl;
		std::cout << "> APPINFO ICON:" << appInfo->icon << std::endl;
	}*/

	// signal connection
	g_signal_connect(G_OBJECT(mWnckWindow), "name-changed",
		G_CALLBACK(+[](WnckWindow* window, GroupWindow* me) {
			me->mGroupMenuItem->updateLabel();
		}),
		this);

	g_signal_connect(G_OBJECT(mWnckWindow), "icon-changed",
		G_CALLBACK(+[](WnckWindow* window, GroupWindow* me) {
			me->mGroupMenuItem->updateIcon();
		}),
		this);

	g_signal_connect(G_OBJECT(mWnckWindow), "state-changed",
		G_CALLBACK(+[](WnckWindow* window, WnckWindowState changed_mask,
						WnckWindowState new_state, GroupWindow* me) {
			me->updateState();
		}),
		this);

	g_signal_connect(G_OBJECT(mWnckWindow), "workspace-changed",
		G_CALLBACK(+[](WnckWindow* window, GroupWindow* me) {
			me->updateState();
		}),
		this);

	// initial state
	updateState();

	mGroupMenuItem->updateIcon();
	mGroupMenuItem->updateLabel();
}

GroupWindow::~GroupWindow()
{
	leaveGroup();
	delete mGroupMenuItem;
}

void GroupWindow::getInGroup()
{
	if (mGroupAssociated)
		return;

	mGroup->add(this);
	mGroupAssociated = true;
}

void GroupWindow::leaveGroup()
{
	if (!mGroupAssociated)
		return;

	mGroup->remove(this);
	mGroup->onWindowUnactivate();
	mGroupAssociated = false;
}

void GroupWindow::onActivate()
{
	Help::Gtk::cssClassAdd(GTK_WIDGET(mGroupMenuItem->mItem), "active");
	gtk_widget_queue_draw(GTK_WIDGET(mGroupMenuItem->mItem));

	mGroup->onWindowActivate(this);
}

void GroupWindow::onUnactivate()
{
	Help::Gtk::cssClassRemove(GTK_WIDGET(mGroupMenuItem->mItem), "active");
	gtk_widget_queue_draw(GTK_WIDGET(mGroupMenuItem->mItem));

	mGroup->onWindowUnactivate();
}

bool GroupWindow::getState(WnckWindowState flagMask)
{
	return (mState & flagMask) != 0;
}

void GroupWindow::activate(guint32 timestamp)
{
	Wnck::activate(this, timestamp);
}

void GroupWindow::minimize()
{
	Wnck::minimize(this);
}

void GroupWindow::showMenu() {}

void GroupWindow::updateState()
{
	mState = Wnck::getState(this);

	bool onWorkspace = true;
	if (Settings::onlyDisplayVisible)
	{
		WnckWorkspace* windowWorkspace = wnck_window_get_workspace(mWnckWindow);
		if (windowWorkspace != NULL)
		{
			WnckWorkspace* activeWorkspace = wnck_screen_get_active_workspace(Wnck::mWnckScreen);

			if (windowWorkspace != activeWorkspace)
				onWorkspace = false;
		}
	}

	bool onTasklist = !(mState & WnckWindowState::WNCK_WINDOW_STATE_SKIP_TASKLIST);

	if (onWorkspace && onTasklist)
		getInGroup();
	else
		leaveGroup();

	gtk_widget_show(GTK_WIDGET(mGroupMenuItem->mItem));
}