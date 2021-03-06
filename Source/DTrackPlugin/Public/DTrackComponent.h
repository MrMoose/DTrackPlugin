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

#include "CoreMinimal.h"
#include "DTrackInterface.h"
#include "Components/ActorComponent.h"
#include "DTrackComponent.generated.h"

UCLASS(ClassGroup="Input Controller", meta=(BlueprintSpawnableComponent))
class DTRACKPLUGIN_API UDTrackComponent : public UActorComponent {

	GENERATED_UCLASS_BODY()

	public:

		UPROPERTY(EditAnywhere, meta = (DisplayName = "DTrack Server IP", ToolTip = "Enter the IP of your DTrack server host. Hostnames will not work."))
		FString m_dtrack_server_ip = "127.0.0.1";

		UPROPERTY(EditAnywhere, meta = (DisplayName = "DTrack Server Port", ToolTip = "Enter the port your server uses"))
		uint32  m_dtrack_server_port = 50105;

		UPROPERTY(EditAnywhere, meta = (DisplayName = "DTrack2 Protocol", ToolTip = "Use the TCP command channel based DTrack2 protocol"))
		bool    m_dtrack_2 = true;

		UPROPERTY(EditAnywhere, meta = (DisplayName = "DTrack Room Calibration", ToolTip = "Set this according to your DTrack system's room calibration"))
		EDTrackCoordinateSystemType m_coordinate_system = EDTrackCoordinateSystemType::CST_Normal;

		virtual void TickComponent(float n_delta_time, enum ELevelTick n_tick_type, FActorComponentTickFunction *n_this_tick_function) override;
		virtual void BeginPlay() override;
		virtual void EndPlay(const EEndPlayReason::Type n_reason) override;

		/// body tracking info came in. Take those and relay to the owning actor's interface
		void body_tracking(const int32 n_body_id, const FVector &n_translation, const FRotator &n_rotation);

		/// Flystick tracking data came in. Take those and relay to the owning actor's interface
		void flystick_tracking(const int32 n_flystick_id, const FVector &n_translation, const FRotator &n_rotation);

		/// Flystick button was pressed or released. Relay to the owning actor's interface
		void flystick_button(const int32 n_flystick_id, const int32 n_button_number, const bool n_pressed);

		/** 
		 * Flystick joystick values changed. I will hand them in all at once assuming multiple dimensions
		 * manifest as multiple joysticks and the user will want to treat them in once call rather than separate 
		 * calls
		 */
		void flystick_joystick(const int32 n_flystick_id, const TArray<float> &n_joysticks);

		/**
		 * Hand and finger tracking data comes in.
		 */
		void hand_tracking(const int32 n_hand_id, const bool n_right, const FVector &n_translation, const FRotator &n_rotation, const TArray<FDTrackFinger> &n_fingers);
	
		/**
		 * Human model tracking data comes in.
		 */
		void human_model(const int32 n_human_id, const TArray<FDTrackJoint> &n_joints);

	private:

		class IDTrackPlugin *m_plugin = nullptr;   //!< will cache that to avoid calling Module getter in every tick

};
