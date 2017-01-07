// Copyright (c) 2017, Advanced Realtime Tracking GmbH
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. Neither the name of copyright holder nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include "DTrackInterface.generated.h"

UINTERFACE(Blueprintable)
class UDTrackInterface : public UInterface {

	GENERATED_UINTERFACE_BODY()
};

/** @brief Blueprint interface for ART tracked objects
	
	Implement this interface in any Blueprint or C++ Actor to receive tracking information
	from the ART tracking system.

	@todo: Document me properly

 */
class DTRACKPLUGIN_API IDTrackInterface {

	GENERATED_IINTERFACE_BODY()

	public:
		/// never called yet
		UFUNCTION(BlueprintImplementableEvent, Category = DTrackEvents)
		void DeviceDisabled();

		/**
		 * This is called for each new set of body tracking data received unless 
		 * frame rate is lower than tracking data frequency.
		 */
		UFUNCTION(BlueprintNativeEvent, Category = DTrackEvents)
		void OnBodyData(const int32 BodyID, const FVector &Position, const FRotator &Rotation);

		/**
		 * This is called for each new set of flystick tracking data.
		 * It is only called for flystick location and rotation. Buttons come separately.
		 */
		UFUNCTION(BlueprintNativeEvent, Category = DTrackEvents)
		void OnFlystickData(const int32 FlystickID, const FVector &Position, const FRotator &Rotation);

		/**
		 * This is called when the user presses buttons on flysticks.
		 * It is only called when a button changes state to either Pressed or not Pressed
		 */
		UFUNCTION(BlueprintNativeEvent, Category = DTrackEvents)
		void OnFlystickButton(const int32 FlystickID, const int32 &ButtonIndex, const bool Pressed);


		virtual FString ToString();
};