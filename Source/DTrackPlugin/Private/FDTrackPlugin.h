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

#include <memory>
#include <string>
#include <vector>

class DTrackSDK;

std::string to_string(const FString &n_string);


class FDTrackPlugin : public IDTrackPlugin {

	public:
		/** IModuleInterface implementation */
		void StartupModule();
		void ShutdownModule();

		/** Manual looping, currently called in main thread */
		void tick(const float n_delta_time, const UDTrackComponent *n_component);



		/** Optional Public API For direct module bind */
		bool IsRemoteEnabled();

		void start_up(class UDTrackComponent *n_client);

		/// tell the plugin we're no longer interested in tracking data
		void remove(class UDTrackComponent *n_client);

		void begin_tracking();

	private:
		
		/// consider the current frame's 6dof bodies and call the component if appropriate
		void handle_bodies(UDTrackComponent *n_component);

		/// consider the current frame's flystick tracking and button and call the component if appropriate
		void handle_flysticks(UDTrackComponent *n_component);
	
		/// treat everything hand and finger tracking relevant
		void handle_fingers(UDTrackComponent *n_component);

		/// extract and hand out human model (mocap?) data
		void handle_human_model(UDTrackComponent *n_component);

		/// this is the DTrack SDK main object. I'll have one one owned here as Í do not know if they can coexist
		std::unique_ptr< DTrackSDK > m_dtrack;
		bool                         m_tracking_active = false;
		bool                         m_dtrack2 = false;

		/**
		 * each frame will only cause the interface to be triggered once.
		 * This only seems to work for DTrack2 though. DTrack gives null every frame
		 * So it's only considered if m_dtrack2
		 */
		unsigned int                 m_last_seen_frame = 0;

		/** 
		 * each flystick gets its button states remembered here.
		 * flystick's ID is index in vector
		 */
		std::vector< std::vector<int> > m_flystick_buttons;

		/// each DTrack component registers itself here and gets called every tick
		TArray< TWeakObjectPtr<UDTrackComponent> > m_clients;

		/// only one of the client components will cause us to tick, so we don't do unnecessary frame ticks
		TWeakObjectPtr<UDTrackComponent>           m_ticker;
};