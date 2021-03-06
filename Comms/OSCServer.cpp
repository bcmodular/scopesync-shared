/**
 * O(pen) S(ound) C(ontrol) message handler
 *
 *  (C) Copyright 2014 bcmodular (http://www.bcmodular.co.uk/)
 *
 * This file is part of ScopeSync.
 *
 * ScopeSync is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ScopeSync is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ScopeSync.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contributors:
 *  Simon Russell
 *  Will Ellis
 *  Jessica Brandt
 */

#include "OSCServer.h"
#include "../Core/ScopeSyncApplication.h"

OSCServer::OSCServer() :
	remoteChanged(false)
{
	OSCServer::setup();
}

OSCServer::~OSCServer()
{
	removeListener(this);

	oscLocalPortNum.removeListener(this);
	oscRemoteHost.removeListener(this);
	oscRemotePortNum.removeListener(this);
}

void OSCServer::setup()
{
	oscLocalPortNum.addListener(this);
	oscRemotePortNum.addListener(this);

	#ifdef __DLL_EFFECT__
		oscRemoteHost.setValue("127.0.0.1");
		userSettings->referToScopeSyncOSCSettings(oscLocalPortNum, oscRemotePortNum);
	#else
		oscRemoteHost.addListener(this);
		userSettings->referToPluginOSCSettings(oscLocalPortNum, oscRemoteHost, oscRemotePortNum);
	#endif // __DLL_EFFECT__

	addListener(this);
}

void OSCServer::updateListenerPort()
{
	if (connect(oscLocalPortNum.getValue()))
	{
		BCMDBG("OSCServer::setLocalPortNumber - set local port number to: " + oscLocalPortNum.toString());
		return;
	}

	AlertWindow::showMessageBoxAsync (
        AlertWindow::WarningIcon,
        "Connection error",
        "OSC Error: could not connect to UDP port: " + oscLocalPortNum.toString(),
        "OK");		
}

void OSCServer::registerOSCListener(ListenerWithOSCAddress<MessageLoopCallback>* newListener, OSCAddress address)
{
	//BCMDBG("OSCServer::registerOSCListener - " + address.toString());
	removeListener(newListener);
	addListener(newListener, address);
}

void OSCServer::unregisterOSCListener(ListenerWithOSCAddress<MessageLoopCallback>* listenerToRemove)
{
	removeListener(listenerToRemove);
}

int OSCServer::getLocalPortNum() const
{
	return oscLocalPortNum.getValue();
}

bool OSCServer::sendFloatMessage(const OSCAddressPattern pattern, float valueToSend)
{
	String logMessage("OSCServer::sendFloatMessage - To Host: " + oscRemoteHost.toString() + ", Port: " + oscRemotePortNum.getValue() + ", address - " + pattern.toString() + ", value: " + String(valueToSend));

	if (connectToListener())
	{
		OSCMessage message(pattern);
		message.addFloat32(valueToSend);

		if (sender.send(message))
			logMessage + " - attempt successful";
		else
			logMessage + " - attempt failed";

		BCMDBG(logMessage);
		return true;
	}

	logMessage += " - failed to connect to Listener";
	BCMDBG(logMessage);
	return false;
}

bool OSCServer::sendIntMessage(const OSCAddressPattern pattern, int valueToSend)
{
	String logMessage("OSCServer::sendIntMessage - To Host: " + oscRemoteHost.toString() + ", Port: " + oscRemotePortNum.getValue() + ", address - " + pattern.toString() + ", value: " + String(valueToSend));

	if (connectToListener())
	{
		OSCMessage message(pattern);
		message.addInt32(valueToSend);

		if (sender.send(message))
			logMessage + " - attempt successful";
		else
			logMessage + " - attempt failed";
		
		BCMDBG(logMessage);
		return true;
	}

	logMessage += " - failed to connect to Listener";
	BCMDBG(logMessage);
	return false;
}

bool OSCServer::connectToListener()
{
	if (remoteChanged) {
        if (sender.connect(oscRemoteHost.toString(), int(oscRemotePortNum.getValue())))
		{
        	remoteChanged = false;
			return true;
		}
		
		AlertWindow::showMessageBoxAsync (
			AlertWindow::WarningIcon,
			"Connection error",
			"OSC Error: could not connect to remote UDP port: " + oscRemotePortNum.toString() + ", on host: " + oscRemoteHost.toString(),
			"OK");

		return false;
    }
	
	return true;
}

void OSCServer::oscMessageReceived(const OSCMessage& message)
{
	(void)message;
	BCMDBG("OSCServer::oscMessageReceived - Port: " + oscLocalPortNum.getValue() + ", address - " + message.getAddressPattern().toString() + " type: " + message[0].getType());
}

void OSCServer::valueChanged(Value& valueThatChanged)
{
	if (valueThatChanged.refersToSameSourceAs(oscLocalPortNum))
		updateListenerPort();
	else // if (valueThatChanged.refersToSameSourceAs(oscRemoteHost) || valueThatChanged.refersToSameSourceAs(oscRemotePortNum))
		remoteChanged = true;
}
