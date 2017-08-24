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

#include "IDTrackPlugin.h"
#include "DTrackPollThread.h"
#include "FDTrackPlugin.h"
#include "Math/UnrealMathUtility.h"
#include "DTrackSDK.hpp"

IMPLEMENT_MODULE(FDTrackPlugin, DTrackPlugin)

DEFINE_LOG_CATEGORY(DTrackPluginLog);

#define LOCTEXT_NAMESPACE "DTrackPlugin"

#define PLUGIN_VERSION "0.3.0"


void FDTrackPlugin::StartupModule() {
	
	UE_LOG(DTrackPluginLog, Log, TEXT("Using DTrack Plugin, threaded version %s"), TEXT(PLUGIN_VERSION));

	m_front.reset(new DataBuffer);
	m_back.reset(new DataBuffer);
	m_injected.reset(new DataBuffer);

	// quick test for extrapolation
// 	FVector current;
// 	FVector last;
// 
// 	m_last_injection_time = 1;
// 	m_current_injection_time = 3;
// 
// 	current.Y = 2;
// 	last.Y = 1;
// 
// 	FVector test;
// 	extrapolate(test, last, current);
// 
// 	// I expect Y to be 2.5
// 	checkf(test.Y == 2.5, TEXT("extrapolation test failed"));
// 
// 	m_last_injection_time = 0;
// 	m_current_injection_time = 0;
}

void FDTrackPlugin::ShutdownModule() {

	// Another wait for potential asyncs. 
	// Should be able to catch them but don't know how
	FPlatformProcess::Sleep(0.1);

	// we should have been stopped but what can you do?
	if (m_polling_thread) {
		m_polling_thread->interrupt();
		m_polling_thread->join();
		delete m_polling_thread;
		m_polling_thread = nullptr;
	}

	m_front.reset();
	m_back.reset();
	m_injected.reset();
}

void FDTrackPlugin::start_up(UDTrackComponent *n_client) {

	if (!m_polling_thread) {
		m_polling_thread = FDTrackPollThread::start(n_client, this);
	}

	// on error, the object is created but an error condition set within
	m_clients.Add(n_client);
}

void FDTrackPlugin::remove(class UDTrackComponent *n_client) {

	m_clients.RemoveAll([&](const TWeakObjectPtr<UDTrackComponent> p) {
		return p.Get() == n_client;
	});

	// we have no reason to run anymore
	if (m_polling_thread && (m_clients.Num() == 0)) {	
		m_polling_thread->interrupt();
		m_polling_thread->join();
		delete m_polling_thread;
		m_polling_thread = nullptr;
	}
}

FCriticalSection *FDTrackPlugin::swapping_mutex() {

	return &m_swapping_mutex;
}

void FDTrackPlugin::tick(const float n_delta_time, const UDTrackComponent *n_component) {

	if (!m_polling_thread) {
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

	// iterate all registered components and call the interface methods upon them
	for (TWeakObjectPtr<UDTrackComponent> c : m_clients) {
		// components might get killed and created along the way.
		// I only operate those which seem to live OK
		UDTrackComponent *component = c.Get();
		if (component) {
			// now handle the different tracking types by calling the component
			
			handle_bodies(component);
			handle_flysticks(component);
			handle_hands(component);
			handle_human_model(component);
		}
	}
}

/************************************************************************/
/* Injection routines                                                   */
/* Called as lambdas from polling thread, executed in game thread       */
/************************************************************************/
void FDTrackPlugin::inject_body_data(const int n_body_id, const FVector &n_translation, const FRotator &n_rotation) {

	check(m_injected);
	TArray<FBody> &body_inject = m_injected->m_body_data;

	if (body_inject.Num() < (n_body_id + 1)) {
		body_inject.SetNumZeroed(n_body_id + 1, false);
	}

	body_inject[n_body_id].m_location = n_translation;
	body_inject[n_body_id].m_rotation = n_rotation;
}

void FDTrackPlugin::inject_flystick_data(const int n_flystick_id, const FVector &n_translation, const FRotator &n_rotation, const TArray<int> &n_button_state, const TArray<float> &n_joystick_state) {

	check(m_injected);
	TArray<FFlystick> &flystick_inject = m_injected->m_flystick_data;

	if (flystick_inject.Num() < (n_flystick_id + 1)) {
		flystick_inject.SetNumZeroed(n_flystick_id + 1, false);
	}

	flystick_inject[n_flystick_id].m_location = n_translation;
	flystick_inject[n_flystick_id].m_rotation = n_rotation;
	flystick_inject[n_flystick_id].m_button_states = n_button_state;
	flystick_inject[n_flystick_id].m_joystick_states = n_joystick_state;
}

void FDTrackPlugin::inject_hand_data(const int n_hand_id, const bool &n_right, const FVector &n_translation, const FRotator &n_rotation, const TArray<FDtrackFinger> &n_fingers) {

	check(m_injected);
	TArray<FDtrackHand> &hand_inject = m_injected->m_hand_data;

	if (hand_inject.Num() < (n_hand_id + 1)) {
		hand_inject.SetNumZeroed(n_hand_id + 1, false);
	}

	hand_inject[n_hand_id].m_right = n_right;
	hand_inject[n_hand_id].m_location = n_translation;
	hand_inject[n_hand_id].m_rotation = n_rotation;
	hand_inject[n_hand_id].m_fingers = n_fingers;
}

void FDTrackPlugin::inject_human_model_data(const int n_human_id, const TArray<FDtrackJoint> &n_joints) {
	
	check(m_injected);
	TArray<FHuman> &human_inject = m_injected->m_human_model_data;

	if (human_inject.Num() < (n_human_id + 1)) {
		human_inject.SetNumZeroed(n_human_id + 1, false);
	}

	human_inject[n_human_id].m_joints = n_joints;
}

void FDTrackPlugin::begin_injection() {

	m_last_injection_time = m_current_injection_time;
	m_current_injection_time = FPlatformTime::Cycles64();
}

void FDTrackPlugin::end_injection() {

	// injection vector becomes front now, front becomes back and back becomes 
	// the new injection data storage
	FScopeLock lock(swapping_mutex());
	std::swap(m_front, m_back);
	std::swap(m_front, m_injected);
}

// y = dst
// y1 = last
// y2 = current
void FDTrackPlugin::extrapolate(FVector &y, const FVector &y1, const FVector &y2) const {

	uint64 now = FPlatformTime::Cycles64();

	// If the values we have are very recent I don't extrapolate and just return the latest
	if (now < m_current_injection_time + 10) {
		y = y2;
		return;
	}

	// this should cover startup conditions
	if (!m_current_injection_time || !m_last_injection_time) {		
		y = y2;
		return;
	}

	// unlikely since we use cycles but we could run twice so close together
	if (m_current_injection_time == m_last_injection_time) {
		y = y2;
		return;
	}


	// f(x) = y1 + ((x - x1) / (x2 - x1)) * (y2 - y1)
	//              ^-----factor--------^

	float factor = ((now - m_last_injection_time) / (m_current_injection_time - m_last_injection_time));

	y.X = y1.X + factor * (y2.X - y1.X);
	y.Y = y1.Y + factor * (y2.Y - y1.Y);
	y.Z = y1.Z + factor * (y2.Z - y1.Z);
}

void FDTrackPlugin::extrapolate(FRotator &n_y, const FRotator &n_y1, const FRotator &n_y2) const {

	uint64 now = FPlatformTime::Cycles64();

	// If the values we have are very recent I don't extrapolate and just return the latest
	if (now < m_current_injection_time + 10) {
		n_y = n_y2;
		return;
	}

	// this should cover startup conditions
	if (!m_current_injection_time || !m_last_injection_time) {
		n_y = n_y2;
		return;
	}

	// unlikely since we use cycles but we could run twice so close together
	if (m_current_injection_time == m_last_injection_time) {
		n_y = n_y2;
		return;
	}

	// I will use quaternions of those because I have no idea what happens when 
	// I extrapolate those Eulers
	FQuat y1 = n_y1.Quaternion();
	FQuat y2 = n_y2.Quaternion();
	FQuat ret;

	// f(x) = y1 + ((x - x1) / (x2 - x1)) * (y2 - y1)
	//              ^-----factor--------^

	float factor = ((now - m_last_injection_time) / (m_current_injection_time - m_last_injection_time));
	ret.W = y1.W + factor * (y2.W - y1.W);
	ret.X = y1.X + factor * (y2.X - y1.X);
	ret.Y = y1.Y + factor * (y2.Y - y1.Y);
	ret.Z = y1.Z + factor * (y2.Z - y1.Z);
	n_y = ret.Rotator();
}


/************************************************************************/
/* Handler methods. Called in game thread tick                          */
/* to relay information to components                                   */
/************************************************************************/
void FDTrackPlugin::handle_bodies(UDTrackComponent *n_component) {
	
	check(m_front);
	check(m_back);

	FScopeLock lock(swapping_mutex());
	for (int32 i = 0; i < m_front->m_body_data.Num(); i++) {

		const FBody &current_body = m_front->m_body_data[i];

		// This should occur only once while starting up
		// No extrapolation with one data set
		if (m_back->m_body_data.Num() != m_front->m_body_data.Num()) {
			n_component->body_tracking(i, current_body.m_location, current_body.m_rotation);
		} else {
			const FBody &last_body = m_back->m_body_data[i];

			FVector extrapolated_location;
			extrapolate(extrapolated_location, last_body.m_location, current_body.m_location);

			FRotator extrapolated_rotation;
			extrapolate(extrapolated_rotation, last_body.m_rotation, current_body.m_rotation);

			n_component->body_tracking(i, extrapolated_location, extrapolated_rotation);
		}
	}
}

void FDTrackPlugin::handle_flysticks(UDTrackComponent *n_component) {

	// treat all flysticks
	FScopeLock lock(swapping_mutex());
	for (int32 i = 0; i < m_front->m_flystick_data.Num(); i++) {

		FFlystick &current_flystick = m_front->m_flystick_data[i];

		// tracking first, it's always called
		n_component->flystick_tracking(i, current_flystick.m_location, current_flystick.m_rotation);

		if (current_flystick.m_button_states.Num()) {
			// compare button states with the last seen state, calling button handlers if appropriate

			// See if we have to resize our actual state vector to accommodate this.
			if (m_last_button_states.size() < (i + 1)) {
				// vector too small, insert new empties to pad
				TArray<int> new_stick;
				new_stick.SetNumZeroed(DTRACKSDK_FLYSTICK_MAX_BUTTON);
				m_last_button_states.resize(i + 1, new_stick);
			}
			
			const TArray<int> &current_states = current_flystick.m_button_states;
			TArray<int> &last_states = m_last_button_states[i];

			// have to go through all the button states now to figure out which ones have differed
			for (int32 b = 0; b < current_states.Num(); b++) {
				if (current_states[b] != last_states[b]) {
					last_states[b] = current_states[b];
					n_component->flystick_button(i, b, (current_states[b] == 1));
				}
			}
		}

		// Call joysticks if we have 'em
		if (current_flystick.m_joystick_states.Num()) {
			n_component->flystick_joystick(i, current_flystick.m_joystick_states);
		}
	}

	// that's it. Flystick all done.
}

void FDTrackPlugin::handle_hands(UDTrackComponent *n_component) {

	// treat all tracked hands
	FScopeLock lock(swapping_mutex());
	for (int32 i = 0; i < m_front->m_hand_data.Num(); i++) {
		const FDtrackHand &hand = m_front->m_hand_data[i];
		n_component->hand_tracking(i, hand.m_right, hand.m_location, hand.m_rotation, hand.m_fingers);
	}
}

void FDTrackPlugin::handle_human_model(UDTrackComponent *n_component) {
	
	FScopeLock lock(swapping_mutex());
	// treat all tracked hands
	for (int32 i = 0; i < m_front->m_human_model_data.Num(); i++) {
		const FHuman &human = m_front->m_human_model_data[i];
		n_component->human_model(i, human.m_joints);
	}
}

#undef LOCTEXT_NAMESPACE
