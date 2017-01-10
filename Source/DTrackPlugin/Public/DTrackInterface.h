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

UENUM(BlueprintType)
enum class EFingerType : uint8 {
	FT_Thumb    UMETA(DisplayName = "Thumb"),
	FT_Index    UMETA(DisplayName = "Index"),
	FT_Middle   UMETA(DisplayName = "Middle"),
	FT_Ring     UMETA(DisplayName = "Ring"),
	FT_Pinky    UMETA(DisplayName = "Pinky")
};

/**
 * This represents one finger (guess which one) as tracked info come in
 */
USTRUCT(BlueprintType)
struct FFinger {

	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, meta = (DisplayName = "Type"))
	EFingerType m_type;

	UPROPERTY(BlueprintReadOnly, meta = (DisplayName = "Location"))
	FVector  m_location;

	UPROPERTY(BlueprintReadOnly, meta = (DisplayName = "Rotation"))
	FRotator m_rotation;

	UPROPERTY(BlueprintReadOnly, meta = (DisplayName = "Radius Of Tip"))
	float    m_tip_radius;

	// phalanges (plural of phalanx) will not only come and getcha with their spears 
	// but are also the bones that form your fingers. Imagine that!

	UPROPERTY(BlueprintReadOnly, meta = (DisplayName = "Inner Phalanx Length"))
	float    m_inner_phalanx_length;

	UPROPERTY(BlueprintReadOnly, meta = (DisplayName = "Middle Phalanx Length"))
	float    m_middle_phalanx_length;

	UPROPERTY(BlueprintReadOnly, meta = (DisplayName = "Outer Phalanx Length"))
	float    m_outer_phalanx_length;

	UPROPERTY(BlueprintReadOnly, meta = (DisplayName = "Inner Middle Phalanx Angle"))
	float    m_inner_middle_phalanx_angle;

	UPROPERTY(BlueprintReadOnly, meta = (DisplayName = "Middle Outer Phalanx Angle"))
	float    m_middle_outer_phalanx_angle;
};


/**
 * This represents one joint of human model tracking.
 */
USTRUCT(BlueprintType)
struct FJoint {

	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, meta = (DisplayName = "ID"))
	int32    m_id;

	UPROPERTY(BlueprintReadOnly, meta = (DisplayName = "Location"))
	FVector  m_location;

	UPROPERTY(BlueprintReadOnly, meta = (DisplayName = "Rotation"))
	FRotator m_rotation;

	/// angles in relation to the joint coordinate system
	/**
	 * @todo if this means Euler angles, hand them out as a rotator
	 *    if they are then identical to m_rotation, remove.
	 */
	UPROPERTY(BlueprintReadOnly, meta = (DisplayName = "Angles"))
	TArray<float> m_angles;
};




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

		/**
		 * Each flystick can have multiple "joysticks".
		 * I suppose those are like the little analog controller sticks on, say PS4 or XBox controllers.
		 * Interestingly they appear to have only one dimension with values ranging between -1 and 1.
		 * I suppose a second dimension will manifest as a second stick, which is why I treat them all
		 * in one call rather than separate calls per joystick.
		 */
		UFUNCTION(BlueprintNativeEvent, Category = DTrackEvents)
		void OnFlystickJoystick(const int32 FlystickID, const TArray<float> &JoystickValues);
		
		/**
		 * Hand and finger tracking data comes in. They are collected in one call assuming they be treated at once.
		 */
		UFUNCTION(BlueprintNativeEvent, Category = DTrackEvents)
		void OnHandTracking(const int32 HandID, const bool Right, const FVector &Translation, const FRotator &Rotation, const TArray<FFinger> &Fingers);

		/**
		 * Human model tracking data comes in. All joints are assembled in this one call
		 */
		UFUNCTION(BlueprintNativeEvent, Category = DTrackEvents)
		void OnHumanModel(const int32 ModelID, const TArray<FJoint> &Joints);

		/// needed by the engine for raw output
		virtual FString ToString();
};