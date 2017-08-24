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

#include <vector>
#include <memory>

class FDTrackPlugin : public IDTrackPlugin {

	public:
		/** IModuleInterface implementation */
		void StartupModule();
		void ShutdownModule();

		/** Manual looping, currently called in main thread */
		void tick(const float n_delta_time, const UDTrackComponent *n_component);

		/// register this component with the tracking system
		void start_up(class UDTrackComponent *n_client);

		/// tell the plugin we're no longer interested in tracking data
		void remove(class UDTrackComponent *n_client);

		FCriticalSection *swapping_mutex();
		
	private:
		
		friend class FDTrackPollThread;

		/// polling thread injects body tracking data for later retrieval
		/// call in game thread, not mutexed!
		void inject_body_data(const int n_body_id, const FVector &n_translation, const FRotator &n_rotation);

		/// polling thread injects flystick data for later retrieval
		void inject_flystick_data(const int n_flystick_id, const FVector &n_translation, const FRotator &n_rotation,
					const TArray<int> &n_button_state, const TArray<float> &n_joystick_state);

		/// polling thread injects hand tracking data for later retrieval
		void inject_hand_data(const int n_hand_id, const bool &n_right, const FVector &n_translation, 
					const FRotator &n_rotation, const TArray<FDtrackFinger> &n_fingers);

		/// polling thread injects hand tracking data for later retrieval
		void inject_human_model_data(const int n_human_id, const TArray<FDtrackJoint> &n_joints);

		/// begin enter values and measure time
		void begin_injection();
		void end_injection();

		void extrapolate(FVector &y, const FVector &y1, const FVector &y2) const;
		void extrapolate(FRotator &n_y, const FRotator &n_y1, const FRotator &n_y2) const;

		/// For front and back buffer of data sent by polling thread
		struct DataBuffer {
			TArray<FBody>        m_body_data;          //!< cached body data being injected by thread
			TArray<FFlystick>    m_flystick_data;      //!< cached flystick tracking info
			TArray<FDtrackHand>  m_hand_data;          //!< cached hand tracking info
			TArray<FHuman>       m_human_model_data;   //!< cached human model info
		};

		/// unreal doesn't seem to have condition variables
		FCriticalSection         m_swapping_mutex;
		uint64                   m_current_injection_time = 0;
		uint64                   m_last_injection_time = 0;


		std::unique_ptr<DataBuffer> m_front;           //!< current data to read by game thread
		std::unique_ptr<DataBuffer> m_back;            //!< last values 
		std::unique_ptr<DataBuffer> m_injected;        //!< values being injected  

		std::vector< TArray<int> > m_last_button_states;

		class FDTrackPollThread *m_polling_thread = nullptr;
			
		/// consider the current frame's 6dof bodies and call the component if appropriate
		void handle_bodies(UDTrackComponent *n_component);

		/// consider the current frame's flystick tracking and button and call the component if appropriate
		void handle_flysticks(UDTrackComponent *n_component);
	
		/// treat everything hand and finger tracking relevant
		void handle_hands(UDTrackComponent *n_component);

		/// extract and hand out human model (mocap?) data
		void handle_human_model(UDTrackComponent *n_component);

		/// each DTrack component registers itself here and gets called every tick
		TArray< TWeakObjectPtr<UDTrackComponent> > m_clients;

		/// only one of the client components will cause us to tick, so we don't do unnecessary frame ticks
		TWeakObjectPtr<UDTrackComponent>           m_ticker;

};