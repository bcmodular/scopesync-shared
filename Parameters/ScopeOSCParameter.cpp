/**
 * Models a Scope OSC parameter
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

#include "BCMParameter.h"
#include "BCMParameterController.h"
#include "../Utils/BCMMath.h"
#include "../Core/Global.h"

ScopeOSCParameter::ScopeOSCParameter(ScopeOSCParamID oscParamID, BCMParameter* owner, ValueTree parameterDefinition, int deviceUID)
	: parameter(owner),
      paramID(oscParamID), 
	  deviceInstance(0),
	  minValue(parameterDefinition.getProperty(Ids::scopeRangeMin)),
	  maxValue(parameterDefinition.getProperty(Ids::scopeRangeMax)),
	  dBRef(parameterDefinition.getProperty(Ids::scopeDBRef)),
	  isListening(false),
	  isSending(false),
	  deviceUID(String(deviceUID))
{
	BCMDBG("ScopeOSCParameter::ScopeOSCParameter - creating new parameter with paramID: " + String(paramID.paramGroup) + ":" + String(paramID.paramId));
}

ScopeOSCParameter::~ScopeOSCParameter()
{
	stopTimer();
	oscServer->unregisterOSCListener(this);
}

String ScopeOSCParameter::getScopeParamText() const
{
	return String(paramID.paramGroup) + ":" + String(paramID.paramId);
}

void ScopeOSCParameter::registerAsListener()
{
	BCMDBG("ScopeOSCParameter::setDeviceInstance - Registering as listener to " + getOSCPath());
	oscServer->registerOSCListener(this, getOSCPath());
}

void ScopeOSCParameter::setDeviceInstance(int newUID)
{
	BCMDBG("ScopeOSCParameter::setDeviceInstance - " + String(newUID));
	deviceInstance = newUID;
	registerAsListener();
}

void ScopeOSCParameter::setConfigurationUID(int newUID)
{
	BCMDBG("ScopeOSCParameter::setConfigurationUID - " + String(newUID));
	configurationUID = newUID;
	registerAsListener();
}

void ScopeOSCParameter::setDeviceUID(int newUID)
{
	deviceUID = String(newUID);
}

void ScopeOSCParameter::sendCurrentValue()
{
	BCMDBG("ScopeOSCParameter::sendCurrentValue - " + getOSCPath() + " " + String(intValue));
	oscServer->sendIntMessage(getOSCPath(), intValue);
	stopListening();
	startTimer(500);
}

void ScopeOSCParameter::sendMinValue()
{
	BCMDBG("ScopeOSCParameter::sendMinValue - " + getOSCPath() + " " + String(minValue));
	oscServer->sendIntMessage(getOSCPath(), minValue);
	stopListening();
	startTimer(500);
}

void ScopeOSCParameter::sendMaxValue()
{
	BCMDBG("ScopeOSCParameter::sendCurrentValue - " + getOSCPath() + " " + String(maxValue));
	oscServer->sendIntMessage(getOSCPath(), maxValue);
	stopListening();
	startTimer(500);
}

void ScopeOSCParameter::updateValue(int newValue)
{
	BCMDBG("ScopeOSCParameter::updateValue " + String(newValue));
	intValue = newValue;
	sendCurrentValue();
}

void ScopeOSCParameter::updateValue(double linearNormalisedValue, double uiValue, double uiMinValue, double uiMaxValue)
{
	BCMDBG("ScopeOSCParameter::updateValue " + String(linearNormalisedValue) + "," + String(uiValue));

	int oldIntValue = intValue;

    if (parameter->isDiscrete())
    {
        int newValue = 0;
        
		ValueTree paramSettings;
		parameter->getSettings(paramSettings);

        if (paramSettings.isValid())
        {
            const int settingIdx = roundToInt(uiValue);
        
            if (settingIdx < paramSettings.getNumChildren())
                newValue = paramSettings.getChild(settingIdx).getProperty(Ids::intValue);
        }
        
        intValue = newValue;
    }
    else
    {
        double valueToScale = linearNormalisedValue;

        if (dBRef != 0.0f)
            valueToScale = dbSkew(valueToScale, dBRef, uiMinValue, uiMaxValue, true);

        intValue = roundToInt(scaleDouble(0.0f, 1.0f, minValue, maxValue, valueToScale));
    }

	if (oldIntValue != intValue)
		sendCurrentValue();
}

void ScopeOSCParameter::startListening()
{
	isListening = true; 
	BCMDBG("ScopeOSCParameter::startListening to " + getOSCPath());
}

void ScopeOSCParameter::stopListening() 
{
	isListening = false; 
	BCMDBG("ScopeOSCParameter::stopListening to " + getOSCPath());
}

void ScopeOSCParameter::startSending()
{
	isSending = true; 
	BCMDBG("ScopeOSCParameter::startSending to " + getOSCPath());
}

void ScopeOSCParameter::stopSending() 
{ 
	isSending = false; 
	BCMDBG("ScopeOSCParameter::stopSending to " + getOSCPath());
}

double ScopeOSCParameter::dbSkew(double valueToSkew, double ref, double uiMinValue, double uiMaxValue, bool invert) const
{
    if (!invert)
    {
        const double minNormalised = ref * pow(10, (uiMinValue/20));

        if (valueToSkew > minNormalised)
            return scaleDouble(uiMinValue, uiMaxValue, 0.0f, 1.0f, (20 * log10(valueToSkew / ref)));
        else
            return minNormalised;
    }
    else
    {
        valueToSkew = scaleDouble(0.0f, 1.0f, uiMinValue, uiMaxValue, valueToSkew);

        if (valueToSkew > uiMinValue && valueToSkew < uiMaxValue)
            return ref * pow(10, (valueToSkew / 20));
        else if (valueToSkew >= 0)
            return 1;
        else
            return 0;
    }
}

String ScopeOSCParameter::getOSCPath() const
{
	String address;

	if (paramID.paramGroup == 0)
		address = "/" + String(deviceInstance) + "/" + deviceUID + "/" + String(paramID.paramGroup) + "/" + String(paramID.paramId) + "/" + String(parameter->getConfigurationUID());
	else
		address = "/" + String(deviceInstance) + "/" + deviceUID + "/" + String(paramID.paramGroup) + "/" + String(paramID.paramId);
	
	BCMDBG("ScopeOSCParameter::getScopeOSCPath = " + address);

	return address;
}

void ScopeOSCParameter::oscMessageReceived(const OSCMessage& message)
{
	String address = message.getAddressPattern().toString();

	String logMessage("ScopeOSCParameter::oscMessageReceived - Port: " + String(oscServer->getLocalPortNum()) + ", Address: " + address);

	if (!isListening)
	{
		logMessage += " - Not currently listening";
		BCMDBG(logMessage);
		return;
	}

	if (message.size() == 1)
	{
		if (message[0].isInt32() && address == getOSCPath())
		{
			intValue = message[0].getInt32();
			
			double newUIValue;
			double newLinearNormalisedValue;

			if (parameter->isDiscrete())
			{
				int newSetting = findNearestParameterSetting(intValue);
            
				newUIValue = newSetting;
				newLinearNormalisedValue = parameter->convertUIToLinearNormalisedValue(newUIValue);
			}
			else
			{
				newLinearNormalisedValue = scaleDouble(minValue, maxValue, 0.0f, 1.0f, intValue);

				if (dBRef != 0.0f)
					newLinearNormalisedValue = dbSkew(newLinearNormalisedValue, dBRef, parameter->getUIRangeMin(), parameter->getUIRangeMax(), false);
					
				newUIValue = parameter->convertLinearNormalisedToUIValue(newLinearNormalisedValue);
			}
        
			parameter->setParameterValues(BCMParameter::scopeOSCUpdate, newLinearNormalisedValue, newUIValue);

			logMessage += " - parameter: " + parameter->getName() + ", new Scope Value : " + String(intValue) + " linearNormalisedValue: " + String(newLinearNormalisedValue) + ", uiValue: " + String(newUIValue);
		}
		else
		{
			logMessage += "- OSC message not processed";
		}
	}
	else
		logMessage += "- empty OSC message";

	BCMDBG(logMessage);
}

void ScopeOSCParameter::timerCallback()
{
	startListening();
	stopTimer();
}

int ScopeOSCParameter::findNearestParameterSetting(int value) const
{
	ValueTree paramSettings;
	parameter->getSettings(paramSettings);

    int nearestItem = 0;
    int smallestGap = INT_MAX;

    for (int i = 0; i < paramSettings.getNumChildren(); i++)
    {
        int settingValue = paramSettings.getChild(i).getProperty(Ids::intValue);
        int gap = abs(value - settingValue);

        if (gap == 0)
        {
            BCMDBG("BCMParameter::findNearestParameterSetting - Found 'exact' match for setting: " + paramSettings.getChild(i).getProperty(Ids::name).toString());

            nearestItem = i;
            break;
        }
        else if (gap < smallestGap)
        {
            smallestGap = gap;
            nearestItem = i;
        }
    }

    return nearestItem;
}
