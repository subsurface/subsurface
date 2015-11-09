#ifndef ISOCIALNETWORKINTEGRATION_H
#define ISOCIALNETWORKINTEGRATION_H

#include <QtPlugin>

/* This Interface represents a Plugin for Social Network integration,
 * with it you may be able to create plugins for facebook, instagram,
 * twitpic, google plus and any other thing you may imagine.
 *
 * We bundle facebook integration as an example.
 */

class ISocialNetworkIntegration : public QObject {
  Q_OBJECT
public:
	ISocialNetworkIntegration(QObject* parent = 0);

	/*!
	 * @name socialNetworkName
	 * @brief The name of this social network
	 * @return The name of this social network
	 *
	 * The name of this social network will be used to populate the Menu to toggle states
	 * between connected/disconnected, and also submit stuff to it.
	 */
	virtual QString socialNetworkName() const = 0;

	/*!
	 * @name socialNetworkIcon
	 * @brief The icon of this social network
	 * @return The icon of this social network
	 *
	 * The icon of this social network will be used to populate the menu, and can also be
	 * used on a toolbar if requested.
	 */
	virtual QString socialNetworkIcon() const = 0;

	/*!
	 * @name isConnected
	 * @brief returns true if connected to this social network, false otherwise
	 * @return true if connected to this social network, false otherwise
	 */
	virtual bool isConnected() = 0;

	/*!
	 * @name requestLogin
	 * @brief try to login on this social network.
	 *
	 * Try to login on this social network. All widget implementation that
	 * manages login should be done inside this function.
	 */
	virtual void requestLogin() = 0;

	/*!
	 * @name requestLogoff
	 * @brief tries to logoff from this social network
	 *
	 * Try to logoff from this social network.
	 */
	virtual void requestLogoff() = 0;

	/*!
	 * @name uploadCurrentDive
	 * @brief send the current dive info to the Social Network
	 *
	 * Should format all the options and pixmaps from the current dive
	 * to update to the social network. All widget stuff related to sendint
	 * dive information should be executed inside this function.
	 */
	virtual void requestUpload() = 0;
};

#endif