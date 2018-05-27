/**
 * Image Loader utility class. Handles loading from file, or via
 * Image Cache. Also owns shared in-memory Image resources.
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

#include "ImageLoader.h"
#include "../Components/BCMLookAndFeel.h"

ImageLoader::ImageLoader()
{
}

ImageLoader::~ImageLoader()
{
    LookAndFeel::setDefaultLookAndFeel(nullptr);
}

Image ImageLoader::loadImage(const String& imageFileName, const String& directoryPath) const
{
	// Firstly look in BinaryData to see if we can find the Image there based on original filename
    for (int i = 0; i < BinaryData::namedResourceListSize; i++)
    {
		if (String(BinaryData::originalFilenames[i]).equalsIgnoreCase(imageFileName))
		{
			int sizeOfImage;
			const char* image = BinaryData::getNamedResource(BinaryData::namedResourceList[i], sizeOfImage);
            return ImageCache::getFromMemory(image, sizeOfImage);
        }
    }

	// We didn't find it in the BinaryData, so now check the filesystem
    if (directoryPath.isNotEmpty())
    {
        File directory(directoryPath);

        if (directory.exists())
        {
            File imageFile = directory.getChildFile(imageFileName);

            if (userSettings->getPropertyBoolValue("useimagecache", true))
                return ImageCache::getFromFile(imageFile);
            else
                return ImageFileFormat::loadFrom(imageFile);
        }
    }

    return Image();
}
