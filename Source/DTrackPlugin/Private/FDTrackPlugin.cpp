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

#include "DTrackPluginPrivatePCH.h"

// this makes UBT allow windows types as (probably) used by the SDK header
// @todo check if this is indeed the case and remove if not needed
#include "AllowWindowsPlatformTypes.h" 

#include "DTrackSDK.hpp"

// revert above allowed types to UBT default
#include "HideWindowsPlatformTypes.h"

#include "IDTrackPlugin.h"
#include "FDTrackPlugin.h"

IMPLEMENT_MODULE(FDTrackPlugin, DTrackPlugin)

DEFINE_LOG_CATEGORY(DTrackPluginLog);

#define LOCTEXT_NAMESPACE "DTrackPlugin"

#define PLUGIN_VERSION "0.1.0"

// translate a DTrack body location (translation in mm) into Unreal Location (in cm)
FVector FDTrackPlugin::from_dtrack_location(const double(&n_translation)[3]) {

	FVector ret;

	// DTrack coordinates come in mm with either Z or Y being up, which has to be configured by the user.
	// I translate to Unreal's Z being up and cm units.
	
	switch (m_coordinate_system) {
		default:
		case ECoordinateSystemType::CST_Normal:
			ret.X =  n_translation[1] / 10.0;
			ret.Y =  n_translation[0] / 10.0;
			ret.Z =  n_translation[2] / 10.0;
			break;
		case ECoordinateSystemType::CST_Adapted:
			ret.X =  n_translation[0] / 10.0;
			ret.Y = -n_translation[1] / 10.0;
			ret.Z =  n_translation[2] / 10.0;
			break;
		case ECoordinateSystemType::CST_Powerwall:
			ret.X = -n_translation[2] / 10.0;
			ret.Y =  n_translation[0] / 10.0;
			ret.Z =  n_translation[1] / 10.0;
			break;
	}

	return ret;
}


// translate a DTrack 3x3 rotation matrix (translation in mm) into Unreal Location (in cm)
FRotator FDTrackPlugin::from_dtrack_rotation(const double (&n_matrix)[9]) {

	FQuat quaternion;
	double w = sqrt(1.0 + n_matrix[0 + 0] + n_matrix[1 + 3] + n_matrix[2 + 6]) / 2.0;
	quaternion.W = w;
	double w4 = 4.0 * w;

	switch (m_coordinate_system) {
		default:
		case ECoordinateSystemType::CST_Normal:
		{
			quaternion.Y = (n_matrix[2 + 3] - n_matrix[1 + 6]) / w4;
			quaternion.X = (n_matrix[0 + 6] - n_matrix[2 + 0]) / w4;
			quaternion.Z = (n_matrix[1 + 0] - n_matrix[0 + 3]) / w4;

			// Now make a rotator from this and adjust for coordinate system differences
			FRotator ret = quaternion.Rotator();
			ret.Roll  = -ret.Roll;
			ret.Pitch = -ret.Pitch;
			ret.Yaw   = -ret.Yaw;
			return ret;
		}
		case ECoordinateSystemType::CST_Adapted:
		{
			quaternion.X = (n_matrix[2 + 3] - n_matrix[1 + 6]) / w4;
			quaternion.Y = (n_matrix[0 + 6] - n_matrix[2 + 0]) / w4;
			quaternion.Z = (n_matrix[1 + 0] - n_matrix[0 + 3]) / w4;

			FRotator ret = quaternion.Rotator();
			ret.Roll = -ret.Roll;
			ret.Yaw  = -ret.Yaw;
			return ret;
		}
		case ECoordinateSystemType::CST_Powerwall:
		{
			quaternion.Y = (n_matrix[2 + 3] - n_matrix[1 + 6]) / w4;
			quaternion.Z = (n_matrix[0 + 6] - n_matrix[2 + 0]) / w4;
			quaternion.X = (n_matrix[1 + 0] - n_matrix[0 + 3]) / w4;

			FRotator ret = quaternion.Rotator();
			ret.Pitch = -ret.Pitch;
			ret.Yaw   = -ret.Yaw;
			return ret;
		}
	}
}

std::string to_string(const FString &n_string) {

	return TCHAR_TO_UTF8(*n_string);
}

void FDTrackPlugin::StartupModule() {
	
	UE_LOG(DTrackPluginLog, Log, TEXT("Using DTrack Plugin version %s"), TEXT(PLUGIN_VERSION));

}

void FDTrackPlugin::ShutdownModule() {
	
	m_dtrack.reset();
}

bool FDTrackPlugin::IsRemoteEnabled() {

	// @todo implement
	
	return m_dtrack->isLocalDataPortValid();
}

void FDTrackPlugin::tick(const float n_delta_time, const UDTrackComponent *n_component) {

	if (!m_dtrack || !m_tracking_active) {
		UE_LOG(DTrackPluginLog, Warning, TEXT("Erraneous plugin tick ignored."));
		return;
	}
	
	// only one component may cause an actual tick to save performance.
	// which one doesn't matter though. So I take any and store it
	if (!m_ticker.IsValid()) {
		for (TWeakObjectPtr<UDTrackComponent> c : m_clients) {
			if (c.IsValid()) {
				m_ticker = c;
				break;
			}
		}
	}
	
	// if this is the case, who calls us then? 
	checkf(m_ticker.IsValid(), TEXT("unknown ticker"));
	if (m_ticker.Get() != n_component) {
		return;
	}

	// receive pending datagrams
	if (!m_dtrack->receive()) {
		UE_LOG(DTrackPluginLog, Warning, TEXT("Receiving DTrack data failed."));
		return;
	}

	// try to sav performance for DTrack2 protocol, which has a frame counter.
	// why does DTrack not have that? Should be easy to implement...
	if (m_dtrack2) {
		unsigned int current_frame = m_dtrack->getMessageFrameNr();
		if (m_last_seen_frame == current_frame) {
			// no new data came in. I can safely bail
			return;
		} else {
			m_last_seen_frame = current_frame;
		}
	}

	// iterate all registered components and call the interface methods upon them
	for (TWeakObjectPtr<UDTrackComponent> c : m_clients) {
		// components might get killed and created along the way.
		// I only operate those which seem to live OK
		UDTrackComponent *component = c.Get();
		if (component) {
			// now handle the different tracking types by calling the component
			handle_bodies(component);
			handle_flysticks(component);
			handle_fingers(component);
			handle_human_model(component);
		}
	}
}

void FDTrackPlugin::handle_bodies(UDTrackComponent *n_component) {

	const DTrack_Body_Type_d *body = nullptr;
	for (int i = 0; i < m_dtrack->getNumBody(); i++) {  // why do people still use int for those counters?
		body = m_dtrack->getBody(i);
		checkf(body, TEXT("DTrack API error, body address null"));

		if (body->quality > 0) {
			// Quality below zero means the body is not visible to the system right now. I won't call the interface

			FVector translation = from_dtrack_location(body->loc);
			FRotator rotation = from_dtrack_rotation(body->rot);

			n_component->body_tracking(body->id, translation, rotation);
		}
	}
}

void FDTrackPlugin::handle_flysticks(UDTrackComponent *n_component) {

	const DTrack_FlyStick_Type_d *flystick = nullptr;
	for (int i = 0; i < m_dtrack->getNumFlyStick(); i++) {
		flystick = m_dtrack->getFlyStick(i);
		checkf(flystick, TEXT("DTrack API error, flystick address null"));

		if (flystick->quality > 0) {
			// Quality below zero means the body is not visible to the system right now. I won't call the interface
			FVector translation = from_dtrack_location(flystick->loc);
			FRotator rotation = from_dtrack_rotation(flystick->rot);
			n_component->flystick_tracking(flystick->id, translation, rotation);
		}

		// Now let's see if we already have a state vector for this flystick's buttons
		if (m_flystick_buttons.size() < (flystick->id + 1)) {
			// vector too small, insert new empties to pad
			std::vector<int> new_stick;
			new_stick.resize(DTRACKSDK_FLYSTICK_MAX_BUTTON, 0);
			m_flystick_buttons.resize(flystick->id + 1, new_stick);
		}

		std::vector<int> &current_stick = m_flystick_buttons[flystick->id];

		// have to go through all the button states now to figure out which ones have differed
		for (int button_idx = 0; button_idx < flystick->num_button; button_idx++) {
			if (flystick->button[button_idx] != current_stick[button_idx]) {
				// if the button has changed state, call the interface and remember current state.
				current_stick[button_idx] = flystick->button[button_idx];
				n_component->flystick_button(flystick->id, button_idx, (flystick->button[button_idx] == 1));
			}
		}

		// have to go through all the joysticks, assemble their values and call the collected interface
		TArray<float> joysticks;  // have to use float as blueprints don't support TArray<double>
		joysticks.AddZeroed(flystick->num_joystick);
		for (int joystick_idx = 0; joystick_idx < flystick->num_joystick; joystick_idx++) {
			joysticks[joystick_idx] = static_cast<float>(flystick->joystick[joystick_idx]);
		}

		if (joysticks.Num()) {
			n_component->flystick_joystick(flystick->id, joysticks);
		}

		// that's it. Flystick all done.
	}
}

void FDTrackPlugin::handle_fingers(UDTrackComponent *n_component) {

	const DTrack_Hand_Type_d *hand = nullptr;
	for (int i = 0; i < m_dtrack->getNumHand(); i++) {
		hand = m_dtrack->getHand(i);
		checkf(hand, TEXT("DTrack API error, hand address is null"));

		if (hand->quality > 0) {
			FVector translation = from_dtrack_location(hand->loc);
			FRotator rotation = from_dtrack_rotation(hand->rot);
			TArray<FFinger> fingers;

			for (int j = 0; j < hand->nfinger; j++) {
				FFinger finger;
				switch (j) {     // this is mostly to allow for the blueprint to be a 
								 // little more expressive than using assumptions about the index' meaning
					case 0: finger.m_type = EFingerType::FT_Thumb; break;
					case 1: finger.m_type = EFingerType::FT_Index; break;
					case 2: finger.m_type = EFingerType::FT_Middle; break;
					case 3: finger.m_type = EFingerType::FT_Ring; break;
					case 4: finger.m_type = EFingerType::FT_Pinky; break;
				}

				finger.m_location = from_dtrack_location(hand->finger[j].loc);
				finger.m_rotation = from_dtrack_rotation(hand->finger[j].rot);
				finger.m_tip_radius = hand->finger[j].radiustip;
				finger.m_inner_phalanx_length = hand->finger[j].lengthphalanx[2];
				finger.m_middle_phalanx_length = hand->finger[j].lengthphalanx[1];
				finger.m_outer_phalanx_length = hand->finger[j].lengthphalanx[0];
				finger.m_inner_middle_phalanx_angle = hand->finger[j].anglephalanx[1];
				finger.m_middle_outer_phalanx_angle = hand->finger[j].anglephalanx[0];
				fingers.Add(std::move(finger));
			}

			n_component->hand_tracking(hand->id, (hand->lr == 1), translation, rotation, fingers);
		}
	}
}

void FDTrackPlugin::handle_human_model(UDTrackComponent *n_component) {
	
	const DTrack_Human_Type_d *human = nullptr;
	for (int i = 0; i < m_dtrack->getNumHuman(); i++) {
		human = m_dtrack->getHuman(i);
		checkf(human, TEXT("DTrack API error, human address is null"));

		TArray<FJoint> joints;

		for (int j = 0; j < human->num_joints; j++) {
			FJoint joint;
			// I'm not sure if I should check for quality as I don't know if the caller
			// would expect number and order of joints to be relevant/constant.
			// They do carry an ID though so I suppose the caller must be aware of that.
			if (human->joint[j].quality > 0.1) {
				joint.m_id = human->joint[j].id;
				joint.m_location = from_dtrack_location(human->joint[j].loc);
				joint.m_rotation = from_dtrack_rotation(human->joint[j].rot);
				joint.m_angles.Add(human->joint[j].ang[0]);   // well, are they Euler angles of the same rot as above or not?
				joint.m_angles.Add(human->joint[j].ang[1]);
				joint.m_angles.Add(human->joint[j].ang[2]);
				joints.Add(std::move(joint));
			}
		}

		n_component->human_model(human->id, joints);
	}
}

void FDTrackPlugin::start_up(UDTrackComponent *n_client) {

	m_tracking_active = false;

	if (!m_dtrack) {
		// There appears to be several glitches in the SDK's constructor.
		// Most notably its missing ability to distinct between DTrack and DTrack2 protocols.
		// Also it looks like it cannot resolve hostnames and must be given an IP.

		// @todo clarify this with ART. DTrack Recorder only supports DTrack protocol and there's no way of knowing for sure.
		if (n_client->m_dtrack_2) {			
			m_dtrack.reset(new DTrackSDK(to_string(n_client->m_dtrack_server_ip), n_client->m_dtrack_server_port));
			m_dtrack2 = true;
		} else {
			m_dtrack.reset(new DTrackSDK(to_string(n_client->m_dtrack_server_ip), 50105, n_client->m_dtrack_server_port));
		}

		// take room calibration settings as well
		m_coordinate_system = n_client->m_coordinate_system;

		// I don't know when this can occur but I guess it's client
		// port collision with fixed UDP ports
		if (!m_dtrack->isLocalDataPortValid()) {
			m_dtrack.reset();
			return;
		}
	}

	// on error, the object is created but an error condition set within
	m_clients.Add(n_client);
}


void FDTrackPlugin::remove(class UDTrackComponent *n_client) {

	m_clients.RemoveAll([&](const TWeakObjectPtr<UDTrackComponent> p) {
		return p.Get() == n_client;
	});

	if (m_dtrack && (m_clients.Num() == 0)) {
		UE_LOG(DTrackPluginLog, Display, TEXT("Stopping DTrack measurement."));
		m_dtrack->stopMeasurement();
		m_dtrack.reset();
	}
}

void FDTrackPlugin::begin_tracking() {

	if (!m_dtrack) {
		return;
	} 

	if (!m_dtrack->startMeasurement()) {
		if (m_dtrack->getLastServerError() == DTrackSDK::ERR_TIMEOUT) {
			UE_LOG(DTrackPluginLog, Error, TEXT("Could not start tracking, timeout"));
		} else if (m_dtrack->getLastServerError() == DTrackSDK::ERR_NET) {
			UE_LOG(DTrackPluginLog, Error, TEXT("Could not start tracking, network error"));
		} else {
			UE_LOG(DTrackPluginLog, Error, TEXT("Could not start tracking"));
		}
		m_tracking_active = false;
	} else {
		m_tracking_active = true;
		UE_LOG(DTrackPluginLog, Display, TEXT("Tracking started successfully"));
	}
}

#undef LOCTEXT_NAMESPACE
